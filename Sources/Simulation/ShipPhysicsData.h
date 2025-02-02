/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

/*
 * Physics data for a ship.
 */
struct ShipPhysicsData final
{
public:

    vec2f Offset;
    float InternalPressure; // atm

    ShipPhysicsData(
        vec2f offset,
        float internalPressure)
        : Offset(std::move(offset))
        , InternalPressure(internalPressure)
    {
    }

    // Defaults
    ShipPhysicsData()
        : Offset(vec2f::zero())
        , InternalPressure(1.0f)
    {}

    ShipPhysicsData(ShipPhysicsData const & other) = default;
    ShipPhysicsData(ShipPhysicsData && other) = default;

    ShipPhysicsData & operator=(ShipPhysicsData const & other) = default;
    ShipPhysicsData & operator=(ShipPhysicsData && other) = default;
};
