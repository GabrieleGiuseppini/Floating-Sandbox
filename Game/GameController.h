/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "ShipMetadata.h"
#include "TextLayer.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

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

    ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath);
    ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath);
    void ReloadLastShip();

    RgbImageData TakeScreenshot();

    float GetCurrentSimulationTime() const { return mWorld->GetCurrentSimulationTime(); }

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

    std::optional<ElementId> Pick(vec2f const & screenCoordinates);
    void MoveBy(ElementId elementId, vec2f const & screenOffset);
    void MoveAllBy(ShipId shipId, vec2f const & screenOffset);
    void RotateBy(ElementId elementId, float screenDeltaY, vec2f const & screenCenter);
    void RotateAllBy(ShipId shipId, float screenDeltaY, vec2f const & screenCenter);
    void DestroyAt(vec2f const & screenCoordinates, float radiusFraction);
    void RepairAt(vec2f const & screenCoordinates, float radiusMultiplier);
    void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates);
    void DrawTo(vec2f const & screenCoordinates, float strengthFraction);
    void SwirlAt(vec2f const & screenCoordinates, float strengthFraction);
    void TogglePinAt(vec2f const & screenCoordinates);
    bool InjectBubblesAt(vec2f const & screenCoordinates);
    bool FloodAt(vec2f const & screenCoordinates, float waterQuantityMultiplier);
    void ToggleAntiMatterBombAt(vec2f const & screenCoordinates);
    void ToggleImpactBombAt(vec2f const & screenCoordinates);
    void ToggleRCBombAt(vec2f const & screenCoordinates);
    void ToggleTimerBombAt(vec2f const & screenCoordinates);
    void DetonateRCBombs();
    void DetonateAntiMatterBombs();
    void AdjustOceanSurfaceTo(std::optional<vec2f> const & screenCoordinates);
    bool AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates);
    bool ScrubThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates);
    std::optional<ElementId> GetNearestPointAt(vec2f const & screenCoordinates) const;
    void QueryNearestPointAt(vec2f const & screenCoordinates) const;

    void SetCanvasSize(int width, int height)
    {
        mRenderContext->SetCanvasSize(width, height);

        // Pickup eventual changes
        mTargetCameraPosition = mCurrentCameraPosition = mRenderContext->GetCameraWorldPosition();
        mTargetZoom = mCurrentZoom = mRenderContext->GetZoom();
    }

    void Pan(vec2f const & screenOffset)
    {
        vec2f worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
        vec2f newTargetCameraPosition = mRenderContext->ClampCameraWorldPosition(mTargetCameraPosition + worldOffset);

        mCurrentCameraPosition = mRenderContext->SetCameraWorldPosition(mTargetCameraPosition); // Skip straight to current target, in case we're already smoothing
        mStartingCameraPosition = mCurrentCameraPosition;
        mTargetCameraPosition = newTargetCameraPosition;

        mStartCameraPositionTimestamp = std::chrono::steady_clock::now();
    }

    void PanImmediate(vec2f const & screenOffset)
    {
        vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);

        auto const newCameraWorldPosition = mRenderContext->SetCameraWorldPosition(
            mRenderContext->GetCameraWorldPosition() + worldOffset);

        mTargetCameraPosition = mCurrentCameraPosition = newCameraWorldPosition;
    }

    void ResetPan()
    {
        auto const newCameraWorldPosition = mRenderContext->SetCameraWorldPosition(vec2f(0, 0));

        mTargetCameraPosition = mCurrentCameraPosition = newCameraWorldPosition;
    }

    void AdjustZoom(float amount)
    {
        float newTargetZoom = mRenderContext->ClampZoom(mTargetZoom * amount);

        mCurrentZoom = mRenderContext->SetZoom(mTargetZoom); // Skip straight to current target, in case we're already smoothing
        mStartingZoom = mCurrentZoom;
        mTargetZoom = newTargetZoom;

        mStartZoomTimestamp = std::chrono::steady_clock::now();
    }

    void ResetZoom()
    {
        auto const newZoom = mRenderContext->SetZoom(1.0);

        mTargetZoom = mCurrentZoom = newZoom;
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

    float GetSpringStiffnessAdjustment() const { return mGameParameters.SpringStiffnessAdjustment; }
    void SetSpringStiffnessAdjustment(float value) { mGameParameters.SpringStiffnessAdjustment = value; }
    float GetMinSpringStiffnessAdjustment() const { return GameParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const { return GameParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const { return mGameParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value) { mGameParameters.SpringDampingAdjustment = value; }
    float GetMinSpringDampingAdjustment() const { return GameParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const { return GameParameters::MaxSpringDampingAdjustment; }

    float GetSpringStrengthAdjustment() const { return mGameParameters.SpringStrengthAdjustment; }
    void SetSpringStrengthAdjustment(float value) { mGameParameters.SpringStrengthAdjustment = value; }
    float GetMinSpringStrengthAdjustment() const { return GameParameters::MinSpringStrengthAdjustment;  }
    float GetMaxSpringStrengthAdjustment() const { return GameParameters::MaxSpringStrengthAdjustment; }

    float GetRotAcceler8r() const { return mGameParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) { mGameParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const { return GameParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const { return GameParameters::MaxRotAcceler8r; }

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

    float GetBasalWaveHeightAdjustment() const { return mGameParameters.BasalWaveHeightAdjustment; }
    void SetBasalWaveHeightAdjustment(float value) { mGameParameters.BasalWaveHeightAdjustment = value; }
    float GetMinBasalWaveHeightAdjustment() const { return GameParameters::MinBasalWaveHeightAdjustment; }
    float GetMaxBasalWaveHeightAdjustment() const { return GameParameters::MaxBasalWaveHeightAdjustment; }

    float GetBasalWaveLengthAdjustment() const { return mGameParameters.BasalWaveLengthAdjustment; }
    void SetBasalWaveLengthAdjustment(float value) { mGameParameters.BasalWaveLengthAdjustment = value; }
    float GetMinBasalWaveLengthAdjustment() const { return GameParameters::MinBasalWaveLengthAdjustment; }
    float GetMaxBasalWaveLengthAdjustment() const { return GameParameters::MaxBasalWaveLengthAdjustment; }

    float GetBasalWaveSpeedAdjustment() const { return mGameParameters.BasalWaveSpeedAdjustment; }
    void SetBasalWaveSpeedAdjustment(float value) { mGameParameters.BasalWaveSpeedAdjustment = value; }
    float GetMinBasalWaveSpeedAdjustment() const { return GameParameters::MinBasalWaveSpeedAdjustment; }
    float GetMaxBasalWaveSpeedAdjustment() const { return GameParameters::MaxBasalWaveSpeedAdjustment; }

    bool GetDoModulateWind() const { return mGameParameters.DoModulateWind; }
    void SetDoModulateWind(bool value) { mGameParameters.DoModulateWind = value; }

    float GetWindSpeedBase() const { return mGameParameters.WindSpeedBase; }
    void SetWindSpeedBase(float value) { mGameParameters.WindSpeedBase = value; }
    float GetMinWindSpeedBase() const { return GameParameters::MinWindSpeedBase; }
    float GetMaxWindSpeedBase() const { return GameParameters::MaxWindSpeedBase; }

    float GetWindSpeedMaxFactor() const { return mGameParameters.WindSpeedMaxFactor; }
    void SetWindSpeedMaxFactor(float value) { mGameParameters.WindSpeedMaxFactor = value; }
    float GetMinWindSpeedMaxFactor() const { return GameParameters::MinWindSpeedMaxFactor; }
    float GetMaxWindSpeedMaxFactor() const { return GameParameters::MaxWindSpeedMaxFactor; }

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

    float GetFloodRadius() const { return mGameParameters.FloodRadius; }
    void SetFloodRadius(float value) { mGameParameters.FloodRadius = value; }
    float GetMinFloodRadius() const { return GameParameters::MinFloodRadius; }
    float GetMaxFloodRadius() const { return GameParameters::MaxFloodRadius; }

    float GetFloodQuantity() const { return mGameParameters.FloodQuantity; }
    void SetFloodQuantity(float value) { mGameParameters.FloodQuantity = value; }
    float GetMinFloodQuantity() const { return GameParameters::MinFloodQuantity; }
    float GetMaxFloodQuantity() const { return GameParameters::MaxFloodQuantity; }

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

    //
    // Render
    //

    rgbColor const & GetFlatSkyColor() const { return mRenderContext->GetFlatSkyColor(); }
    void SetFlatSkyColor(rgbColor const & color) { mRenderContext->SetFlatSkyColor(color); }

    float GetAmbientLightIntensity() const { return mRenderContext->GetAmbientLightIntensity(); }
    void SetAmbientLightIntensity(float value) { mRenderContext->SetAmbientLightIntensity(value); }

    float GetWaterContrast() const { return mRenderContext->GetWaterContrast(); }
    void SetWaterContrast(float value) { mRenderContext->SetWaterContrast(value); }

    float GetOceanTransparency() const { return mRenderContext->GetOceanTransparency(); }
    void SetOceanTransparency(float value) { mRenderContext->SetOceanTransparency(value); }

    bool GetShowShipThroughOcean() const { return mRenderContext->GetShowShipThroughOcean(); }
    void SetShowShipThroughOcean(bool value) { mRenderContext->SetShowShipThroughOcean(value); }

    float GetWaterLevelOfDetail() const { return mRenderContext->GetWaterLevelOfDetail(); }
    void SetWaterLevelOfDetail(float value) { mRenderContext->SetWaterLevelOfDetail(value); }
    float GetMinWaterLevelOfDetail() const { return Render::RenderContext::MinWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const { return Render::RenderContext::MaxWaterLevelOfDetail; }

    ShipRenderMode GetShipRenderMode() const { return mRenderContext->GetShipRenderMode(); }
    void SetShipRenderMode(ShipRenderMode shipRenderMode) { mRenderContext->SetShipRenderMode(shipRenderMode); }

    DebugShipRenderMode GetDebugShipRenderMode() const { return mRenderContext->GetDebugShipRenderMode(); }
    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode) { mRenderContext->SetDebugShipRenderMode(debugShipRenderMode); }

    OceanRenderMode GetOceanRenderMode() const { return mRenderContext->GetOceanRenderMode(); }
    void SetOceanRenderMode(OceanRenderMode oceanRenderMode) { mRenderContext->SetOceanRenderMode(oceanRenderMode); }

    rgbColor const & GetDepthOceanColorStart() const { return mRenderContext->GetDepthOceanColorStart(); }
    void SetDepthOceanColorStart(rgbColor const & color) { mRenderContext->SetDepthOceanColorStart(color); }

    rgbColor const & GetDepthOceanColorEnd() const { return mRenderContext->GetDepthOceanColorEnd(); }
    void SetDepthOceanColorEnd(rgbColor const & color) { mRenderContext->SetDepthOceanColorEnd(color); }

    rgbColor const & GetFlatOceanColor() const { return mRenderContext->GetFlatOceanColor(); }
    void SetFlatOceanColor(rgbColor const & color) { mRenderContext->SetFlatOceanColor(color); }

    LandRenderMode GetLandRenderMode() const { return mRenderContext->GetLandRenderMode(); }
    void SetLandRenderMode(LandRenderMode landRenderMode) { mRenderContext->SetLandRenderMode(landRenderMode); }

    rgbColor const & GetFlatLandColor() const { return mRenderContext->GetFlatLandColor(); }
    void SetFlatLandColor(rgbColor const & color) { mRenderContext->SetFlatLandColor(color); }

    VectorFieldRenderMode GetVectorFieldRenderMode() const { return mRenderContext->GetVectorFieldRenderMode(); }
    void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) { mRenderContext->SetVectorFieldRenderMode(VectorFieldRenderMode); }

    bool GetShowShipStress() const { return mRenderContext->GetShowStressedSprings(); }
    void SetShowShipStress(bool value) { mRenderContext->SetShowStressedSprings(value); }

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
        , mSkippedFirstStatPublishes(0)
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
    int mSkippedFirstStatPublishes;
};
