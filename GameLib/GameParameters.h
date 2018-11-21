/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "Vectors.h"

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


    //
    // The number of iterations we run in the various dynamics updates for
    // each simulation step
    //
    // The number of mechanical iterations dictates how stiff bodies are:
    // - Less iterations => softer (jelly) body
    // - More iterations => hard body (never breaks though) 
    //

    template <typename T>
    static constexpr T NumMechanicalDynamicsIterations = 12;

    template <typename T>
    static constexpr T NumWaterDynamicsIterations = 1;


    //
    // The dt of each iteration in the dynamics updates
    //

    template <typename T>
    static constexpr T MechanicalDynamicsSimulationStepTimeDuration = SimulationStepTimeDuration<T> / NumMechanicalDynamicsIterations<T>;

    template <typename T>
    static constexpr T WaterDynamicsSimulationStepTimeDuration = SimulationStepTimeDuration<T> / NumWaterDynamicsIterations<T>;


    //
    // Gravity
    //

    static constexpr vec2f Gravity = vec2f(0.0f, -9.80f);
    static constexpr vec2f GravityNormalized = vec2f(0.0f, -1.0f);
    static constexpr float GravityMagnitude = 9.80f;


    //
    // Tunable parameters
    //

    // Dynamics

    float StiffnessAdjustment;
    static constexpr float MinStiffnessAdjustment = 0.001f;
    static constexpr float MaxStiffnessAdjustment = 2.4f;

    float StrengthAdjustment;
    static constexpr float MinStrengthAdjustment = 0.001f;
    static constexpr float MaxStrengthAdjustment = 20.0f;

    float BuoyancyAdjustment;
    static constexpr float MinBuoyancyAdjustment = 0.0f;
    static constexpr float MaxBuoyancyAdjustment = 4.0f;

    // Water

    float WaterIntakeAdjustment;
    static constexpr float MinWaterIntakeAdjustment = 0.1f;
    static constexpr float MaxWaterIntakeAdjustment = 10.0f;

    float WaterCrazyness;
    static constexpr float MinWaterCrazyness = 0.0f;
    static constexpr float MaxWaterCrazyness = 2.0f;

    float WaterQuickness;
    static constexpr float MinWaterQuickness = 0.001f;
    static constexpr float MaxWaterQuickness = 1.0f;

    // Ephemeral particles

    static constexpr ElementCount MaxEphemeralParticles = 512;

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
    static constexpr float MinSparkleParticlesVelocity = 15.0f;
    static constexpr float MaxSparkleParticlesVelocity = 25.0f;
    static constexpr std::chrono::milliseconds MinSparkleParticlesLifetime = std::chrono::milliseconds(400);
    static constexpr std::chrono::milliseconds MaxSparkleParticlesLifetime = std::chrono::milliseconds(1000);


    // Misc

    float WaveHeight;
    static constexpr float MinWaveHeight = 0.0f;
    static constexpr float MaxWaveHeight = 30.0f;

    float SeaDepth;
    static constexpr float MinSeaDepth = 20.0f;
    static constexpr float MaxSeaDepth = 10000.0f;

    float OceanFloorBumpiness;
    static constexpr float MinOceanFloorBumpiness = 0.0f;
    static constexpr float MaxOceanFloorBumpiness = 6.0f;

    float DestroyRadius;
    static constexpr float MinDestroyRadius = 0.1f;
    static constexpr float MaxDestroyRadius = 10.0f;

    float BombBlastRadius;
    static constexpr float MinBombBlastRadius = 0.1f;
    static constexpr float MaxBombBlastRadius = 20.0f;

    float AntiMatterBombImplosionStrength;
    static constexpr float MinAntiMatterBombImplosionStrength = 0.1f;
    static constexpr float MaxAntiMatterBombImplosionStrength = 10.0f;

    static constexpr float BombNeighborhoodRadius = 3.5f;

    std::chrono::seconds TimerBombInterval;

    float BombMass;

    float ToolSearchRadius;

    float LightDiffusionAdjustment;

    size_t NumberOfStars;
    static constexpr size_t MinNumberOfStars = 0;
    static constexpr size_t MaxNumberOfStars = 10000;

    size_t NumberOfClouds;
    static constexpr size_t MinNumberOfClouds = 0;
    static constexpr size_t MaxNumberOfClouds = 500;

    float WindSpeed;
    static constexpr float MinWindSpeed = 0.f;
    static constexpr float MaxWindSpeed = 50.0f;

    bool IsUltraViolentMode;


    //
    // Limits
    //

    static constexpr float MinZoom = 0.0001f;
    static constexpr float MaxZoom = 2000.0f;

    static constexpr size_t MaxBombs = 64u;
    static constexpr size_t MaxPinnedPoints = 64u;    

    // 8 neighbours and 1 rope spring, when this is a rope endpoint
    static constexpr size_t MaxSpringsPerPoint = 8u + 1u;

    static constexpr size_t MaxTrianglesPerPoint = 8u;    
};
