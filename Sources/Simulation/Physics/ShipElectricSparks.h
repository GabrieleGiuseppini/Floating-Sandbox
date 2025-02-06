/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-05-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Render/RenderContext.h>

#include <Core/Buffer.h>
#include <Core/BufferAllocator.h>
#include <Core/Vectors.h>

#include <vector>

namespace Physics
{

class ShipElectricSparks
{
public:

    ShipElectricSparks(
        IShipPhysicsHandler & shipPhysicsHandler,
        Points const & points,
        Springs const & springs);

    bool ApplySparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float lengthMultiplier,
        float currentSimulationTime,
        Points const & points,
        Springs const & springs,
        SimulationParameters const & simulationParameters);

    void Update();

    void Upload(
        Points const & points,
        ShipId shipId,
        RenderContext & renderContext) const;

private:

    void PropagateSparks(
        ElementIndex initialPointIndex,
        std::uint64_t counter,
        float lengthMultiplier,
        float currentSimulationTime,
        Points const & points,
        Springs const & springs,
        SimulationParameters const & simulationParameters);

private:

    // The handler to invoke for acting on the ship
    IShipPhysicsHandler & mShipPhysicsHandler;

    // Flag remembering whether a spring is electrified or not;
    // cardinality=springs
    Buffer<bool> mIsSpringElectrifiedOld;
    Buffer<bool> mIsSpringElectrifiedNew;

    // Work buffer for flagging points as visited during an interaction;
    // cardinality=points
    Buffer<std::uint64_t> mPointElectrificationCounter;

    // Flag remembering whether electric sparks have been populated prior to the next Update() step
    bool mAreSparksPopulatedBeforeNextUpdate;

    //
    // Rendering
    //

    struct RenderableElectricSpark
    {
        ElementIndex StartPointIndex;
        vec2f StartPointPosition;
        float StartSize;
        vec2f EndPointPosition;
        float EndSize;
        vec2f Direction;

        // Index of the spark that preceded this one, or None if this the first spark
        std::optional<size_t> PreviousSparkIndex;

        // Index of the (first) spark that follows this one, or None if this the last spark
        std::optional<size_t> NextSparkIndex;

        RenderableElectricSpark(
            ElementIndex startPointIndex,
            vec2f startPointPosition,
            float startSize,
            vec2f endPointPosition,
            float endSize,
            vec2f direction,
            std::optional<size_t> previousSparkIndex)
            : StartPointIndex(startPointIndex)
            , StartPointPosition(startPointPosition)
            , StartSize(startSize)
            , EndPointPosition(endPointPosition)
            , EndSize(endSize)
            , Direction(direction)
            , PreviousSparkIndex(previousSparkIndex)
            , NextSparkIndex(std::nullopt) // Expected to be populated later
        {}
    };

    std::vector<RenderableElectricSpark> mSparksToRender;
};

}
