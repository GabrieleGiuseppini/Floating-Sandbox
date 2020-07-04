/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "OceanFloorTerrain.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/UniqueBuffer.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/*
 * The interface presented by the GameController class to the external projects
 * for managing settings.
 */
struct IGameControllerSettings
{
    virtual float GetNumMechanicalDynamicsIterationsAdjustment() const = 0;
    virtual void SetNumMechanicalDynamicsIterationsAdjustment(float value) = 0;

    virtual float GetSpringStiffnessAdjustment() const = 0;
    virtual void SetSpringStiffnessAdjustment(float value) = 0;

    virtual float GetSpringDampingAdjustment() const = 0;
    virtual void SetSpringDampingAdjustment(float value) = 0;

    virtual float GetSpringStrengthAdjustment() const = 0;
    virtual void SetSpringStrengthAdjustment(float value) = 0;

    virtual float GetGlobalDampingAdjustment() const = 0;
    virtual void SetGlobalDampingAdjustment(float value) = 0;

    virtual float GetRotAcceler8r() const = 0;
    virtual void SetRotAcceler8r(float value) = 0;

    // Water

    virtual float GetWaterDensityAdjustment() const = 0;
    virtual void SetWaterDensityAdjustment(float value) = 0;

    virtual float GetWaterDragAdjustment() const = 0;
    virtual void SetWaterDragAdjustment(float value) = 0;

    virtual float GetWaterIntakeAdjustment() const = 0;
    virtual void SetWaterIntakeAdjustment(float value) = 0;

    virtual float GetWaterDiffusionSpeedAdjustment() const = 0;
    virtual void SetWaterDiffusionSpeedAdjustment(float value) = 0;

    virtual float GetWaterCrazyness() const = 0;
    virtual void SetWaterCrazyness(float value) = 0;

    // Ephemeral Particles

    virtual float GetSmokeEmissionDensityAdjustment() const = 0;
    virtual void SetSmokeEmissionDensityAdjustment(float value) = 0;

    virtual float GetSmokeParticleLifetimeAdjustment() const = 0;
    virtual void SetSmokeParticleLifetimeAdjustment(float value) = 0;

    virtual float GetBasalWaveHeightAdjustment() const = 0;
    virtual void SetBasalWaveHeightAdjustment(float value) = 0;

    virtual float GetBasalWaveLengthAdjustment() const = 0;
    virtual void SetBasalWaveLengthAdjustment(float value) = 0;

    virtual float GetBasalWaveSpeedAdjustment() const = 0;
    virtual void SetBasalWaveSpeedAdjustment(float value) = 0;

    virtual std::chrono::minutes GetTsunamiRate() const = 0;
    virtual void SetTsunamiRate(std::chrono::minutes value) = 0;

    virtual std::chrono::minutes GetRogueWaveRate() const = 0;
    virtual void SetRogueWaveRate(std::chrono::minutes value) = 0;

    virtual bool GetDoModulateWind() const = 0;
    virtual void SetDoModulateWind(bool value) = 0;

    virtual float GetWindSpeedBase() const = 0;
    virtual void SetWindSpeedBase(float value) = 0;

    virtual float GetWindSpeedMaxFactor() const = 0;
    virtual void SetWindSpeedMaxFactor(float value) = 0;

	// Storm

	virtual std::chrono::minutes GetStormRate() const = 0;
	virtual void SetStormRate(std::chrono::minutes value) = 0;

	virtual std::chrono::seconds GetStormDuration() const = 0;
	virtual void SetStormDuration(std::chrono::seconds value) = 0;

	virtual float GetStormStrengthAdjustment() const = 0;
	virtual void SetStormStrengthAdjustment(float value) = 0;

	virtual bool GetDoRainWithStorm() const = 0;
	virtual void SetDoRainWithStorm(bool value) = 0;

    virtual float GetRainFloodAdjustment() const = 0;
    virtual void SetRainFloodAdjustment(float value) = 0;

    // Heat

    virtual float GetAirTemperature() const = 0;
    virtual void SetAirTemperature(float value) = 0;

    virtual float GetWaterTemperature() const = 0;
    virtual void SetWaterTemperature(float value) = 0;

    virtual unsigned int GetMaxBurningParticles() const = 0;
    virtual void SetMaxBurningParticles(unsigned int value) = 0;

    virtual float GetThermalConductivityAdjustment() const = 0;
    virtual void SetThermalConductivityAdjustment(float value) = 0;

    virtual float GetHeatDissipationAdjustment() const = 0;
    virtual void SetHeatDissipationAdjustment(float value) = 0;

    virtual float GetIgnitionTemperatureAdjustment() const = 0;
    virtual void SetIgnitionTemperatureAdjustment(float value) = 0;

    virtual float GetMeltingTemperatureAdjustment() const = 0;
    virtual void SetMeltingTemperatureAdjustment(float value) = 0;

    virtual float GetCombustionSpeedAdjustment() const = 0;
    virtual void SetCombustionSpeedAdjustment(float value) = 0;

    virtual float GetCombustionHeatAdjustment() const = 0;
    virtual void SetCombustionHeatAdjustment(float value) = 0;

    virtual float GetHeatBlasterHeatFlow() const = 0;
    virtual void SetHeatBlasterHeatFlow(float value) = 0;

    virtual float GetHeatBlasterRadius() const = 0;
    virtual void SetHeatBlasterRadius(float value) = 0;

    // Electricals

    virtual float GetLuminiscenceAdjustment() const = 0;
    virtual void SetLuminiscenceAdjustment(float value) = 0;

    virtual float GetLightSpreadAdjustment() const = 0;
    virtual void SetLightSpreadAdjustment(float value) = 0;

    virtual float GetElectricalElementHeatProducedAdjustment() const = 0;
    virtual void SetElectricalElementHeatProducedAdjustment(float value) = 0;

    virtual float GetEngineThrustAdjustment() const = 0;
    virtual void SetEngineThrustAdjustment(float value) = 0;

    virtual float GetWaterPumpPowerAdjustment() const = 0;
    virtual void SetWaterPumpPowerAdjustment(float value) = 0;

    // Misc

    virtual OceanFloorTerrain const & GetOceanFloorTerrain() const = 0;
    virtual void SetOceanFloorTerrain(OceanFloorTerrain const & value) = 0;

    virtual float GetSeaDepth() const = 0;
    virtual void SetSeaDepth(float value) = 0;
	virtual void SetSeaDepthImmediate(float value) = 0;

    virtual float GetOceanFloorBumpiness() const = 0;
    virtual void SetOceanFloorBumpiness(float value) = 0;

    virtual float GetOceanFloorDetailAmplification() const = 0;
    virtual void SetOceanFloorDetailAmplification(float value) = 0;
	virtual void SetOceanFloorDetailAmplificationImmediate(float value) = 0;

    virtual float GetOceanFloorElasticity() const = 0;
    virtual void SetOceanFloorElasticity(float value) = 0;

    virtual float GetOceanFloorFriction() const = 0;
    virtual void SetOceanFloorFriction(float value) = 0;

    virtual float GetDestroyRadius() const = 0;
    virtual void SetDestroyRadius(float value) = 0;

    virtual float GetRepairRadius() const = 0;
    virtual void SetRepairRadius(float value) = 0;

    virtual float GetRepairSpeedAdjustment() const = 0;
    virtual void SetRepairSpeedAdjustment(float value) = 0;

    virtual float GetBombBlastRadius() const = 0;
    virtual void SetBombBlastRadius(float value) = 0;

    virtual float GetBombBlastForceAdjustment() const = 0;
    virtual void SetBombBlastForceAdjustment(float value) = 0;

    virtual float GetBombBlastHeat() const = 0;
    virtual void SetBombBlastHeat(float value) = 0;

    virtual float GetAntiMatterBombImplosionStrength() const = 0;
    virtual void SetAntiMatterBombImplosionStrength(float value) = 0;

    virtual float GetFloodRadius() const = 0;
    virtual void SetFloodRadius(float value) = 0;

    virtual float GetFloodQuantity() const = 0;
    virtual void SetFloodQuantity(float value) = 0;

    virtual bool GetUltraViolentMode() const = 0;
    virtual void SetUltraViolentMode(bool value) = 0;

    virtual bool GetDoGenerateDebris() const = 0;
    virtual void SetDoGenerateDebris(bool value) = 0;

    virtual bool GetDoGenerateSparklesForCuts() const = 0;
    virtual void SetDoGenerateSparklesForCuts(bool value) = 0;

    virtual bool GetDoGenerateAirBubbles() const = 0;
    virtual void SetDoGenerateAirBubbles(bool value) = 0;

    virtual float GetAirBubblesDensity() const = 0;
    virtual void SetAirBubblesDensity(float value) = 0;

    virtual bool GetDoDisplaceOceanSurfaceAtAirBubblesSurfacing() const = 0;
    virtual void SetDoDisplaceOceanSurfaceAtAirBubblesSurfacing(bool value) = 0;

    virtual bool GetDoGenerateEngineWakeParticles() const = 0;
    virtual void SetDoGenerateEngineWakeParticles(bool value) = 0;

    virtual unsigned int GetNumberOfStars() const = 0;
    virtual void SetNumberOfStars(unsigned int value) = 0;

    virtual unsigned int GetNumberOfClouds() const = 0;
    virtual void SetNumberOfClouds(unsigned int value) = 0;

    //
    // Render parameters
    //

    virtual rgbColor const & GetFlatSkyColor() const = 0;
    virtual void SetFlatSkyColor(rgbColor const & color) = 0;

    virtual float GetAmbientLightIntensity() const = 0;
    virtual void SetAmbientLightIntensity(float value) = 0;

    virtual float GetOceanTransparency() const = 0;
    virtual void SetOceanTransparency(float value) = 0;

    virtual float GetOceanDarkeningRate() const = 0;
    virtual void SetOceanDarkeningRate(float value) = 0;

    virtual rgbColor const & GetFlatLampLightColor() const = 0;
    virtual void SetFlatLampLightColor(rgbColor const & color) = 0;

    virtual rgbColor const & GetDefaultWaterColor() const = 0;
    virtual void SetDefaultWaterColor(rgbColor const & color) = 0;

    virtual float GetWaterContrast() const = 0;
    virtual void SetWaterContrast(float value) = 0;

    virtual float GetWaterLevelOfDetail() const = 0;
    virtual void SetWaterLevelOfDetail(float value) = 0;

    virtual bool GetShowShipThroughOcean() const = 0;
    virtual void SetShowShipThroughOcean(bool value) = 0;

    virtual DebugShipRenderMode GetDebugShipRenderMode() const = 0;
    virtual void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode) = 0;

    virtual OceanRenderMode GetOceanRenderMode() const = 0;
    virtual void SetOceanRenderMode(OceanRenderMode oceanRenderMode) = 0;

    virtual size_t GetTextureOceanTextureIndex() const = 0;
    virtual void SetTextureOceanTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetDepthOceanColorStart() const = 0;
    virtual void SetDepthOceanColorStart(rgbColor const & color) = 0;

    virtual rgbColor const & GetDepthOceanColorEnd() const = 0;
    virtual void SetDepthOceanColorEnd(rgbColor const & color) = 0;

    virtual rgbColor const & GetFlatOceanColor() const = 0;
    virtual void SetFlatOceanColor(rgbColor const & color) = 0;

    virtual LandRenderMode GetLandRenderMode() const = 0;
    virtual void SetLandRenderMode(LandRenderMode landRenderMode) = 0;

    virtual size_t GetTextureLandTextureIndex() const = 0;
    virtual void SetTextureLandTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetFlatLandColor() const = 0;
    virtual void SetFlatLandColor(rgbColor const & color) = 0;

    virtual VectorFieldRenderMode GetVectorFieldRenderMode() const = 0;
    virtual void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) = 0;

    virtual bool GetShowShipStress() const = 0;
    virtual void SetShowShipStress(bool value) = 0;

    virtual bool GetDrawHeatOverlay() const = 0;
    virtual void SetDrawHeatOverlay(bool value) = 0;

    virtual float GetHeatOverlayTransparency() const = 0;
    virtual void SetHeatOverlayTransparency(float value) = 0;

    virtual ShipFlameRenderMode GetShipFlameRenderMode() const = 0;
    virtual void SetShipFlameRenderMode(ShipFlameRenderMode shipFlameRenderMode) = 0;

    virtual float GetShipFlameSizeAdjustment() const = 0;
    virtual void SetShipFlameSizeAdjustment(float value) = 0;

    virtual bool GetDrawHeatBlasterFlame() const = 0;
    virtual void SetDrawHeatBlasterFlame(bool value) = 0;
};
