/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "GameTypes.h"
#include "GameWallClock.h"
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
#include <functional>
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
        bool isStatusTextEnabled,
        bool isExtendedStatusTextEnabled,
        std::function<void()> swapRenderBuffersFunction,
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

    std::shared_ptr<IGameEventHandler> GetGameEventHandler()
    {
        assert(!!mGameEventDispatcher);
        return mGameEventDispatcher;
    }

    void RegisterGameEventHandler(IGameEventHandler * gameEventHandler);

    void ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath);
    void AddShip(std::filesystem::path const & shipDefinitionFilepath);
    void ReloadLastShip();

    void RunGameIteration();
    void LowFrequencyUpdate();

    void Update();
    void Render();


    //
    // Interactions
    //

    void SetPaused(bool isPaused);
    void SetMoveToolEngaged(bool isEngaged);
    void SetStatusTextEnabled(bool isEnabled);
    void SetExtendedStatusTextEnabled(bool isEnabled);

    void MoveBy(ShipId shipId, vec2f const & screenOffset);
    void RotateBy(ShipId shipId, float screenDeltaY, vec2f const & screenCenter);
    void DestroyAt(vec2f const & screenCoordinates, float radiusMultiplier);
    void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates);
    void DrawTo(vec2f const & screenCoordinates, float strengthMultiplier);
    void SwirlAt(vec2f const & screenCoordinates, float strengthMultiplier);
    void TogglePinAt(vec2f const & screenCoordinates);
    void ToggleAntiMatterBombAt(vec2f const & screenCoordinates);
    void ToggleImpactBombAt(vec2f const & screenCoordinates);
    void ToggleRCBombAt(vec2f const & screenCoordinates);
    void ToggleTimerBombAt(vec2f const & screenCoordinates);
    void DetonateRCBombs();
    void DetonateAntiMatterBombs();
    std::optional<ObjectId> GetNearestPointAt(vec2f const & screenCoordinates) const;
    void QueryNearestPointAt(vec2f const & screenCoordinates) const;

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

    float GetNumMechanicalDynamicsIterationsAdjustment() const { return mGameParameters.NumMechanicalDynamicsIterationsAdjustment; }
    void SetNumMechanicalDynamicsIterationsAdjustment(float value) { mGameParameters.NumMechanicalDynamicsIterationsAdjustment = value; }
    float GetMinNumMechanicalDynamicsIterationsAdjustment() const { return GameParameters::MinNumMechanicalDynamicsIterationsAdjustment; }
    float GetMaxNumMechanicalDynamicsIterationsAdjustment() const { return GameParameters::MaxNumMechanicalDynamicsIterationsAdjustment; }

    float GetStiffnessAdjustment() const { return mGameParameters.StiffnessAdjustment; }
    void SetStiffnessAdjustment(float value) { mGameParameters.StiffnessAdjustment = value; }
    float GetMinStiffnessAdjustment() const { return GameParameters::MinStiffnessAdjustment; }
    float GetMaxStiffnessAdjustment() const { return GameParameters::MaxStiffnessAdjustment; }

    float GetStrengthAdjustment() const { return mGameParameters.StrengthAdjustment; }
    void SetStrengthAdjustment(float value) { mGameParameters.StrengthAdjustment = value; }
    float GetMinStrengthAdjustment() const { return GameParameters::MinStrengthAdjustment;  }
    float GetMaxStrengthAdjustment() const { return GameParameters::MaxStrengthAdjustment; }

    float GetWaterDensityAdjustment() const { return mGameParameters.WaterDensityAdjustment; }
    void SetWaterDensityAdjustment(float value) { mGameParameters.WaterDensityAdjustment = value; }
    float GetMinWaterDensityAdjustment() const { return GameParameters::MinWaterDensityAdjustment; }
    float GetMaxWaterDensityAdjustment() const { return GameParameters::MaxWaterDensityAdjustment; }

    float GetWaterDragAdjustment() const { return mGameParameters.WaterDragAdjustment; }
    void SetWaterDragAdjustment(float value) { mGameParameters.WaterDragAdjustment = value; }
    float GetMinWaterDragAdjustment() const { return GameParameters::MinWaterDragAdjustment; }
    float GetMaxWaterDragAdjustment() const { return GameParameters::MaxWaterDragAdjustment; }

    float GetWaterIntakeAdjustment() const { return mGameParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) { mGameParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const { return GameParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const { return GameParameters::MaxWaterIntakeAdjustment; }

    float GetWaterCrazyness() const { return mGameParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) { mGameParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const { return GameParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const { return GameParameters::MaxWaterCrazyness; }

    float GetWaterDiffusionSpeedAdjustment() const { return mGameParameters.WaterDiffusionSpeedAdjustment; }
    void SetWaterDiffusionSpeedAdjustment(float value) { mGameParameters.WaterDiffusionSpeedAdjustment = value; }
    float GetMinWaterDiffusionSpeedAdjustment() const { return GameParameters::MinWaterDiffusionSpeedAdjustment; }
    float GetMaxWaterDiffusionSpeedAdjustment() const { return GameParameters::MaxWaterDiffusionSpeedAdjustment; }

    float GetWaveHeight() const { return mGameParameters.WaveHeight; }
    void SetWaveHeight(float value) { mGameParameters.WaveHeight = value; }
    float GetMinWaveHeight() const { return GameParameters::MinWaveHeight; }
    float GetMaxWaveHeight() const { return GameParameters::MaxWaveHeight; }

    float GetSeaDepth() const { return mGameParameters.SeaDepth; }
    void SetSeaDepth(float value) { mGameParameters.SeaDepth = value; }
    float GetMinSeaDepth() const { return GameParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const { return GameParameters::MaxSeaDepth; }

    float GetOceanFloorBumpiness() const { return mGameParameters.OceanFloorBumpiness; }
    void SetOceanFloorBumpiness(float value) { mGameParameters.OceanFloorBumpiness = value; }
    float GetMinOceanFloorBumpiness() const { return GameParameters::MinOceanFloorBumpiness; }
    float GetMaxOceanFloorBumpiness() const { return GameParameters::MaxOceanFloorBumpiness; }

    float GetOceanFloorDetailAmplification() const { return mGameParameters.OceanFloorDetailAmplification; }
    void SetOceanFloorDetailAmplification(float value) { mGameParameters.OceanFloorDetailAmplification = value; }
    float GetMinOceanFloorDetailAmplification() const { return GameParameters::MinOceanFloorDetailAmplification; }
    float GetMaxOceanFloorDetailAmplification() const { return GameParameters::MaxOceanFloorDetailAmplification; }

    float GetDestroyRadius() const { return mGameParameters.DestroyRadius; }
    void SetDestroyRadius(float value) { mGameParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const { return GameParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const { return GameParameters::MaxDestroyRadius; }

    float GetBombBlastRadius() const { return mGameParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) { mGameParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const { return GameParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const { return GameParameters::MaxBombBlastRadius; }

    float GetAntiMatterBombImplosionStrength() const { return mGameParameters.AntiMatterBombImplosionStrength; }
    void SetAntiMatterBombImplosionStrength(float value) { mGameParameters.AntiMatterBombImplosionStrength = value; }
    float GetMinAntiMatterBombImplosionStrength() const { return GameParameters::MinAntiMatterBombImplosionStrength; }
    float GetMaxAntiMatterBombImplosionStrength() const { return GameParameters::MaxAntiMatterBombImplosionStrength; }

    float GetLuminiscenceAdjustment() const { return mGameParameters.LuminiscenceAdjustment; }
    void SetLuminiscenceAdjustment(float value) { mGameParameters.LuminiscenceAdjustment = value; }
    float GetMinLuminiscenceAdjustment() const { return GameParameters::MinLuminiscenceAdjustment; }
    float GetMaxLuminiscenceAdjustment() const { return GameParameters::MaxLuminiscenceAdjustment; }

    float GetLightSpreadAdjustment() const { return mGameParameters.LightSpreadAdjustment; }
    void SetLightSpreadAdjustment(float value) { mGameParameters.LightSpreadAdjustment = value; }
    float GetMinLightSpreadAdjustment() const { return GameParameters::MinLightSpreadAdjustment; }
    float GetMaxLightSpreadAdjustment() const { return GameParameters::MaxLightSpreadAdjustment; }

    bool GetUltraViolentMode() const { return mGameParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) { mGameParameters.IsUltraViolentMode = value; }

    bool GetDoGenerateDebris() const { return mGameParameters.DoGenerateDebris; }
    void SetDoGenerateDebris(bool value) { mGameParameters.DoGenerateDebris = value; }

    bool GetDoGenerateSparkles() const { return mGameParameters.DoGenerateSparkles; }
    void SetDoGenerateSparkles(bool value) { mGameParameters.DoGenerateSparkles = value; }

    bool GetDoGenerateAirBubbles() const { return mGameParameters.DoGenerateAirBubbles; }
    void SetDoGenerateAirBubbles(bool value) { mGameParameters.DoGenerateAirBubbles = value; }

    size_t GetNumberOfStars() const { return mGameParameters.NumberOfStars; }
    void SetNumberOfStars(size_t value) { mGameParameters.NumberOfStars = value; }
    size_t GetMinNumberOfStars() const { return GameParameters::MinNumberOfStars; }
    size_t GetMaxNumberOfStars() const { return GameParameters::MaxNumberOfStars; }

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

    float GetWaterContrast() const { return mRenderContext->GetWaterContrast(); }
    void SetWaterContrast(float value) { mRenderContext->SetWaterContrast(value); }

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

    bool GetWireframeMode() const { return mRenderContext->GetWireframeMode(); }
    void SetWireframeMode(bool wireframeMode) { mRenderContext->SetWireframeMode(wireframeMode); }

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::function<void()> swapRenderBuffersFunction,
        std::unique_ptr<GameEventDispatcher> gameEventDispatcher,
        std::unique_ptr<TextLayer> textLayer,
        MaterialDatabase materialDatabase,
        std::shared_ptr<ResourceLoader> resourceLoader)
        : mGameParameters()
        , mLastShipLoadedFilepath()
        , mIsPaused(false)
        , mIsMoveToolEngaged(false)
        // Doers
        , mRenderContext(std::move(renderContext))
        , mSwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
        , mGameEventDispatcher(std::move(gameEventDispatcher))
        , mResourceLoader(std::move(resourceLoader))
        , mTextLayer(std::move(textLayer))
        , mWorld(new Physics::World(
            mGameEventDispatcher,
            mGameParameters,
            *mResourceLoader))
        , mMaterialDatabase(std::move(materialDatabase))
         // Smoothing
        , mCurrentZoom(mRenderContext->GetZoom())
        , mTargetZoom(mCurrentZoom)
        , mStartingZoom(mCurrentZoom)
        , mStartZoomTimestamp()
        , mCurrentCameraPosition(mRenderContext->GetCameraWorldPosition())
        , mTargetCameraPosition(mCurrentCameraPosition)
        , mStartingCameraPosition(mCurrentCameraPosition)
        , mStartCameraPositionTimestamp()
        // Stats
        , mTotalFrameCount(0u)
        , mLastFrameCount(0u)
        , mRenderStatsOriginTimestampReal(std::chrono::steady_clock::time_point::min())
        , mRenderStatsLastTimestampReal(std::chrono::steady_clock::time_point::min())
        , mTotalUpdateDuration(std::chrono::steady_clock::duration::zero())
        , mLastTotalUpdateDuration(std::chrono::steady_clock::duration::zero())
        , mTotalRenderDuration(std::chrono::steady_clock::duration::zero())
        , mLastTotalRenderDuration(std::chrono::steady_clock::duration::zero())
        , mOriginTimestampGame(GameWallClock::time_point::min())
    {
    }

    void InternalUpdate();

    void InternalRender();

    static void SmoothToTarget(
        float & currentValue,
        float startingValue,
        float targetValue,
        std::chrono::steady_clock::time_point startingTime);

    void Reset(std::unique_ptr<Physics::World> newWorld);

    void OnShipAdded(
        ShipDefinition shipDefinition,
        std::filesystem::path const & shipDefinitionFilepath,
        ShipId shipId);

    void PublishStats(std::chrono::steady_clock::time_point nowReal);

private:

    //
    // Our current state
    //

    GameParameters mGameParameters;
    std::filesystem::path mLastShipLoadedFilepath;
    bool mIsPaused;
    bool mIsMoveToolEngaged;


    //
    // The doers
    //

    std::unique_ptr<Render::RenderContext> mRenderContext;
    std::function<void()> const mSwapRenderBuffersFunction;
    std::shared_ptr<GameEventDispatcher> mGameEventDispatcher;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<TextLayer> mTextLayer;

    //
    // The world
    //

    std::unique_ptr<Physics::World> mWorld;
    MaterialDatabase mMaterialDatabase;


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


    //
    // Stats
    //

    uint64_t mTotalFrameCount;
    uint64_t mLastFrameCount;
    std::chrono::steady_clock::time_point mRenderStatsOriginTimestampReal;
    std::chrono::steady_clock::time_point mRenderStatsLastTimestampReal;
    std::chrono::steady_clock::duration mTotalUpdateDuration;
    std::chrono::steady_clock::duration mLastTotalUpdateDuration;
    std::chrono::steady_clock::duration mTotalRenderDuration;
    std::chrono::steady_clock::duration mLastTotalRenderDuration;
    GameWallClock::time_point mOriginTimestampGame;
};
