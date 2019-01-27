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

    // Create text layer
    std::unique_ptr<TextLayer> textLayer = std::make_unique<TextLayer>(
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
            std::move(textLayer),
            std::move(materialDatabase),
            resourceLoader));
}

void GameController::RegisterGameEventHandler(IGameEventHandler * gameEventHandler)
{
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->RegisterSink(gameEventHandler);
}

void GameController::ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath)
{
    // Create a new world
    auto newWorld = std::make_unique<Physics::World>(
        mGameEventDispatcher,
        mGameParameters,
        *mResourceLoader);

    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

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
}

void GameController::AddShip(std::filesystem::path const & shipDefinitionFilepath)
{
    // Load ship definition
    auto shipDefinition = ShipDefinition::Load(shipDefinitionFilepath);

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

ImageData GameController::TakeScreenshot()
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
    assert(!!mTextLayer);
    mTextLayer->SetStatusTextEnabled(isEnabled);
}

void GameController::SetExtendedStatusTextEnabled(bool isEnabled)
{
    assert(!!mTextLayer);
    mTextLayer->SetExtendedStatusTextEnabled(isEnabled);
}

void GameController::MoveBy(
    ShipId shipId,
    vec2f const & screenOffset)
{
    vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        shipId,
        worldOffset,
        mGameParameters);
}

void GameController::RotateBy(
    ShipId shipId,
    float screenDeltaY,
    vec2f const & screenCenter)
{
    float const angle =
        2.0f * Pi<float>
        / static_cast<float>(mRenderContext->GetCanvasSizeHeight())
        * screenDeltaY;

    vec2f const worldCenter = mRenderContext->ScreenToWorld(screenCenter);

    // Apply action
    assert(!!mWorld);
    mWorld->RotateBy(
        shipId,
        angle,
        worldCenter,
        mGameParameters);
}

void GameController::DestroyAt(
    vec2f const & screenCoordinates,
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
    float strengthMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 2000.0f * strengthMultiplier;

    // Apply action
    assert(!!mWorld);
    mWorld->DrawTo(
        worldCoordinates,
        strength,
        mGameParameters);
}

void GameController::SwirlAt(
    vec2f const & screenCoordinates,
    float strengthMultiplier)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 30.0f * strengthMultiplier;

    // Apply action
    assert(!!mWorld);
    mWorld->SwirlAt(
        worldCoordinates,
        strength,
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
        1.0f,
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

bool GameController::AdjustOceanFloorTo(vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    return mWorld->AdjustOceanFloorTo(worldCoordinates.x, worldCoordinates.y);
}

std::optional<ObjectId> GameController::GetNearestPointAt(vec2f const & screenCoordinates) const
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

////////////////////////////////////////////////////////////////////////////////////////

void GameController::InternalUpdate()
{
    // Update world
    assert(!!mWorld);
    mWorld->Update(
        mGameParameters,
        *mRenderContext);

    // Update text layer
    mTextLayer->Update();

    // Flush events
    mGameEventDispatcher->Flush();
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
    // Render text layer
    //

    assert(!!mTextLayer);
    mTextLayer->Render(*mRenderContext);


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
    assert(!!mTextLayer);
    mTextLayer->SetStatusText(
        lastFps,
        totalFps,
        std::chrono::duration<float>(GameWallClock::GetInstance().Now() - mOriginTimestampGame),
        mIsPaused,
        mRenderContext->GetZoom(),
        totalURRatio,
        lastURRatio,
        mRenderContext->GetStatistics());
}