/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <optional>

/*
 * Physics data for a ship.
 */
struct ShipPhysicsData final
{
public:

    vec2f Offset;
    std::optional<float> InternalPressure; // atm

    ShipPhysicsData(
        vec2f offset,
        std::optional<float> internalPressure)
        : Offset(std::move(offset))
        , InternalPressure(internalPressure)
    {
    }

    ShipPhysicsData()
        : Offset(vec2f::zero())
        , InternalPressure()
    {}

    ShipPhysicsData(ShipPhysicsData const & other) = default;
    ShipPhysicsData(ShipPhysicsData && other) = default;

    ShipPhysicsData & operator=(ShipPhysicsData const & other) = default;
    ShipPhysicsData & operator=(ShipPhysicsData && other) = default;
};
