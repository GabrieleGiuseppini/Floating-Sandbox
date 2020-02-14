/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "IGameController.h"
#include "IGameControllerSettings.h"
#include "IGameControllerSettingsOptions.h"
#include "IGameEventHandlers.h"
#include "MaterialDatabase.h"
#include "PerfStats.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "ShipMetadata.h"
#include "TextLayer.h"

#include <GameCore/Colors.h>
#include <GameCore/GameChronometer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageData.h>
#include <GameCore/ParameterSmoother.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/*
 * This class is responsible for managing the game, from its lifetime to the user
 * interactions.
 */
class GameController final
    : public IGameController
	, public IGameControllerSettings
	, public IGameControllerSettingsOptions
    , public ILifecycleGameEventHandler
    , public IWavePhenomenaGameEventHandler
{
public:

    static std::unique_ptr<GameController> Create(
        std::function<void()> swapRenderBuffersFunction,
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

public:

    /////////////////////////////////////////////////////////
    // IGameController
    /////////////////////////////////////////////////////////

    void RegisterRenderEventHandler(IRenderGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterRenderEventHandler(handler);
    }

    void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterLifecycleEventHandler(handler);
    }

    void RegisterStructuralEventHandler(IStructuralGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStructuralEventHandler(handler);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterWavePhenomenaEventHandler(handler);
    }

    void RegisterCombustionEventHandler(ICombustionGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterCombustionEventHandler(handler);
    }

    void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStatisticsEventHandler(handler);
    }

	void RegisterAtmosphereEventHandler(IAtmosphereGameEventHandler* handler) override
	{
		assert(!!mGameEventDispatcher);
		mGameEventDispatcher->RegisterAtmosphereEventHandler(handler);
	}

    void RegisterElectricalElementEventHandler(IElectricalElementGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterElectricalElementEventHandler(handler);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterGenericEventHandler(handler);
    }

    ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath) override;
    ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath) override;
    void ReloadLastShip() override;

    RgbImageData TakeScreenshot() override;

    void RunGameIteration() override;
    void LowFrequencyUpdate() override;

    void PulseUpdate() override
    {
        mIsPulseUpdateSet = true;
    }

    //
    // Game Control and notifications
    //

    void SetPaused(bool isPaused) override;
    void SetMoveToolEngaged(bool isEngaged) override;
	void DisplaySettingsLoadedNotification() override;

	bool GetShowStatusText() const override;
	void SetShowStatusText(bool value) override;
	bool GetShowExtendedStatusText() const override;
	void SetShowExtendedStatusText(bool value) override;

    //
    // World probing
    //

    float GetCurrentSimulationTime() const override;
    bool IsUnderwater(vec2f const & screenCoordinates) const override;

    //
    // Interactions
    //

    void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ElementId> & elementId) override;
    void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ShipId> & shipId) override;
    void MoveBy(ElementId elementId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) override;
    void MoveBy(ShipId shipId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) override;
    void RotateBy(ElementId elementId, float screenDeltaY, vec2f const & screenCenter, float inertialScreenDeltaY) override;
    void RotateBy(ShipId shipId, float screenDeltaY, vec2f const & screenCenter, float intertialScreenDeltaY) override;
    void DestroyAt(vec2f const & screenCoordinates, float radiusFraction) override;
    void RepairAt(vec2f const & screenCoordinates, float radiusMultiplier, RepairSessionId sessionId, RepairSessionStepId sessionStepId) override;
    void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    bool ApplyHeatBlasterAt(vec2f const & screenCoordinates, HeatBlasterActionType action) override;
    bool ExtinguishFireAt(vec2f const & screenCoordinates) override;
    void DrawTo(vec2f const & screenCoordinates, float strengthFraction) override;
    void SwirlAt(vec2f const & screenCoordinates, float strengthFraction) override;
    void TogglePinAt(vec2f const & screenCoordinates) override;
    bool InjectBubblesAt(vec2f const & screenCoordinates) override;
    bool FloodAt(vec2f const & screenCoordinates, float waterQuantityMultiplier) override;
    void ToggleAntiMatterBombAt(vec2f const & screenCoordinates) override;
    void ToggleImpactBombAt(vec2f const & screenCoordinates) override;
    void ToggleRCBombAt(vec2f const & screenCoordinates) override;
    void ToggleTimerBombAt(vec2f const & screenCoordinates) override;
    void DetonateRCBombs() override;
    void DetonateAntiMatterBombs() override;
    void AdjustOceanSurfaceTo(std::optional<vec2f> const & screenCoordinates) override;
    bool AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    bool ScrubThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    void ApplyThanosSnapAt(vec2f const & screenCoordinates) override;
    std::optional<ElementId> GetNearestPointAt(vec2f const & screenCoordinates) const;
    void QueryNearestPointAt(vec2f const & screenCoordinates) const;

    void TriggerTsunami() override;
    void TriggerRogueWave() override;
    void TriggerStorm() override;
	void TriggerLightning() override;

    void SetSwitchState(ElectricalElementId electricalElementId, ElectricalState switchState) override;

    //
    // Render controls
    //

    void SetCanvasSize(int width, int height) override;
    void Pan(vec2f const & screenOffset) override;
    void PanImmediate(vec2f const & screenOffset) override;
    void ResetPan() override;
    void AdjustZoom(float amount) override;
    void ResetZoom() override;
    vec2f ScreenToWorld(vec2f const & screenCoordinates) const override;

	//
	// Interaction parameters
	//

	bool GetShowTsunamiNotifications() const override { return mShowTsunamiNotifications; }
	void SetShowTsunamiNotifications(bool value) override { mShowTsunamiNotifications = value; }

	/////////////////////////////////////////////////////////
	// IGameControllerSettings and IGameControllerSettingsOptions
	/////////////////////////////////////////////////////////

    //
    // Game parameters
    //

    float GetNumMechanicalDynamicsIterationsAdjustment() const override { return mGameParameters.NumMechanicalDynamicsIterationsAdjustment; }
    void SetNumMechanicalDynamicsIterationsAdjustment(float value) override { mGameParameters.NumMechanicalDynamicsIterationsAdjustment = value; }
    float GetMinNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MinNumMechanicalDynamicsIterationsAdjustment; }
    float GetMaxNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MaxNumMechanicalDynamicsIterationsAdjustment; }

    float GetSpringStiffnessAdjustment() const override { return mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStiffnessAdjustment(float value) override { mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStiffnessAdjustment() const override { return GameParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const override { return GameParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const override { return mGameParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value) override { mGameParameters.SpringDampingAdjustment = value; }
    float GetMinSpringDampingAdjustment() const override { return GameParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const override { return GameParameters::MaxSpringDampingAdjustment; }

    float GetSpringStrengthAdjustment() const override { return mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStrengthAdjustment(float value) override { mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStrengthAdjustment() const override { return GameParameters::MinSpringStrengthAdjustment;  }
    float GetMaxSpringStrengthAdjustment() const override { return GameParameters::MaxSpringStrengthAdjustment; }

    float GetGlobalDampingAdjustment() const override { return mGameParameters.GlobalDampingAdjustment; }
    void SetGlobalDampingAdjustment(float value) override { mGameParameters.GlobalDampingAdjustment = value; }
    float GetMinGlobalDampingAdjustment() const override { return GameParameters::MinGlobalDampingAdjustment; }
    float GetMaxGlobalDampingAdjustment() const override { return GameParameters::MaxGlobalDampingAdjustment; }

    float GetRotAcceler8r() const override { return mGameParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) override { mGameParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const override { return GameParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const override { return GameParameters::MaxRotAcceler8r; }

    float GetWaterDensityAdjustment() const override { return mGameParameters.WaterDensityAdjustment; }
    void SetWaterDensityAdjustment(float value) override { mGameParameters.WaterDensityAdjustment = value; }
    float GetMinWaterDensityAdjustment() const override { return GameParameters::MinWaterDensityAdjustment; }
    float GetMaxWaterDensityAdjustment() const override { return GameParameters::MaxWaterDensityAdjustment; }

    float GetWaterDragAdjustment() const override { return mGameParameters.WaterDragAdjustment; }
    void SetWaterDragAdjustment(float value) override { mGameParameters.WaterDragAdjustment = value; }
    float GetMinWaterDragAdjustment() const override { return GameParameters::MinWaterDragAdjustment; }
    float GetMaxWaterDragAdjustment() const override { return GameParameters::MaxWaterDragAdjustment; }

    float GetWaterIntakeAdjustment() const override { return mGameParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) override { mGameParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const override { return GameParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const override { return GameParameters::MaxWaterIntakeAdjustment; }

    float GetWaterCrazyness() const override { return mGameParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) override { mGameParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const override { return GameParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const override { return GameParameters::MaxWaterCrazyness; }

    float GetWaterDiffusionSpeedAdjustment() const override { return mGameParameters.WaterDiffusionSpeedAdjustment; }
    void SetWaterDiffusionSpeedAdjustment(float value) override { mGameParameters.WaterDiffusionSpeedAdjustment = value; }
    float GetMinWaterDiffusionSpeedAdjustment() const override { return GameParameters::MinWaterDiffusionSpeedAdjustment; }
    float GetMaxWaterDiffusionSpeedAdjustment() const override { return GameParameters::MaxWaterDiffusionSpeedAdjustment; }

    float GetSmokeEmissionDensityAdjustment() const override { return mGameParameters.SmokeEmissionDensityAdjustment; }
    void SetSmokeEmissionDensityAdjustment(float value) override { mGameParameters.SmokeEmissionDensityAdjustment = value; }
    float GetMinSmokeEmissionDensityAdjustment() const override { return GameParameters::MinSmokeEmissionDensityAdjustment; }
    float GetMaxSmokeEmissionDensityAdjustment() const override { return GameParameters::MaxSmokeEmissionDensityAdjustment; }

    float GetSmokeParticleLifetimeAdjustment() const override { return mGameParameters.SmokeParticleLifetimeAdjustment; }
    void SetSmokeParticleLifetimeAdjustment(float value) override { mGameParameters.SmokeParticleLifetimeAdjustment = value; }
    float GetMinSmokeParticleLifetimeAdjustment() const override { return GameParameters::MinSmokeParticleLifetimeAdjustment; }
    float GetMaxSmokeParticleLifetimeAdjustment() const override { return GameParameters::MaxSmokeParticleLifetimeAdjustment; }

    float GetBasalWaveHeightAdjustment() const override { return mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].GetValue(); }
    void SetBasalWaveHeightAdjustment(float value) override { mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].SetValue(value); }
    float GetMinBasalWaveHeightAdjustment() const override { return GameParameters::MinBasalWaveHeightAdjustment; }
    float GetMaxBasalWaveHeightAdjustment() const override { return GameParameters::MaxBasalWaveHeightAdjustment; }

    float GetBasalWaveLengthAdjustment() const override { return mGameParameters.BasalWaveLengthAdjustment; }
    void SetBasalWaveLengthAdjustment(float value) override { mGameParameters.BasalWaveLengthAdjustment = value; }
    float GetMinBasalWaveLengthAdjustment() const override { return GameParameters::MinBasalWaveLengthAdjustment; }
    float GetMaxBasalWaveLengthAdjustment() const override { return GameParameters::MaxBasalWaveLengthAdjustment; }

    float GetBasalWaveSpeedAdjustment() const override { return mGameParameters.BasalWaveSpeedAdjustment; }
    void SetBasalWaveSpeedAdjustment(float value) override { mGameParameters.BasalWaveSpeedAdjustment = value; }
    float GetMinBasalWaveSpeedAdjustment() const override { return GameParameters::MinBasalWaveSpeedAdjustment; }
    float GetMaxBasalWaveSpeedAdjustment() const override { return GameParameters::MaxBasalWaveSpeedAdjustment; }

    std::chrono::minutes GetTsunamiRate() const override { return mGameParameters.TsunamiRate; }
    void SetTsunamiRate(std::chrono::minutes value) override { mGameParameters.TsunamiRate = value; }
    std::chrono::minutes GetMinTsunamiRate() const override { return GameParameters::MinTsunamiRate; }
    std::chrono::minutes GetMaxTsunamiRate() const override { return GameParameters::MaxTsunamiRate; }

    std::chrono::minutes GetRogueWaveRate() const override { return mGameParameters.RogueWaveRate; }
    void SetRogueWaveRate(std::chrono::minutes value) override { mGameParameters.RogueWaveRate = value; }
    std::chrono::minutes GetMinRogueWaveRate() const override { return GameParameters::MinRogueWaveRate; }
    std::chrono::minutes GetMaxRogueWaveRate() const override { return GameParameters::MaxRogueWaveRate; }

    bool GetDoModulateWind() const override { return mGameParameters.DoModulateWind; }
    void SetDoModulateWind(bool value) override { mGameParameters.DoModulateWind = value; }

    float GetWindSpeedBase() const override { return mGameParameters.WindSpeedBase; }
    void SetWindSpeedBase(float value) override { mGameParameters.WindSpeedBase = value; }
    float GetMinWindSpeedBase() const override { return GameParameters::MinWindSpeedBase; }
    float GetMaxWindSpeedBase() const override { return GameParameters::MaxWindSpeedBase; }

    float GetWindSpeedMaxFactor() const override { return mGameParameters.WindSpeedMaxFactor; }
    void SetWindSpeedMaxFactor(float value) override { mGameParameters.WindSpeedMaxFactor = value; }
    float GetMinWindSpeedMaxFactor() const override { return GameParameters::MinWindSpeedMaxFactor; }
    float GetMaxWindSpeedMaxFactor() const override { return GameParameters::MaxWindSpeedMaxFactor; }

	// Storm

	std::chrono::minutes GetStormRate() const override { return mGameParameters.StormRate; }
	void SetStormRate(std::chrono::minutes value) override { mGameParameters.StormRate = value; }
	std::chrono::minutes GetMinStormRate() const override { return GameParameters::MinStormRate; }
	std::chrono::minutes GetMaxStormRate() const override { return GameParameters::MaxStormRate; }

	std::chrono::seconds GetStormDuration() const override { return mGameParameters.StormDuration; }
	void SetStormDuration(std::chrono::seconds value) override { mGameParameters.StormDuration = value; }
	std::chrono::seconds GetMinStormDuration() const override { return GameParameters::MinStormDuration; }
	std::chrono::seconds GetMaxStormDuration() const override { return GameParameters::MaxStormDuration; }

	float GetStormStrengthAdjustment() const override { return mGameParameters.StormStrengthAdjustment; }
	void SetStormStrengthAdjustment(float value) override { mGameParameters.StormStrengthAdjustment = value; }
	float GetMinStormStrengthAdjustment() const override { return GameParameters::MinStormStrengthAdjustment; }
	float GetMaxStormStrengthAdjustment() const override { return GameParameters::MaxStormStrengthAdjustment; }

	bool GetDoRainWithStorm() const override { return mGameParameters.DoRainWithStorm; }
	void SetDoRainWithStorm(bool value) override { mGameParameters.DoRainWithStorm = value; }

    float GetRainFloodAdjustment() const override { return mGameParameters.RainFloodAdjustment; }
    void SetRainFloodAdjustment(float value) override { mGameParameters.RainFloodAdjustment = value; }
    float GetMinRainFloodAdjustment() const override { return GameParameters::MinRainFloodAdjustment; }
    float GetMaxRainFloodAdjustment() const override { return GameParameters::MaxRainFloodAdjustment; }

    // Heat

    float GetAirTemperature() const override { return mGameParameters.AirTemperature; }
    void SetAirTemperature(float value) override { mGameParameters.AirTemperature = value; }
    float GetMinAirTemperature() const override { return GameParameters::MinAirTemperature; }
    float GetMaxAirTemperature() const override { return GameParameters::MaxAirTemperature; }

    float GetWaterTemperature() const override { return mGameParameters.WaterTemperature; }
    void SetWaterTemperature(float value) override { mGameParameters.WaterTemperature = value; }
    float GetMinWaterTemperature() const override { return GameParameters::MinWaterTemperature; }
    float GetMaxWaterTemperature() const override { return GameParameters::MaxWaterTemperature; }

    unsigned int GetMaxBurningParticles() const override { return mGameParameters.MaxBurningParticles; }
    void SetMaxBurningParticles(unsigned int value) override { mGameParameters.MaxBurningParticles = value; }
    unsigned int GetMinMaxBurningParticles() const override { return GameParameters::MinMaxBurningParticles; }
    unsigned int GetMaxMaxBurningParticles() const override { return GameParameters::MaxMaxBurningParticles; }

    float GetThermalConductivityAdjustment() const override { return mGameParameters.ThermalConductivityAdjustment; }
    void SetThermalConductivityAdjustment(float value) override { mGameParameters.ThermalConductivityAdjustment = value; }
    float GetMinThermalConductivityAdjustment() const override { return GameParameters::MinThermalConductivityAdjustment; }
    float GetMaxThermalConductivityAdjustment() const override { return GameParameters::MaxThermalConductivityAdjustment; }

    float GetHeatDissipationAdjustment() const override { return mGameParameters.HeatDissipationAdjustment; }
    void SetHeatDissipationAdjustment(float value) override { mGameParameters.HeatDissipationAdjustment = value; }
    float GetMinHeatDissipationAdjustment() const override { return GameParameters::MinHeatDissipationAdjustment; }
    float GetMaxHeatDissipationAdjustment() const override { return GameParameters::MaxHeatDissipationAdjustment; }

    float GetIgnitionTemperatureAdjustment() const override { return mGameParameters.IgnitionTemperatureAdjustment; }
    void SetIgnitionTemperatureAdjustment(float value) override { mGameParameters.IgnitionTemperatureAdjustment = value; }
    float GetMinIgnitionTemperatureAdjustment() const override { return GameParameters::MinIgnitionTemperatureAdjustment; }
    float GetMaxIgnitionTemperatureAdjustment() const override { return GameParameters::MaxIgnitionTemperatureAdjustment; }

    float GetMeltingTemperatureAdjustment() const override { return mGameParameters.MeltingTemperatureAdjustment; }
    void SetMeltingTemperatureAdjustment(float value) override { mGameParameters.MeltingTemperatureAdjustment = value; }
    float GetMinMeltingTemperatureAdjustment() const override { return GameParameters::MinMeltingTemperatureAdjustment; }
    float GetMaxMeltingTemperatureAdjustment() const override { return GameParameters::MaxMeltingTemperatureAdjustment; }

    float GetCombustionSpeedAdjustment() const override { return mGameParameters.CombustionSpeedAdjustment; }
    void SetCombustionSpeedAdjustment(float value) override { mGameParameters.CombustionSpeedAdjustment = value; }
    float GetMinCombustionSpeedAdjustment() const override { return GameParameters::MinCombustionSpeedAdjustment; }
    float GetMaxCombustionSpeedAdjustment() const override { return GameParameters::MaxCombustionSpeedAdjustment; }

    float GetCombustionHeatAdjustment() const override { return mGameParameters.CombustionHeatAdjustment; }
    void SetCombustionHeatAdjustment(float value) override { mGameParameters.CombustionHeatAdjustment = value; }
    float GetMinCombustionHeatAdjustment() const override { return GameParameters::MinCombustionHeatAdjustment; }
    float GetMaxCombustionHeatAdjustment() const override { return GameParameters::MaxCombustionHeatAdjustment; }

    float GetHeatBlasterHeatFlow() const override { return mGameParameters.HeatBlasterHeatFlow; }
    void SetHeatBlasterHeatFlow(float value) override { mGameParameters.HeatBlasterHeatFlow = value; }
    float GetMinHeatBlasterHeatFlow() const override { return GameParameters::MinHeatBlasterHeatFlow; }
    float GetMaxHeatBlasterHeatFlow() const override { return GameParameters::MaxHeatBlasterHeatFlow; }

    float GetHeatBlasterRadius() const override { return mGameParameters.HeatBlasterRadius; }
    void SetHeatBlasterRadius(float value) override { mGameParameters.HeatBlasterRadius = value; }
    float GetMinHeatBlasterRadius() const override { return GameParameters::MinHeatBlasterRadius; }
    float GetMaxHeatBlasterRadius() const override { return GameParameters::MaxHeatBlasterRadius; }

    float GetElectricalElementHeatProducedAdjustment() const override { return mGameParameters.ElectricalElementHeatProducedAdjustment; }
    void SetElectricalElementHeatProducedAdjustment(float value) override { mGameParameters.ElectricalElementHeatProducedAdjustment = value; }
    float GetMinElectricalElementHeatProducedAdjustment() const override { return GameParameters::MinElectricalElementHeatProducedAdjustment; }
    float GetMaxElectricalElementHeatProducedAdjustment() const override { return GameParameters::MaxElectricalElementHeatProducedAdjustment; }

    // Misc

	OceanFloorTerrain const & GetOceanFloorTerrain() const override { return mWorld->GetOceanFloorTerrain(); }
    void SetOceanFloorTerrain(OceanFloorTerrain const & value) override { mWorld->SetOceanFloorTerrain(value); }

    float GetSeaDepth() const override { return mFloatParameterSmoothers[SeaDepthParameterSmoother].GetValue(); }
    void SetSeaDepth(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValue(value); }
	void SetSeaDepthImmediate(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValueImmediate(value); }
    float GetMinSeaDepth() const override { return GameParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const override { return GameParameters::MaxSeaDepth; }

    float GetOceanFloorBumpiness() const override { return mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].GetValue(); }
    void SetOceanFloorBumpiness(float value) override { mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].SetValue(value); }
    float GetMinOceanFloorBumpiness() const override { return GameParameters::MinOceanFloorBumpiness; }
    float GetMaxOceanFloorBumpiness() const override { return GameParameters::MaxOceanFloorBumpiness; }

    float GetOceanFloorDetailAmplification() const override { return mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].GetValue(); }
    void SetOceanFloorDetailAmplification(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValue(value); }
	void SetOceanFloorDetailAmplificationImmediate(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValueImmediate(value); }
    float GetMinOceanFloorDetailAmplification() const override { return GameParameters::MinOceanFloorDetailAmplification; }
    float GetMaxOceanFloorDetailAmplification() const override { return GameParameters::MaxOceanFloorDetailAmplification; }

    float GetOceanFloorElasticity() const override { return mGameParameters.OceanFloorElasticity; }
    void SetOceanFloorElasticity(float value) override { mGameParameters.OceanFloorElasticity = value; }
    float GetMinOceanFloorElasticity() const override { return GameParameters::MinOceanFloorElasticity; }
    float GetMaxOceanFloorElasticity() const override { return GameParameters::MaxOceanFloorElasticity; }

    float GetOceanFloorFriction() const override { return mGameParameters.OceanFloorFriction; }
    void SetOceanFloorFriction(float value) override { mGameParameters.OceanFloorFriction = value; }
    float GetMinOceanFloorFriction() const override { return GameParameters::MinOceanFloorFriction; }
    float GetMaxOceanFloorFriction() const override { return GameParameters::MaxOceanFloorFriction; }

    float GetDestroyRadius() const override { return mGameParameters.DestroyRadius; }
    void SetDestroyRadius(float value) override { mGameParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const override { return GameParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const override { return GameParameters::MaxDestroyRadius; }

    float GetRepairRadius() const override { return mGameParameters.RepairRadius; }
    void SetRepairRadius(float value) override { mGameParameters.RepairRadius = value; }
    float GetMinRepairRadius() const override { return GameParameters::MinRepairRadius; }
    float GetMaxRepairRadius() const override { return GameParameters::MaxRepairRadius; }

    float GetRepairSpeedAdjustment() const override { return mGameParameters.RepairSpeedAdjustment; }
    void SetRepairSpeedAdjustment(float value) override { mGameParameters.RepairSpeedAdjustment = value; }
    float GetMinRepairSpeedAdjustment() const override { return GameParameters::MinRepairSpeedAdjustment; }
    float GetMaxRepairSpeedAdjustment() const override { return GameParameters::MaxRepairSpeedAdjustment; }

    float GetBombBlastRadius() const override { return mGameParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) override { mGameParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const override { return GameParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const override { return GameParameters::MaxBombBlastRadius; }

    float GetBombBlastForceAdjustment() const override { return mGameParameters.BombBlastForceAdjustment; }
    void SetBombBlastForceAdjustment(float value) override { mGameParameters.BombBlastForceAdjustment = value; }
    float GetMinBombBlastForceAdjustment() const override { return GameParameters::MinBombBlastForceAdjustment; }
    float GetMaxBombBlastForceAdjustment() const override { return GameParameters::MaxBombBlastForceAdjustment; }

    float GetBombBlastHeat() const override { return mGameParameters.BombBlastHeat; }
    void SetBombBlastHeat(float value) override { mGameParameters.BombBlastHeat = value; }
    float GetMinBombBlastHeat() const override { return GameParameters::MinBombBlastHeat; }
    float GetMaxBombBlastHeat() const override { return GameParameters::MaxBombBlastHeat; }

    float GetAntiMatterBombImplosionStrength() const override { return mGameParameters.AntiMatterBombImplosionStrength; }
    void SetAntiMatterBombImplosionStrength(float value) override { mGameParameters.AntiMatterBombImplosionStrength = value; }
    float GetMinAntiMatterBombImplosionStrength() const override { return GameParameters::MinAntiMatterBombImplosionStrength; }
    float GetMaxAntiMatterBombImplosionStrength() const override { return GameParameters::MaxAntiMatterBombImplosionStrength; }

    float GetFloodRadius() const override { return mGameParameters.FloodRadius; }
    void SetFloodRadius(float value) override { mGameParameters.FloodRadius = value; }
    float GetMinFloodRadius() const override { return GameParameters::MinFloodRadius; }
    float GetMaxFloodRadius() const override { return GameParameters::MaxFloodRadius; }

    float GetFloodQuantity() const override { return mGameParameters.FloodQuantity; }
    void SetFloodQuantity(float value) override { mGameParameters.FloodQuantity = value; }
    float GetMinFloodQuantity() const override { return GameParameters::MinFloodQuantity; }
    float GetMaxFloodQuantity() const override { return GameParameters::MaxFloodQuantity; }

    float GetLuminiscenceAdjustment() const override { return mGameParameters.LuminiscenceAdjustment; }
    void SetLuminiscenceAdjustment(float value) override { mGameParameters.LuminiscenceAdjustment = value; }
    float GetMinLuminiscenceAdjustment() const override { return GameParameters::MinLuminiscenceAdjustment; }
    float GetMaxLuminiscenceAdjustment() const override { return GameParameters::MaxLuminiscenceAdjustment; }

    float GetLightSpreadAdjustment() const override { return mGameParameters.LightSpreadAdjustment; }
    void SetLightSpreadAdjustment(float value) override { mGameParameters.LightSpreadAdjustment = value; }
    float GetMinLightSpreadAdjustment() const override { return GameParameters::MinLightSpreadAdjustment; }
    float GetMaxLightSpreadAdjustment() const override { return GameParameters::MaxLightSpreadAdjustment; }

    bool GetUltraViolentMode() const override { return mGameParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) override { mGameParameters.IsUltraViolentMode = value; }

    bool GetDoGenerateDebris() const override { return mGameParameters.DoGenerateDebris; }
    void SetDoGenerateDebris(bool value) override { mGameParameters.DoGenerateDebris = value; }

    bool GetDoGenerateSparklesForCuts() const override { return mGameParameters.DoGenerateSparklesForCuts; }
    void SetDoGenerateSparklesForCuts(bool value) override { mGameParameters.DoGenerateSparklesForCuts = value; }

    bool GetDoGenerateAirBubbles() const override { return mGameParameters.DoGenerateAirBubbles; }
    void SetDoGenerateAirBubbles(bool value) override { mGameParameters.DoGenerateAirBubbles = value; }

    float GetAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - mGameParameters.CumulatedIntakenWaterThresholdForAirBubbles; }
    void SetAirBubblesDensity(float value) override { mGameParameters.CumulatedIntakenWaterThresholdForAirBubbles = GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - value; }
    float GetMinAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles; }
    float GetMaxAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles -  GameParameters::MinCumulatedIntakenWaterThresholdForAirBubbles; }

    unsigned int GetNumberOfStars() const override { return mGameParameters.NumberOfStars; }
    void SetNumberOfStars(unsigned int value) override { mGameParameters.NumberOfStars = value; }
    unsigned int GetMinNumberOfStars() const override { return GameParameters::MinNumberOfStars; }
    unsigned int GetMaxNumberOfStars() const override { return GameParameters::MaxNumberOfStars; }

    unsigned int GetNumberOfClouds() const override { return mGameParameters.NumberOfClouds; }
    void SetNumberOfClouds(unsigned int value) override { mGameParameters.NumberOfClouds = value; }
    unsigned int GetMinNumberOfClouds() const override { return GameParameters::MinNumberOfClouds; }
    unsigned int GetMaxNumberOfClouds() const override { return GameParameters::MaxNumberOfClouds; }

    //
    // Render parameters
    //

    rgbColor const & GetFlatSkyColor() const override { return mRenderContext->GetFlatSkyColor(); }
    void SetFlatSkyColor(rgbColor const & color) override { mRenderContext->SetFlatSkyColor(color); }

    float GetAmbientLightIntensity() const override { return mRenderContext->GetAmbientLightIntensity(); }
    void SetAmbientLightIntensity(float value) override { mRenderContext->SetAmbientLightIntensity(value); }

    float GetOceanTransparency() const override { return mRenderContext->GetOceanTransparency(); }
    void SetOceanTransparency(float value) override { mRenderContext->SetOceanTransparency(value); }

    float GetOceanDarkeningRate() const override { return mRenderContext->GetOceanDarkeningRate(); }
    void SetOceanDarkeningRate(float value) override { mRenderContext->SetOceanDarkeningRate(value); }

    rgbColor const & GetFlatLampLightColor() const override { return mRenderContext->GetFlatLampLightColor(); }
    void SetFlatLampLightColor(rgbColor const & color) override { mRenderContext->SetFlatLampLightColor(color); }

    rgbColor const & GetDefaultWaterColor() const override { return mRenderContext->GetDefaultWaterColor(); }
    void SetDefaultWaterColor(rgbColor const & color) override { mRenderContext->SetDefaultWaterColor(color); }

    float GetWaterContrast() const override { return mRenderContext->GetWaterContrast(); }
    void SetWaterContrast(float value) override { mRenderContext->SetWaterContrast(value); }

    float GetWaterLevelOfDetail() const override { return mRenderContext->GetWaterLevelOfDetail(); }
    void SetWaterLevelOfDetail(float value) override { mRenderContext->SetWaterLevelOfDetail(value); }
    float GetMinWaterLevelOfDetail() const override { return Render::RenderContext::MinWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const override { return Render::RenderContext::MaxWaterLevelOfDetail; }

    bool GetShowShipThroughOcean() const override { return mRenderContext->GetShowShipThroughOcean(); }
    void SetShowShipThroughOcean(bool value) override { mRenderContext->SetShowShipThroughOcean(value); }

    ShipRenderMode GetShipRenderMode() const override { return mRenderContext->GetShipRenderMode(); }
    void SetShipRenderMode(ShipRenderMode shipRenderMode) override { mRenderContext->SetShipRenderMode(shipRenderMode); }

    DebugShipRenderMode GetDebugShipRenderMode() const override { return mRenderContext->GetDebugShipRenderMode(); }
    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode) override { mRenderContext->SetDebugShipRenderMode(debugShipRenderMode); }

    OceanRenderMode GetOceanRenderMode() const override { return mRenderContext->GetOceanRenderMode(); }
    void SetOceanRenderMode(OceanRenderMode oceanRenderMode) override { mRenderContext->SetOceanRenderMode(oceanRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const override { return mRenderContext->GetTextureOceanAvailableThumbnails(); }
    size_t GetTextureOceanTextureIndex() const override { return mRenderContext->GetTextureOceanTextureIndex(); }
    void SetTextureOceanTextureIndex(size_t index) override { mRenderContext->SetTextureOceanTextureIndex(index); }

    rgbColor const & GetDepthOceanColorStart() const override { return mRenderContext->GetDepthOceanColorStart(); }
    void SetDepthOceanColorStart(rgbColor const & color) override { mRenderContext->SetDepthOceanColorStart(color); }

    rgbColor const & GetDepthOceanColorEnd() const override { return mRenderContext->GetDepthOceanColorEnd(); }
    void SetDepthOceanColorEnd(rgbColor const & color) override { mRenderContext->SetDepthOceanColorEnd(color); }

    rgbColor const & GetFlatOceanColor() const override { return mRenderContext->GetFlatOceanColor(); }
    void SetFlatOceanColor(rgbColor const & color) override { mRenderContext->SetFlatOceanColor(color); }

    LandRenderMode GetLandRenderMode() const override { return mRenderContext->GetLandRenderMode(); }
    void SetLandRenderMode(LandRenderMode landRenderMode) override { mRenderContext->SetLandRenderMode(landRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const override { return mRenderContext->GetTextureLandAvailableThumbnails(); }
    size_t GetTextureLandTextureIndex() const override { return mRenderContext->GetTextureLandTextureIndex(); }
    void SetTextureLandTextureIndex(size_t index) override { mRenderContext->SetTextureLandTextureIndex(index); }

    rgbColor const & GetFlatLandColor() const override { return mRenderContext->GetFlatLandColor(); }
    void SetFlatLandColor(rgbColor const & color) override { mRenderContext->SetFlatLandColor(color); }

    VectorFieldRenderMode GetVectorFieldRenderMode() const override { return mRenderContext->GetVectorFieldRenderMode(); }
    void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) override { mRenderContext->SetVectorFieldRenderMode(VectorFieldRenderMode); }

    bool GetShowShipStress() const override { return mRenderContext->GetShowStressedSprings(); }
    void SetShowShipStress(bool value) override { mRenderContext->SetShowStressedSprings(value); }

    bool GetDrawHeatOverlay() const override { return mRenderContext->GetDrawHeatOverlay(); }
    void SetDrawHeatOverlay(bool value) override { mRenderContext->SetDrawHeatOverlay(value); }

    float GetHeatOverlayTransparency() const override { return mRenderContext->GetHeatOverlayTransparency(); }
    void SetHeatOverlayTransparency(float value) override { mRenderContext->SetHeatOverlayTransparency(value); }

    ShipFlameRenderMode GetShipFlameRenderMode() const override { return mRenderContext->GetShipFlameRenderMode(); }
    void SetShipFlameRenderMode(ShipFlameRenderMode shipFlameRenderMode) override { mRenderContext->SetShipFlameRenderMode(shipFlameRenderMode); }

    float GetShipFlameSizeAdjustment() const override { return mFloatParameterSmoothers[FlameSizeAdjustmentParameterSmoother].GetValue(); }
    void SetShipFlameSizeAdjustment(float value) override { mFloatParameterSmoothers[FlameSizeAdjustmentParameterSmoother].SetValue(value); }
    float GetMinShipFlameSizeAdjustment() const override { return Render::RenderContext::MinShipFlameSizeAdjustment; }
    float GetMaxShipFlameSizeAdjustment() const override { return Render::RenderContext::MaxShipFlameSizeAdjustment; }

    //
    // Interaction parameters
    //

    bool GetDrawHeatBlasterFlame() const override { return mDrawHeatBlasterFlame; }
    void SetDrawHeatBlasterFlame(bool value) override { mDrawHeatBlasterFlame = value; }

private:

    //
    // Event handlers
    //

    virtual void OnTsunami(float x) override;

    virtual void OnShipRepaired(ShipId shipId) override;

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::function<void()> swapRenderBuffersFunction,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
		std::unique_ptr<TextLayer> textLayer,
        MaterialDatabase materialDatabase,
        std::shared_ptr<ResourceLoader> resourceLoader);

    void Reset(std::unique_ptr<Physics::World> newWorld);

    void OnShipAdded(
        ShipDefinition shipDefinition,
        std::filesystem::path const & shipDefinitionFilepath,
        ShipId shipId);

    void PublishStats(std::chrono::steady_clock::time_point nowReal);

	void DisplayInertialVelocity(float inertialVelocityMagnitude);

private:

    //
    // State machines
    //

    struct TsunamiNotificationStateMachine;
    struct TsunamiNotificationStateMachineDeleter { void operator()(TsunamiNotificationStateMachine *) const; };
    std::unique_ptr<TsunamiNotificationStateMachine, TsunamiNotificationStateMachineDeleter> mTsunamiNotificationStateMachine;

    void StartTsunamiNotificationStateMachine(float x);

    struct ThanosSnapStateMachine;
    struct ThanosSnapStateMachineDeleter { void operator()(ThanosSnapStateMachine *) const; };
    std::vector<std::unique_ptr<ThanosSnapStateMachine, ThanosSnapStateMachineDeleter >> mThanosSnapStateMachines;

    void StartThanosSnapStateMachine(float x, float currentSimulationTime);
    bool UpdateThanosSnapStateMachine(ThanosSnapStateMachine & stateMachine, float currentSimulationTime);

    void ResetStateMachines();
    void UpdateStateMachines(float currentSimulationTime);

private:

    //
    // Our current state
    //

    GameParameters mGameParameters;
    std::filesystem::path mLastShipLoadedFilepath;
    bool mIsPaused;
    bool mIsPulseUpdateSet;
    bool mIsMoveToolEngaged;

    // When set, will be uploaded to the RenderContext to display the HeatBlaster flame
    std::optional<std::tuple<vec2f, float, HeatBlasterActionType>> mHeatBlasterFlameToRender;

    // When set, will be uploaded to the RenderContext to display the fire extinguisher spray
    std::optional<std::tuple<vec2f, float>> mFireExtinguisherSprayToRender;


    //
    // The parameters that we own
    //

    bool mShowTsunamiNotifications;
    bool mDrawHeatBlasterFlame;


    //
    // The doers
    //

    std::shared_ptr<Render::RenderContext> mRenderContext;
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
    // Parameter smoothing
    //

    static constexpr size_t SpringStiffnessAdjustmentParameterSmoother = 0;
    static constexpr size_t SpringStrengthAdjustmentParameterSmoother = 1;
    static constexpr size_t SeaDepthParameterSmoother = 2;
    static constexpr size_t OceanFloorBumpinessParameterSmoother = 3;
    static constexpr size_t OceanFloorDetailAmplificationParameterSmoother = 4;
    static constexpr size_t FlameSizeAdjustmentParameterSmoother = 5;
    static constexpr size_t BasalWaveHeightAdjustmentParameterSmoother = 6;
    std::vector<ParameterSmoother<float>> mFloatParameterSmoothers;

    std::unique_ptr<ParameterSmoother<float>> mZoomParameterSmoother;
    std::unique_ptr<ParameterSmoother<vec2f>> mCameraWorldPositionParameterSmoother;


    //
    // Stats
    //

    std::chrono::steady_clock::time_point mRenderStatsOriginTimestampReal;
    std::chrono::steady_clock::time_point mRenderStatsLastTimestampReal;
    GameWallClock::time_point mOriginTimestampGame;
    PerfStats mTotalPerfStats;
    PerfStats mLastPublishedTotalPerfStats;
    uint64_t mTotalFrameCount;
    uint64_t mLastPublishedTotalFrameCount;
    int mSkippedFirstStatPublishes;
};
