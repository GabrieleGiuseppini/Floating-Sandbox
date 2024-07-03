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
    virtual float GetSimulationStepTimeDuration() const = 0;

    virtual unsigned int GetMaxNumSimulationThreads() const = 0;
    virtual void SetMaxNumSimulationThreads(unsigned int value) = 0;

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

    virtual float GetElasticityAdjustment() const = 0;
    virtual void SetElasticityAdjustment(float value) = 0;

    virtual float GetStaticFrictionAdjustment() const = 0;
    virtual void SetStaticFrictionAdjustment(float value) = 0;

    virtual float GetKineticFrictionAdjustment() const = 0;
    virtual void SetKineticFrictionAdjustment(float value) = 0;

    virtual float GetRotAcceler8r() const = 0;
    virtual void SetRotAcceler8r(float value) = 0;

    virtual float GetStaticPressureForceAdjustment() const = 0;
    virtual void SetStaticPressureForceAdjustment(float value) = 0;

    virtual float GetTimeOfDay() const = 0;
    virtual void SetTimeOfDay(float value) = 0;

    // Air

    virtual float GetAirDensityAdjustment() const = 0;
    virtual void SetAirDensityAdjustment(float value) = 0;

    virtual float GetAirFrictionDragAdjustment() const = 0;
    virtual void SetAirFrictionDragAdjustment(float value) = 0;

    virtual float GetAirPressureDragAdjustment() const = 0;
    virtual void SetAirPressureDragAdjustment(float value) = 0;

    // Water

    virtual float GetWaterDensityAdjustment() const = 0;
    virtual void SetWaterDensityAdjustment(float value) = 0;

    virtual float GetWaterFrictionDragAdjustment() const = 0;
    virtual void SetWaterFrictionDragAdjustment(float value) = 0;

    virtual float GetWaterPressureDragAdjustment() const = 0;
    virtual void SetWaterPressureDragAdjustment(float value) = 0;

    virtual float GetWaterImpactForceAdjustment() const = 0;
    virtual void SetWaterImpactForceAdjustment(float value) = 0;

    virtual float GetHydrostaticPressureCounterbalanceAdjustment() const = 0;
    virtual void SetHydrostaticPressureCounterbalanceAdjustment(float value) = 0;

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

    // Wind

    virtual bool GetDoModulateWind() const = 0;
    virtual void SetDoModulateWind(bool value) = 0;

    virtual float GetWindSpeedBase() const = 0;
    virtual void SetWindSpeedBase(float value) = 0;

    virtual float GetWindSpeedMaxFactor() const = 0;
    virtual void SetWindSpeedMaxFactor(float value) = 0;

    // Waves

    virtual float GetBasalWaveHeightAdjustment() const = 0;
    virtual void SetBasalWaveHeightAdjustment(float value) = 0;

    virtual float GetBasalWaveLengthAdjustment() const = 0;
    virtual void SetBasalWaveLengthAdjustment(float value) = 0;

    virtual float GetBasalWaveSpeedAdjustment() const = 0;
    virtual void SetBasalWaveSpeedAdjustment(float value) = 0;

    virtual std::chrono::minutes GetTsunamiRate() const = 0;
    virtual void SetTsunamiRate(std::chrono::minutes value) = 0;

    virtual std::chrono::seconds GetRogueWaveRate() const = 0;
    virtual void SetRogueWaveRate(std::chrono::seconds value) = 0;

    virtual bool GetDoDisplaceWater() const = 0;
    virtual void SetDoDisplaceWater(bool value) = 0;

    virtual float GetWaterDisplacementWaveHeightAdjustment() const = 0;
    virtual void SetWaterDisplacementWaveHeightAdjustment(float value) = 0;

    virtual float GetWaveSmoothnessAdjustment() const = 0;
    virtual void SetWaveSmoothnessAdjustment(float value) = 0;

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

    virtual float GetLightningBlastProbability() const = 0;
    virtual void SetLightningBlastProbability(float value) = 0;

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

    virtual float GetLaserRayHeatFlow() const = 0;
    virtual void SetLaserRayHeatFlow(float value) = 0;

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

    // Fishes

    virtual unsigned int GetNumberOfFishes() const = 0;
    virtual void SetNumberOfFishes(unsigned int value) = 0;

    virtual float GetFishSizeMultiplier() const = 0;
    virtual void SetFishSizeMultiplier(float value) = 0;

    virtual float GetFishSpeedAdjustment() const = 0;
    virtual void SetFishSpeedAdjustment(float value) = 0;

    virtual bool GetDoFishShoaling() const = 0;
    virtual void SetDoFishShoaling(bool value) = 0;

    virtual float GetFishShoalRadiusAdjustment() const = 0;
    virtual void SetFishShoalRadiusAdjustment(float value) = 0;

    // NPCs

    virtual float GetNpcSizeAdjustment() const = 0;
    virtual void SetNpcSizeAdjustment(float value) = 0;


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

    virtual float GetOceanFloorElasticityCoefficient() const = 0;
    virtual void SetOceanFloorElasticityCoefficient(float value) = 0;

    virtual float GetOceanFloorFrictionCoefficient() const = 0;
    virtual void SetOceanFloorFrictionCoefficient(float value) = 0;

    virtual float GetOceanFloorSiltHardness() const = 0;
    virtual void SetOceanFloorSiltHardness(float value) = 0;

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

    virtual float GetInjectPressureQuantity() const = 0;
    virtual void SetInjectPressureQuantity(float value) = 0;

    virtual float GetBlastToolRadius() const = 0;
    virtual void SetBlastToolRadius(float value) = 0;

    virtual float GetBlastToolForceAdjustment() const = 0;
    virtual void SetBlastToolForceAdjustment(float value) = 0;

    virtual float GetScrubRotToolRadius() const = 0;
    virtual void SetScrubRotToolRadius(float value) = 0;

    virtual float GetWindMakerToolWindSpeed() const = 0;
    virtual void SetWindMakerToolWindSpeed(float value) = 0;

    virtual bool GetUltraViolentMode() const = 0;
    virtual void SetUltraViolentMode(bool value) = 0;

    virtual bool GetDoGenerateDebris() const = 0;
    virtual void SetDoGenerateDebris(bool value) = 0;

    virtual bool GetDoGenerateSparklesForCuts() const = 0;
    virtual void SetDoGenerateSparklesForCuts(bool value) = 0;

    virtual float GetAirBubblesDensity() const = 0;
    virtual void SetAirBubblesDensity(float value) = 0;

    virtual bool GetDoGenerateEngineWakeParticles() const = 0;
    virtual void SetDoGenerateEngineWakeParticles(bool value) = 0;

    virtual unsigned int GetNumberOfStars() const = 0;
    virtual void SetNumberOfStars(unsigned int value) = 0;

    virtual unsigned int GetNumberOfClouds() const = 0;
    virtual void SetNumberOfClouds(unsigned int value) = 0;

    virtual bool GetDoDayLightCycle() const = 0;
    virtual void SetDoDayLightCycle(bool value) = 0;

    virtual std::chrono::minutes GetDayLightCycleDuration() const = 0;
    virtual void SetDayLightCycleDuration(std::chrono::minutes value) = 0;

    virtual float GetShipStrengthRandomizationDensityAdjustment() const = 0;
    virtual void SetShipStrengthRandomizationDensityAdjustment(float value) = 0;

    virtual float GetShipStrengthRandomizationExtent() const = 0;
    virtual void SetShipStrengthRandomizationExtent(float value) = 0;

    //
    // Render parameters
    //

    virtual rgbColor const & GetFlatSkyColor() const = 0;
    virtual void SetFlatSkyColor(rgbColor const & color) = 0;

    virtual bool GetDoMoonlight() const = 0;
    virtual void SetDoMoonlight(bool value) = 0;

    virtual rgbColor const & GetMoonlightColor() const = 0;
    virtual void SetMoonlightColor(rgbColor const & color) = 0;

    virtual bool GetDoCrepuscularGradient() const = 0;
    virtual void SetDoCrepuscularGradient(bool value) = 0;

    virtual rgbColor const & GetCrepuscularColor() const = 0;
    virtual void SetCrepuscularColor(rgbColor const & color) = 0;

    virtual float GetOceanTransparency() const = 0;
    virtual void SetOceanTransparency(float value) = 0;

    virtual float GetOceanDarkeningRate() const = 0;
    virtual void SetOceanDarkeningRate(float value) = 0;

    virtual float GetShipAmbientLightSensitivity() const = 0;
    virtual void SetShipAmbientLightSensitivity(float value) = 0;

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

    virtual OceanRenderModeType GetOceanRenderMode() const = 0;
    virtual void SetOceanRenderMode(OceanRenderModeType oceanRenderMode) = 0;

    virtual size_t GetTextureOceanTextureIndex() const = 0;
    virtual void SetTextureOceanTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetDepthOceanColorStart() const = 0;
    virtual void SetDepthOceanColorStart(rgbColor const & color) = 0;

    virtual rgbColor const & GetDepthOceanColorEnd() const = 0;
    virtual void SetDepthOceanColorEnd(rgbColor const & color) = 0;

    virtual rgbColor const & GetFlatOceanColor() const = 0;
    virtual void SetFlatOceanColor(rgbColor const & color) = 0;

    virtual OceanRenderDetailType GetOceanRenderDetail() const = 0;
    virtual void SetOceanRenderDetail(OceanRenderDetailType oceanRenderDetail) = 0;

    virtual LandRenderModeType GetLandRenderMode() const = 0;
    virtual void SetLandRenderMode(LandRenderModeType landRenderMode) = 0;

    virtual size_t GetTextureLandTextureIndex() const = 0;
    virtual void SetTextureLandTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetFlatLandColor() const = 0;
    virtual void SetFlatLandColor(rgbColor const & color) = 0;

    virtual ShipViewModeType GetShipViewMode() const = 0;
    virtual void SetShipViewMode(ShipViewModeType shipViewMode) = 0;

    virtual DebugShipRenderModeType GetDebugShipRenderMode() const = 0;
    virtual void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode) = 0;

    virtual VectorFieldRenderModeType GetVectorFieldRenderMode() const = 0;
    virtual void SetVectorFieldRenderMode(VectorFieldRenderModeType VectorFieldRenderMode) = 0;

    virtual bool GetShowShipStress() const = 0;
    virtual void SetShowShipStress(bool value) = 0;

    virtual bool GetShowShipFrontiers() const = 0;
    virtual void SetShowShipFrontiers(bool value) = 0;

    virtual bool GetShowAABBs() const = 0;
    virtual void SetShowAABBs(bool value) = 0;

    virtual HeatRenderModeType GetHeatRenderMode() const = 0;
    virtual void SetHeatRenderMode(HeatRenderModeType value) = 0;

    virtual float GetHeatSensitivity() const = 0;
    virtual void SetHeatSensitivity(float value) = 0;

    virtual StressRenderModeType GetStressRenderMode() const = 0;
    virtual void SetStressRenderMode(StressRenderModeType value) = 0;

    virtual bool GetDrawExplosions() const = 0;
    virtual void SetDrawExplosions(bool value) = 0;

    virtual bool GetDrawFlames() const = 0;
    virtual void SetDrawFlames(bool value) = 0;

    virtual float GetShipFlameSizeAdjustment() const = 0;
    virtual void SetShipFlameSizeAdjustment(float value) = 0;

    virtual bool GetDrawHeatBlasterFlame() const = 0;
    virtual void SetDrawHeatBlasterFlame(bool value) = 0;
};
