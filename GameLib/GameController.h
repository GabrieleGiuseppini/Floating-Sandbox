/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "GameTypes.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ProgressCallback.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "TextLayer.h"
#include "Vectors.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

/*
 * This class is responsible for managing the game, from its lifetime to the user
 * interactions.
 */
class GameController
{
public:

    static std::unique_ptr<GameController> Create(
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

    std::shared_ptr<IGameEventHandler> GetGameEventHandler()
    {
        assert(!!mGameEventDispatcher);
        return mGameEventDispatcher;
    }

    void RegisterGameEventHandler(IGameEventHandler * gameEventHandler);

    void ResetAndLoadShip(std::filesystem::path const & filepath);
    void AddShip(std::filesystem::path const & filepath);
    void ReloadLastShip();

    void Update();
    void Render();


    //
    // Interactions
    //

    void SetStatusTextEnabled(bool isEnabled);
    void SetStatusText(
        float immediateFps,
        float averageFps,
        std::chrono::duration<float> elapsedGameSeconds);

    void DestroyAt(vec2f const & screenCoordinates, float radiusMultiplier);
    void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates);
    void DrawTo(vec2f const & screenCoordinates, float strengthMultiplier);
    void SwirlAt(vec2f const & screenCoordinates, float strengthMultiplier);
    void TogglePinAt(vec2f const & screenCoordinates);
    void ToggleTimerBombAt(vec2f const & screenCoordinates);
    void ToggleRCBombAt(vec2f const & screenCoordinates);
    void DetonateRCBombs();
    ElementIndex GetNearestPointAt(vec2f const & screenCoordinates) const;

    void SetCanvasSize(int width, int height) { mRenderContext->SetCanvasSize(width, height); }

    void Pan(vec2f const & screenOffset)
    {
        vec2f worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

        mCurrentCameraPosition = mTargetCameraPosition; // Skip straight to current target, in case we're already smoothing
        mStartingCameraPosition = mCurrentCameraPosition;
        mTargetCameraPosition = mTargetCameraPosition + worldOffset;

        mStartCameraPositionTimestamp = std::chrono::steady_clock::now();
    }

    void PanImmediate(vec2f const & screenOffset)
    {
        vec2f worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
        mRenderContext->AdjustCameraWorldPosition(worldOffset);

        mTargetCameraPosition = mCurrentCameraPosition = mRenderContext->GetCameraWorldPosition();
    }

    void ResetPan()
    {
        mRenderContext->SetCameraWorldPosition(vec2f(0, 0));

        mTargetCameraPosition = mCurrentCameraPosition = mRenderContext->GetCameraWorldPosition();
    }

    void AdjustZoom(float amount) 
    { 
        float newTargetZoom = mTargetZoom * amount;
        if (newTargetZoom < GameParameters::MinZoom)
            newTargetZoom = GameParameters::MinZoom;
        else if (newTargetZoom > GameParameters::MaxZoom)
            newTargetZoom = GameParameters::MaxZoom;

        if (newTargetZoom != mTargetZoom)
        {
            mCurrentZoom = mTargetZoom; // Skip straight to current target, in case we're already smoothing
            mStartingZoom = mCurrentZoom;
            mTargetZoom = newTargetZoom;

            mStartZoomTimestamp = std::chrono::steady_clock::now();
        }
    }

    void ResetZoom() 
    { 
        mRenderContext->SetZoom(1.0); 

        mTargetZoom = mCurrentZoom = mRenderContext->GetZoom();
    }

    vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return mRenderContext->ScreenToWorld(screenCoordinates);
    }

    inline bool IsUnderwater(vec2f const & screenCoordinates) const
    {
        return mWorld->IsUnderwater(ScreenToWorld(screenCoordinates));
    }

    //
    // Physics
    //

    float GetStiffnessAdjustment() const { return mGameParameters.StiffnessAdjustment; }
    void SetStiffnessAdjustment(float value) { mGameParameters.StiffnessAdjustment = value; }
    float GetMinStiffnessAdjustment() const { return GameParameters::MinStiffnessAdjustment; }
    float GetMaxStiffnessAdjustment() const { return GameParameters::MaxStiffnessAdjustment; }

    float GetStrengthAdjustment() const { return mGameParameters.StrengthAdjustment; }
    void SetStrengthAdjustment(float value) { mGameParameters.StrengthAdjustment = value; }
    float GetMinStrengthAdjustment() const { return GameParameters::MinStrengthAdjustment;  }
    float GetMaxStrengthAdjustment() const { return GameParameters::MaxStrengthAdjustment; }

    float GetBuoyancyAdjustment() const { return mGameParameters.BuoyancyAdjustment; }
    void SetBuoyancyAdjustment(float value) { mGameParameters.BuoyancyAdjustment = value; }
    float GetMinBuoyancyAdjustment() const { return GameParameters::MinBuoyancyAdjustment; }
    float GetMaxBuoyancyAdjustment() const { return GameParameters::MaxBuoyancyAdjustment; }

    float GetWaterIntakeAdjustment() const { return mGameParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) { mGameParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const { return GameParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const { return GameParameters::MaxWaterIntakeAdjustment; }

    float GetWaterCrazyness() const { return mGameParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) { mGameParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const { return GameParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const { return GameParameters::MaxWaterCrazyness; }

    float GetWaterQuickness() const { return mGameParameters.WaterQuickness; }
    void SetWaterQuickness(float value) { mGameParameters.WaterQuickness = value; }
    float GetMinWaterQuickness() const { return GameParameters::MinWaterQuickness; }
    float GetMaxWaterQuickness() const { return GameParameters::MaxWaterQuickness; }

    float GetWaveHeight() const { return mGameParameters.WaveHeight; }
    void SetWaveHeight(float value) { mGameParameters.WaveHeight = value; }
    float GetMinWaveHeight() const { return GameParameters::MinWaveHeight; }
    float GetMaxWaveHeight() const { return GameParameters::MaxWaveHeight; }

    float GetSeaDepth() const { return mGameParameters.SeaDepth; }
    void SetSeaDepth(float value) { mGameParameters.SeaDepth = value; }
    float GetMinSeaDepth() const { return GameParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const { return GameParameters::MaxSeaDepth; }

    float GetDestroyRadius() const { return mGameParameters.DestroyRadius; }
    void SetDestroyRadius(float value) { mGameParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const { return GameParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const { return GameParameters::MaxDestroyRadius; }

    float GetBombBlastRadius() const { return mGameParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) { mGameParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const { return GameParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const { return GameParameters::MaxBombBlastRadius; }

    float GetLightDiffusionAdjustment() const { return mGameParameters.LightDiffusionAdjustment; }
    void SetLightDiffusionAdjustment(float value) { mGameParameters.LightDiffusionAdjustment = value; }

    bool GetUltraViolentMode() const { return mGameParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) { mGameParameters.IsUltraViolentMode = value; }

    size_t GetNumberOfClouds() const { return mGameParameters.NumberOfClouds; }
    void SetNumberOfClouds(size_t value) { mGameParameters.NumberOfClouds = value; }
    size_t GetMinNumberOfClouds() const { return GameParameters::MinNumberOfClouds; }
    size_t GetMaxNumberOfClouds() const { return GameParameters::MaxNumberOfClouds; }

    float GetWindSpeed() const { return mGameParameters.WindSpeed; }
    void SetWindSpeed(float value) { mGameParameters.WindSpeed = value; }
    float GetMinWindSpeed() const { return GameParameters::MinWindSpeed; }
    float GetMaxWindSpeed() const { return GameParameters::MaxWindSpeed; }

    //
    // Render
    //

    float GetAmbientLightIntensity() const { return mRenderContext->GetAmbientLightIntensity(); }
    void SetAmbientLightIntensity(float value) { mRenderContext->SetAmbientLightIntensity(value); }

    float GetSeaWaterTransparency() const { return mRenderContext->GetSeaWaterTransparency(); }
    void SetSeaWaterTransparency(float value) { mRenderContext->SetSeaWaterTransparency(value); }

    bool GetShowShipThroughSeaWater() const { return mRenderContext->GetShowShipThroughSeaWater(); }
    void SetShowShipThroughSeaWater(bool value) { mRenderContext->SetShowShipThroughSeaWater(value); }

    float GetWaterLevelOfDetail() const { return mRenderContext->GetWaterLevelOfDetail(); }
    void SetWaterLevelOfDetail(float value) { mRenderContext->SetWaterLevelOfDetail(value); }
    float GetMinWaterLevelOfDetail() const { return Render::RenderContext::MinWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const { return Render::RenderContext::MaxWaterLevelOfDetail; }

    ShipRenderMode GetShipRenderMode() const { return mRenderContext->GetShipRenderMode(); }
    void SetShipRenderMode(ShipRenderMode shipRenderMode) { mRenderContext->SetShipRenderMode(shipRenderMode); }

    VectorFieldRenderMode GetVectorFieldRenderMode() const { return mRenderContext->GetVectorFieldRenderMode(); }
    void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) { mRenderContext->SetVectorFieldRenderMode(VectorFieldRenderMode); }

    bool GetShowShipStress() const { return mRenderContext->GetShowStressedSprings(); }
    void SetShowShipStress(bool value) { mRenderContext->SetShowStressedSprings(value); }

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::shared_ptr<ResourceLoader> resourceLoader,
        std::shared_ptr<TextLayer> textLayer,
        MaterialDatabase && materials)
        : mGameParameters()
        , mLastShipLoadedFilePath()
        , mRenderContext(std::move(renderContext))
        , mGameEventDispatcher(std::move(gameEventDispatcher))
        , mResourceLoader(std::move(resourceLoader))
        , mTextLayer(std::move(textLayer))
        , mWorld(new Physics::World(
            mGameEventDispatcher,
            mGameParameters))
        , mMaterials(std::move(materials))        
         // Smoothing
        , mCurrentZoom(mRenderContext->GetZoom())
        , mTargetZoom(mCurrentZoom)
        , mStartingZoom(mCurrentZoom)
        , mStartZoomTimestamp()
        , mCurrentCameraPosition(mRenderContext->GetCameraWorldPosition())
        , mTargetCameraPosition(mCurrentCameraPosition)
        , mStartingCameraPosition(mCurrentCameraPosition)
        , mStartCameraPositionTimestamp()
    {
    }
    
    static void SmoothToTarget(
        float & currentValue,
        float startingValue,
        float targetValue,
        std::chrono::steady_clock::time_point startingTime);

    void Reset();

    void AddShip(ShipDefinition shipDefinition);

private:

    //
    // Our current state
    //

    GameParameters mGameParameters;
    std::filesystem::path mLastShipLoadedFilePath;

    //
    // The doers 
    //

    std::unique_ptr<Render::RenderContext> mRenderContext;
    std::shared_ptr<GameEventDispatcher> mGameEventDispatcher;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<TextLayer> mTextLayer;

    //
    // The world
    //

    std::unique_ptr<Physics::World> mWorld;
    MaterialDatabase mMaterials;
        

    //
    // The current render parameters that we're smoothing to
    //

    static constexpr int SmoothMillis = 500;

    float mCurrentZoom;
    float mTargetZoom;
    float mStartingZoom;
    std::chrono::steady_clock::time_point mStartZoomTimestamp;

    vec2f mCurrentCameraPosition;
    vec2f mTargetCameraPosition;
    vec2f mStartingCameraPosition;
    std::chrono::steady_clock::time_point mStartCameraPositionTimestamp;
};
