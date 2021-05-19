/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include "ComputerCalibration.h"

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
    // State
    , mGameParameters()
    , mIsPaused(false)
    , mIsPulseUpdateSet(false)
    , mIsMoveToolEngaged(false)
    // Parameters that we own
    , mDoShowTsunamiNotifications(true)
    , mDoDrawHeatBlasterFlame(true)
    , mDoAutoZoomOnShipLoad(true)
    // Doers
    , mRenderContext(std::move(renderContext))
    , mGameEventDispatcher(std::move(gameEventDispatcher))
    , mNotificationLayer(
        mGameParameters.IsUltraViolentMode,
        false /*loaded value will come later*/,
        mGameParameters.DoDayLightCycle,
        mGameEventDispatcher)
    , mShipTexturizer(resourceLocator)
    // World
    , mFishSpeciesDatabase(std::move(fishSpeciesDatabase))
    , mMaterialDatabase(std::move(materialDatabase))
    , mWorld(new Physics::World(
        OceanFloorTerrain::LoadFromImage(resourceLocator.GetDefaultOceanFloorTerrainFilePath()),
        mFishSpeciesDatabase,
        mGameEventDispatcher,
        std::make_shared<TaskThreadPool>(),
        mGameParameters,
        mRenderContext->GetVisibleWorld()))
    // Smoothing
    , mFloatParameterSmoothers()
    , mZoomParameterSmoother()
    , mCameraWorldPositionParameterSmoother()
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
    // Verify materials' textures
    mShipTexturizer.VerifyMaterialDatabase(mMaterialDatabase);

    // Register ourselves as event handler for the events we care about
    mGameEventDispatcher->RegisterLifecycleEventHandler(this);
    mGameEventDispatcher->RegisterWavePhenomenaEventHandler(this);

    //
    // Initialize parameter smoothers
    //

    std::chrono::milliseconds constexpr ParameterSmoothingTrajectoryTime = std::chrono::milliseconds(1000);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

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
        ParameterSmoothingTrajectoryTime);

    // ---------------------------------

    std::chrono::milliseconds constexpr ControlParameterSmoothingTrajectoryTime = std::chrono::milliseconds(500);

    mZoomParameterSmoother = std::make_unique<ParameterSmoother<float>>(
        [this]() -> float const &
        {
            return this->mRenderContext->GetZoom();
        },
        [this](float const & value) -> float const &
        {
            return this->mRenderContext->SetZoom(value);
        },
        [this](float const & value)
        {
            return this->mRenderContext->ClampZoom(value);
        },
        ControlParameterSmoothingTrajectoryTime);

    mCameraWorldPositionParameterSmoother = std::make_unique<ParameterSmoother<vec2f>>(
        [this]() -> vec2f const &
        {
            return this->mRenderContext->GetCameraWorldPosition();
        },
        [this](vec2f const & value) -> vec2f const &
        {
            return this->mRenderContext->SetCameraWorldPosition(value);
        },
        [this](vec2f const & value)
        {
            return this->mRenderContext->ClampCameraWorldPosition(value);
        },
        ControlParameterSmoothingTrajectoryTime);

    //
    // Calibrate game
    //

    progressCallback(1.0f, ProgressMessageType::Calibrating);

    auto const & score = ComputerCalibrator::Calibrate();

    ComputerCalibrator::TuneGame(score, mGameParameters, *mRenderContext);
}

void GameController::RebindOpenGLContext()
{
    assert(!!mRenderContext);
    mRenderContext->RebindContext();
}

ShipMetadata GameController::ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath)
{
    return ResetAndLoadShip(shipDefinitionFilepath, StrongTypedTrue<DoAutoZoom>);
}

ShipMetadata GameController::ResetAndReloadShip(std::filesystem::path const & shipDefinitionFilepath)
{
    return ResetAndLoadShip(shipDefinitionFilepath, StrongTypedFalse<DoAutoZoom>);
}

ShipMetadata GameController::AddShip(std::filesystem::path const & shipDefinitionFilepath)
{
    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

    // Pre-validate ship's texture
    if (shipDefinition.TextureLayerImage.has_value())
        mRenderContext->ValidateShipTexture(*shipDefinition.TextureLayerImage);

    // Remember metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Load ship into current world
    auto [shipId, textureImage] = mWorld->AddShip(
        std::move(shipDefinition),
        mMaterialDatabase,
        mShipTexturizer,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    OnShipAdded(
        shipId,
        std::move(textureImage),
        shipMetadata,
        StrongTypedFalse<struct DoAutoZoom>);

    return shipMetadata;
}

RgbImageData GameController::TakeScreenshot()
{
    return mRenderContext->TakeScreenshot();
}

void GameController::RunGameIteration()
{
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
        mRenderContext->UpdateStart();

        auto const netStartTime = GameChronometer::now();

        float const nowGame = GameWallClock::GetInstance().NowAsFloat();

        //
        // Update parameter smoothers
        //

        std::for_each(
            mFloatParameterSmoothers.begin(),
            mFloatParameterSmoothers.end(),
            [nowGame](auto & ps)
            {
                ps.Update(nowGame);
            });

        //
        // Update world
        //

        assert(!!mWorld);
        mWorld->Update(
            mGameParameters,
            mRenderContext->GetVisibleWorld(),
            *mTotalPerfStats);

        // Flush events
        mGameEventDispatcher->Flush();

        //
        // Update misc
        //

        // Update state machines
        UpdateStateMachines(mWorld->GetCurrentSimulationTime());

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

        // Smooth render controls
        // Note: some Upload()'s need to use ViewModel values, which have then to match the
        // ViewModel values used by the subsequent render
        {
            float const nowReal = GameWallClock::GetInstance().ContinuousNowAsFloat(); // Real wall clock, unpaused
            mZoomParameterSmoother->Update(nowReal);
            mCameraWorldPositionParameterSmoother->Update(nowReal);
        }

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

void GameController::SetPaused(bool isPaused)
{
    // Freeze time
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

void GameController::ScareFish(
    LogicalPixelCoordinates const & screenCoordinates,
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
    LogicalPixelCoordinates const & screenCoordinates,
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
    LogicalPixelCoordinates const & screenCoordinates,
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

std::optional<ElementId> GameController::PickObjectForPickAndPull(LogicalPixelCoordinates const & screenCoordinates)
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
    LogicalPixelCoordinates const & screenTarget)
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
    LogicalPixelCoordinates const & screenCoordinates,
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
    LogicalPixelSize const & screenOffset,
    LogicalPixelSize const & inertialScreenOffset)
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
    LogicalPixelSize const & screenOffset,
    LogicalPixelSize const & inertialScreenOffset)
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
    LogicalPixelCoordinates const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalPixelSize().height)
        * screenDeltaY
        * 1.5f; // More responsive

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalPixelSize().height)
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
    LogicalPixelCoordinates const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalPixelSize().height)
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalPixelSize().height)
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
    LogicalPixelCoordinates const & screenCoordinates,
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
    LogicalPixelCoordinates const & screenCoordinates,
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

void GameController::SawThrough(
    LogicalPixelCoordinates const & startScreenCoordinates,
    LogicalPixelCoordinates const & endScreenCoordinates,
    bool isFirstSegment)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->SawThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        isFirstSegment,
        mGameParameters);
}

bool GameController::ApplyHeatBlasterAt(
    LogicalPixelCoordinates const & screenCoordinates,
    HeatBlasterActionType action)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mGameParameters.HeatBlasterRadius;
    if (mGameParameters.IsUltraViolentMode)
        radius *= 10.0f;

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

bool GameController::ExtinguishFireAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mGameParameters.FireExtinguisherRadius;
    if (mGameParameters.IsUltraViolentMode)
        radius *= 10.0f;

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
    LogicalPixelCoordinates const & screenCoordinates,
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

void GameController::DrawTo(
    LogicalPixelCoordinates const & screenCoordinates,
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
    LogicalPixelCoordinates const & screenCoordinates,
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

void GameController::TogglePinAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mGameParameters);
}

bool GameController::InjectBubblesAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->InjectBubblesAt(
        worldCoordinates,
        mGameParameters);
}

bool GameController::FloodAt(
    LogicalPixelCoordinates const & screenCoordinates,
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

void GameController::ToggleAntiMatterBombAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleAntiMatterBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleImpactBombAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleImpactBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::TogglePhysicsProbeAt(LogicalPixelCoordinates const & screenCoordinates)
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

void GameController::ToggleRCBombAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleRCBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleTimerBombAt(LogicalPixelCoordinates const & screenCoordinates)
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

void GameController::AdjustOceanSurfaceTo(std::optional<LogicalPixelCoordinates> const & screenCoordinates)
{
    std::optional<vec2f> const worldCoordinates = !!screenCoordinates
        ? mRenderContext->ScreenToWorld(*screenCoordinates)
        : std::optional<vec2f>();

    // Apply action
    assert(!!mWorld);
    mWorld->AdjustOceanSurfaceTo(worldCoordinates);
}

std::optional<bool> GameController::AdjustOceanFloorTo(
    LogicalPixelCoordinates const & startScreenCoordinates,
    LogicalPixelCoordinates const & endScreenCoordinates)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    assert(!!mWorld);
    return mWorld->AdjustOceanFloorTo(
        startWorldCoordinates.x,
        startWorldCoordinates.y,
        endWorldCoordinates.x,
        endWorldCoordinates.y);
}

bool GameController::ScrubThrough(
    LogicalPixelCoordinates const & startScreenCoordinates,
    LogicalPixelCoordinates const & endScreenCoordinates)
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
    LogicalPixelCoordinates const & startScreenCoordinates,
    LogicalPixelCoordinates const & endScreenCoordinates)
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

void GameController::ApplyThanosSnapAt(LogicalPixelCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    StartThanosSnapStateMachine(worldCoordinates.x, mWorld->GetCurrentSimulationTime());
}

std::optional<ElementId> GameController::GetNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    return mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
}

void GameController::QueryNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const
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
    mWorld->TriggerLightning();
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
    int telegraphValue)
{
    assert(!!mWorld);
    mWorld->SetEngineControllerState(
        electricalElementId,
        telegraphValue,
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

void GameController::SetCanvasSize(LogicalPixelSize const & canvasSize)
{
    // Tell RenderContext
    mRenderContext->SetCanvasLogicalPixelSize(canvasSize);

    // Pickup eventual changes to view model properties
    mZoomParameterSmoother->SetValueImmediate(mRenderContext->GetZoom());
    mCameraWorldPositionParameterSmoother->SetValueImmediate(mRenderContext->GetCameraWorldPosition());
}

void GameController::Pan(LogicalPixelSize const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

    mCameraWorldPositionParameterSmoother->SetValue(
        newTargetCameraWorldPosition,
        GameWallClock::GetInstance().ContinuousNowAsFloat());
}

void GameController::PanImmediate(LogicalPixelSize const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void GameController::PanToWorldEnd(int side)
{
    vec2f const newTargetCameraWorldPosition = vec2f(
        side == 0
            ? -GameParameters::HalfMaxWorldWidth
            : GameParameters::HalfMaxWorldWidth,
        mCameraWorldPositionParameterSmoother->GetValue().y);

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void GameController::ResetPan()
{
    vec2f const newTargetCameraWorldPosition = vec2f(0, 0);

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void GameController::AdjustZoom(float amount)
{
    float const newTargetZoom = mZoomParameterSmoother->GetValue() * amount;

    mZoomParameterSmoother->SetValue(
        newTargetZoom,
        GameWallClock::GetInstance().ContinuousNowAsFloat());
}

void GameController::ResetZoom()
{
    float const newTargetZoom = 1.0f;

    mZoomParameterSmoother->SetValueImmediate(newTargetZoom);
}

vec2f GameController::ScreenToWorld(LogicalPixelCoordinates const & screenCoordinates) const
{
    return mRenderContext->ScreenToWorld(screenCoordinates);
}

vec2f GameController::ScreenOffsetToWorldOffset(LogicalPixelSize const & screenOffset) const
{
    return mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
}

////////////////////////////////////////////////////////////////////////////////////////

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

ShipMetadata GameController::ResetAndLoadShip(
    std::filesystem::path const & shipDefinitionFilepath,
    StrongTypedBool<struct DoAutoZoom> doAutoZoom)
{
    assert(!!mWorld);

    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

    // Pre-validate ship's texture
    if (shipDefinition.TextureLayerImage.has_value())
        mRenderContext->ValidateShipTexture(*shipDefinition.TextureLayerImage);

    // Save metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        OceanFloorTerrain(mWorld->GetOceanFloorTerrain()),
        mFishSpeciesDatabase,
        mGameEventDispatcher,
        std::make_shared<TaskThreadPool>(),
        mGameParameters,
        mRenderContext->GetVisibleWorld());

    // Add ship to new world
    auto [shipId, textureImage] = newWorld->AddShip(
        std::move(shipDefinition),
        mMaterialDatabase,
        mShipTexturizer,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    Reset(std::move(newWorld));

    OnShipAdded(
        shipId,
        std::move(textureImage),
        shipMetadata,
        doAutoZoom);

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
    ResetStateMachines();

    // Reset notification layer
    mNotificationLayer.Reset();

    // Reset rendering engine
    assert(!!mRenderContext);
    mRenderContext->Reset();

    // Notify
    mGameEventDispatcher->OnGameReset();
}

void GameController::OnShipAdded(
    ShipId shipId,
    RgbaImageData && textureImage,
    ShipMetadata const & shipMetadata,
    StrongTypedBool<struct DoAutoZoom> doAutoZoom)
{
    // Auto-zoom (if requested)
    if (doAutoZoom && mDoAutoZoomOnShipLoad)
    {
        // Calculate zoom to fit width and height (plus a nicely-looking margin)
        vec2f const shipSize = mWorld->GetShipSize(shipId);
        float const newZoom = std::min(
            mRenderContext->CalculateZoomForWorldWidth(shipSize.x + 5.0f),
            mRenderContext->CalculateZoomForWorldHeight(shipSize.y + 3.0f));

        // If calculated zoom requires zoom out: use it
        if (newZoom <= this->mRenderContext->GetZoom())
        {
            mZoomParameterSmoother->SetValueImmediate(newZoom);
        }
        else if (newZoom > 1.0f)
        {
            // Would need to zoom-in closer...
            // ...let's stop at 1.0 then
            mZoomParameterSmoother->SetValueImmediate(1.0f);
        }
    }

    // Add ship to rendering engine
    mRenderContext->AddShip(
        shipId,
        mWorld->GetShipPointCount(shipId),
        std::move(textureImage));

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