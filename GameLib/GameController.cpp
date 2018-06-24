/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include "GameMath.h"
#include "Log.h"

std::unique_ptr<GameController> GameController::Create(
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
{
    // Load materials
    auto materials = resourceLoader->LoadMaterials();

    // Create game dispatcher
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher = std::make_shared<GameEventDispatcher>();

    // Create render context
    std::unique_ptr<RenderContext> renderContext = std::make_unique<RenderContext>(
        *resourceLoader,
        materials.GetRopeMaterial().RenderColour,
        [&progressCallback](float progress, std::string const & message)
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
            std::move(resourceLoader),
            std::move(materials)));
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

void GameController::DoStep()
{
	// Update world
	assert(!!mWorld);
    mWorld->Update(mGameParameters);

    // Flush events
    mGameEventDispatcher->Flush();
}

void GameController::Render()
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
	// Render world
    //

	assert(!!mWorld);
    mWorld->Render(mGameParameters, *mRenderContext);
}

/////////////////////////////////////////////////////////////
// Interactions
/////////////////////////////////////////////////////////////

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
		mGameParameters.DestroyRadius * radiusMultiplier);
}

void GameController::SawThrough(
    vec2 const & startScreenCoordinates,
    vec2 const & endScreenCoordinates)
{
    vec2f startWorldCoordinates = mRenderContext->ScreenToWorld(startScreenCoordinates);
    vec2f endWorldCoordinates = mRenderContext->ScreenToWorld(endScreenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->SawThrough(startWorldCoordinates, endWorldCoordinates);
}

void GameController::DrawTo(
    vec2 const & screenCoordinates,
    float strengthMultiplier)
{
	vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 50000.0f * strengthMultiplier;
    if (mGameParameters.IsUltraViolentMode)
        strength *= 10.0f;

	// Apply action
	assert(!!mWorld);
    mWorld->DrawTo(
        worldCoordinates, 
        strength);
}

void GameController::SwirlAt(
    vec2 const & screenCoordinates,
    float strengthMultiplier)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    float strength = 1000.0f * strengthMultiplier;
    if (mGameParameters.IsUltraViolentMode)
        strength *= 20.0f;

    // Apply action
    assert(!!mWorld);
    mWorld->SwirlAt(worldCoordinates, strength);
}

void GameController::TogglePinAt(vec2 const & screenCoordinates)
{
    vec2f worldCoordinates = mRenderContext->ScreenToWorld(screenCoordinates);

    // Apply action
    assert(!!mWorld);
    mWorld->TogglePinAt(
        worldCoordinates,
        mGameParameters);

    // TODOTEST: flush
    ////// Flush events
    ////mGameEventDispatcher->Flush();
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

void GameController::DetonateRCBombs()
{
    // Apply action
    assert(!!mWorld);
    mWorld->DetonateRCBombs();
}

ElementIndex GameController::GetNearestPointAt(vec2 const & screenCoordinates) const
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
    mWorld.reset(new Physics::World(mGameEventDispatcher, mGameParameters));

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
    mRenderContext->AddShip(shipId, std::move(shipDefinition.TextureImage));

    // Notify
    mGameEventDispatcher->OnShipLoaded(shipId, shipDefinition.ShipName);
}
