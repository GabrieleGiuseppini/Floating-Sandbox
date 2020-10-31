/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <chrono>

/*
 * Parameters that affect the game's physics and its world.
 */
struct GameParameters
{
    GameParameters();

    //
    // The dt of each step
    //

    template <typename T>
    static constexpr T SimulationStepTimeDuration = 1.0f / 64.0f; // 64 frames/sec == 1 second, matches Windows' timer resolution

    template <typename T>
    inline T MechanicalSimulationStepTimeDuration() const
    {
        return MechanicalSimulationStepTimeDuration(NumMechanicalDynamicsIterations<T>());
    }

    template <typename T>
    static inline T MechanicalSimulationStepTimeDuration(T numMechanicalDynamicsIterations)
    {
        return SimulationStepTimeDuration<T> / numMechanicalDynamicsIterations;
    }


    //
    // The low-frequency update dt
    //

    template <typename T>
    static constexpr T LowFrequencySimulationStepTimeDuration = 1.0f;


    //
    // Physical Constants
    //

    // Gravity
    static constexpr vec2f Gravity = vec2f(0.0f, -9.80f);
    static constexpr vec2f GravityNormalized = vec2f(0.0f, -1.0f);
    static float constexpr GravityMagnitude = 9.80f; // m/s

    // Air
    static float constexpr AirMass = 1.2754f; // Kg

    // Water
    static float constexpr WaterMass = 1000.0f; // Kg

    // Temperature at which all the constants are taken at
    static float constexpr Temperature0 = 298.15f; // 25C



    //
    // Tunable parameters
    //

    // Dynamics

    // Fraction of a spring displacement that is removed during a spring relaxation
    // iteration. The remaining spring displacement is (1.0 - this fraction).
    static float constexpr SpringReductionFraction = 0.5f; // Before 1.15.2 was 0.4, and materials stiffnesses were higher

    // The empirically-determined constant for the spring damping.
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode
    static float constexpr SpringDampingCoefficient = 0.03f;

    //
    // The number of mechanical iterations dictates how stiff bodies are:
    // - Less iterations => softer (jelly) body
    // - More iterations => hard body (never breaks though)
    //

    float NumMechanicalDynamicsIterationsAdjustment;
    static float constexpr MinNumMechanicalDynamicsIterationsAdjustment = 0.5f;
    static float constexpr MaxNumMechanicalDynamicsIterationsAdjustment = 20.0f;

    template <typename T>
    inline T NumMechanicalDynamicsIterations() const
    {
        return static_cast<T>(
            static_cast<float>(BasisNumMechanicalDynamicsIterations)
            * NumMechanicalDynamicsIterationsAdjustment);
    }

    float SpringStiffnessAdjustment;
    static float constexpr MinSpringStiffnessAdjustment = 0.001f;
    static float constexpr MaxSpringStiffnessAdjustment = 2.0f;

    float SpringDampingAdjustment;
    static float constexpr MinSpringDampingAdjustment = 0.001f;
    static float constexpr MaxSpringDampingAdjustment = 4.0f;

    float SpringStrengthAdjustment;
    static float constexpr MinSpringStrengthAdjustment = 0.01f;
    static float constexpr MaxSpringStrengthAdjustment = 50.0f;

    static float constexpr GlobalDamping = 0.0004f; // // We've shipped 1.7.5 with 0.0003, but splinter springs used to dance for too long

    float GlobalDampingAdjustment;
    static float constexpr MinGlobalDampingAdjustment = 0.0f;
    static float constexpr MaxGlobalDampingAdjustment = 10.0f;

    float RotAcceler8r;
    static float constexpr MinRotAcceler8r = 0.0f;
    static float constexpr MaxRotAcceler8r = 1000.0f;

    // Water

    float WaterDensityAdjustment;
    static float constexpr MinWaterDensityAdjustment = 0.0f;
    static float constexpr MaxWaterDensityAdjustment = 4.0f;

    static float constexpr WaterDragLinearCoefficient =
        0.020f  // ~= 1.0f - powf(0.6f, 0.02f)
        * 5.0f;  // Once we were comfortable with square law at |v|=5, now we use linear law and want to maintain the same force there

    float WaterDragAdjustment;
    static float constexpr MinWaterDragAdjustment = 0.0f;
    static float constexpr MaxWaterDragAdjustment = 1000.0f; // Safe to avoid drag instability (2 * m / (dt * C) at minimal mass, 1Kg)

    float WaterIntakeAdjustment;
    static float constexpr MinWaterIntakeAdjustment = 0.001f;
    static float constexpr MaxWaterIntakeAdjustment = 10.0f;

    float WaterDiffusionSpeedAdjustment;
    static float constexpr MinWaterDiffusionSpeedAdjustment = 0.001f;
    static float constexpr MaxWaterDiffusionSpeedAdjustment = 2.0f;

    float WaterCrazyness;
    static float constexpr MinWaterCrazyness = 0.0f;
    static float constexpr MaxWaterCrazyness = 2.0f;

    // Ephemeral particles

    static constexpr ElementCount MaxEphemeralParticles = 4096;

    bool DoGenerateDebris;
    static constexpr unsigned int MinDebrisParticlesPerEvent = 4;
    static constexpr unsigned int MaxDebrisParticlesPerEvent = 9;
    static float constexpr MinDebrisParticlesVelocity = 12.5f;
    static float constexpr MaxDebrisParticlesVelocity = 20.0f;
    static constexpr float MinDebrisParticlesLifetime = 0.4f;
    static constexpr float MaxDebrisParticlesLifetime = 0.9f;

    float SmokeEmissionDensityAdjustment;
    static float constexpr MinSmokeEmissionDensityAdjustment = 0.1f;
    static float constexpr MaxSmokeEmissionDensityAdjustment = 10.0f;
    static constexpr float MinSmokeParticlesLifetime = 3.5f;
    static constexpr float MaxSmokeParticlesLifetime = 6.0f;
    float SmokeParticleLifetimeAdjustment;
    static float constexpr MinSmokeParticleLifetimeAdjustment = 0.1f;
    static float constexpr MaxSmokeParticleLifetimeAdjustment = 10.0f;

    bool DoGenerateSparklesForCuts;
    static constexpr unsigned int MinSparkleParticlesForCutEvent = 4;
    static constexpr unsigned int MaxSparkleParticlesForCutEvent = 10;
    static float constexpr MinSparkleParticlesForCutVelocity = 75.0f;
    static float constexpr MaxSparkleParticlesForCutVelocity = 150.0f;
    static constexpr float MinSparkleParticlesForCutLifetime = 0.2f;
    static constexpr float MaxSparkleParticlesForCutLifetime = 0.5f;

    static constexpr unsigned int MinSparkleParticlesForLightningEvent = 4;
    static constexpr unsigned int MaxSparkleParticlesForLightningEvent = 10;
    static float constexpr MinSparkleParticlesForLightningVelocity = 75.0f;
    static float constexpr MaxSparkleParticlesForLightningVelocity = 150.0f;
    static constexpr float MinSparkleParticlesForLightningLifetime = 0.2f;
    static constexpr float MaxSparkleParticlesForLightningLifetime = 0.5f;

    bool DoGenerateAirBubbles;
    float CumulatedIntakenWaterThresholdForAirBubbles;
    static float constexpr MinCumulatedIntakenWaterThresholdForAirBubbles = 2.0f;
    static float constexpr MaxCumulatedIntakenWaterThresholdForAirBubbles = 128.0f;

    bool DoDisplaceOceanSurfaceAtAirBubblesSurfacing;

    bool DoGenerateEngineWakeParticles;

    // Wind

    static constexpr vec2f WindDirection = vec2f(1.0f, 0.0f);

    bool DoModulateWind;

    float WindSpeedBase; // Beaufort scale, km/h
    static float constexpr MinWindSpeedBase = -100.f;
    static float constexpr MaxWindSpeedBase = 100.0f;

    float WindSpeedMaxFactor; // Multiplier on base
    static float constexpr MinWindSpeedMaxFactor = 1.f;
    static float constexpr MaxWindSpeedMaxFactor = 10.0f;

    float WindGustFrequencyAdjustment;
    static float constexpr MinWindGustFrequencyAdjustment = 0.1f;
    static float constexpr MaxWindGustFrequencyAdjustment = 10.0f;

    // Waves

    float BasalWaveHeightAdjustment;
    static float constexpr MinBasalWaveHeightAdjustment = 0.0f;
    static float constexpr MaxBasalWaveHeightAdjustment = 100.0f;

    float BasalWaveLengthAdjustment;
    static float constexpr MinBasalWaveLengthAdjustment = 0.3f;
    static float constexpr MaxBasalWaveLengthAdjustment = 20.0f;

    float BasalWaveSpeedAdjustment;
    static float constexpr MinBasalWaveSpeedAdjustment = 0.75f;
    static float constexpr MaxBasalWaveSpeedAdjustment = 20.0f;

    std::chrono::minutes TsunamiRate;
    static std::chrono::minutes constexpr MinTsunamiRate = std::chrono::minutes(0);
    static std::chrono::minutes constexpr MaxTsunamiRate = std::chrono::minutes(240);

    std::chrono::minutes RogueWaveRate;
    static std::chrono::minutes constexpr MinRogueWaveRate = std::chrono::minutes(0);
    static std::chrono::minutes constexpr MaxRogueWaveRate = std::chrono::minutes(15);

    // Storm

    std::chrono::minutes StormRate;
    static std::chrono::minutes constexpr MinStormRate = std::chrono::minutes(0);
    static std::chrono::minutes constexpr MaxStormRate = std::chrono::minutes(120);

    std::chrono::seconds StormDuration;
    static std::chrono::seconds constexpr MinStormDuration = std::chrono::seconds(10);
    static std::chrono::seconds constexpr MaxStormDuration = std::chrono::seconds(60 * 20);

    float StormStrengthAdjustment;
    static float constexpr MinStormStrengthAdjustment = 0.1f;
    static float constexpr MaxStormStrengthAdjustment = 10.0f;

    float LightningBlastRadius;

    float LightningBlastHeat; // KJoules/sec

    bool DoRainWithStorm;

    // Conversion between adimensional rain density and m/h:
    // rain quantity (in m/h) at density = 1.0
    static float constexpr MaxRainQuantity = 0.05f; // 50mm/h == violent shower

    // How much rain affects water intaken
    float RainFloodAdjustment;
    static float constexpr MinRainFloodAdjustment = 0.0f;
    static float constexpr MaxRainFloodAdjustment = 3600.0f / (MaxRainQuantity * SimulationStepTimeDuration<float>); // Guarantees that max is one meter/frame

    // Heat and combustion

    float AirTemperature; // Kelvin
    static float constexpr MinAirTemperature = 273.15f; // 0C
    static float constexpr MaxAirTemperature = 2073.15f; // 1800C

    static float constexpr AirConvectiveHeatTransferCoefficient = 100.45f; // J/(s*m2*K) - arbitrary, higher than real

    static float constexpr AirThermalExpansionCoefficient = 0.0034f; // 1/K

    float WaterTemperature; // Kelvin
    static float constexpr MinWaterTemperature = 273.15f; // 0C
    static float constexpr MaxWaterTemperature = 2073.15f; // 1800C

    static float constexpr WaterConvectiveHeatTransferCoefficient = 2500.0f; // J/(s*m2*K) - arbitrary, higher than real

    static float constexpr WaterThermalExpansionCoefficient = 0.000207f; // 1/K

    static float constexpr IgnitionTemperatureHighWatermark = 0.0f;
    static float constexpr IgnitionTemperatureLowWatermark = -30.0f;

    static float constexpr SmotheringWaterLowWatermark = 0.05f;
    static float constexpr SmotheringWaterHighWatermark = 0.1f;

    static float constexpr SmotheringDecayLowWatermark = 0.0005f;
    static float constexpr SmotheringDecayHighWatermark = 0.05f;

    unsigned int MaxBurningParticles;
    static unsigned int constexpr MaxMaxBurningParticles = 1000;
    static unsigned int constexpr MinMaxBurningParticles = 10;

    float ThermalConductivityAdjustment;
    static float constexpr MinThermalConductivityAdjustment = 0.1f;
    static float constexpr MaxThermalConductivityAdjustment = 100.0f;

    float HeatDissipationAdjustment;
    static float constexpr MinHeatDissipationAdjustment = 0.01f;
    static float constexpr MaxHeatDissipationAdjustment = 20.0f;

    float IgnitionTemperatureAdjustment;
    static float constexpr MinIgnitionTemperatureAdjustment = 0.1f;
    static float constexpr MaxIgnitionTemperatureAdjustment = 1000.0f;

    float MeltingTemperatureAdjustment;
    static float constexpr MinMeltingTemperatureAdjustment = 0.1f;
    static float constexpr MaxMeltingTemperatureAdjustment = 1000.0f;

    float CombustionSpeedAdjustment;
    static float constexpr MinCombustionSpeedAdjustment = 0.1f;
    static float constexpr MaxCombustionSpeedAdjustment = 100.0f;

    static float constexpr CombustionHeat = 100.0f * 1000.0f; // 100KJ

    float CombustionHeatAdjustment;
    static float constexpr MinCombustionHeatAdjustment = 0.1f;
    static float constexpr MaxCombustionHeatAdjustment = 100.0f;

    float HeatBlasterHeatFlow; // KJoules/sec
    static float constexpr MinHeatBlasterHeatFlow = 200.0f;
    static float constexpr MaxHeatBlasterHeatFlow = 100000.0f;

    float HeatBlasterRadius;
    static float constexpr MinHeatBlasterRadius = 1.0f;
    static float constexpr MaxHeatBlasterRadius = 100.0f;

    // Electricals

    float LuminiscenceAdjustment;
    static float constexpr MinLuminiscenceAdjustment = 0.0f;
    static float constexpr MaxLuminiscenceAdjustment = 4.0f;

    float LightSpreadAdjustment;
    static float constexpr MinLightSpreadAdjustment = 0.0f;
    static float constexpr MaxLightSpreadAdjustment = 10.0f;

    float ElectricalElementHeatProducedAdjustment;
    static float constexpr MinElectricalElementHeatProducedAdjustment = 0.0f;
    static float constexpr MaxElectricalElementHeatProducedAdjustment = 1000.0f;

    bool DoShowElectricalNotifications;

    float EngineThrustAdjustment;
    static float constexpr MinEngineThrustAdjustment = 0.1f;
    static float constexpr MaxEngineThrustAdjustment = 10.0f;

    float WaterPumpPowerAdjustment;
    static float constexpr MinWaterPumpPowerAdjustment = 0.1f;
    static float constexpr MaxWaterPumpPowerAdjustment = 20.0f;

    // Fishes

    unsigned int NumberOfFishes;
    static constexpr unsigned int MinNumberOfFishes = 0;
    static constexpr unsigned int MaxNumberOfFishes = 2056;

    float FishSizeAdjustment;
    static constexpr float MinFishSizeAdjustment = 1.0f;
    static constexpr float MaxFishSizeAdjustment = 100.0f;

    // Misc

    float SeaDepth;
    static float constexpr MinSeaDepth = -50.0f;
    static float constexpr MaxSeaDepth = 10000.0f;

    // The number of ocean floor terrain samples for the entire world width;
    // a higher value means more resolution, at the expense of cache misses
    template <typename T>
    static T constexpr OceanFloorTerrainSamples = 2048;

    float OceanFloorBumpiness;
    static float constexpr MinOceanFloorBumpiness = 0.0f;
    static float constexpr MaxOceanFloorBumpiness = 6.0f;

    float OceanFloorDetailAmplification;
    static float constexpr MinOceanFloorDetailAmplification = 0.0f;
    static float constexpr MaxOceanFloorDetailAmplification = 200.0f;

    float OceanFloorElasticity;
    static float constexpr MinOceanFloorElasticity = 0.0f;
    static float constexpr MaxOceanFloorElasticity = 0.95f;

    float OceanFloorFriction;
    static float constexpr MinOceanFloorFriction = 0.05f;
    static float constexpr MaxOceanFloorFriction = 1.0f;

    unsigned int NumberOfStars;
    static constexpr unsigned int MinNumberOfStars = 0;
    static constexpr unsigned int MaxNumberOfStars = 10000;

    unsigned int NumberOfClouds;
    static constexpr unsigned int MinNumberOfClouds = 0;
    static constexpr unsigned int MaxNumberOfClouds = 500;

    bool DoDayLightCycle;

    std::chrono::minutes DayLightCycleDuration;
    static std::chrono::minutes constexpr MinDayLightCycleDuration = std::chrono::minutes(1);
    static std::chrono::minutes constexpr MaxDayLightCycleDuration = std::chrono::minutes(60);

    // Interactions

    float ToolSearchRadius;

    float DestroyRadius;
    static float constexpr MinDestroyRadius = 5.0f;
    static float constexpr MaxDestroyRadius = 100.0f;

    float RepairRadius;
    static float constexpr MinRepairRadius = 0.1f;
    static float constexpr MaxRepairRadius = 10.0f;

    float RepairSpeedAdjustment;
    static float constexpr MinRepairSpeedAdjustment = 0.25f;
    static float constexpr MaxRepairSpeedAdjustment = 10.0f;

    static float constexpr DrawForce = 40000.0f;

    static float constexpr SwirlForce = 600.0f;

    float BombBlastRadius;
    static float constexpr MinBombBlastRadius = 0.1f;
    static float constexpr MaxBombBlastRadius = 20.0f;

    float BombBlastForceAdjustment;
    static float constexpr MinBombBlastForceAdjustment = 0.1f;
    static float constexpr MaxBombBlastForceAdjustment = 100.0f;

    float BombBlastHeat; // KJoules/sec
    static float constexpr MinBombBlastHeat = 0.0f;
    static float constexpr MaxBombBlastHeat = 10000000.0f;

    float AntiMatterBombImplosionStrength;
    static float constexpr MinAntiMatterBombImplosionStrength = 0.1f;
    static float constexpr MaxAntiMatterBombImplosionStrength = 10.0f;

    static float constexpr BombNeighborhoodRadius = 3.5f;

    static float constexpr BombsTemperatureTrigger = 373.15f; // 100C

    std::chrono::seconds TimerBombInterval;

    float BombMass;

    float FloodRadius;
    static float constexpr MinFloodRadius = 0.1f;
    static float constexpr MaxFloodRadius = 10.0f;

    float FloodQuantity;
    static float constexpr MinFloodQuantity = 0.1f;
    static float constexpr MaxFloodQuantity = 100.0f;

    float FireExtinguisherRadius;

    float ScrubRadius;

    bool IsUltraViolentMode;

    float MoveToolInertia;

    //
    // Limits
    //

    static float constexpr MaxWorldWidth = 5000.0f;
    static float constexpr HalfMaxWorldWidth = MaxWorldWidth / 2.0f;

    static float constexpr MaxWorldHeight = 22000.0f;
    static float constexpr HalfMaxWorldHeight = MaxWorldHeight / 2.0f;

    static_assert(HalfMaxWorldHeight >= MaxSeaDepth); // Make sure deepest bottom of the ocean is visible


    static size_t constexpr MaxBombs = 64u;
    static size_t constexpr MaxPinnedPoints = 64u;
    static size_t constexpr MaxThanosSnaps = 8u;

    static size_t constexpr MaxSpringsPerPoint = 8u + 1u; // 8 neighbours and 1 rope spring, when this is a rope endpoint
    static size_t constexpr MaxTrianglesPerPoint = 8u;

    static unsigned int constexpr EngineTelegraphDegreesOfFreedom = 11;
    static_assert((EngineTelegraphDegreesOfFreedom % 2) != 0); // Make sure there's room for central position, and it's symmetric

private:

    //
    // The basis number of iterations we run in the mechanical dynamics update for
    // each simulation step.
    //
    // The actual number of iterations is the product of this value with
    // MechanicalIterationsAdjust.
    //

    static constexpr size_t BasisNumMechanicalDynamicsIterations = 30;
};
