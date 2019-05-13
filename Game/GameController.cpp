/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

std::unique_ptr<GameController> GameController::Create(
    bool isStatusTextEnabled,
    bool isExtendedStatusTextEnabled,
    std::function<void()> swapRenderBuffersFunction,
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
{
    // Load materials
    MaterialDatabase materialDatabase = MaterialDatabase::Load(*resourceLoader);

    // Create game dispatcher
    std::unique_ptr<GameEventDispatcher> gameEventDispatcher = std::make_unique<GameEventDispatcher>();

    // Create render context
    std::unique_ptr<Render::RenderContext> renderContext = std::make_unique<Render::RenderContext>(
        *resourceLoader,
        [&progressCallback](float progress, std::string const & message)
        {
            progressCallback(0.9f * progress, message);
        });

    // Create status text
    std::unique_ptr<StatusText> statusText = std::make_unique<StatusText>(
        isStatusTextEnabled,
        isExtendedStatusTextEnabled);

    //
    // Create controller
    //

    return std::unique_ptr<GameController>(
        new GameController(
            std::move(renderContext),
            std::move(swapRenderBuffersFunction),
            std::move(gameEventDispatcher),
            std::move(statusText),
            std::move(materialDatabase),
            resourceLoader));
}

ShipMetadata GameController::ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath)
{
    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        mGameEventDispatcher,
        mGameParameters,
        *mResourceLoader);

    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

    // Save metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Add ship to new world
    ShipId shipId = newWorld->AddShip(
        shipDefinition,
        mMaterialDatabase,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    Reset(std::move(newWorld));

    OnShipAdded(
        std::move(shipDefinition),
        shipDefinitionFilepath,
        shipId);

    return shipMetadata;
}

ShipMetadata GameController::AddShip(std::filesystem::path const & shipDefinitionFilepath)
{
    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

    // Save metadata
    ShipMetadata shipMetadata(shipDefinition.Metadata);

    // Load ship into current world
    ShipId shipId = mWorld->AddShip(
        shipDefinition,
        mMaterialDatabase,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    OnShipAdded(
        std::move(shipDefinition),
        shipDefinitionFilepath,
        shipId);

    return shipMetadata;
}

void GameController::ReloadLastShip()
{
    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        mGameEventDispatcher,
        mGameParameters,
        *mResourceLoader);

    // Load ship definition

    if (mLastShipLoadedFilepath.empty())
    {
        throw std::runtime_error("No ship has been loaded yet");
    }

    auto shipDefinition = ShipDefinition::Load(mLastShipLoadedFilepath);

    // Load ship into new world
    ShipId shipId = newWorld->AddShip(
        shipDefinition,
        mMaterialDatabase,
        mGameParameters);

    //
    // No errors, so we may continue
    //

    Reset(std::move(newWorld));

    OnShipAdded(
        std::move(shipDefinition),
        mLastShipLoadedFilepath,
        shipId);
}

RgbImageData GameController::TakeScreenshot()
{
    return mRenderContext->TakeScreenshot();
}

void GameController::RunGameIteration()
{
    ///////////////////////////////////////////////////////////
    // Update simulation
    ///////////////////////////////////////////////////////////

    // Make sure we're not paused
    if (!mIsPaused && !mIsMoveToolEngaged)
    {
        auto const startTime = std::chrono::steady_clock::now();

        InternalUpdate();

        auto const endTime = std::chrono::steady_clock::now();
        mTotalUpdateDuration += endTime - startTime;
    }


    ///////////////////////////////////////////////////////////
    // Render
    ///////////////////////////////////////////////////////////

    //
    // Initialize render stats, if needed
    //

    if (mRenderStatsOriginTimestampReal == std::chrono::steady_clock::time_point::min())
    {
        assert(mRenderStatsLastTimestampReal == std::chrono::steady_clock::time_point::min());

        std::chrono::steady_clock::time_point nowReal = std::chrono::steady_clock::now();
        mRenderStatsOriginTimestampReal = nowReal;
        mRenderStatsLastTimestampReal = nowReal;

        mTotalFrameCount = 0u;
        mLastFrameCount = 0u;

        // In order to start from zero at first render, take global origin here
        mOriginTimestampGame = nowReal;

        // Render initial status text
        PublishStats(nowReal);
    }


    //
    // Render
    //

    auto const startTime = std::chrono::steady_clock::now();

    // Flip the (previous) back buffer onto the screen
    mSwapRenderBuffersFunction();

    // Render
    InternalRender();

    auto const endTime = std::chrono::steady_clock::now();
    mTotalRenderDuration += endTime - startTime;


    //
    // Update stats
    //

    ++mTotalFrameCount;
    ++mLastFrameCount;
}

void GameController::LowFrequencyUpdate()
{
    std::chrono::steady_clock::time_point nowReal = std::chrono::steady_clock::now();

    if (mSkippedFirstStatPublishes >= 1)
    {
        //
        // Publish stats
        //


        PublishStats(nowReal);

        //
        // Reset stats
        //

        mLastFrameCount = 0u;
        mLastTotalUpdateDuration = mTotalUpdateDuration;
        mLastTotalRenderDuration = mTotalRenderDuration;
    }
    else
    {
        //
        // Skip the first few publish as rates are too polluted
        //

        mTotalFrameCount = 0u;
        mLastFrameCount = 0u;
        mRenderStatsOriginTimestampReal = nowReal;

        ++mSkippedFirstStatPublishes;
    }

    mRenderStatsLastTimestampReal = nowReal;
}

void GameController::Update()
{
    InternalUpdate();
}

void GameController::Render()
{
    InternalRender();
}

/////////////////////////////////////////////////////////////
// Interactions
/////////////////////////////////////////////////////////////

void GameController::SetPaused(bool isPaused)
{
    mIsPaused = isPaused;
}

void GameController::SetMoveToolEngaged(bool isEngaged)
{
    mIsMoveToolEngaged = isEngaged;
}

void GameController::SetStatusTextEnabled(bool isEnabled)
{
    assert(!!mStatusText);
    mStatusText->SetStatusTextEnabled(isEnabled);
}

void GameController::SetExtendedStatusTextEnabled(bool isEnabled)
{
    assert(!!mStatusText);
    mStatusText->SetExtendedStatusTextEnabled(isEnabled);
}

std::optional<ElementId> GameController::Pick(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    return mWorld->Pick(
        worldCoordinates,
        mGameParameters);
}

void GameController::MoveBy(
    ElementId elementId,
    vec2f const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        elementId,
        worldOffset,
        mGameParameters);
}

void GameController::MoveAllBy(
    ShipId shipId,
    vec2f const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveAllBy(
        shipId,
        worldOffset,
        mGameParameters);
}

void GameController::RotateBy(
    ElementId elementId,
    float screenDeltaY,
    vec2f const & screenCenter)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    // Apply action
    assert(!!mWorld);
    mWorld->RotateBy(
        elementId,
        angle,
        worldCenter,
        mGameParameters);
}

void GameController::RotateAllBy(
    ShipId shipId,
    float screenDeltaY,
    vec2f const & screenCenter)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasHeight())
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    // Apply action
    assert(!!mWorld);
    mWorld->RotateAllBy(
        shipId,
        angle,
        worldCenter,
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
    float radiusMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->RepairAt(
        worldCoordinates,
        radiusMultiplier,
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

bool GameController::AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates)
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

////////////////////////////////////////////////////////////////////////////////////////

void GameController::OnTsunami(float x)
{
    if (mShowTsunamiNotifications)
    {
        // Fire notification event
        mGameEventDispatcher->OnTsunamiNotification(x);

        // Start visual notification
        mTsunamiNotificationStateMachine.emplace(mRenderContext);
    }
}

void GameController::InternalUpdate()
{
    auto now = GameWallClock::GetInstance().Now();

    // Update parameter smoothers
    std::for_each(
        mParameterSmoothers.begin(),
        mParameterSmoothers.end(),
        [&now](auto & ps)
        {
            ps.Update(now);
        });

    // Update world
    assert(!!mWorld);
    mWorld->Update(
        mGameParameters,
        *mRenderContext);

    // Flush events
    mGameEventDispatcher->Flush();

    // Update own state
    if (!!mTsunamiNotificationStateMachine)
    {
        if (!mTsunamiNotificationStateMachine->Update())
            mTsunamiNotificationStateMachine.reset();
    }
}

void GameController::InternalRender()
{
    //
    // Do zoom smoothing
    //

    if (mCurrentZoom != mTargetZoom)
    {
        SmoothToTarget(
            mCurrentZoom,
            mStartingZoom,
            mTargetZoom,
            mStartZoomTimestamp);

        mRenderContext->SetZoom(mCurrentZoom);
    }


    //
    // Do camera smoothing
    //

    if (mCurrentCameraPosition != mTargetCameraPosition)
    {
        SmoothToTarget(
            mCurrentCameraPosition.x,
            mStartingCameraPosition.x,
            mTargetCameraPosition.x,
            mStartCameraPositionTimestamp);

        SmoothToTarget(
            mCurrentCameraPosition.y,
            mStartingCameraPosition.y,
            mTargetCameraPosition.y,
            mStartCameraPositionTimestamp);

        mRenderContext->SetCameraWorldPosition(mCurrentCameraPosition);
    }


    //
    // Start rendering
    //

    mRenderContext->RenderStart();


    //
    // Render world
    //

    assert(!!mWorld);
    mWorld->Render(mGameParameters, *mRenderContext);


    //
    // Render status text
    //

    assert(!!mStatusText);
    mStatusText->Render(*mRenderContext);


    //
    // Finish rendering
    //

    mRenderContext->RenderEnd();
}

void GameController::SmoothToTarget(
    float & currentValue,
    float startingValue,
    float targetValue,
    std::chrono::steady_clock::time_point startingTime)
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // Amplitude - summing up pieces from zero to PI yields PI/2
    float amp = (targetValue - startingValue) / (Pi<float> / 2.0f);

    // X - after SmoothMillis we want PI
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startingTime);
    float x = static_cast<float>(elapsed.count()) * Pi<float> / static_cast<float>(SmoothMillis);
    float dv = amp * sinf(x) * sinf(x);

    float oldCurrentValue = currentValue;
    currentValue += dv;

    // Check if we've overshot
    if ((targetValue - oldCurrentValue) * (targetValue - currentValue) < 0.0f)
    {
        // Overshot
        currentValue = targetValue;
    }
}

void GameController::Reset(std::unique_ptr<Physics::World> newWorld)
{
    // Reset world
    assert(!!mWorld);
    mWorld = std::move(newWorld);

    // Reset state
    mTsunamiNotificationStateMachine.reset();

    // Reset rendering engine
    assert(!!mRenderContext);
    mRenderContext->Reset();

    // Notify
    mGameEventDispatcher->OnGameReset();
}

void GameController::OnShipAdded(
    ShipDefinition shipDefinition,
    std::filesystem::path const & shipDefinitionFilepath,
    ShipId shipId)
{
    // Add ship to rendering engine
    mRenderContext->AddShip(
        shipId,
        mWorld->GetShipPointCount(shipId),
        std::move(shipDefinition.TextureLayerImage),
        shipDefinition.TextureOrigin);

    // Notify
    mGameEventDispatcher->OnShipLoaded(
        shipId,
        shipDefinition.Metadata.ShipName,
        shipDefinition.Metadata.Author);

    // Remember last loaded ship
    mLastShipLoadedFilepath = shipDefinitionFilepath;
}

void GameController::PublishStats(std::chrono::steady_clock::time_point nowReal)
{
    // Calculate fps

    auto totalElapsedReal = std::chrono::duration<float>(nowReal - mRenderStatsOriginTimestampReal);
    auto lastElapsedReal = std::chrono::duration<float>(nowReal - mRenderStatsLastTimestampReal);

    float const totalFps =
        totalElapsedReal.count() != 0.0f
        ? static_cast<float>(mTotalFrameCount) / totalElapsedReal.count()
        : 0.0f;

    float const lastFps =
        lastElapsedReal.count() != 0.0f
        ? static_cast<float>(mLastFrameCount) / lastElapsedReal.count()
        : 0.0f;

    // Calculate UR ratio

    auto totalUpdateDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalUpdateDuration);
    auto totalRenderDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalRenderDuration);
    auto lastUpdateDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalUpdateDuration - mLastTotalUpdateDuration);
    auto lastRenderDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalRenderDuration - mLastTotalRenderDuration);

    float const totalURRatio = totalRenderDurationNs.count() != 0
        ? static_cast<float>(totalUpdateDurationNs.count()) / static_cast<float>(totalRenderDurationNs.count())
        : 0.0f;

    float const lastURRatio = lastRenderDurationNs.count() != 0
        ? static_cast<float>(lastUpdateDurationNs.count()) / static_cast<float>(lastRenderDurationNs.count())
        : 0.0f;


    // Publish frame rate
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->OnFrameRateUpdated(lastFps, totalFps);

    // Publish UR ratio
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->OnUpdateToRenderRatioUpdated(lastURRatio);

    // Update status text
    assert(!!mStatusText);
    mStatusText->SetText(
        lastFps,
        totalFps,
        std::chrono::duration<float>(GameWallClock::GetInstance().Now() - mOriginTimestampGame),
        mIsPaused,
        mRenderContext->GetZoom(),
        mRenderContext->GetCameraWorldPosition(),
        totalURRatio,
        lastURRatio,
        mRenderContext->GetStatistics());
}

////////////////////////////////////////////////////////////////////////////////////////

GameController::TsunamiNotificationStateMachine::TsunamiNotificationStateMachine(
    std::shared_ptr<Render::RenderContext> renderContext)
    : mRenderContext(std::move(renderContext))
    , mCurrentState(StateType::RumblingFadeIn)
    , mCurrentStateStartTime(GameWallClock::GetInstance().NowAsFloat())
{
    // Create text
    mTextHandle = mRenderContext->AddText(
        {"TSUNAMI WARNING!"},
        TextPositionType::TopRight,
        0.0f,
        FontType::GameText);
}

GameController::TsunamiNotificationStateMachine::~TsunamiNotificationStateMachine()
{
    mRenderContext->ResetPixelOffset();

    assert(NoneRenderedTextHandle != mTextHandle);
    mRenderContext->ClearText(mTextHandle);
}

bool GameController::TsunamiNotificationStateMachine::Update()
{
    float constexpr TremorAmplitude = 5.0f;
    float constexpr TremorAngularVelocity = 2.0f * Pi<float> * 6.0f;

    float const now = GameWallClock::GetInstance().NowAsFloat();

    switch (mCurrentState)
    {
        case StateType::RumblingFadeIn:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 1s);

            // Set tremor
            mRenderContext->SetPixelOffset(progress * TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::Rumbling1;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Rumbling1:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 4500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::WarningFadeIn;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::WarningFadeIn:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // Fade-in text
            mRenderContext->UpdateText(
                mTextHandle,
                std::min(progress, 1.0f));

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::Warning;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Warning:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 5s);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::WarningFadeOut;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::WarningFadeOut:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Fade-out text
            mRenderContext->UpdateText(
                mTextHandle,
                1.0f - std::min(progress, 1.0f));

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::Rumbling2;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Rumbling2:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::RumblingFadeOut;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::RumblingFadeOut:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 2s);

            // Set tremor
            mRenderContext->SetPixelOffset((1.0f - progress) * TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                // We're done
                return false;
            }

            return true;
        }
    }

    assert(false);
    return true;
}