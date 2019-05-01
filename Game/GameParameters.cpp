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
    , RotAcceler8r(1.0f)
    // Water
    , WaterDensityAdjustment(1.0f)
    , WaterDragAdjustment(1.0f)
    , WaterIntakeAdjustment(1.0f)
    , WaterDiffusionSpeedAdjustment(1.0f)
    , WaterCrazyness(1.0f)
    // Ephemeral particles
    , DoGenerateDebris(true)
    , DoGenerateSparkles(true)
    , DoGenerateAirBubbles(true)
    // Wind
    , DoModulateWind(true)
    , WindSpeedBase(-20.0f)
    , WindSpeedMaxFactor(2.5f)
    , WindGustFrequencyAdjustment(1.0f)
    // Waves
    , BasalWaveHeight(2.5f)
    , BasalWaveLengthAdjustment(0.017182f)
    , BasalWaveSpeedAdjustment(0.5f)
    // Misc
    , SeaDepth(300.0f)
    , OceanFloorBumpiness(1.0f)
    , OceanFloorDetailAmplification(10.0f)
    , LuminiscenceAdjustment(1.0f)
    , LightSpreadAdjustment(1.0f)
    , NumberOfStars(1536)
    , NumberOfClouds(48)
    // Interactions
    , ToolSearchRadius(2.0f)
    , DestroyRadius(8.0f)
    , RepairRadius(4.5f)
    , BombBlastRadius(2.0f)
    , AntiMatterBombImplosionStrength(3.0f)
    , TimerBombInterval(10)
    , BombMass(5000.0f)
    , FloodRadius(0.75f)
    , FloodQuantity(1.0f)
    , ScrubRadius(7.0f)
    , IsUltraViolentMode(false)
    , MoveToolInertia(8.0f)
{
}