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
* Parameters that affect the game (physics, world).
*/
struct GameParameters
{
    GameParameters();

    //
    // The dt of each step
    //

    template <typename T>
    static constexpr T SimulationStepTimeDuration = 0.02f;

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
    static constexpr float GravityMagnitude = 9.80f;

    // Air mass
    static constexpr float AirMass = 1.2754f;

    // Water mass
    static constexpr float WaterMass = 1000.0f;



    //
    // Tunable parameters
    //

    // Dynamics

    // Fraction of a spring displacement that is removed during a spring relaxation
    // iteration. The remaining spring displacement is (1.0 - this fraction).
    static constexpr float SpringReductionFraction = 0.4f;

    // The empirically-determined constant for the spring damping.
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode
    static constexpr float SpringDampingCoefficient = 0.03f;

    //
    // The number of mechanical iterations dictates how stiff bodies are:
    // - Less iterations => softer (jelly) body
    // - More iterations => hard body (never breaks though)
    //

    float NumMechanicalDynamicsIterationsAdjustment;
    static constexpr float MinNumMechanicalDynamicsIterationsAdjustment = 0.5f;
    static constexpr float MaxNumMechanicalDynamicsIterationsAdjustment = 20.0f;

    template <typename T>
    inline T NumMechanicalDynamicsIterations() const
    {
        return static_cast<T>(
            static_cast<float>(BasisNumMechanicalDynamicsIterations)
            * NumMechanicalDynamicsIterationsAdjustment);
    }

    float SpringStiffnessAdjustment;
    static constexpr float MinSpringStiffnessAdjustment = 0.001f;
    static constexpr float MaxSpringStiffnessAdjustment = 2.4f;

    float SpringDampingAdjustment;
    static constexpr float MinSpringDampingAdjustment = 0.001f;
    static constexpr float MaxSpringDampingAdjustment = 4.0f;

    float SpringStrengthAdjustment;
    static constexpr float MinSpringStrengthAdjustment = 0.01f;
    static constexpr float MaxSpringStrengthAdjustment = 10.0f;

    static constexpr float GlobalDamp = 0.9996f; // // We've shipped 1.7.5 with 0.9997, but splinter springs used to dance for too long

    float RotAcceler8r;
    static constexpr float MinRotAcceler8r = 0.0f;
    static constexpr float MaxRotAcceler8r = 1000.0f;

    // Water

    float WaterDensityAdjustment;
    static constexpr float MinWaterDensityAdjustment = 0.0f;
    static constexpr float MaxWaterDensityAdjustment = 4.0f;

    static constexpr float WaterDragLinearCoefficient =
        0.020f  // ~= 1.0f - powf(0.6f, 0.02f)
        * 5.0f;  // Once we were comfortable with square law at |v|=5, now we use linear law and want to maintain the same force there

    float WaterDragAdjustment;
    static constexpr float MinWaterDragAdjustment = 0.0f;
    static constexpr float MaxWaterDragAdjustment = 1000.0f; // Safe to avoid drag instability (2 * m / (dt * C) at minimal mass, 1Kg)

    float WaterIntakeAdjustment;
    static constexpr float MinWaterIntakeAdjustment = 0.1f;
    static constexpr float MaxWaterIntakeAdjustment = 10.0f;

    float WaterDiffusionSpeedAdjustment;
    static constexpr float MinWaterDiffusionSpeedAdjustment = 0.001f;
    static constexpr float MaxWaterDiffusionSpeedAdjustment = 2.0f;

    float WaterCrazyness;
    static constexpr float MinWaterCrazyness = 0.0f;
    static constexpr float MaxWaterCrazyness = 2.0f;

    // Ephemeral particles

    static constexpr ElementCount MaxEphemeralParticles = 4096;

    bool DoGenerateDebris;
    static constexpr size_t MinDebrisParticlesPerEvent = 4;
    static constexpr size_t MaxDebrisParticlesPerEvent = 9;
    static constexpr float MinDebrisParticlesVelocity = 12.5f;
    static constexpr float MaxDebrisParticlesVelocity = 20.0f;
    static constexpr std::chrono::milliseconds MinDebrisParticlesLifetime = std::chrono::milliseconds(400);
    static constexpr std::chrono::milliseconds MaxDebrisParticlesLifetime = std::chrono::milliseconds(900);

    bool DoGenerateSparkles;
    static constexpr size_t MinSparkleParticlesPerEvent = 2;
    static constexpr size_t MaxSparkleParticlesPerEvent = 8;
    static constexpr float MinSparkleParticlesVelocity = 50.0f;
    static constexpr float MaxSparkleParticlesVelocity = 70.0f;
    static constexpr std::chrono::milliseconds MinSparkleParticlesLifetime = std::chrono::milliseconds(200);
    static constexpr std::chrono::milliseconds MaxSparkleParticlesLifetime = std::chrono::milliseconds(500);

    bool DoGenerateAirBubbles;
    static constexpr float CumulatedIntakenWaterThresholdForAirBubbles = 7.0f;
    static constexpr float MinAirBubblesVortexAmplitude = 0.05f;
    static constexpr float MaxAirBubblesVortexAmplitude = 2.0f;
    static constexpr float MinAirBubblesVortexFrequency = 1.0f;
    static constexpr float MaxAirBubblesVortexFrequency = 2.5f;

    // Wind

    static constexpr vec2f WindDirection = vec2f(1.0f, 0.0f);

    bool DoModulateWind;

    float WindSpeedBase; // Beaufort scale, km/h
    static constexpr float MinWindSpeedBase = -100.f;
    static constexpr float MaxWindSpeedBase = 100.0f;

    float WindSpeedMaxFactor; // Multiplier on base
    static constexpr float MinWindSpeedMaxFactor = 1.f;
    static constexpr float MaxWindSpeedMaxFactor = 10.0f;

    float WindGustFrequencyAdjustment;
    static constexpr float MinWindGustFrequencyAdjustment = 0.1f;
    static constexpr float MaxWindGustFrequencyAdjustment = 10.0f;

    // Waves

    float BasalWaveHeightAdjustment;
    static constexpr float MinBasalWaveHeightAdjustment = 0.0f;
    static constexpr float MaxBasalWaveHeightAdjustment = 100.0f;

    float BasalWaveLengthAdjustment;
    static constexpr float MinBasalWaveLengthAdjustment = 0.5f;
    static constexpr float MaxBasalWaveLengthAdjustment = 30.0f;

    float BasalWaveSpeedAdjustment;
    static constexpr float MinBasalWaveSpeedAdjustment = 0.75f;
    static constexpr float MaxBasalWaveSpeedAdjustment = 20.0f;

    // Misc

    float SeaDepth;
    static constexpr float MinSeaDepth = 20.0f;
    static constexpr float MaxSeaDepth = 10000.0f;

    float OceanFloorBumpiness;
    static constexpr float MinOceanFloorBumpiness = 0.0f;
    static constexpr float MaxOceanFloorBumpiness = 6.0f;

    float OceanFloorDetailAmplification;
    static constexpr float MinOceanFloorDetailAmplification = 0.0f;
    static constexpr float MaxOceanFloorDetailAmplification = 200.0f;

    float LuminiscenceAdjustment;
    static constexpr float MinLuminiscenceAdjustment = 0.1f;
    static constexpr float MaxLuminiscenceAdjustment = 10.0f;

    float LightSpreadAdjustment;
    static constexpr float MinLightSpreadAdjustment = 0.0f;
    static constexpr float MaxLightSpreadAdjustment = 5.0f;

    size_t NumberOfStars;
    static constexpr size_t MinNumberOfStars = 0;
    static constexpr size_t MaxNumberOfStars = 10000;

    size_t NumberOfClouds;
    static constexpr size_t MinNumberOfClouds = 0;
    static constexpr size_t MaxNumberOfClouds = 500;

    // Interactions

    float ToolSearchRadius;

    float DestroyRadius;
    static constexpr float MinDestroyRadius = 5.0f;
    static constexpr float MaxDestroyRadius = 100.0f;

    float RepairRadius;

    static constexpr float DrawForce = 40000.0f;

    static constexpr float SwirlForce = 600.0f;

    float BombBlastRadius;
    static constexpr float MinBombBlastRadius = 0.1f;
    static constexpr float MaxBombBlastRadius = 20.0f;

    float AntiMatterBombImplosionStrength;
    static constexpr float MinAntiMatterBombImplosionStrength = 0.1f;
    static constexpr float MaxAntiMatterBombImplosionStrength = 10.0f;

    static constexpr float BombNeighborhoodRadius = 3.5f;

    std::chrono::seconds TimerBombInterval;

    float BombMass;

    float FloodRadius;
    static constexpr float MinFloodRadius = 0.1f;
    static constexpr float MaxFloodRadius = 10.0f;

    float FloodQuantity;
    static constexpr float MinFloodQuantity = 0.1f;
    static constexpr float MaxFloodQuantity = 100.0f;

    float ScrubRadius;

    bool IsUltraViolentMode;

    float MoveToolInertia;

    //
    // Limits
    //

    static constexpr float MaxWorldWidth = 10000.0f;
    static constexpr float HalfMaxWorldWidth = MaxWorldWidth / 2.0f;

    static constexpr float MaxWorldHeight = 40000.0f;
    static constexpr float HalfMaxWorldHeight = MaxWorldHeight / 2.0f;

    static_assert(MaxWorldHeight >= MaxSeaDepth * 2);


    static constexpr size_t MaxBombs = 64u;
    static constexpr size_t MaxPinnedPoints = 64u;

    // 8 neighbours and 1 rope spring, when this is a rope endpoint
    static constexpr size_t MaxSpringsPerPoint = 8u + 1u;

    static constexpr size_t MaxTrianglesPerPoint = 8u;


private:

    //
    // The basis number of iterations we run in the mechanical dynamics update for
    // each simulation step.
    //
    // The actual number of iterations is the product of this value with
    // MechanicalIterationsAdjust.
    //

    static constexpr size_t BasisNumMechanicalDynamicsIterations = 24;
};
