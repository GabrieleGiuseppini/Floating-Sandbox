/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

GameParameters::GameParameters()
    // Dynamics
    : NumMechanicalDynamicsIterationsAdjustment(1.0f)
    , StiffnessAdjustment(1.0f)
    , StrengthAdjustment(1.0f)
    // Water
    , WaterDensityAdjustment(1.0f)
    , WaterIntakeAdjustment(1.0f)
    , WaterCrazyness(1.0f)
    , WaterQuickness(0.5f)
    // Ephemeral particles
    , DoGenerateDebris(true)
    , DoGenerateSparkles(true)
    // Misc
    , WaveHeight(2.5f)
    , SeaDepth(200.0f)
    , OceanFloorBumpiness(1.0f)
    , OceanFloorDetailAmplification(10.0f)
    , DestroyRadius(0.75f)
    , BombBlastRadius(2.5f)
    , AntiMatterBombImplosionStrength(3.0f)
    , BombMass(5000.0f)
    , TimerBombInterval(10)
    , ToolSearchRadius(2.0f)
    , LightDiffusionAdjustment(0.859375f)
    , NumberOfStars(1536)
    , NumberOfClouds(50)
    , WindSpeed(3.0f)
    , IsUltraViolentMode(false)
{
}
