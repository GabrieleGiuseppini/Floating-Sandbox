/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <ctime>
#include <iomanip>
#include <sstream>

std::unique_ptr<GameController> GameController::Create(
    ImageSize const & initialCanvasSize,
    std::function<void()> makeRenderContextCurrentFunction,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
{
    // Load materials
    MaterialDatabase materialDatabase = MaterialDatabase::Load(resourceLocator);

    // Create game event dispatcher
    auto gameEventDispatcher = std::make_shared<GameEventDispatcher>();

    // Create perf stats
    std::unique_ptr<PerfStats> perfStats = std::make_unique<PerfStats>();

    // Create render context
    std::unique_ptr<Render::RenderContext> renderContext = std::make_unique<Render::RenderContext>(
        initialCanvasSize,
        std::move(makeRenderContextCurrentFunction),
        std::move(swapRenderBuffersFunction),
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
            std::move(materialDatabase),
            resourceLocator));
}

GameController::GameController(
    std::unique_ptr<Render::RenderContext> renderContext,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::unique_ptr<PerfStats> perfStats,
    MaterialDatabase materialDatabase,
    ResourceLocator const & resourceLocator)
    // State machines
    : mTsunamiNotificationStateMachine()
    , mThanosSnapStateMachines()
    , mDayLightCycleStateMachine()
    // State
    , mGameParameters()
    , mLastShipLoadedFilepath()
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
        mGameParameters.DoDayLightCycle)
    , mShipTexturizer(resourceLocator)
    , mWorld(new Physics::World(
        OceanFloorTerrain::LoadFromImage(resourceLocator.GetDefaultOceanFloorTerrainFilePath()),
        mGameEventDispatcher,
        std::make_shared<TaskThreadPool>(),
        mGameParameters))
    , mMaterialDatabase(std::move(materialDatabase))
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
}

void GameController::RebindOpenGLContext(std::function<void()> rebindContextFunction)
{
    assert(!!mRenderContext);
    mRenderContext->RebindContext(std::move(rebindContextFunction));
}

ShipMetadata GameController::ResetAndLoadFallbackShip(ResourceLocator const & resourceLocator)
{
    std::filesystem::path const shipDefinitionFilePath = resourceLocator.GetFallbackShipDefinitionFilePath();

    return ResetAndLoadShip(shipDefinitionFilePath);
}

ShipMetadata GameController::ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath)
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
        mGameEventDispatcher,
        std::make_shared<TaskThreadPool>(),
        mGameParameters);

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
        shipDefinitionFilepath,
        mDoAutoZoomOnShipLoad);

    return shipMetadata;
}

ShipMetadata GameController::AddDefaultShip(ResourceLocator const & resourceLocator)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    auto const tm = std::localtime(&now_c);
    bool const isSpecialDay =
        (tm->tm_mon == 0 && tm->tm_mday == 17)  // Jan 17: Floating Sandbox's birthday
        ;

    std::filesystem::path const shipDefinitionFilePath = isSpecialDay
        ? resourceLocator.GetFallbackShipDefinitionFilePath()
        : resourceLocator.GetDefaultShipDefinitionFilePath();

    return AddShip(shipDefinitionFilePath);
}

ShipMetadata GameController::AddFallbackShip(ResourceLocator const & resourceLocator)
{
    std::filesystem::path const shipDefinitionFilePath = resourceLocator.GetFallbackShipDefinitionFilePath();

    return AddShip(shipDefinitionFilePath);
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
        shipDefinitionFilepath,
        false);

    return shipMetadata;
}

void GameController::ReloadLastShip()
{
    assert(!!mWorld);

    if (mLastShipLoadedFilepath.empty())
    {
        throw std::runtime_error("No ship has been loaded yet");
    }

    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(mLastShipLoadedFilepath);

    // Pre-validate ship's texture
    if (shipDefinition.TextureLayerImage.has_value())
        mRenderContext->ValidateShipTexture(*shipDefinition.TextureLayerImage);

    // Remember metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        OceanFloorTerrain(mWorld->GetOceanFloorTerrain()),
        mGameEventDispatcher,
        std::make_shared<TaskThreadPool>(),
        mGameParameters);

    // Load ship into new world
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
        mLastShipLoadedFilepath,
        false);
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
            *mRenderContext,
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

void GameController::PickObjectToMove(
    vec2f const & screenCoordinates,
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

std::optional<ElementId> GameController::PickObjectForPickAndPull(vec2f const & screenCoordinates)
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
    vec2f const & screenTarget)
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
    vec2f const & screenCoordinates,
    std::optional<ShipId> & shipId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    auto const elementIndex = mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
    if (elementIndex.has_value())
        shipId = elementIndex->GetShipId();
    else
        shipId = std::nullopt;
}

void GameController::MoveBy(
    ElementId elementId,
    vec2f const & screenOffset,
    vec2f const & inertialScreenOffset)
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

    // Eventually display inertial velocity
    if (worldOffset.length() == 0.0f)
    {
        DisplayInertialVelocity(inertialVelocity.length());
    }
}

void GameController::MoveBy(
    ShipId shipId,
    vec2f const & screenOffset,
    vec2f const & inertialScreenOffset)
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

    // Eventually display velocity
    if (worldOffset.length() == 0.0f)
    {
        DisplayInertialVelocity(inertialVelocity.length());
    }
}

void GameController::RotateBy(
    ElementId elementId,
    float screenDeltaY,
    vec2f const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
        * screenDeltaY
        * 1.5f; // More responsive

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
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
    vec2f const & screenCenter,
    float inertialScreenDeltaY)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    float const inertialAngle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
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
    vec2f const & screenCoordinates,
    float radiusFraction)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->DestroyAt(
        worldCoordinates,
        radiusFraction,
        mGameParameters);
}

void GameController::RepairAt(
    vec2f const & screenCoordinates,
    float radiusMultiplier,
    RepairSessionId sessionId,
    RepairSessionStepId sessionStepId)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->RepairAt(
        worldCoordinates,
        radiusMultiplier,
        sessionId,
        sessionStepId,
        mGameParameters);
}

void GameController::SawThrough(
    vec2f const & startScreenCoordinates,
    vec2f const & endScreenCoordinates)
{
    vec2f const startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f const endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->SawThrough(
        startWorldCoordinates,
        endWorldCoordinates,
        mGameParameters);
}

bool GameController::ApplyHeatBlasterAt(
    vec2f const & screenCoordinates,
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

bool GameController::ExtinguishFireAt(vec2f const & screenCoordinates)
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

void GameController::DrawTo(
    vec2f const & screenCoordinates,
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
    vec2f const & screenCoordinates,
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

void GameController::TogglePinAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mGameParameters);
}

bool GameController::InjectBubblesAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->InjectBubblesAt(
        worldCoordinates,
        mGameParameters);
}

bool GameController::FloodAt(
    vec2f const & screenCoordinates,
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

void GameController::ToggleAntiMatterBombAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleAntiMatterBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleImpactBombAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleImpactBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleRCBombAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleRCBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleTimerBombAt(vec2f const & screenCoordinates)
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

void GameController::AdjustOceanSurfaceTo(std::optional<vec2f> const & screenCoordinates)
{
    std::optional<vec2f> const worldCoordinates = !!screenCoordinates
        ? mRenderContext->ScreenToWorld(*screenCoordinates)
        : std::optional<vec2f>();

    // Apply action
    assert(!!mWorld);
    mWorld->AdjustOceanSurfaceTo(worldCoordinates);
}

std::optional<bool> GameController::AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates)
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
    vec2f const & startScreenCoordinates,
    vec2f const & endScreenCoordinates)
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

void GameController::ApplyThanosSnapAt(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    StartThanosSnapStateMachine(worldCoordinates.x, mWorld->GetCurrentSimulationTime());
}

std::optional<ElementId> GameController::GetNearestPointAt(vec2f const & screenCoordinates) const
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    return mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
}

void GameController::QueryNearestPointAt(vec2f const & screenCoordinates) const
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

void GameController::DestroyTriangle(ElementId triangleId)
{
    assert(!!mWorld);
    mWorld->DestroyTriangle(triangleId);
}

//
// Render controls
//

void GameController::SetCanvasSize(int width, int height)
{
    // Tell RenderContext
    mRenderContext->SetCanvasSize(width, height);

    // Pickup eventual changes to view model properties
    mZoomParameterSmoother->SetValueImmediate(mRenderContext->GetZoom());
    mCameraWorldPositionParameterSmoother->SetValueImmediate(mRenderContext->GetCameraWorldPosition());
}

void GameController::Pan(vec2f const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

    mCameraWorldPositionParameterSmoother->SetValue(
        newTargetCameraWorldPosition,
        GameWallClock::GetInstance().ContinuousNowAsFloat());
}

void GameController::PanImmediate(vec2f const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

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

vec2f GameController::ScreenToWorld(vec2f const & screenCoordinates) const
{
    return mRenderContext->ScreenToWorld(screenCoordinates);
}

vec2f GameController::ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
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
}

////////////////////////////////////////////////////////////////////////////////////////

void GameController::Reset(std::unique_ptr<Physics::World> newWorld)
{
    // Reset world
    assert(!!mWorld);
    mWorld = std::move(newWorld);

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
    RgbaImageData&& textureImage,
    ShipMetadata const& shipMetadata,
    std::filesystem::path const& shipDefinitionFilepath,
    bool doAutoZoom)
{
    // Auto-zoom (if requested)
    if (doAutoZoom)
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
        shipMetadata.ShipName,
        shipMetadata.Author);

    // Announce
    mWorld->Announce();

    // Remember last loaded ship
    mLastShipLoadedFilepath = shipDefinitionFilepath;
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

void GameController::DisplayInertialVelocity(float inertialVelocityMagnitude)
{
    if (inertialVelocityMagnitude >= 5.0f)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
            << inertialVelocityMagnitude
            << " M/SEC";

        mNotificationLayer.AddEphemeralTextLine(ss.str());
    }
}