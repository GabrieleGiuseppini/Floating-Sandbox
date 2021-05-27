/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameParameters.h"

GameParameters::GameParameters()
    // Dynamics
    : NumMechanicalDynamicsIterationsAdjustment(1.0f)
    , SpringStiffnessAdjustment(1.0f)
    , SpringDampingAdjustment(1.0f)
    , SpringStrengthAdjustment(1.0f)
    , GlobalDampingAdjustment(1.0f)
    , RotAcceler8r(1.0f)
    // Air
    , AirFrictionDragAdjustment(1.0f)
    , AirPressureDragAdjustment(1.0f)
    // Water
    , WaterDensityAdjustment(1.0f)
    , WaterFrictionDragAdjustment(1.0f)
    , WaterPressureDragAdjustment(1.0f)
    , HydrostaticPressureAdjustment(1.0f)
    , WaterIntakeAdjustment(1.0f)
    , WaterDiffusionSpeedAdjustment(1.0f)
    , WaterCrazyness(0.8125f)
    // Ephemeral particles
    , DoGenerateDebris(true)
    , SmokeEmissionDensityAdjustment(1.0f)
    , SmokeParticleLifetimeAdjustment(1.0f)
    , DoGenerateSparklesForCuts(true)
    , AirBubblesDensity(120.0f)
    , DoGenerateEngineWakeParticles(true)
    // Wind
    , DoModulateWind(true)
    , WindSpeedBase(-20.0f)
    , WindSpeedMaxFactor(2.5f)
    , WindGustFrequencyAdjustment(1.0f)
    // Waves
    , BasalWaveHeightAdjustment(1.0f)
    , BasalWaveLengthAdjustment(1.0f)
    , BasalWaveSpeedAdjustment(2.0f)
    , TsunamiRate(12)
    , RogueWaveRate(32)
    , DoDisplaceWater(true)
    , WaterDisplacementWaveHeightAdjustment(1.0f)
    , WaveSmoothnessAdjustment(0.203125f)
    // Storm
    , StormRate(60)
    , StormDuration(60 * 4) // 4 minutes
    , StormStrengthAdjustment(1.0f)
    , LightningBlastRadius(8.0f)
    , LightningBlastHeat(4000.0f)
    , DoRainWithStorm(true)
    , RainFloodAdjustment(10000.0f) // Partially visible after 4 minutes
    // Heat and combustion
    , AirTemperature(298.15f) // 25C
    , WaterTemperature(288.15f) // 15C
    , MaxBurningParticles(112)
    , ThermalConductivityAdjustment(1.0f)
    , HeatDissipationAdjustment(1.0f)
    , IgnitionTemperatureAdjustment(1.0f)
    , MeltingTemperatureAdjustment(1.0f)
    , CombustionSpeedAdjustment(1.0f)
    , CombustionHeatAdjustment(1.0f)
    , HeatBlasterHeatFlow(2000.0f) // 900KJ: 80kg of iron (~=1 particle) get dT=1500 in 60 seconds
    , HeatBlasterRadius(8.0f)
    // Electricals
    , LuminiscenceAdjustment(1.0f)
    , LightSpreadAdjustment(1.0f)
    , ElectricalElementHeatProducedAdjustment(1.0f)
    , DoShowElectricalNotifications(true)
    , EngineThrustAdjustment(1.0f)
    , WaterPumpPowerAdjustment(1.0f)
    // Fishes
    , NumberOfFishes(76)
    , FishSizeMultiplier(25.0f)
    , FishSpeedAdjustment(1.0f)
    , DoFishShoaling(true)
    , FishShoalRadiusAdjustment(1.0f)
    // Misc
    , SeaDepth(300.0f)
    , OceanFloorBumpiness(1.0f)
    , OceanFloorDetailAmplification(10.0f)
    , OceanFloorElasticity(0.5f)
    , OceanFloorFriction(0.25f)
    , NumberOfStars(1536)
    , NumberOfClouds(48)
    , DoDayLightCycle(false)
    , DayLightCycleDuration(std::chrono::minutes(4))
    // Interactions
    , ToolSearchRadius(2.0f)
    , DestroyRadius(0.5f)
    , RepairRadius(4.0f)
    , RepairSpeedAdjustment(1.0f)
    , BombBlastRadius(6.0f)
    , BombBlastForceAdjustment(1.0f)
    , BombBlastHeat(50000.0f)
    , AntiMatterBombImplosionStrength(3.0f)
    , TimerBombInterval(10)
    , FloodRadius(0.75f)
    , FloodQuantity(1.0f)
    , FireExtinguisherRadius(5.0f)
    , BlastToolRadius(6.0f)
    , BlastToolForceAdjustment(1.0f)
    , ScrubRotRadius(5.0f)
    , IsUltraViolentMode(false)
    , MoveToolInertia(3.0f)
{
}