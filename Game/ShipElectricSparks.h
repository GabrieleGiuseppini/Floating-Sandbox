/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-05-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/BufferAllocator.h>
#include <GameCore/Vectors.h>

#include <vector>

namespace Physics
{

class ShipElectricSparks
{
public:

    ShipElectricSparks(
        Points const & points,
        Springs const & springs);

    bool ApplySparkAt(
        vec2f const & targetPos,
        std::uint64_t counter,
        float currentSimulationTime,
        Points const & points,
        Springs const & springs,
        ElectricalElements const & electricalElements,
        GameParameters const & gameParameters);

    void Update();

    void Upload(
        Points const & points,
        ShipId shipId,
        Render::RenderContext & renderContext) const;

private:

    void PropagateSparks(
        ElementIndex startingPointIndex,
        std::uint64_t counter,
        float currentSimulationTime,
        Points const & points,
        Springs const & springs,
        ElectricalElements const & electricalElements,
        GameParameters const & gameParameters);

    void OnPointElectrified(
        ElementIndex pointIndex,
        float currentSimulationTime,
        Points const & points,
        Springs const & springs,
        ElectricalElements const & electricalElements,
        GameParameters const & gameParameters);

private:

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
        ElementIndex PreviousPointIndex;

        ElementIndex StartPointIndex;
        float StartSize;
        ElementIndex EndPointIndex;
        float EndSize;

        ElementIndex NextPointIndex;

        RenderableElectricSpark(
            ElementIndex previousPointIndex,
            ElementIndex startPointIndex,
            float startSize,
            ElementIndex endPointIndex,
            float endSize,
            ElementIndex nextPointIndex)
            : PreviousPointIndex(previousPointIndex)
            , StartPointIndex(startPointIndex)
            , StartSize(startSize)
            , EndPointIndex(endPointIndex)
            , EndSize(endSize)
            , NextPointIndex(nextPointIndex)
        {}
    };

    std::vector<RenderableElectricSpark> mSparksToRender;
};

}
