/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-07-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/GameMath.h>

#include <cmath>

namespace Physics
{

/*
 * Collection of some of the most reused formulae in the simulation.
 */
class Formulae
{
public:

    static float CalculateAirDensity(
        float airTemperature,
        GameParameters const & gameParameters)
    {
        return GameParameters::AirMass
            / (1.0f + GameParameters::AirThermalExpansionCoefficient * (airTemperature - GameParameters::Temperature0))
            * gameParameters.AirDensityAdjustment;
    }

    static float CalculateWaterDensity(
        float waterTemperature,
        GameParameters const & gameParameters)
    {
        return GameParameters::WaterMass
            / (1.0f + GameParameters::WaterThermalExpansionCoefficient * (waterTemperature - GameParameters::Temperature0))
            * gameParameters.WaterDensityAdjustment;
    }

    // Calculates the ideal pressure at the bottom of 1 cubic meter of water at this temperature
    static float CalculateVolumetricWaterPressure(
        float waterTemperature,
        GameParameters const & gameParameters)
    {
        return CalculateWaterDensity(waterTemperature, gameParameters)
            * GameParameters::GravityMagnitude;
    }
};

}
