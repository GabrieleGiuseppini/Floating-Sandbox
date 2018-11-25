/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include "GameMath.h"
#include "Log.h"

std::unique_ptr<GameController> GameController::Create(
    bool isStatusTextEnabled,
    bool isExtendedStatusTextEnabled,
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
{
    // Load materials
    std::unique_ptr<MaterialDatabase> materials = resourceLoader->LoadMaterials();

    // Create game dispatcher
    std::unique_ptr<GameEventDispatcher> gameEventDispatcher = std::make_unique<GameEventDispatcher>();

    // Create render context
    std::unique_ptr<Render::RenderContext> renderContext = std::make_unique<Render::RenderContext>(
        *resourceLoader,
        materials->GetRopeMaterial().RenderColour,
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
            std::move(gameEventDispatcher),
            std::move(textLayer),
            std::move(materials),
            resourceLoader));
}

void GameController::RegisterGameEventHandler(IGameEventHandler * gameEventHandler)
{
    assert(!!mGameEventDispatcher);
    mGameEventDispatcher->RegisterSink(gameEventHandler);
}

void GameController::ResetAndLoadShip(std::filesystem::path const & filepath)
{
    auto shipDefinition = mResourceLoader->LoadShipDefinition(filepath);

    Reset();

    AddShip(std::move(shipDefinition));

    mLastShipLoadedFilePath = filepath;
}

void GameController::AddShip(std::filesystem::path const & filepath)
{
    auto shipDefinition = mResourceLoader->LoadShipDefinition(filepath);

    AddShip(std::move(shipDefinition));

    mLastShipLoadedFilePath = filepath;
}

void GameController::ReloadLastShip()
{
    if (mLastShipLoadedFilePath.empty())
    {
        throw std::runtime_error("No ship has been loaded yet");
    }

    auto shipDefinition = mResourceLoader->LoadShipDefinition(mLastShipLoadedFilePath);

    Reset();

    AddShip(std::move(shipDefinition));
}

void GameController::Update()
{
    auto const startTime = std::chrono::steady_clock::now();

    // Update world
    assert(!!mWorld);
    mWorld->Update(mGameParameters);

    // Update text layer
    mTextLayer->Update();

    // Flush events
    mGameEventDispatcher->Flush();

    // Update stats
    auto const endTime = std::chrono::steady_clock::now();
    mTotalUpdateDuration += endTime - startTime;
}

void GameController::LowFrequencyUpdate()
{
    //
    // Publish stats
    //

    std::chrono::steady_clock::time_point nowReal = std::chrono::steady_clock::now();
    PublishStats(nowReal);

    //
    // Reset stats
    //

    mLastFrameCount = 0u;
    mRenderStatsLastTimestampReal = nowReal;
    mLastTotalUpdateDuration = mTotalUpdateDuration;
    mLastTotalRenderDuration = mTotalRenderDuration;
}

void GameController::Render()
{
    auto const startTime = std::chrono::steady_clock::now();

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
    // Stop render
    //

    mRenderContext->RenderEnd();


    //
    // Update stats
    //

    ++mTotalFrameCount;
    ++mLastFrameCount;

    // Update stats
    auto const endTime = std::chrono::steady_clock::now();
    mTotalRenderDuration += endTime - startTime;
}

/////////////////////////////////////////////////////////////
// Interactions
/////////////////////////////////////////////////////////////

void GameController::SetPaused(bool isPaused)
{
    mIsPaused = isPaused;
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
    vec2f worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

    // Apply action
    assert(!!mWorld);
    mWorld->MoveBy(
        shipId,
        worldOffset);
}

void GameController::DestroyAt(
    vec2f const & screenCoordinates,
    float radiusMultiplier)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    LogMessage("DestroyAt: ", worldCoordinates.toString(), " * ", radiusMultiplier);

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
    vec2f startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

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
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 1000.0f * strengthMultiplier;
    if (mGameParameters.IsUltraViolentMode)
        strength *= 20.0f;

    // Apply action
    assert(!!mWorld);
    mWorld->DrawTo(
        worldCoordinates,
        strength);
}

void GameController::SwirlAt(
    vec2f const & screenCoordinates,
    float strengthMultiplier)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 30.0f * strengthMultiplier;
    if (mGameParameters.IsUltraViolentMode)
        strength *= 40.0f;

    // Apply action
    assert(!!mWorld);
    mWorld->SwirlAt(worldCoordinates, strength);
}

void GameController::TogglePinAt(vec2f const & screenCoordinates)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleTimerBombAt(vec2f const & screenCoordinates)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleTimerBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleRCBombAt(vec2f const & screenCoordinates)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleRCBombAt(
        worldCoordinates,
        mGameParameters);
}

void GameController::ToggleAntiMatterBombAt(vec2f const & screenCoordinates)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->ToggleAntiMatterBombAt(
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

std::optional<ObjectId> GameController::GetNearestPointAt(vec2f const & screenCoordinates) const
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    assert(!!mWorld);
    return mWorld->GetNearestPointAt(worldCoordinates, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////

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

void GameController::Reset()
{
    // Reset world
    assert(!!mWorld);
    mWorld.reset(
        new Physics::World(
            mGameEventDispatcher,
            mGameParameters,
            *mResourceLoader));

    // Reset rendering engine
    assert(!!mRenderContext);
    mRenderContext->Reset();

    // Notify
    mGameEventDispatcher->OnGameReset();
}

void GameController::AddShip(ShipDefinition shipDefinition)
{
    // Add ship to world
    int shipId = mWorld->AddShip(
        shipDefinition,
        mMaterials,
        mGameParameters);

    // Add ship to rendering engine
    mRenderContext->AddShip(
        shipId,
        mWorld->GetShipPointCount(shipId),
        std::move(shipDefinition.TextureImage));

    // Notify
    mGameEventDispatcher->OnShipLoaded(
        shipId,
        shipDefinition.Metadata.ShipName,
        shipDefinition.Metadata.Author);
}

void GameController::PublishStats(std::chrono::steady_clock::time_point nowReal)
{
    // Calculate fps

    auto totalElapsedReal = std::chrono::duration<float>(nowReal - mRenderStatsOriginTimestampReal);
    auto lastElapsedReal = std::chrono::duration<float>(nowReal - mRenderStatsLastTimestampReal);

    float totalFps =
        totalElapsedReal.count() != 0.0f
        ? static_cast<float>(mTotalFrameCount) / totalElapsedReal.count()
        : 0.0f;

    float lastFps =
        lastElapsedReal.count() != 0.0f
        ? static_cast<float>(mLastFrameCount) / lastElapsedReal.count()
        : 0.0f;

    // Calculate UR ratio

    auto totalUpdateDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalUpdateDuration);
    auto totalRenderDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalRenderDuration);
    auto lastUpdateDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalUpdateDuration - mLastTotalUpdateDuration);
    auto lastRenderDurationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(mTotalRenderDuration - mLastTotalRenderDuration);

    float totalURRatio = totalRenderDurationNs.count() != 0
        ? static_cast<float>(totalUpdateDurationNs.count()) / static_cast<float>(totalRenderDurationNs.count())
        : 0.0f;

    float lastURRatio = lastRenderDurationNs.count() != 0
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
        lastURRatio);
}