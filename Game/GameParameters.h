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
    static T constexpr SimulationStepTimeDuration = 1.0f / 64.0f; // 64 frames/sec == 1 second, matches Windows' timer resolution

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
    // Low-frequency updates
    //

    static int constexpr ParticleUpdateLowFrequencyPeriod = 36; // Number of simulation steps for low-frequency particle updates

    template <typename T>
    static T constexpr ParticleUpdateLowFrequencyStepTimeDuration = SimulationStepTimeDuration<T> * static_cast<T>(ParticleUpdateLowFrequencyPeriod);


    //
    // Physical Constants
    //

    // Gravity
    static vec2f constexpr Gravity = vec2f(0.0f, -9.80f);
    static vec2f constexpr GravityDir = vec2f(0.0f, -1.0f);
    static float constexpr GravityMagnitude = 9.80f; // m/s

    // Air
    static float constexpr AirMass = 1.2754f; // Kg
    static float constexpr AirPressureAtSeaLevel = 101325.0f; // No immediate relation to mass of air, due to compressibility of air; Pa == 1atm

    // Water
    static float constexpr WaterMass = 1000.0f; // Kg

    // Temperature at which all the constants are taken at
    static float constexpr Temperature0 = 298.15f; // 25C


    //
    // Tunable parameters
    //

    // General Dynamics

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
    static float constexpr MaxNumMechanicalDynamicsIterationsAdjustment = 5.0f;

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

    // 1.15 was 0.0004; now splinter springs and debris dance forever when WaterIntake is at its minimum
    // 1.16 was 0.000300009
    static float constexpr GlobalDamping = 0.00010749653315f;

    float GlobalDampingAdjustment;
    static float constexpr MinGlobalDampingAdjustment = 0.0f;
    static float constexpr MaxGlobalDampingAdjustment = 10.0f;

    float ElasticityAdjustment;
    static float constexpr MinElasticityAdjustment = 0.0f;
    static float constexpr MaxElasticityAdjustment = 4.0f;

    float StaticFrictionAdjustment;
    static float constexpr MinStaticFrictionAdjustment = 0.05f; // Keeps some sanity at lower end
    static float constexpr MaxStaticFrictionAdjustment = 40.0f;

    float KineticFrictionAdjustment;
    static float constexpr MinKineticFrictionAdjustment = 0.05f; // Keeps some sanity at lower end
    static float constexpr MaxKineticFrictionAdjustment = 40.0f;

    float RotAcceler8r;
    static float constexpr MinRotAcceler8r = 0.0f;
    static float constexpr MaxRotAcceler8r = 1000.0f;

    float StaticPressureForceAdjustment;
    static float constexpr MinStaticPressureForceAdjustment = 0.0f;
    static float constexpr MaxStaticPressureForceAdjustment = 20.0f;

    // Air

    float AirDensityAdjustment;
    static float constexpr MinAirDensityAdjustment = 0.001f;
    static float constexpr MaxAirDensityAdjustment = 1000.0f;

    static float constexpr AirFrictionDragCoefficient = 0.003f;

    float AirFrictionDragAdjustment;
    static float constexpr MinAirFrictionDragAdjustment = 0.0f;
    static float constexpr MaxAirFrictionDragAdjustment = 1000.0f;

    static float constexpr AirPressureDragCoefficient = 5.0f; // Empirical, based on balance between lightweight structure instabilities, and noticeable aerodynamics

    float AirPressureDragAdjustment;
    static float constexpr MinAirPressureDragAdjustment = 0.0f;
    static float constexpr MaxAirPressureDragAdjustment = 10.0f;

    // Water

    float WaterDensityAdjustment;
    static float constexpr MinWaterDensityAdjustment = 0.001f;
    static float constexpr MaxWaterDensityAdjustment = 100.0f;

    static float constexpr WaterFrictionDragCoefficient = 0.75f;

    float WaterFrictionDragAdjustment;
    static float constexpr MinWaterFrictionDragAdjustment = 0.0f;
    static float constexpr MaxWaterFrictionDragAdjustment = 10000.0f;

    static float constexpr WaterPressureDragCoefficient = 1030.0f; // Empirical

    float WaterPressureDragAdjustment;
    static float constexpr MinWaterPressureDragAdjustment = 0.0f;
    static float constexpr MaxWaterPressureDragAdjustment = 1000.0f;

    float WaterImpactForceAdjustment;
    static float constexpr MinWaterImpactForceAdjustment = 0.0f;
    static float constexpr MaxWaterImpactForceAdjustment = 10.0f;

    float HydrostaticPressureCounterbalanceAdjustment;
    static float constexpr MinHydrostaticPressureCounterbalanceAdjustment = 0.0f;
    static float constexpr MaxHydrostaticPressureCounterbalanceAdjustment = 1.0f;

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
    static float constexpr MinSparkleParticlesForCutVelocity = 60.0f; // 75.0f;
    static float constexpr MaxSparkleParticlesForCutVelocity = 120.0f; // 150.0f;
    static constexpr float MinSparkleParticlesForCutLifetime = 0.2f;
    static constexpr float MaxSparkleParticlesForCutLifetime = 0.5f;

    static constexpr unsigned int MinSparkleParticlesForLightningEvent = 4;
    static constexpr unsigned int MaxSparkleParticlesForLightningEvent = 10;
    static float constexpr MinSparkleParticlesForLightningVelocity = 75.0f;
    static float constexpr MaxSparkleParticlesForLightningVelocity = 150.0f;
    static constexpr float MinSparkleParticlesForLightningLifetime = 0.2f;
    static constexpr float MaxSparkleParticlesForLightningLifetime = 0.5f;

    float AirBubblesDensity;
    static float constexpr MinAirBubblesDensity = 0.0f;
    static float constexpr MaxAirBubblesDensity = 128.0f;

    static float constexpr AirBubblesDensityToCumulatedIntakenWater(float airBubblesDensity)
    {
        return 128.0f - airBubblesDensity;
    }

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

    std::chrono::seconds RogueWaveRate;
    static std::chrono::seconds constexpr MinRogueWaveRate = std::chrono::seconds(0);
    static std::chrono::seconds constexpr MaxRogueWaveRate = std::chrono::seconds(15 * 60);

    bool DoDisplaceWater;

    float WaterDisplacementWaveHeightAdjustment;
    static float constexpr MinWaterDisplacementWaveHeightAdjustment = 0.1f;
    static float constexpr MaxWaterDisplacementWaveHeightAdjustment = 2.5f;

    float WaveSmoothnessAdjustment;
    static float constexpr MinWaveSmoothnessAdjustment = 0.0f;
    static float constexpr MaxWaveSmoothnessAdjustment = 1.0f;

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

    float LightningBlastProbability; // Probability of interactive lightning hitting ship; also scales storm's probability

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

    float LaserRayHeatFlow; // KJoules/sec
    static float constexpr MinLaserRayHeatFlow = 50000.0f;
    static float constexpr MaxLaserRayHeatFlow = 2000000.0f;

    // Water reactions

    static float constexpr WaterReactionHeat = 100.0f * 1000.0f; // 100KJ, for exothermic reactions

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
    static float constexpr MaxEngineThrustAdjustment = 20.0f;

    float WaterPumpPowerAdjustment;
    static float constexpr MinWaterPumpPowerAdjustment = 0.1f;
    static float constexpr MaxWaterPumpPowerAdjustment = 20.0f;

    // Fishes

    unsigned int NumberOfFishes;
    static constexpr unsigned int MinNumberOfFishes = 0;
    static constexpr unsigned int MaxNumberOfFishes = 2056;

    float FishSizeMultiplier;
    static constexpr float MinFishSizeMultiplier = 1.0f;
    static constexpr float MaxFishSizeMultiplier = 100.0f;

    float FishSpeedAdjustment;
    static constexpr float MinFishSpeedAdjustment = 0.1f;
    static constexpr float MaxFishSpeedAdjustment = 10.0f;

    bool DoFishShoaling;

    float FishShoalRadiusAdjustment;
    static float constexpr MinFishShoalRadiusAdjustment = 0.1f;
    static float constexpr MaxFishShoalRadiusAdjustment = 100.0f;

    //
    // NPCs
    //

    static float constexpr NpcDamping = 0.0078f; // NPCs have their own as the "physics" one is applied over multiple sub-steps, while the NPCs' one is applied in one step

    float NpcSpringReductionFractionAdjustment;
    static float constexpr MinNpcSpringReductionFractionAdjustment = 0.0f;
    static float constexpr MaxNpcSpringReductionFractionAdjustment = 5.0f;

    float NpcSpringDampingCoefficientAdjustment;
    static float constexpr MinNpcSpringDampingCoefficientAdjustment = 0.0f;
    static float constexpr MaxNpcSpringDampingCoefficientAdjustment = 2.0f;

    float NpcSizeAdjustment;
    static float constexpr MinNpcSizeAdjustment = 0.2f;
    static float constexpr MaxNpcSizeAdjustment = 10.0f;

    float HumanNpcEquilibriumTorqueStiffnessCoefficient;
    static float constexpr MinHumanNpcEquilibriumTorqueStiffnessCoefficient = 0.0f;
    static float constexpr MaxHumanNpcEquilibriumTorqueStiffnessCoefficient = 0.01f;

    float HumanNpcEquilibriumTorqueDampingCoefficient;
    static float constexpr MinHumanNpcEquilibriumTorqueDampingCoefficient = 0.0f;
    static float constexpr MaxHumanNpcEquilibriumTorqueDampingCoefficient = 0.01f;

    static float constexpr HumanNpcWalkingSpeedBase = 1.0f; // m/s
    float HumanNpcWalkingSpeedAdjustment;
    static float constexpr MinHumanNpcWalkingSpeedAdjustment = 0.5f;
    static float constexpr MaxHumanNpcWalkingSpeedAdjustment = 2.5f;

    static float constexpr MaxHumanNpcTotalWalkingSpeedAdjustment = 3.5f; // Absolute cap

    static float constexpr MaxHumanNpcWalkSinSlope = 0.87f; // Max sin of slope we're willing to climb up: ___*\___<W---  (60.5 degrees)

    struct HumanNpcGeometry
    {
        static float constexpr BodyLengthMean = 1.65f;
        static float constexpr BodyLengthStdDev = 0.065f;
        static float constexpr BodyWidthNarrowMultiplierStdDev = 0.045f;
        static float constexpr BodyWidthWideMultiplierStdDev = 0.15f;

        // All fractions below ar relative to BodyLength

        static float constexpr HeadLengthFraction = 1.0f / 8.0f;
        static float constexpr HeadWidthFraction = 1.0f / 10.0f;
        static float constexpr HeadDepthFraction = HeadWidthFraction;

        static float constexpr TorsoLengthFraction = 1.0f / 2.0f - HeadLengthFraction;
        static float constexpr TorsoWidthFraction = 1.0f / 7.0f;
        static float constexpr TorsoDepthFraction = 1.0f / 6.0f;

        static float constexpr ArmLengthFraction = 3.0f / 8.0f;
        static float constexpr ArmWidthFraction = 1.0f / 10.0f;
        static float constexpr ArmDepthFraction = ArmWidthFraction;

        static float constexpr LegLengthFraction = 1.0f / 2.0f;
        static float constexpr LegWidthFraction = 1.0f / 10.0f;
        static float constexpr LegDepthFraction = LegWidthFraction;

        static_assert(LegLengthFraction + TorsoLengthFraction + HeadLengthFraction == 1.0f);

        static float constexpr StepLengthFraction = 0.43f; // From foot to foot at longest separation
    };

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

    float OceanFloorElasticityCoefficient;
    static float constexpr MinOceanFloorElasticityCoefficient = 0.0f;
    static float constexpr MaxOceanFloorElasticityCoefficient = 1.0f;

    float OceanFloorFrictionCoefficient;
    static float constexpr MinOceanFloorFrictionCoefficient = 0.05f; // Keeps some sanity at lower end
    static float constexpr MaxOceanFloorFrictionCoefficient = 1.0f;

    float OceanFloorSiltHardness;
    static float constexpr MinOceanFloorSiltHardness = 0.0f;
    static float constexpr MaxOceanFloorSiltHardness = 1.0f;

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
    static float constexpr MinDestroyRadius = 0.5f;
    static float constexpr MaxDestroyRadius = 50.0f;

    float RepairRadius;
    static float constexpr MinRepairRadius = 0.5f;
    static float constexpr MaxRepairRadius = 50.0f;

    float RepairSpeedAdjustment;
    static float constexpr MinRepairSpeedAdjustment = 0.25f;
    static float constexpr MaxRepairSpeedAdjustment = 5.0f;

    static float constexpr DrawForce = 40000.0f;

    static float constexpr SwirlForce = 600.0f;

    float BombBlastRadius;
    static float constexpr MinBombBlastRadius = 0.1f;
    static float constexpr MaxBombBlastRadius = 15.0f;

    float BombBlastForceAdjustment;
    static float constexpr MinBombBlastForceAdjustment = 0.1f;
    static float constexpr MaxBombBlastForceAdjustment = 10.0f;

    float BombBlastHeat; // KJoules/sec
    static float constexpr MinBombBlastHeat = 0.0f;
    static float constexpr MaxBombBlastHeat = 10000000.0f;

    float AntiMatterBombImplosionStrength;
    static float constexpr MinAntiMatterBombImplosionStrength = 0.1f;
    static float constexpr MaxAntiMatterBombImplosionStrength = 10.0f;

    static float constexpr BombsTemperatureTrigger = 373.15f; // 100C

    std::chrono::seconds TimerBombInterval;

    static float constexpr BombMass = 5000.0f; // Quite some fat bomb!

    float InjectPressureQuantity; // atm
    static float constexpr MinInjectPressureQuantity = 0.1f;
    static float constexpr MaxInjectPressureQuantity = 1000.0f;

    float FloodRadius;
    static float constexpr MinFloodRadius = 0.1f;
    static float constexpr MaxFloodRadius = 10.0f;

    float FloodQuantity;
    static float constexpr MinFloodQuantity = 0.1f;
    static float constexpr MaxFloodQuantity = 100.0f;

    float FireExtinguisherRadius;

    float BlastToolRadius;
    static float constexpr MinBlastToolRadius = 0.1f;
    static float constexpr MaxBlastToolRadius = 15.0f;

    float BlastToolForceAdjustment;
    static float constexpr MinBlastToolForceAdjustment = 0.1f;
    static float constexpr MaxBlastToolForceAdjustment = 10.0f;

    float ScrubRotToolRadius;
    static float constexpr MinScrubRotToolRadius = 1.0f;
    static float constexpr MaxScrubRotToolRadius = 20.0f;

    float WindMakerToolWindSpeed; // Beaufort scale, km/h
    static float constexpr MinWindMakerToolWindSpeed = 20.0f;
    static float constexpr MaxWindMakerToolWindSpeed = 200.0f;

    bool IsUltraViolentMode;

    float MoveToolInertia;

    //
    // Limits
    //

    static float constexpr MaxWorldWidth = 10000.0f;
    static float constexpr HalfMaxWorldWidth = MaxWorldWidth / 2.0f;

    static float constexpr MaxWorldHeight = 22000.0f;
    static float constexpr HalfMaxWorldHeight = MaxWorldHeight / 2.0f;

    static_assert(HalfMaxWorldHeight >= MaxSeaDepth); // Make sure deepest bottom of the ocean is visible

    static size_t constexpr MaxSpringsPerPoint = 8u + 1u; // 8 neighbours and 1 rope spring, when this is a rope endpoint
    static size_t constexpr MaxTrianglesPerPoint = 8u;

    static size_t constexpr MaxNpcs = 1024u;
    static size_t constexpr MaxParticlesPerNpc = 4u;
    static size_t constexpr MaxSpringsPerNpc = 6u;

    static size_t constexpr MaxGadgets = 64u;
    static size_t constexpr MaxPinnedPoints = 64u;
    static size_t constexpr MaxThanosSnaps = 8u;

    static unsigned int constexpr EngineControllerTelegraphDegreesOfFreedom = 11;
    static_assert((EngineControllerTelegraphDegreesOfFreedom % 2) != 0); // Make sure there's room for central position, and it's symmetric

    static float constexpr EngineControllerJetThrottleIdleFraction = 0.20f; // Fraction of 0.0->1.0 range occupied by "idle"

private:

    //
    // The basis number of iterations we run in the mechanical dynamics update for
    // each simulation step.
    //
    // The actual number of iterations is the product of this value with
    // MechanicalIterationsAdjust.
    //

    static constexpr size_t BasisNumMechanicalDynamicsIterations = 40;
};
