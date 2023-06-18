/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include "ComputerCalibration.h"
#include "ShipDeSerializer.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <ctime>
#include <iomanip>
#include <sstream>

std::unique_ptr<GameController> GameController::Create(
    RenderDeviceProperties const & renderDeviceProperties,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
{
    // Load fish species
    FishSpeciesDatabase fishSpeciesDatabase = FishSpeciesDatabase::Load(resourceLocator);

    // Load materials
    MaterialDatabase materialDatabase = MaterialDatabase::Load(resourceLocator);

    // Create game event dispatcher
    auto gameEventDispatcher = std::make_shared<GameEventDispatcher>();

    // Create perf stats
    std::unique_ptr<PerfStats> perfStats = std::make_unique<PerfStats>();

    // Create render context
    std::unique_ptr<Render::RenderContext> renderContext = std::make_unique<Render::RenderContext>(
        renderDeviceProperties,
        *perfStats,
        resourceLocator,
        [&progressCallback](float progress, ProgressMessageType message)
        {
            progressCallback(0.9f * progress, message);
        });

    //
    // Create controller
    //

    return std::unique_ptr<GameController>(
        new GameController(
            std::move(renderContext),
            std::move(gameEventDispatcher),
            std::move(perfStats),
            std::move(fishSpeciesDatabase),
            std::move(materialDatabase),
            resourceLocator,
            progressCallback));
}

GameController::GameController(
    std::unique_ptr<Render::RenderContext> renderContext,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::unique_ptr<PerfStats> perfStats,
    FishSpeciesDatabase && fishSpeciesDatabase,
    MaterialDatabase && materialDatabase,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
    // State machines
    : mTsunamiNotificationStateMachine()
    , mThanosSnapStateMachines()
    , mDayLightCycleStateMachine()
    // World
    , mWorld()
    , mFishSpeciesDatabase(std::move(fishSpeciesDatabase))
    , mMaterialDatabase(std::move(materialDatabase))
    // Ship factory
    , mShipStrengthRandomizer()
    , mShipTexturizer(mMaterialDatabase, resourceLocator)
    // State
    , mGameParameters()
    , mIsFrozen(false)
    , mIsPaused(false)
    , mIsPulseUpdateSet(false)
    , mIsMoveToolEngaged(false)
    // Parameters that we own
    , mTimeOfDay(0.0f) // We'll set it later
    , mDoShowTsunamiNotifications(true)
    , mDoDrawHeatBlasterFlame(true)    
    // Doers
    , mRenderContext(std::move(renderContext))
    , mGameEventDispatcher(std::move(gameEventDispatcher))
    , mNotificationLayer(
        mGameParameters.IsUltraViolentMode,
        false /*isSoundMuted; loaded value will come later*/,
        mGameParameters.DoDayLightCycle,
        false /*isAutoFocusOn; loaded value will come later*/,
        mRenderContext->GetDisplayUnitsSystem(),
        mGameEventDispatcher)
    , mThreadManager(
        mRenderContext->IsRenderingMultiThreaded(),
        8) // We start "zuinig", as we do not want to pay a ThreadPool price for too many threads
    , mViewManager(*mRenderContext, mNotificationLayer)
    // Smoothing
    , mFloatParameterSmoothers()
    // Stats
    , mStatsOriginTimestampReal(std::chrono::steady_clock::time_point::min())
    , mStatsLastTimestampReal(std::chrono::steady_clock::time_point::min())
    , mOriginTimestampGame(GameWallClock::GetInstance().Now())
    , mTotalPerfStats(std::move(perfStats))
    , mLastPublishedTotalPerfStats()
    , mTotalFrameCount(0u)
    , mLastPublishedTotalFrameCount(0u)
    , mSkippedFirstStatPublishes(0)
{
    // Initialize time-of-day
    SetTimeOfDay(1.0f);

    // Create world
    mWorld = std::make_unique<Physics::World>(
        OceanFloorTerrain::LoadFromImage(resourceLocator.GetDefaultOceanFloorTerrainFilePath()),
        CalculateAreCloudShadowsEnabled(mRenderContext->GetOceanRenderDetail()),
        mFishSpeciesDatabase,
        mGameEventDispatcher,
        mGameParameters,
        mRenderContext->GetVisibleWorld());

    // Register ourselves as event handler for the events we care about
    mGameEventDispatcher->RegisterLifecycleEventHandler(this);
    mGameEventDispatcher->RegisterWavePhenomenaEventHandler(this);

    //
    // Initialize parameter smoothers
    //

    float constexpr GenericParameterConvergenceFactor = 0.1f;
    float constexpr GenericParameterTerminationThreshold = 0.0005f;

    assert(mFloatParameterSmoothers.size() == SpringStiffnessAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.SpringStiffnessAdjustment;
        },
        [this](float const & value)
        {
            this->mGameParameters.SpringStiffnessAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == SpringStrengthAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.SpringStrengthAdjustment;
        },
        [this](float const & value)
        {
            this->mGameParameters.SpringStrengthAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == SeaDepthParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.SeaDepth;
        },
        [this](float const & value)
        {
            this->mGameParameters.SeaDepth = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == OceanFloorBumpinessParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.OceanFloorBumpiness;
        },
        [this](float const & value)
        {
            this->mGameParameters.OceanFloorBumpiness = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == OceanFloorDetailAmplificationParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.OceanFloorDetailAmplification;
        },
        [this](float const & value)
        {
            this->mGameParameters.OceanFloorDetailAmplification = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == FlameSizeAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mRenderContext->GetShipFlameSizeAdjustment();
        },
        [this](float const & value)
        {
            this->mRenderContext->SetShipFlameSizeAdjustment(value);
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == BasalWaveHeightAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.BasalWaveHeightAdjustment;
        },
        [this](float const & value)
        {
            this->mGameParameters.BasalWaveHeightAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == FishSizeMultiplierParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mGameParameters.FishSizeMultiplier;
        },
        [this](float const & value)
        {
            this->mGameParameters.FishSizeMultiplier = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    //
    // Calibrate game
    //

    progressCallback(1.0f, ProgressMessageType::Calibrating);

    auto const & score = ComputerCalibrator::Calibrate();

    ComputerCalibrator::TuneGame(score, mGameParameters, *mRenderContext);
}

GameController::~GameController()
{
    LogMessage("GameController::~GameController()");
}

void GameController::RebindOpenGLContext()
{
    assert(!!mRenderContext);
    mRenderContext->RebindContext();
}

ShipMetadata GameController::ResetAndLoadShip(ShipLoadSpecifications const & loadSpecs)
{
    return InternalResetAndLoadShip(loadSpecs);
}

ShipMetadata GameController::ResetAndReloadShip(ShipLoadSpecifications const & loadSpecs)
{
    return InternalResetAndLoadShip(loadSpecs);
}

ShipMetadata GameController::AddShip(ShipLoadSpecifications const & loadSpecs)
{
    // Load ship definition
    auto shipDefinition = ShipDeSerializer::LoadShip(loadSpecs.DefinitionFilepath, mMaterialDatabase);

    // Pre-validate ship's texture, if any
    if (shipDefinition.Layers.TextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.TextureLayer->Buffer);

    // Remember metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    //
    // Produce ship
    //

    auto const shipId = mWorld->GetNextShipId();

    auto [ship, textureImage] = ShipFactory::Create(
        shipId,
        *mWorld,
        std::move(shipDefinition),
        loadSpecs.LoadOptions,
        mMaterialDatabase,
        mShipTexturizer,
        mShipStrengthRandomizer,
        mGameEventDispatcher,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    InternalAddShip(
        std::move(ship),
        std::move(textureImage),
        shipMetadata);

    return shipMetadata;
}

RgbImageData GameController::TakeScreenshot()
{
    return mRenderContext->TakeScreenshot();
}

void GameController::RunGameIteration()
{
    assert(!mIsFrozen); // Not supposed to be invoked at all if we're frozen

    //
    // Initialize stats, if needed
    //

    if (mStatsOriginTimestampReal == std::chrono::steady_clock::time_point::min())
    {
        assert(mStatsLastTimestampReal == std::chrono::steady_clock::time_point::min());

        std::chrono::steady_clock::time_point const nowReal = std::chrono::steady_clock::now();

        mStatsOriginTimestampReal = nowReal;
        mStatsLastTimestampReal = nowReal;

        // In order to start from zero at first render, take global origin here
        mOriginTimestampGame = nowReal;

        // Render initial status text
        PublishStats(nowReal);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Update
    ////////////////////////////////////////////////////////////////////////////

    // Decide whether we are going to run a simulation update
    bool const doUpdate = ((!mIsPaused || mIsPulseUpdateSet) && !mIsMoveToolEngaged);

    // Clear pulse
    mIsPulseUpdateSet = false;

    if (doUpdate)
    {
        auto const startTime = GameChronometer::now();

        // Tell RenderContext we're starting an update
        // (waits until the last upload has completed)
        mRenderContext->UpdateStart();

        auto const netStartTime = GameChronometer::now();

        float const nowGame = GameWallClock::GetInstance().NowAsFloat();

        //
        // Update parameter smoothers
        //

        std::for_each(
            mFloatParameterSmoothers.begin(),
            mFloatParameterSmoothers.end(),
            [](auto & ps)
            {
                ps.Update();
            });

        //
        // Update world
        //

        assert(!!mWorld);
        mWorld->Update(
            mGameParameters,
            mRenderContext->GetVisibleWorld(),
            mRenderContext->GetStressRenderMode(),
            mThreadManager,
            *mTotalPerfStats);

        // Flush events
        mGameEventDispatcher->Flush();

        //
        // Update misc
        //

        // Update state machines
        UpdateAllStateMachines(mWorld->GetCurrentSimulationTime());

        // Update notification layer
        mNotificationLayer.Update(nowGame);

        // Tell RenderContext we've finished an update
        mRenderContext->UpdateEnd();

        mTotalPerfStats->TotalNetUpdateDuration.Update(GameChronometer::now() - netStartTime);
        mTotalPerfStats->TotalUpdateDuration.Update(GameChronometer::now() - startTime);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Render Upload
    ////////////////////////////////////////////////////////////////////////////

    // Tell RenderContext we're starting a new rendering cycle
    mRenderContext->RenderStart();

    {
        mRenderContext->UploadStart();

        auto const netStartTime = GameChronometer::now();

        // Update view manager
        // Note: some Upload()'s need to use ViewModel values, which have then to match the
        // ViewModel values used by the subsequent render
        mViewManager.Update(mWorld->GetAllAABBs());

        //
        // Upload world
        //

        assert(!!mWorld);
        mWorld->RenderUpload(
            mGameParameters,
            *mRenderContext,
            *mTotalPerfStats);

        //
        // Upload notification layer
        //

        mNotificationLayer.RenderUpload(*mRenderContext);

        mRenderContext->UploadEnd();

        mTotalPerfStats->TotalNetRenderUploadDuration.Update(GameChronometer::now() - netStartTime);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Render Draw
    ////////////////////////////////////////////////////////////////////////////

    {
        auto const startTime = GameChronometer::now();

        // Render
        mRenderContext->Draw();

        mTotalPerfStats->TotalMainThreadRenderDrawDuration.Update(GameChronometer::now() - startTime);
    }

    // Tell RenderContext we've finished a rendering cycle
    mRenderContext->RenderEnd();

    //
    // Update stats
    //

    ++mTotalFrameCount;
}

void GameController::LowFrequencyUpdate()
{
    std::chrono::steady_clock::time_point const nowReal = std::chrono::steady_clock::now();

    if (mSkippedFirstStatPublishes >= 1)
    {
        //
        // Publish stats
        //

        PublishStats(nowReal);
    }
    else
    {
        //
        // Skip the first few publishes, as rates would be too polluted
        //

        mStatsOriginTimestampReal = nowReal;

        mTotalPerfStats->Reset();
        mTotalFrameCount = 0u;

        ++mSkippedFirstStatPublishes;
    }

    mStatsLastTimestampReal = nowReal;

    mLastPublishedTotalPerfStats = *mTotalPerfStats;
    mLastPublishedTotalFrameCount = mTotalFrameCount;
}

void GameController::StartRecordingEvents(std::function<void(uint32_t, RecordedEvent const &)> onEventCallback)
{
    mEventRecorder = std::make_unique<EventRecorder>(onEventCallback);

    mWorld->SetEventRecorder(mEventRecorder.get());
}

RecordedEvents GameController::StopRecordingEvents()
{
    assert(!!mEventRecorder);

    mWorld->SetEventRecorder(nullptr);

    auto recordedEvents = mEventRecorder->StopRecording();
    mEventRecorder.reset();

    return recordedEvents;
}

void GameController::ReplayRecordedEvent(RecordedEvent const & event)
{
    mWorld->ReplayRecordedEvent(
        event,
        mGameParameters); // NOTE: using now's game parameters...but we don't want to capture these in the recorded event (at least at this moment)
}

/////////////////////////////////////////////////////////////
// Interactions
/////////////////////////////////////////////////////////////

void GameController::Freeze()
{
    assert(!mIsFrozen);

    // Wait for pending render tasks
    mRenderContext->WaitForPendingTasks();

    // Pause time
    GameWallClock::GetInstance().SetPaused(true);

    mIsFrozen = true;
}

void GameController::Thaw()
{
    assert(mIsFrozen);

    // Resume time
    GameWallClock::GetInstance().SetPaused(mIsPaused); // If we're paused, returned to paused

    mIsFrozen = false;
}

void GameController::SetPaused(bool isPaused)
{
    assert(!mIsFrozen); // Not supposed to be invoked while frozen, for no particular reason other than simplicity of state mgmt...

    // Pause/resume time
    GameWallClock::GetInstance().SetPaused(isPaused);

    // Change state
    mIsPaused = isPaused;
}

void GameController::SetMoveToolEngaged(bool isEngaged)
{
    mIsMoveToolEngaged = isEngaged;
}

void GameController::DisplaySettingsLoadedNotification()
{
    mNotificationLayer.AddEphemeralTextLine("SETTINGS LOADED");
}

bool GameController::GetShowStatusText() const
{
    return mNotificationLayer.IsStatusTextEnabled();
}

void GameController::SetShowStatusText(bool value)
{
    mNotificationLayer.SetStatusTextEnabled(value);
}

bool GameController::GetShowExtendedStatusText() const
{
    return mNotificationLayer.IsExtendedStatusTextEnabled();
}

void GameController::SetShowExtendedStatusText(bool value)
{
    mNotificationLayer.SetExtendedStatusTextEnabled(value);
}

void GameController::NotifySoundMuted(bool isSoundMuted)
{
    mNotificationLayer.SetSoundMuteIndicator(isSoundMuted);
}

void GameController::ToggleToFullDayOrNight()
{
    if (mTimeOfDay >= 0.5f)
        SetTimeOfDay(0.0f);
    else
        SetTimeOfDay(1.0f);
}

void GameController::ScareFish(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radius,
    std::chrono::milliseconds delay)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ScareFish(
        worldCoordinates,
        radius,
        delay);
}

void GameController::AttractFish(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radius,
    std::chrono::milliseconds delay)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->AttractFish(
        worldCoordinates,
        radius,
        delay);
}

void GameController::PickObjectToMove(
    DisplayLogicalCoordinates const & screenCoordinates,
    std::optional<ElementId> & elementId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->PickPointToMove(
        worldCoordinates,
        elementId,
        mGameParameters);
}

std::optional<ElementId> GameController::PickObjectForPickAndPull(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->PickObjectForPickAndPull(
        worldCoordinates,
        mGameParameters);
}

void GameController::Pull(
    ElementId elementId,
    DisplayLogicalCoordinates const & screenTarget)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenTarget);

    // Apply action
    assert(!!mWorld);
    mWorld->Pull(
        elementId,
        worldCoordinates,
        mGameParameters);
}

void GameController::PickObjectToMove(
    DisplayLogicalCoordinates const & screenCoordinates,
    std::optional<ShipId> & shipId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    auto const elementIndex = mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
    if (elementIndex.has_value())
        shipId = std::optional<ShipId>(elementIndex->GetShipId());
    else
        shipId = std::optional<ShipId>(std::nullopt);
}

void GameController::MoveBy(
    ElementId elementId,
    DisplayLogicalSize const & screenOffset,
    DisplayLogicalSize const & inertialScreenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
    vec2f const inertialVelocity = mRenderContext->ScreenOffsetToWorldOffset(inertialScreenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        elementId,
        worldOffset,
        inertialVelocity,
        mGameParameters);
}

void GameController::MoveBy(
    ShipId shipId,
    DisplayLogicalSize const & screenOffset,
    DisplayLogicalSize const & inertialScreenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
    vec2f const inertialVelocity = mRenderContext->ScreenOffsetToWorldOffset(inertialScreenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        shipId,
        worldOffset,
        inertialVelocity,
        mGameParameters);
}

void GameController::RotateBy(
    ElementId elementId,
    float screenDeltaY,
    DisplayLogicalCoordinates const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * screenDeltaY
        * 1.5f; // More responsive

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * inertialScreenDeltaY;

    // Apply action
    assert(!!mWorld);
    mWorld->RotateBy(
        elementId,
        angle,
        worldCenter,
        inertialAngle,
        mGameParameters);
}

void GameController::RotateBy(
    ShipId shipId,
    float screenDeltaY,
    DisplayLogicalCoordinates const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * inertialScreenDeltaY;

    // Apply action
    assert(!!mWorld);
    mWorld->RotateBy(
        shipId,
        angle,
        worldCenter,
        inertialAngle,
        mGameParameters);
}

void GameController::DestroyAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->DestroyAt(
        worldCoordinates,
        radiusMultiplier,
        mGameParameters);
}

void GameController::RepairAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusMultiplier,
    SequenceNumber repairStepId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->RepairAt(
        worldCoordinates,
        radiusMultiplier,
        repairStepId,
        mGameParameters);
}

bool GameController::SawThrough(
    DisplayLogicalCoordinates const & startScreenCoordinates,
    DisplayLogicalCoordinates const & endScreenCoordinates,
    bool isFirstSegment)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->SawThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        isFirstSegment,
        mGameParameters);
}

bool GameController::ApplyHeatBlasterAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    HeatBlasterActionType action)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mGameParameters.HeatBlasterRadius;
    if (mGameParameters.IsUltraViolentMode)
    {
        radius *= 5.0f;
    }

    // Apply action
    assert(!!mWorld);
    bool isApplied = mWorld->ApplyHeatBlasterAt(
        worldCoordinates,
        action,
        radius,
        mGameParameters);

    if (isApplied)
    {
        if (mDoDrawHeatBlasterFlame)
        {
            // Draw notification (one frame only)
            mNotificationLayer.SetHeatBlaster(
                worldCoordinates,
                radius,
                action);
        }
    }

    return isApplied;
}

bool GameController::ExtinguishFireAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mGameParameters.FireExtinguisherRadius;
    if (mGameParameters.IsUltraViolentMode)
    {
        radius *= 5.0f;
    }

    // Apply action
    assert(!!mWorld);
    bool isApplied = mWorld->ExtinguishFireAt(
        worldCoordinates,
        radius,
        mGameParameters);

    if (isApplied)
    {
        // Draw notification (one frame only)
        mNotificationLayer.SetFireExtinguisherSpray(
            worldCoordinates,
            radius);
    }

    return isApplied;
}

void GameController::ApplyBlastAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusMultiplier,
    float forceMultiplier,
    float renderProgress,
    float personalitySeed)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float const radius =
        mGameParameters.BlastToolRadius
        * radiusMultiplier
        * (mGameParameters.IsUltraViolentMode ? 2.5f : 1.0f);

    // Apply action
    assert(!!mWorld);
    mWorld->ApplyBlastAt(
        worldCoordinates,
        radius,
        forceMultiplier,
        mGameParameters);

    // Draw notification (one frame only)
    mNotificationLayer.SetBlastToolHalo(
        worldCoordinates,
        radius,
        renderProgress,
        personalitySeed);
}

bool GameController::ApplyElectricSparkAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    std::uint64_t counter,
    float lengthMultiplier,
    float currentSimulationTime)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->ApplyElectricSparkAt(
        worldCoordinates,
        counter,
        lengthMultiplier,
        currentSimulationTime,
        mGameParameters);
}

void GameController::ApplyRadialWindFrom(
    DisplayLogicalCoordinates const & sourcePos,
    float preFrontSimulationTimeElapsed,
    float preFrontIntensityMultiplier,
    float mainFrontSimulationTimeElapsed,
    float mainFrontIntensityMultiplier)
{
    vec2f const sourceWorldCoordinates = mRenderContext->ScreenToWorld(sourcePos);

    // Calculate wind speed, in m/s
    float const effectiveBaseWindSpeed =
        mGameParameters.WindMakerToolWindSpeed * 1000.0f / 3600.0f
        * (mGameParameters.IsUltraViolentMode ? 3.5f : 1.0f);
    float const preFrontWindSpeed = effectiveBaseWindSpeed * preFrontIntensityMultiplier;
    float const mainFrontWindSpeed = effectiveBaseWindSpeed * mainFrontIntensityMultiplier;

    // Calculate distance traveled along fronts
    float preFrontRadius = preFrontWindSpeed * preFrontSimulationTimeElapsed;
    float mainFrontRadius = mainFrontWindSpeed * mainFrontSimulationTimeElapsed;

    // Apply action
    assert(!!mWorld);
    mWorld->ApplyRadialWindFrom(
        sourceWorldCoordinates,
        preFrontRadius,
        preFrontWindSpeed,
        mainFrontRadius,
        mainFrontWindSpeed,
        mGameParameters);

    // Draw notification (one frame only)
    mNotificationLayer.SetWindSphere(
        sourceWorldCoordinates,
        preFrontRadius,
        preFrontIntensityMultiplier,
        mainFrontRadius,
        mainFrontIntensityMultiplier);
}

bool GameController::ApplyLaserCannonThrough(
    DisplayLogicalCoordinates const & startScreenCoordinates, 
    DisplayLogicalCoordinates const & endScreenCoordinates,
    std::optional<float> strength)
{
    bool hasCut = false;

    vec2f const startWorld = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorld = mRenderContext->ScreenToWorld(endScreenCoordinates);

    if (strength)
    {
        // Apply action
        assert(!!mWorld);
        hasCut = mWorld->ApplyLaserCannonThrough(
            startWorld,
            endWorld,
            *strength,
            mGameParameters);
    }

    // Draw notification at end (one frame only)
    mNotificationLayer.SetLaserCannon(
        endScreenCoordinates,
        strength);

    return hasCut;
}

void GameController::DrawTo(
    DisplayLogicalCoordinates const & screenCoordinates,
    float strengthFraction)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->DrawTo(
        worldCoordinates,
        strengthFraction,
        mGameParameters);
}

void GameController::SwirlAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float strengthFraction)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->SwirlAt(
        worldCoordinates,
        strengthFraction,
        mGameParameters);
}

void GameController::TogglePinAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::RemoveAllPins()
{
    // Apply action
    assert(!!mWorld);
    mWorld->RemoveAllPins();
}

std::optional<ToolApplicationLocus> GameController::InjectPressureAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float pressureQuantityMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    auto const applicationLocus = mWorld->InjectPressureAt(
        worldCoordinates,
        pressureQuantityMultiplier,
        mGameParameters);

    if (applicationLocus.has_value()
        && (*applicationLocus & ToolApplicationLocus::Ship) == ToolApplicationLocus::Ship)
    {
        // Draw notification (one frame only)
        mNotificationLayer.SetPressureInjectionHalo(
            worldCoordinates,
            pressureQuantityMultiplier);
    }

    return applicationLocus;
}

bool GameController::FloodAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float waterQuantityMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->FloodAt(
        worldCoordinates,
        waterQuantityMultiplier,
        mGameParameters);
}

void GameController::ToggleAntiMatterBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleAntiMatterBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleImpactBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleImpactBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::TogglePhysicsProbeAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    auto const toggleResult = mWorld->TogglePhysicsProbeAt(
        worldCoordinates,
        mGameParameters);

    // Tell physics probe panel whether we've removed or added a probe
    if (toggleResult.has_value())
    {
        mNotificationLayer.SetPhysicsProbePanelState(*toggleResult);
    }
}

void GameController::ToggleRCBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleRCBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleTimerBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleTimerBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::DetonateRCBombs()
{
    // Apply action
    assert(!!mWorld);
    mWorld->DetonateRCBombs();
}

void GameController::DetonateAntiMatterBombs()
{
    // Apply action
    assert(!!mWorld);
    mWorld->DetonateAntiMatterBombs();
}

void GameController::AdjustOceanSurfaceTo(
    DisplayLogicalCoordinates const & screenCoordinates, 
    int screenRadius)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);
    float const worldRadius = mRenderContext->ScreenOffsetToWorldOffset(screenRadius);

    // Apply action
    assert(mWorld);
    mWorld->AdjustOceanSurfaceTo(worldCoordinates, worldRadius);
}

std::optional<bool> GameController::AdjustOceanFloorTo(
    vec2f const & startWorldPosition, 
    vec2f const & endWorldPosition)
{
    assert(!!mWorld);
    return mWorld->AdjustOceanFloorTo(
        startWorldPosition.x,
        startWorldPosition.y,
        endWorldPosition.x,
        endWorldPosition.y);
}

bool GameController::ScrubThrough(
    DisplayLogicalCoordinates const & startScreenCoordinates,
    DisplayLogicalCoordinates const & endScreenCoordinates)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->ScrubThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        mGameParameters);
}

bool GameController::RotThrough(
    DisplayLogicalCoordinates const & startScreenCoordinates,
    DisplayLogicalCoordinates const & endScreenCoordinates)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->RotThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        mGameParameters);
}

void GameController::ApplyThanosSnapAt(
    DisplayLogicalCoordinates const & screenCoordinates, 
    bool isSparseMode)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    StartThanosSnapStateMachine(worldCoordinates.x, isSparseMode, mWorld->GetCurrentSimulationTime());
}

std::optional<ElementId> GameController::GetNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    return mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
}

void GameController::QueryNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    mWorld->QueryNearestPointAt(worldCoordinates, 1.0f);
}

void GameController::TriggerTsunami()
{
    assert(!!mWorld);
    mWorld->TriggerTsunami();
}

void GameController::TriggerRogueWave()
{
    assert(!!mWorld);
    mWorld->TriggerRogueWave();
}

void GameController::TriggerStorm()
{
    assert(!!mWorld);
    mWorld->TriggerStorm();
}

void GameController::TriggerLightning()
{
    assert(!!mWorld);
    mWorld->TriggerLightning(mGameParameters);
}

void GameController::HighlightElectricalElement(ElectricalElementId electricalElementId)
{
    assert(!!mWorld);
    mWorld->HighlightElectricalElement(electricalElementId);
}

void GameController::SetSwitchState(
    ElectricalElementId electricalElementId,
    ElectricalState switchState)
{
    assert(!!mWorld);
    mWorld->SetSwitchState(
        electricalElementId,
        switchState,
        mGameParameters);
}

void GameController::SetEngineControllerState(
    ElectricalElementId electricalElementId,
    float controllerValue)
{
    assert(!!mWorld);
    mWorld->SetEngineControllerState(
        electricalElementId,
        controllerValue,
        mGameParameters);
}

bool GameController::DestroyTriangle(ElementId triangleId)
{
    assert(!!mWorld);
    return mWorld->DestroyTriangle(triangleId);
}

bool GameController::RestoreTriangle(ElementId triangleId)
{
    assert(!!mWorld);
    return mWorld->RestoreTriangle(triangleId);
}

//
// Render controls
//

void GameController::SetCanvasSize(DisplayLogicalSize const & canvasSize)
{
    // Tell RenderContext
    mRenderContext->SetCanvasLogicalSize(canvasSize);

    // Tell view manager
    mViewManager.OnViewModelUpdated();
}

void GameController::Pan(DisplayLogicalSize const & screenOffset)
{
    mViewManager.Pan(mRenderContext->ScreenOffsetToWorldOffset(screenOffset));
}

void GameController::PanToWorldEnd(int side)
{
    mViewManager.PanToWorldX(
        side == 0
        ? -GameParameters::HalfMaxWorldWidth
        : GameParameters::HalfMaxWorldWidth);
}

void GameController::AdjustZoom(float amount)
{
    mViewManager.AdjustZoom(amount);
}

void GameController::ResetView()
{
    if (mWorld)
    {
        mViewManager.ResetView(mWorld->GetAllAABBs());
    }
}

void GameController::FocusOnShip()
{
    if (mWorld)
    {
        mViewManager.FocusOnShip(mWorld->GetAllAABBs());
    }
}

vec2f GameController::ScreenToWorld(DisplayLogicalCoordinates const & screenCoordinates) const
{
    return mRenderContext->ScreenToWorld(screenCoordinates);
}

vec2f GameController::ScreenOffsetToWorldOffset(DisplayLogicalSize const & screenOffset) const
{
    return mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
}

////////////////////////////////////////////////////////////////////////////////////////

void GameController::SetTimeOfDay(float value)
{
    mTimeOfDay = value;

    // Calculate new ambient light
    mRenderContext->SetAmbientLightIntensity(
        SmoothStep(
            0.0f,
            1.0f,
            mTimeOfDay));

    // Calcualate new sun rays inclination:
    // ToD = 1 => inclination = +1 (45 degrees)
    // ToD = 0 => inclination = -1 (45 degrees)
    mRenderContext->SetSunRaysInclination(2.0f * mTimeOfDay - 1.0f);
}

void GameController::SetDoDayLightCycle(bool value)
{
    mGameParameters.DoDayLightCycle = value;

    if (value)
    {
        StartDayLightCycleStateMachine();
    }
    else
    {
        StopDayLightCycleStateMachine();
    }
}

void GameController::SetOceanRenderDetail(OceanRenderDetailType oceanRenderDetail)
{ 
    mRenderContext->SetOceanRenderDetail(oceanRenderDetail); 
    mWorld->SetAreCloudShadowsEnabled(CalculateAreCloudShadowsEnabled(oceanRenderDetail));
}

////////////////////////////////////////////////////////////////////////////////////////

void GameController::OnTsunami(float x)
{
    if (mDoShowTsunamiNotifications)
    {
        // Start state machine
        StartTsunamiNotificationStateMachine(x);
    }
}

void GameController::OnShipRepaired(ShipId /*shipId*/)
{
    mNotificationLayer.AddEphemeralTextLine("SHIP REPAIRED!");

    LogMessage("Ship repaired!");
}

////////////////////////////////////////////////////////////////////////////////////////

ShipMetadata GameController::InternalResetAndLoadShip(ShipLoadSpecifications const & loadSpecs)
{
    assert(!!mWorld);

    // Load ship definition
    auto shipDefinition = ShipDeSerializer::LoadShip(loadSpecs.DefinitionFilepath, mMaterialDatabase);

    // Pre-validate ship's texture
    if (shipDefinition.Layers.TextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.TextureLayer->Buffer);

    // Save metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        OceanFloorTerrain(mWorld->GetOceanFloorTerrain()),
        CalculateAreCloudShadowsEnabled(mRenderContext->GetOceanRenderDetail()),
        mFishSpeciesDatabase,
        mGameEventDispatcher,
        mGameParameters,
        mRenderContext->GetVisibleWorld());

    // Produce ship
    auto const shipId = newWorld->GetNextShipId();
    auto [ship, textureImage] = ShipFactory::Create(
        shipId,
        *newWorld,
        std::move(shipDefinition),
        loadSpecs.LoadOptions,
        mMaterialDatabase,
        mShipTexturizer,
        mShipStrengthRandomizer,
        mGameEventDispatcher,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    Reset(std::move(newWorld));

    InternalAddShip(
        std::move(ship),
        std::move(textureImage),
        shipMetadata);

    return shipMetadata;
}

void GameController::Reset(std::unique_ptr<Physics::World> newWorld)
{
    // Reset world
    assert(!!mWorld);
    mWorld = std::move(newWorld);

    // Set event recorder (if any)
    mWorld->SetEventRecorder(mEventRecorder.get());

    // Reset state machines
    ResetAllStateMachines();

    // Reset perf stats
    mTotalPerfStats->Reset();
    mLastPublishedTotalPerfStats.Reset();

    // Reset notification layer
    mNotificationLayer.Reset();

    // Reset rendering engine
    assert(!!mRenderContext);
    mRenderContext->Reset();

    // Notify
    mGameEventDispatcher->OnGameReset();
}

void GameController::InternalAddShip(
    std::unique_ptr<Physics::Ship> ship,
    RgbaImageData && textureImage,
    ShipMetadata const & shipMetadata)
{
    ShipId const shipId = ship->GetId();

    // Set recorder in ship (if any)
    ship->SetEventRecorder(mEventRecorder.get());

    // Add ship to our world
    mWorld->AddShip(std::move(ship));

    // Add ship to rendering engine
    mRenderContext->AddShip(
        shipId,
        mWorld->GetShipPointCount(shipId),
        std::move(textureImage));

    // Tell view manager
    mViewManager.OnNewShip(mWorld->GetAllAABBs());

    // Notify ship load
    mGameEventDispatcher->OnShipLoaded(
        shipId,
        shipMetadata);

    // Announce
    mWorld->Announce();
}

void GameController::PublishStats(std::chrono::steady_clock::time_point nowReal)
{
    PerfStats const lastDeltaPerfStats = *mTotalPerfStats - mLastPublishedTotalPerfStats;
    uint64_t const lastDeltaFrameCount = mTotalFrameCount - mLastPublishedTotalFrameCount;

    // Calculate fps

    auto totalElapsedReal = std::chrono::duration<float>(nowReal - mStatsOriginTimestampReal);
    auto lastElapsedReal = std::chrono::duration<float>(nowReal - mStatsLastTimestampReal);

    float const totalFps =
        totalElapsedReal.count() != 0.0f
        ? static_cast<float>(mTotalFrameCount) / totalElapsedReal.count()
        : 0.0f;

    float const lastFps =
        lastElapsedReal.count() != 0.0f
        ? static_cast<float>(lastDeltaFrameCount) / lastElapsedReal.count()
        : 0.0f;

    // Publish frame rate
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->OnFrameRateUpdated(lastFps, totalFps);

    // Publish update time
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->OnCurrentUpdateDurationUpdated(lastDeltaPerfStats.TotalUpdateDuration.ToRatio<std::chrono::milliseconds>());

    // Update status text
    mNotificationLayer.SetStatusTexts(
        lastFps,
        totalFps,
        lastDeltaPerfStats,
        *mTotalPerfStats,
        std::chrono::duration<float>(GameWallClock::GetInstance().Now() - mOriginTimestampGame),
        mIsPaused,
        mRenderContext->GetZoom(),
        mRenderContext->GetCameraWorldPosition(),
        mRenderContext->GetStatistics());
}

bool GameController::CalculateAreCloudShadowsEnabled(OceanRenderDetailType oceanRenderDetail)
{
    // Note: also RenderContext infers applicability of shadows via detail, independently
    return (oceanRenderDetail == OceanRenderDetailType::Detailed);
}