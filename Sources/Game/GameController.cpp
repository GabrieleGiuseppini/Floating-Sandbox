/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include "ComputerCalibration.h"
#include "ShipDeSerializer.h"

#include <Simulation/ShipFactory.h>

#include <Render/GameTextureDatabases.h>

#include <Core/Conversions.h>
#include <Core/GameMath.h>
#include <Core/Log.h>
#include <Core/TextureAtlas.h>

#include <ctime>
#include <iomanip>
#include <sstream>

std::unique_ptr<GameController> GameController::Create(
    RenderDeviceProperties const & renderDeviceProperties,
    ThreadManager & threadManager,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
{
    // Load material database
    MaterialDatabase materialDatabase = MaterialDatabase::Load(gameAssetManager);

    // Load fish species database
    FishSpeciesDatabase fishSpeciesDatabase = FishSpeciesDatabase::Load(gameAssetManager);

    // Load NPC teture atlas
    auto npcTextureAtlas = TextureAtlas<GameTextureDatabases::NpcTextureDatabase>::Deserialize(gameAssetManager);

    // Load NPC database
    NpcDatabase npcDatabase = NpcDatabase::Load(
        gameAssetManager,
        materialDatabase,
        npcTextureAtlas);

    // Create perf stats
    std::unique_ptr<PerfStats> perfStats = std::make_unique<PerfStats>();

    // Create render context
    std::unique_ptr<RenderContext> renderContext = std::make_unique<RenderContext>(
        renderDeviceProperties,
        FloatSize(SimulationParameters::MaxWorldWidth, SimulationParameters::MaxWorldHeight),
        std::move(npcTextureAtlas),
        *perfStats,
        threadManager,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.9f)); // Progress: 0.0-0.9

    //
    // Create controller
    //

    return std::unique_ptr<GameController>(
        new GameController(
            std::move(renderContext),
            std::move(perfStats),
            std::move(fishSpeciesDatabase),
            std::move(npcDatabase),
            std::move(materialDatabase),
            threadManager,
            gameAssetManager,
            progressCallback));
}

GameController::GameController(
    std::unique_ptr<RenderContext> renderContext,
    std::unique_ptr<PerfStats> perfStats,
    FishSpeciesDatabase && fishSpeciesDatabase,
    NpcDatabase && npcDatabase,
    MaterialDatabase && materialDatabase,
    ThreadManager & threadManager,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    // State machines
    : mTsunamiNotificationStateMachine()
    , mThanosSnapStateMachines()
    , mDayLightCycleStateMachine()
    // World
    , mWorld()
    , mFishSpeciesDatabase(std::move(fishSpeciesDatabase))
    , mNpcDatabase(std::move(npcDatabase))
    , mMaterialDatabase(std::move(materialDatabase))
    // Ship factory
    , mShipStrengthRandomizer()
    , mShipTexturizer(mMaterialDatabase, gameAssetManager)
    // State
    , mSimulationParameters()
    , mIsFrozen(false)
    , mIsPaused(false)
    , mIsPulseUpdateSet(false)
    , mIsMoveToolEngaged(false)
    // Parameters that we own
    , mTimeOfDay(0.0f) // We'll set it later
    , mIsShiftOn(false)
    , mDoShowTsunamiNotifications(true)
    , mDoShowNpcNotifications(true)
    , mDoDrawHeatBlasterFlame(true)
    , mAutoFocusTarget(AutoFocusTargetKindType::Ship) // Default: continuous auto-focus is ON on ships
    // Doers
    , mRenderContext(std::move(renderContext))
    , mSimulationEventDispatcher()
    , mGameEventDispatcher()
    , mNotificationLayer(
        mSimulationParameters.IsUltraViolentMode,
        false /*isSoundMuted; loaded value will come later*/,
        mSimulationParameters.DoDayLightCycle,
        false /*isAutoFocusOn; loaded value will come later*/,
        mIsShiftOn,
        mRenderContext->GetDisplayUnitsSystem(),
        mSimulationEventDispatcher)
    , mThreadManager(threadManager)
    , mViewManager(mAutoFocusTarget, *mRenderContext)
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
        OceanFloorHeightMap::LoadFromImage(gameAssetManager.LoadPngImageRgb(gameAssetManager.GetDefaultOceanFloorHeightMapFilePath())),
        mFishSpeciesDatabase,
        mRenderContext->GetUnderwaterPlantsSpeciesCount(),
        mNpcDatabase,
        mSimulationEventDispatcher,
        mSimulationParameters);

    // Register ourselves as event handler for the events we care about
    mSimulationEventDispatcher.RegisterGenericShipEventHandler(this);
    mSimulationEventDispatcher.RegisterWavePhenomenaEventHandler(this);
    mSimulationEventDispatcher.RegisterNpcEventHandler(this);

    //
    // Initialize parameter smoothers
    //

    float constexpr GenericParameterConvergenceFactor = 0.1f;
    float constexpr GenericParameterTerminationThreshold = 0.0005f;

    assert(mFloatParameterSmoothers.size() == SpringStiffnessAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.SpringStiffnessAdjustment;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.SpringStiffnessAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == SpringStrengthAdjustmentParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.SpringStrengthAdjustment;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.SpringStrengthAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == SeaDepthParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.SeaDepth;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.SeaDepth = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == OceanFloorBumpinessParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.OceanFloorBumpiness;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.OceanFloorBumpiness = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == OceanFloorDetailAmplificationParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.OceanFloorDetailAmplification;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.OceanFloorDetailAmplification = value;
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
            return this->mSimulationParameters.BasalWaveHeightAdjustment;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.BasalWaveHeightAdjustment = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == FishSizeMultiplierParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.FishSizeMultiplier;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.FishSizeMultiplier = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    assert(mFloatParameterSmoothers.size() == NpcSizeMultiplierParameterSmoother);
    mFloatParameterSmoothers.emplace_back(
        [this]() -> float const &
        {
            return this->mSimulationParameters.NpcSizeMultiplier;
        },
        [this](float const & value)
        {
            this->mSimulationParameters.NpcSizeMultiplier = value;
        },
        GenericParameterConvergenceFactor,
        GenericParameterTerminationThreshold);

    //
    // Calibrate game
    //

    progressCallback(1.0f, ProgressMessageType::Calibrating);

    auto const & score = ComputerCalibrator::Calibrate();

    ComputerCalibrator::TuneGame(score, mSimulationParameters, *mRenderContext);

    //
    // Reconcialiate notifications with startup parameters
    //

    mNotificationLayer.SetAutoFocusIndicator(mAutoFocusTarget.has_value());
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

ShipMetadata GameController::ResetAndLoadShip(
    ShipLoadSpecifications const & loadSpecs,
    IAssetManager const & assetManager)
{
    return InternalResetAndLoadShip(loadSpecs, assetManager);
}

ShipMetadata GameController::ResetAndReloadShip(
    ShipLoadSpecifications const & loadSpecs,
    IAssetManager const & assetManager)
{
    return InternalResetAndLoadShip(loadSpecs, assetManager);
}

ShipMetadata GameController::AddShip(
    ShipLoadSpecifications const & loadSpecs,
    IAssetManager const & assetManager)
{
    // Load ship definition
    auto shipDefinition = ShipDeSerializer::LoadShip(loadSpecs.DefinitionFilepath, mMaterialDatabase);

    // Pre-validate ship's textures, if any
    if (shipDefinition.Layers.ExteriorTextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.ExteriorTextureLayer->Buffer);
    if (shipDefinition.Layers.InteriorTextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.InteriorTextureLayer->Buffer);

    // Remember metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    //
    // Produce ship
    //

    auto const shipId = mWorld->GetNextShipId();

    auto [ship, exteriorTextureImage, interiorViewImage] = ShipFactory::Create(
        shipId,
        *mWorld,
        std::move(shipDefinition),
        loadSpecs.LoadOptions,
        mMaterialDatabase,
        mShipTexturizer,
        mShipStrengthRandomizer,
        mSimulationEventDispatcher,
        assetManager,
        mSimulationParameters);

    //
    // No errors, so we may continue
    //

    InternalAddShip(
        std::move(ship),
        std::move(exteriorTextureImage),
        std::move(interiorViewImage),
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
        mOriginTimestampGame = GameWallClock::GetInstance().Now();

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
        auto const startTime = GameChronometer::Now();

        // Tell RenderContext we're starting an update
        // (waits until the last upload has completed)
        mRenderContext->UpdateStart();

        auto const netStartTime = GameChronometer::Now();

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
            mSimulationParameters,
            mRenderContext->GetViewModel(),
            mRenderContext->GetStressRenderMode(),
            mThreadManager,
            *mTotalPerfStats);

        // Flush events
        mSimulationEventDispatcher.Flush();
        mGameEventDispatcher.Flush();

        //
        // Update misc
        //

        // Update state machines
        UpdateAllStateMachines(mWorld->GetCurrentSimulationTime());

        // Update notification layer
        mNotificationLayer.Update(nowGame, mWorld->GetCurrentSimulationTime());

        // Tell RenderContext we've finished an update
        mRenderContext->UpdateEnd();

        auto const endTime = GameChronometer::Now();
        mTotalPerfStats->Update<PerfMeasurement::TotalNetUpdate>(endTime - netStartTime);
        mTotalPerfStats->Update<PerfMeasurement::TotalUpdate>(endTime - startTime);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Render Upload
    ////////////////////////////////////////////////////////////////////////////

    // Tell RenderContext we're starting a new rendering cycle
    mRenderContext->RenderStart();

    {
        mRenderContext->UploadStart();

        auto const netStartTime = GameChronometer::Now();

        // Update view manager
        // Note: some Upload()'s need to use ViewModel values, which have then to match the
        // ViewModel values used by the subsequent render
        UpdateAutoFocus();
        mViewManager.Update();

        //
        // Upload world
        //

        assert(!!mWorld);
        mWorld->RenderUpload(
            mSimulationParameters,
            *mRenderContext);

        //
        // Upload notification layer
        //

        mNotificationLayer.RenderUpload(*mRenderContext);

        mRenderContext->UploadEnd();

        mTotalPerfStats->Update<PerfMeasurement::TotalNetRenderUpload>(GameChronometer::Now() - netStartTime);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Render Draw
    ////////////////////////////////////////////////////////////////////////////

    {
        auto const startTime = GameChronometer::Now();

        // Render
        mRenderContext->Draw(mWorld->GetCurrentSimulationTime());

        mTotalPerfStats->Update<PerfMeasurement::TotalMainThreadRenderDraw>(GameChronometer::Now() - startTime);
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
        mSimulationParameters); // NOTE: using now's game parameters...but we don't want to capture these in the recorded event (at least at this moment)
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
    mNotificationLayer.PublishNotificationText("SETTINGS LOADED");
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

void GameController::DisplayEphemeralTextLine(std::string const & text)
{
    mNotificationLayer.PublishNotificationText(text);
}

void GameController::NotifySoundMuted(bool isSoundMuted)
{
    mNotificationLayer.SetSoundMuteIndicator(isSoundMuted);
}

bool GameController::IsShiftOn() const
{
    return mIsShiftOn;
}

void GameController::SetShiftOn(bool value)
{
    mIsShiftOn = value;
    mNotificationLayer.SetShiftIndicator(mIsShiftOn);
}

void GameController::ShowInteractiveToolDashedLine(
    DisplayLogicalCoordinates const & start,
    DisplayLogicalCoordinates const & end)
{
    mNotificationLayer.ShowInteractiveToolDashedLine(start, end);
}

void GameController::ShowInteractiveToolDashedRect(
    DisplayLogicalCoordinates const & corner1,
    DisplayLogicalCoordinates const & corner2)
{
    mNotificationLayer.ShowInteractiveToolDashedLine(corner1, { corner2.x, corner1.y });
    mNotificationLayer.ShowInteractiveToolDashedLine({ corner2.x, corner1.y }, corner2);
    mNotificationLayer.ShowInteractiveToolDashedLine(corner2, { corner1.x, corner2.y });
    mNotificationLayer.ShowInteractiveToolDashedLine({ corner1.x, corner2.y }, corner1);
}

void GameController::ToggleToFullDayOrNight()
{
    if (mTimeOfDay >= 0.5f)
        SetTimeOfDay(0.0f);
    else
        SetTimeOfDay(1.0f);
}

void GameController::PickObjectToMove(
    DisplayLogicalCoordinates const & screenCoordinates,
    std::optional<GlobalConnectedComponentId> & connectedComponentId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);
    float const worldSearchRadius = mRenderContext->ScreenOffsetToWorldOffset(mSimulationParameters.ShipSearchRadiusScreen);

    // Apply action
    assert(!!mWorld);
    mWorld->PickConnectedComponentToMove(
        worldCoordinates,
        worldSearchRadius,
        connectedComponentId);
}

std::optional<GlobalElementId> GameController::PickObjectForPickAndPull(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);
    float const worldSearchRadius = mRenderContext->ScreenOffsetToWorldOffset(mSimulationParameters.ShipSearchRadiusScreen);

    // Apply action
    assert(!!mWorld);
    return mWorld->PickObjectForPickAndPull(
        worldCoordinates,
        worldSearchRadius);
}

void GameController::Pull(
    GlobalElementId elementId,
    DisplayLogicalCoordinates const & screenTarget)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenTarget);

    // Apply action
    assert(!!mWorld);
    mWorld->Pull(
        elementId,
        worldCoordinates,
        mSimulationParameters);
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
    GlobalConnectedComponentId const & connectedComponentId,
    DisplayLogicalSize const & screenOffset,
    DisplayLogicalSize const & inertialScreenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
    vec2f const inertialVelocity = mRenderContext->ScreenOffsetToWorldOffset(inertialScreenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        connectedComponentId,
        worldOffset,
        inertialVelocity,
        mSimulationParameters);
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
        mSimulationParameters);
}

void GameController::RotateBy(
    GlobalConnectedComponentId const & connectedComponentId,
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
        connectedComponentId,
        angle,
        worldCenter,
        inertialAngle,
        mSimulationParameters);
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
        mSimulationParameters);
}

std::tuple<vec2f, float> GameController::SetupMoveGrippedBy(
    DisplayLogicalCoordinates const & screenGripCenter,
    DisplayLogicalSize const & screenGripOffset)
{
    vec2f const worldGripCenter = mRenderContext->ScreenToWorld(screenGripCenter);
    float const worldGripRadius = mRenderContext->ScreenOffsetToWorldOffset(screenGripOffset).length();

    // Notify
    mNotificationLayer.SetGripCircle(
        worldGripCenter,
        worldGripRadius);

    return { worldGripCenter, worldGripRadius };
}

void GameController::MoveGrippedBy(
    vec2f const & worldGripCenter,
    float worldGripRadius,
    DisplayLogicalSize const & screenOffset,
    DisplayLogicalSize const & inertialScreenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
    vec2f const inertialWorldOffset = mRenderContext->ScreenOffsetToWorldOffset(inertialScreenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveGrippedBy(
        {
            GrippedMoveParameters{
                worldGripCenter,
                worldGripRadius,
                worldOffset,
                inertialWorldOffset / SimulationParameters::SimulationStepTimeDuration<float>
            }
        },
        mSimulationParameters);

    // Notify
    mNotificationLayer.SetGripCircle(
        worldGripCenter,
        worldGripRadius);
}

void GameController::RotateGrippedBy(
    vec2f const & worldGripCenter,
    float worldGripRadius,
    float screenDeltaY,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * screenDeltaY;

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasLogicalSize().height)
        * inertialScreenDeltaY;

    // Apply action
    assert(!!mWorld);
    mWorld->RotateGrippedBy(
        worldGripCenter,
        worldGripRadius,
        angle,
        inertialAngle,
        mSimulationParameters);

    // Notify
    mNotificationLayer.SetGripCircle(
        worldGripCenter,
        worldGripRadius);
}

void GameController::EndMoveGrippedBy()
{
    // Apply action
    assert(!!mWorld);
    mWorld->EndMoveGrippedBy(mSimulationParameters);
}

void GameController::DestroyAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusMultiplier,
    SessionId const & sessionId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float const worldRadius =
        mSimulationParameters.DestroyRadius
        * radiusMultiplier
        * (mSimulationParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    // Apply action
    assert(!!mWorld);
    mWorld->DestroyAt(
        worldCoordinates,
        worldRadius,
        sessionId,
        mSimulationParameters);
}

void GameController::RepairAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusMultiplier,
    SequenceNumber repairStepId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float const worldRadius =
        mSimulationParameters.RepairRadius
        * radiusMultiplier;

    // Apply action
    assert(!!mWorld);
    mWorld->RepairAt(
        worldCoordinates,
        worldRadius,
        repairStepId,
        mSimulationParameters);
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
        mSimulationParameters);
}

bool GameController::ApplyHeatBlasterAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    HeatBlasterActionType action)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mSimulationParameters.HeatBlasterRadius;
    if (mSimulationParameters.IsUltraViolentMode)
    {
        radius *= 5.0f;
    }

    // Apply action
    assert(!!mWorld);
    bool isApplied = mWorld->ApplyHeatBlasterAt(
        worldCoordinates,
        action,
        radius,
        mSimulationParameters);

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

bool GameController::ExtinguishFireAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float strengthMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Calculate radius
    float radius = mSimulationParameters.FireExtinguisherRadius;
    if (mSimulationParameters.IsUltraViolentMode)
    {
        radius *= 5.0f;
    }

    // Apply action
    assert(!!mWorld);
    bool isApplied = mWorld->ExtinguishFireAt(
        worldCoordinates,
        strengthMultiplier,
        radius,
        mSimulationParameters);

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
        mSimulationParameters.BlastToolRadius
        * radiusMultiplier
        * (mSimulationParameters.IsUltraViolentMode ? 2.5f : 1.0f);

    // Apply action
    assert(!!mWorld);
    mWorld->ApplyBlastAt(
        worldCoordinates,
        radius,
        forceMultiplier,
        mSimulationParameters);

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
    float lengthMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->ApplyElectricSparkAt(
        worldCoordinates,
        counter,
        lengthMultiplier,
        mSimulationParameters);
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
        Conversions::KmhToMs(mSimulationParameters.WindMakerToolWindSpeed)
        * (mSimulationParameters.IsUltraViolentMode ? 3.5f : 1.0f);
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
        mSimulationParameters);

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
            mSimulationParameters);
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
        mSimulationParameters);
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
        mSimulationParameters);
}

static int constexpr AntiGravityFieldSearchRadiusPixels = 25; // To have a screen- constant-size geometry

ElementIndex GameController::BeginPlaceAntiGravityField(DisplayLogicalCoordinates const & startScreenCoordinates)
{
    vec2f const worldStartCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    float const worldSearchRadius = mRenderContext->ScreenOffsetToWorldOffset(AntiGravityFieldSearchRadiusPixels);

    // Apply action
    assert(!!mWorld);
    return mWorld->BeginPlaceAntiGravityField(worldStartCoordinates, worldSearchRadius);
}

void GameController::UpdatePlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    DisplayLogicalCoordinates const & endScreenCoordinates)
{
    vec2f const worldEndCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->UpdatePlaceAntiGravityField(antiGravityFieldId, worldEndCoordinates);
}

void GameController::EndPlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    DisplayLogicalCoordinates const & endScreenCoordinates)
{
    vec2f const worldEndCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);
    float const worldSearchRadius = mRenderContext->ScreenOffsetToWorldOffset(AntiGravityFieldSearchRadiusPixels);

    // Apply action
    assert(!!mWorld);
    mWorld->EndPlaceAntiGravityField(antiGravityFieldId, worldEndCoordinates, worldSearchRadius);
}

void GameController::AbortPlaceAntiGravityField(ElementIndex antiGravityFieldId)
{
    // Apply action
    assert(!!mWorld);
    mWorld->AbortPlaceAntiGravityField(antiGravityFieldId);
}

void GameController::TogglePinAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mSimulationParameters);
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
        mSimulationParameters);

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
    float flowSign)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->FloodAt(
        worldCoordinates,
        mSimulationParameters.FloodRadius,
        flowSign,
        mSimulationParameters);
}

void GameController::ToggleAntiMatterBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleAntiMatterBombAt(
        worldCoordinates,
        mSimulationParameters);
}

void GameController::ToggleFireExtinguishingBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleFireExtinguishingBombAt(
        worldCoordinates,
        mSimulationParameters);
}

void GameController::ToggleImpactBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleImpactBombAt(
        worldCoordinates,
        mSimulationParameters);
}

void GameController::TogglePhysicsProbeAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    auto const toggleResult = mWorld->TogglePhysicsProbeAt(
        worldCoordinates,
        mSimulationParameters);

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
        mSimulationParameters);
}

void GameController::ToggleTimerBombAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleTimerBombAt(
        worldCoordinates,
        mSimulationParameters);
}

void GameController::DetonateRCBombs()
{
    // Apply action
    assert(!!mWorld);
    mWorld->DetonateRCBombs(mSimulationParameters);
}

void GameController::DetonateAntiMatterBombs()
{
    // Apply action
    assert(!!mWorld);
    mWorld->DetonateAntiMatterBombs();
}

void GameController::ApplyInteractiveWaveAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    int screenRadius)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);
    float const worldRadius = mRenderContext->ScreenOffsetToWorldOffset(screenRadius);

    // Apply action
    assert(mWorld);
    mWorld->ApplyInteractiveWaveAt(worldCoordinates, worldRadius);
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

    float const worldRadius = mSimulationParameters.ScrubRotToolRadius; // Yes, using the same for symmetry

    // Apply action
    assert(!!mWorld);
    return mWorld->ScrubThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        worldRadius);
}

bool GameController::RotThrough(
    DisplayLogicalCoordinates const & startScreenCoordinates,
    DisplayLogicalCoordinates const & endScreenCoordinates)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    float const worldRadius = mSimulationParameters.ScrubRotToolRadius; // Yes, using the same for symmetry

    // Apply action
    assert(!!mWorld);
    return mWorld->RotThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        worldRadius,
        mSimulationParameters);
}

void GameController::ApplyThanosSnapAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    bool isSparseMode)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    StartThanosSnapStateMachine(worldCoordinates.x, isSparseMode, mWorld->GetCurrentSimulationTime());
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

void GameController::SetLampAt(
    DisplayLogicalCoordinates const & screenCoordinates,
    float radiusScreenFraction)
{
    mRenderContext->SetLamp(screenCoordinates, radiusScreenFraction);

     // Scare fish
    assert(!!mWorld);
    mWorld->ScareFish(
        mRenderContext->ScreenToWorld(screenCoordinates),
        mRenderContext->ScreenFractionToWorldOffset(radiusScreenFraction),
        std::chrono::milliseconds(75));
}

void GameController::ResetLamp()
{
    mRenderContext->ResetLamp();
}

void GameController::TriggerLightningAt(DisplayLogicalCoordinates const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Do action
    assert(!!mWorld);
    mWorld->TriggerLightningAt(worldCoordinates);
}

NpcKindType GameController::GetNpcKind(NpcId id)
{
    assert(!!mWorld);
    return mWorld->GetNpcKind(id);
}

std::optional<PickedNpc> GameController::BeginPlaceNewFurnitureNpc(
    std::optional<NpcSubKindIdType> subKind,
    DisplayLogicalCoordinates const & screenCoordinates,
    bool doMoveWholeMesh)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    auto const result = mWorld->BeginPlaceNewFurnitureNpc(
        subKind,
        worldCoordinates,
        doMoveWholeMesh || mIsPaused);

    if (result.Npc.has_value())
    {
        OnBeginPlaceNewNpc(result.Npc->Id, true);
        return result.Npc;
    }
    else
    {
        NotifyNpcPlacementError(result.FailureReason);
        return std::nullopt;
    }
}

std::optional<PickedNpc> GameController::BeginPlaceNewHumanNpc(
    std::optional<NpcSubKindIdType> subKind,
    DisplayLogicalCoordinates const & screenCoordinates,
    bool doMoveWholeMesh)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    auto const result = mWorld->BeginPlaceNewHumanNpc(
        subKind,
        worldCoordinates,
        doMoveWholeMesh || mIsPaused);

    if (result.Npc.has_value())
    {
        OnBeginPlaceNewNpc(result.Npc->Id, true);
        return result.Npc;
    }
    else
    {
        NotifyNpcPlacementError(result.FailureReason);
        return std::nullopt;
    }
}

std::optional<PickedNpc> GameController::ProbeNpcAt(DisplayLogicalCoordinates const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);
    float const npcProbeSearchRadius = 1.0f / std::sqrtf(mRenderContext->GetZoom());

    assert(!!mWorld);
    return mWorld->ProbeNpcAt(
        worldCoordinates,
        npcProbeSearchRadius,
        mSimulationParameters);
}

std::vector<NpcId> GameController::ProbeNpcsInRect(
    DisplayLogicalCoordinates const & corner1ScreenCoordinates,
    DisplayLogicalCoordinates const & corner2ScreenCoordinates) const
{
    vec2f const corner1WorldCoordinates = mRenderContext->ScreenToWorld(corner1ScreenCoordinates);
    vec2f const corner2WorldCoordinates = mRenderContext->ScreenToWorld(corner2ScreenCoordinates);

    assert(!!mWorld);
    return mWorld->ProbeNpcsInRect(corner1WorldCoordinates, corner2WorldCoordinates);
}

void GameController::BeginMoveNpc(
    NpcId id,
    int particleOrdinal,
    bool doMoveWholeMesh)
{
    assert(!!mWorld);
    mWorld->BeginMoveNpc(
        id,
        particleOrdinal,
        doMoveWholeMesh || mIsPaused);
}

void GameController::BeginMoveNpcs(std::vector<NpcId> const & ids)
{
    assert(!!mWorld);
    mWorld->BeginMoveNpcs(ids);
}

void GameController::MoveNpcTo(
    NpcId id,
    DisplayLogicalCoordinates const & screenCoordinates,
    vec2f const & worldOffset,
    bool doMoveWholeMesh)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    mWorld->MoveNpcTo(
        id,
        worldCoordinates,
        worldOffset,
        doMoveWholeMesh || mIsPaused);
}

void GameController::MoveNpcsBy(
    std::vector<NpcId> const & ids,
    DisplayLogicalSize const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    assert(!!mWorld);
    mWorld->MoveNpcsBy(
        ids,
        worldOffset);
}

void GameController::EndMoveNpc(NpcId id)
{
    assert(!!mWorld);
    mWorld->EndMoveNpc(id);
}

void GameController::CompleteNewNpc(NpcId id)
{
    assert(!!mWorld);
    mWorld->CompleteNewNpc(id);
}

void GameController::RemoveNpc(NpcId id)
{
    assert(!!mWorld);
    mWorld->RemoveNpc(id);
}

void GameController::RemoveNpcsInRect(
    DisplayLogicalCoordinates const & corner1ScreenCoordinates,
    DisplayLogicalCoordinates const & corner2ScreenCoordinates)
{
    vec2f const corner1WorldCoordinates = mRenderContext->ScreenToWorld(corner1ScreenCoordinates);
    vec2f const corner2WorldCoordinates = mRenderContext->ScreenToWorld(corner2ScreenCoordinates);

    assert(!!mWorld);
    mWorld->RemoveNpcsInRect(corner1WorldCoordinates, corner2WorldCoordinates);
}

void GameController::AbortNewNpc(NpcId id)
{
    assert(!!mWorld);
    mWorld->AbortNewNpc(id);
}

void GameController::AddNpcGroup(NpcKindType kind)
{
    assert(!!mWorld);
    auto const result = mWorld->AddNpcGroup(
        kind,
        mRenderContext->GetVisibleWorld(),
        mSimulationParameters);

    if (result != NpcPlacementFailureReasonType::Success)
    {
        NotifyNpcPlacementError(result);
    }
}

void GameController::TurnaroundNpc(NpcId id)
{
    assert(!!mWorld);
    mWorld->TurnaroundNpc(id);
}

void GameController::TurnaroundNpcsInRect(
    DisplayLogicalCoordinates const & corner1ScreenCoordinates,
    DisplayLogicalCoordinates const & corner2ScreenCoordinates)
{
    vec2f const corner1WorldCoordinates = mRenderContext->ScreenToWorld(corner1ScreenCoordinates);
    vec2f const corner2WorldCoordinates = mRenderContext->ScreenToWorld(corner2ScreenCoordinates);

    assert(!!mWorld);
    mWorld->TurnaroundNpcsInRect(corner1WorldCoordinates, corner2WorldCoordinates);
}

std::optional<NpcId> GameController::GetCurrentlySelectedNpc() const
{
    assert(!!mWorld);
    return mWorld->GetSelectedNpc();
}

void GameController::SelectNpc(std::optional<NpcId> id)
{
    assert(!!mWorld);
    mWorld->SelectNpc(id);

    if (mViewManager.GetAutoFocusTarget() == AutoFocusTargetKindType::SelectedNpc)
    {
        mViewManager.ResetAutoFocusAlterations();
    }
}

void GameController::SelectNextNpc()
{
    assert(!!mWorld);
    mWorld->SelectNextNpc(); // We'll pick this up later at UpdateAutoFocus() if we're focusing on it

    if (mViewManager.GetAutoFocusTarget() == AutoFocusTargetKindType::SelectedNpc)
    {
        mViewManager.ResetAutoFocusAlterations();
    }
}

void GameController::HighlightNpcs(std::vector<NpcId> const & ids)
{
    assert(!!mWorld);
    mWorld->HighlightNpcs(ids);
}

void GameController::HighlightNpcsInRect(
    DisplayLogicalCoordinates const & corner1ScreenCoordinates,
    DisplayLogicalCoordinates const & corner2ScreenCoordinates)
{
    vec2f const corner1WorldCoordinates = mRenderContext->ScreenToWorld(corner1ScreenCoordinates);
    vec2f const corner2WorldCoordinates = mRenderContext->ScreenToWorld(corner2ScreenCoordinates);

    assert(!!mWorld);
    mWorld->HighlightNpcsInRect(corner1WorldCoordinates, corner2WorldCoordinates);
}

std::optional<GlobalElementId> GameController::GetNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const
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

void GameController::QueryNearestNpcAt(DisplayLogicalCoordinates const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    mWorld->QueryNearestNpcAt(worldCoordinates, 1.0f);
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
    mWorld->TriggerLightning(mSimulationParameters);
}

void GameController::HighlightElectricalElement(GlobalElectricalElementId electricalElementId)
{
    assert(!!mWorld);
    mWorld->HighlightElectricalElement(electricalElementId);
}

void GameController::SetSwitchState(
    GlobalElectricalElementId electricalElementId,
    ElectricalState switchState)
{
    assert(!!mWorld);
    mWorld->SetSwitchState(
        electricalElementId,
        switchState,
        mSimulationParameters);
}

void GameController::SetEngineControllerState(
    GlobalElectricalElementId electricalElementId,
    float controllerValue)
{
    assert(!!mWorld);
    mWorld->SetEngineControllerState(
        electricalElementId,
        controllerValue,
        mSimulationParameters);
}

bool GameController::DestroyTriangle(GlobalElementId triangleId)
{
    assert(!!mWorld);
    return mWorld->DestroyTriangle(triangleId);
}

bool GameController::RestoreTriangle(GlobalElementId triangleId)
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
    switch (side)
    {
        case 0:
        {
            mViewManager.PanToWorldX(-SimulationParameters::HalfMaxWorldWidth);
            break;
        }

        case 1:
        {
            mViewManager.PanToWorldX(SimulationParameters::HalfMaxWorldWidth);
            break;
        }

        case 2:
        {
            mViewManager.PanToWorldY(SimulationParameters::HalfMaxWorldHeight);
            break;
        }

        default:
        {
            mViewManager.PanToWorldY(-SimulationParameters::HalfMaxWorldHeight);
            break;
        }
    }
}

void GameController::AdjustZoom(float amount)
{
    mViewManager.AdjustZoom(amount);
}

void GameController::ResetView()
{
    //
    // If there's auto-focus on <X>, we re-center; if not, we focus one-off on ships
    //

    // Calculate effective target (none if Ship, but no AABBs exist)
    auto const effectiveAutoFocusTarget = CalculateEffectiveAutoFocusTarget();

    // Set in view manager, unconditionally (resetting its state)
    mViewManager.SetAutoFocusTarget(effectiveAutoFocusTarget);

    if (effectiveAutoFocusTarget.has_value())
    {
        mViewManager.ResetAutoFocusAlterations();
    }
    else
    {
        FocusOnShips();
    }
}

void GameController::FocusOnShips()
{
    if (mWorld)
    {
        auto aabb = mWorld->GetAllShipExternalAABBs().MakeWeightedUnion();
        if (!aabb.has_value())
        {
            // No triangles - particle-only ship
            aabb = mWorld->CalculateAllShipParticleAABB();
        }

        assert(aabb.has_value());
        mViewManager.FocusOn(*aabb, 1.0f, 1.0f, 1.0f, 1.0f, false);
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

std::optional<AutoFocusTargetKindType> GameController::GetAutoFocusTarget() const
{
    return mAutoFocusTarget;
}

void GameController::SetAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & autoFocusTarget)
{
    if (autoFocusTarget == AutoFocusTargetKindType::SelectedNpc)
    {
        // Assumed to be invoked when at least an NPC exists
        // (thanks to UI constraints)
        assert(mWorld->GetNpcs().HasNpcs());

        // Select first NPC as a courtesy if none is selected
        if (!mWorld->GetNpcs().GetCurrentlySelectedNpc().has_value())
        {
            mWorld->SelectFirstNpc();
        }
    }

    // Switch
    InternalSwitchAutoFocusTarget(autoFocusTarget);
    mViewManager.SetAutoFocusTarget(autoFocusTarget);

    // Reset user offsets if we're switching to an actual auto-focus
    if (autoFocusTarget.has_value())
    {
        mViewManager.ResetAutoFocusAlterations();
    }
}

////////////////////////////////////////////////////////////////////////////////////////

void GameController::SetTimeOfDay(float value)
{
    assert(value >= 0.0f && value <= 1.0f);

    mTimeOfDay = value;

    // Calculate new ambient light
    mRenderContext->SetAmbientLightIntensity(
        SmoothStep(
            0.0f,
            1.0f,
            mTimeOfDay));

    // Calculate new sun rays inclination:
    // ToD = 1 => inclination = +1 (45 degrees)
    // ToD = 0 => inclination = -1 (45 degrees)
    mRenderContext->SetSunRaysInclination(2.0f * mTimeOfDay - 1.0f);
}

void GameController::SetDoDayLightCycle(bool value)
{
    mSimulationParameters.DoDayLightCycle = value;

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
    mNotificationLayer.PublishNotificationText("SHIP REPAIRED!");

    LogMessage("Ship repaired!");
}

void GameController::OnHumanNpcCountsUpdated(
    size_t insideShipCount,
    size_t outsideShipCount)
{
    if (mDoShowNpcNotifications)
    {
        std::stringstream ss;
        ss << insideShipCount << " IN/" << outsideShipCount << " OUT";
        mNotificationLayer.PublishNotificationText(ss.str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////

ShipMetadata GameController::InternalResetAndLoadShip(
    ShipLoadSpecifications const & loadSpecs,
    IAssetManager const & assetManager)
{
    assert(!!mWorld);

    // Load ship definition
    auto shipDefinition = ShipDeSerializer::LoadShip(loadSpecs.DefinitionFilepath, mMaterialDatabase);

    // Pre-validate ship's textures, if any
    if (shipDefinition.Layers.ExteriorTextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.ExteriorTextureLayer->Buffer);
    if (shipDefinition.Layers.InteriorTextureLayer)
        mRenderContext->ValidateShipTexture(shipDefinition.Layers.InteriorTextureLayer->Buffer);

    // Save metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        OceanFloorHeightMap(mWorld->GetOceanFloorHeightMap()),
        mFishSpeciesDatabase,
        mRenderContext->GetUnderwaterPlantsSpeciesCount(),
        mNpcDatabase,
        mSimulationEventDispatcher,
        mSimulationParameters);

    // Produce ship
    auto const shipId = newWorld->GetNextShipId();
    auto [ship, exteriorTextureImage, interiorViewImage] = ShipFactory::Create(
        shipId,
        *newWorld,
        std::move(shipDefinition),
        loadSpecs.LoadOptions,
        mMaterialDatabase,
        mShipTexturizer,
        mShipStrengthRandomizer,
        mSimulationEventDispatcher,
        assetManager,
        mSimulationParameters);

    //
    // No errors, so we may continue
    //

    Reset(std::move(newWorld));

    InternalAddShip(
        std::move(ship),
        std::move(exteriorTextureImage),
        std::move(interiorViewImage),
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

    // Reset stats
    ResetStats();

    // Reset notification layer
    mNotificationLayer.Reset();

    // Reset rendering engine
    assert(!!mRenderContext);
    mRenderContext->Reset();

    // Notify
    mGameEventDispatcher.OnGameReset();
}

void GameController::InternalAddShip(
    std::unique_ptr<Physics::Ship> ship,
    RgbaImageData && exteriorTextureImage,
    RgbaImageData && interiorViewImage,
    ShipMetadata const & shipMetadata)
{
    ShipId const shipId = ship->GetId();

    // Set recorder in ship (if any)
    ship->SetEventRecorder(mEventRecorder.get());

    // Add ship to our world
    mWorld->AddShip(std::move(ship));

    // Calculate ship particle render mode:
    //  - If at least one ship has at least one spring or one triangle, then Fragment mode;
    //  - Else: Particle mode
    if (mWorld->GetAllShipSpringCount() == 0 && mWorld->GetAllShipTriangleCount() == 0)
    {
        mRenderContext->SetShipParticleRenderMode(ShipParticleRenderModeType::Particle);
    }
    else
    {
        mRenderContext->SetShipParticleRenderMode(ShipParticleRenderModeType::Fragment);
    }

    // Add ship to rendering engine
    mRenderContext->AddShip(
        shipId,
        mWorld->GetShipPointCount(shipId),
        SimulationParameters::MaxEphemeralParticles,
        SimulationParameters::MaxSpringsPerPoint,
        std::move(exteriorTextureImage),
        std::move(interiorViewImage));

    // Tell view manager
    UpdateViewOnShipLoad();

    // Notify ship load
    mGameEventDispatcher.OnShipLoaded(
        shipId,
        shipMetadata);

    // Announce
    mWorld->Announce();
}

void GameController::ResetStats()
{
    mTotalPerfStats->Reset();
    mLastPublishedTotalPerfStats.Reset();
    mTotalFrameCount = 0;
    mLastPublishedTotalFrameCount = 0;
    mStatsOriginTimestampReal = std::chrono::steady_clock::time_point::min();
    mStatsLastTimestampReal = std::chrono::steady_clock::time_point::min();
    mSkippedFirstStatPublishes = 0;
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
    mGameEventDispatcher.OnFrameRateUpdated(lastFps, totalFps);

    // Publish update time
    mGameEventDispatcher.OnCurrentUpdateDurationUpdated(lastDeltaPerfStats.GetMeasurement<PerfMeasurement::TotalUpdate>().ToRatio<std::chrono::milliseconds>());

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

void GameController::OnBeginPlaceNewNpc(
    NpcId const & npcId,
    bool doAnchorToScreen)
{
    if (mViewManager.GetDoAutoFocusOnNpcPlacement()
        && !mViewManager.GetAutoFocusTarget().has_value())
    {
        // Focus on this NPC (one-off)

        // We want to zoom so that the NPC appears as large as 1/this of screen
        float constexpr NpcMagnification = 13.0f;

        assert(!!mWorld);
        auto const aabb = mWorld->GetNpcs().GetNpcAABB(npcId);
        mViewManager.FocusOn(aabb, NpcMagnification, NpcMagnification, 1.0f / 8.0f, 2.0f, doAnchorToScreen);
    }
}

void GameController::NotifyNpcPlacementError(NpcPlacementFailureReasonType reason)
{
    switch (reason)
    {
        case NpcPlacementFailureReasonType::Success:
        {
            assert(false);
            break;
        }

        case NpcPlacementFailureReasonType::TooManyNpcs:
        {
            mNotificationLayer.PublishNotificationText("TOO MANY NPCS!");
            break;
        }

        case NpcPlacementFailureReasonType::TooManyCaptains:
        {
            mNotificationLayer.PublishNotificationText("TOO MANY CAPTAINS!");
            break;
        }
    }
}

void GameController::UpdateViewOnShipLoad()
{
    // Update limit auto-focus on ship limit
    auto const largestAABB = mWorld->GetLargestShipExternalAABB();
    if (largestAABB.has_value())
    {
        mViewManager.SetAutoFocusOnShipLimitAABB(*largestAABB);
    }

    //////////////////////////////////

    // Calculate effective target (none if Ship, but no AABBs exist)
    auto const effectiveAutoFocusTarget = CalculateEffectiveAutoFocusTarget();

    // Set in view manager
    if (effectiveAutoFocusTarget != mViewManager.GetAutoFocusTarget())
    {
        mViewManager.SetAutoFocusTarget(effectiveAutoFocusTarget);
    }

    if (effectiveAutoFocusTarget == AutoFocusTargetKindType::Ship)
    {
        // Reset user offsets
        mViewManager.ResetAutoFocusAlterations();

        // We'll focus at next frame

        return;
    }
    else if (effectiveAutoFocusTarget == AutoFocusTargetKindType::SelectedNpc)
    {
        // Check whether we still have a selected NPC after the load
        if (!mWorld->GetNpcs().GetCurrentlySelectedNpc().has_value())
        {
            // Disable auto-focus
            InternalSwitchAutoFocusTarget(std::nullopt);
            mViewManager.SetAutoFocusTarget(std::nullopt);

            // Continue, we might want to auto-focus on ship if enabled
        }
        else
        {
            return;
        }
    }

    if (mViewManager.GetDoAutoFocusOnShipLoad()
        || (!effectiveAutoFocusTarget.has_value() && mAutoFocusTarget == AutoFocusTargetKindType::Ship)) // Even if no AABBs, focus on particles
    {
        // Focus on ships (one-off)
        FocusOnShips();
    }
}

void GameController::UpdateAutoFocus()
{
    assert(!!mWorld);

    auto const currentAutoFocusTarget = mViewManager.GetAutoFocusTarget();
    if (currentAutoFocusTarget == AutoFocusTargetKindType::Ship)
    {
        // Auto-focus on ship
        mViewManager.UpdateAutoFocus(mWorld->GetAllShipExternalAABBs().MakeWeightedUnion());
    }
    else if (currentAutoFocusTarget == AutoFocusTargetKindType::SelectedNpc)
    {
        // Check whether we have a selected NPC
        auto const selectedNpc = mWorld->GetNpcs().GetCurrentlySelectedNpc();
        if (selectedNpc.has_value())
        {
            assert(mWorld->GetNpcs().HasNpc(*selectedNpc)); // Still exists if selected

            // Auto-focus on NPC
            auto const aabb = mWorld->GetNpcs().GetNpcAABB(*selectedNpc);
            mViewManager.UpdateAutoFocus(aabb);
        }
        else
        {
            // Disable auto-focus
            InternalSwitchAutoFocusTarget(std::nullopt);
            mViewManager.SetAutoFocusTarget(std::nullopt);
        }
    }
    else
    {
        assert(!currentAutoFocusTarget.has_value());

        // Nop
    }
}

std::optional<AutoFocusTargetKindType> GameController::CalculateEffectiveAutoFocusTarget() const
{
    // If Ship but no "normal" AABBs, then no auto-focus
    if (mAutoFocusTarget == AutoFocusTargetKindType::Ship
        && (!mWorld || !mWorld->GetAllShipExternalAABBs().MakeWeightedUnion().has_value()))
    {
        // Cannot do Ship
        return std::nullopt;
    }

    return mAutoFocusTarget;
}

void GameController::InternalSwitchAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & autoFocusTarget)
{
    if (mAutoFocusTarget != autoFocusTarget)
    {
        mAutoFocusTarget = autoFocusTarget;

        // Tell the world
        mGameEventDispatcher.OnAutoFocusTargetChanged(mAutoFocusTarget);

        // Reconciliate notification
        mNotificationLayer.SetAutoFocusIndicator(mAutoFocusTarget.has_value());
    }
}
