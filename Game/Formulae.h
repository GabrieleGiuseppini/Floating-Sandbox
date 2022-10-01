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

    // Calculates the ideal pressure at the bottom of 1 cubic meter of water at this temperature,
    // in the void
    static float CalculateVolumetricWaterPressure(
        float waterTemperature,
        GameParameters const & gameParameters)
    {
        return CalculateWaterDensity(waterTemperature, gameParameters)
            * GameParameters::GravityMagnitude;
    }

    // Calculates the pressure exherted by the 1m2 column of air at the given y
    static float CalculateAirColumnPressureAt(
        float y,
        float airDensity,
        GameParameters const & /*gameParameters*/)
    {
        // While the real barometric formula is exponential, here we simplify it as linear:
        //      - Pressure is zero at y = MaxWorldHeight+10%
        //      - Pressure is AirPressureAtSeaLevel at y = 0
        float const seaLevelPressure =
            GameParameters::AirPressureAtSeaLevel
            * (airDensity / GameParameters::AirMass); // Adjust for density, assuming linear relationship
        return seaLevelPressure
            * (GameParameters::HalfMaxWorldHeight * 1.1f - y) / (GameParameters::HalfMaxWorldHeight * 1.1f);
    }

    // Calculates the pressure exherted by a 1m2 column of water of the given height
    static float CalculateWaterColumnPressure(
        float height,
        float waterDensity,
        GameParameters const & /*gameParameters*/)
    {
        return waterDensity * height // Volume
            * GameParameters::GravityMagnitude;
    }

    // Calculates the total (air above + water) pressure at the given y, in N/m2 (Pa)
    static float CalculateTotalPressureAt(
        float y,
        float oceanSurfaceY,
        float airDensity,
        float waterDensity,
        GameParameters const & gameParameters)
    {
        float const airPressure = CalculateAirColumnPressureAt(
            std::max(y, oceanSurfaceY),
            airDensity,
            gameParameters);

        float const waterPressure = CalculateWaterColumnPressure(
            std::max(oceanSurfaceY - y, 0.0f),
            waterDensity,
            gameParameters);

        return airPressure + waterPressure;
    }
};

}
