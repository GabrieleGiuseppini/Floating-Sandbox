/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-05-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

#include <algorithm>
#include <cassert>

namespace Physics {

ShipElectricSparks::ShipElectricSparks(Springs const & springs)
    : mSpringCount(springs.GetElementCount())
    , mIsArcElectrified(mSpringCount, 0, false)
    , mIsArcElectrifiedBackup(mSpringCount, 0, false)
    , mAreSparksPopulatedBeforeNextUpdate(false)
    , mSparksToRender()
{
}

bool ShipElectricSparks::ApplySparkAt(
    vec2f const & targetPos,
    std::uint64_t counter,
    float progress,
    Points const & points,
    Springs const & springs,
    GameParameters const & gameParameters)
{
    //
    // Find closest point, and check whether there _is_ actually a closest point
    //

    float nearestDistance = 1.5f;
    ElementIndex nearestPointIndex = NoneElementIndex;

    for (auto pointIndex : points.RawShipPoints()) // No point in visiting ephemeral points
    {
        vec2f const pointRadius = points.GetPosition(pointIndex) - targetPos;
        float const squarePointDistance = pointRadius.squareLength();
        if (squarePointDistance < nearestDistance)
        {
            nearestDistance = squarePointDistance;
            nearestPointIndex = pointIndex;
        }
    }

    if (nearestPointIndex != NoneElementIndex)
    {
        PropagateSparks(
            nearestPointIndex,
            counter,
            progress,
            points,
            springs);

        return true;
    }
    else
    {
        // No luck
        return false;
    }
}

void ShipElectricSparks::Update()
{
    if (!mAreSparksPopulatedBeforeNextUpdate)
    {
        mSparksToRender.clear();
    }

    mAreSparksPopulatedBeforeNextUpdate = false;
}

void ShipElectricSparks::Upload(
    Points const & points,
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    shipRenderContext.UploadElectricSparksStart(mSparksToRender.size());

    for (auto const & electricSpark : mSparksToRender)
    {
        shipRenderContext.UploadElectricSpark(
            points.GetPlaneId(electricSpark.StartPointIndex),
            points.GetPosition(electricSpark.StartPointIndex),
            electricSpark.StartSize,
            points.GetPosition(electricSpark.EndPointIndex),
            electricSpark.EndSize);
    }

    shipRenderContext.UploadElectricSparksEnd();
}

void ShipElectricSparks::PropagateSparks(
    ElementIndex startingPointIndex,
    std::uint64_t counter,
    float progress,
    // TODO: see if both needed
    Points const & points,
    Springs const & springs)
{
    //
    // Constants
    //

    size_t constexpr MedianNumberOfStartingArcs = 5;
    size_t constexpr MinNumberOfStartingArcs = 4;
    size_t constexpr MaxNumberOfStartingArcs = 7;

    //
    // The algorithm works by running a number of "expansions", each expansion
    // propagating the existing sparks one extra (or two) springs outwardly.
    //

    // The information associated with a point that the next expansion will start from
    struct SparkPointToVisit
    {
        ElementIndex PointIndex;
        vec2f Direction; // Normalized direction that we reached this point to from the origin
        float Size; // Cumulative size so far
        float EquivalentPathLength; // Cumulative equivalent length of path so far
        ElementIndex IncomingSpringIndex; // The index of the spring that we traveled to reach this point

        SparkPointToVisit(
            ElementIndex pointIndex,
            vec2f && direction,
            float size,
            float equivalentPathLength,
            ElementIndex incomingSpringIndex)
            : PointIndex(pointIndex)
            , Direction(std::move(direction))
            , Size(size)
            , EquivalentPathLength(equivalentPathLength)
            , IncomingSpringIndex(incomingSpringIndex)
        {}
    };

    //
    // Initialize
    //

    // Prepare IsElectrified buffer
    mIsArcElectrifiedBackup.fill(false);
    bool * const oldIsElectrified = mIsArcElectrified.data();
    bool * const newIsElectrified = mIsArcElectrifiedBackup.data();

    // Clear the sparks to render after this step
    mSparksToRender.clear();

    // Calculate max number of expansions for this iteration
    float const MaxNumberOfExpansions = std::min(
        static_cast<float>(counter + 1),
        50.0f);// TODO: should this be based off total number of springs?

    //
    // Jump-start: find the initial springs outgoing from the starting point
    //

    std::vector<ElementIndex> startingSprings;

    {
        std::vector<std::tuple<ElementIndex, float>> otherSprings;

        //
        // 1. Springs already electrified
        //

        for (auto const & cs : points.GetConnectedSprings(startingPointIndex).ConnectedSprings)
        {
            if (mIsArcElectrified[cs.SpringIndex])
            {
                startingSprings.emplace_back(cs.SpringIndex);
            }
            else
            {
                otherSprings.emplace_back(
                    cs.SpringIndex,
                    points.GetRandomNormalizedUniformPersonalitySeed(cs.OtherEndpointIndex));
            }
        }

        //
        // 2. Remaining springs
        //

        if (startingSprings.size() < MedianNumberOfStartingArcs)
        {
            // Choose number of starting arcs
            size_t const startingArcCount = GameRandomEngine::GetInstance().GenerateUniformInteger(MinNumberOfStartingArcs, MaxNumberOfStartingArcs);

            // Sort remaining
            std::sort(
                otherSprings.begin(),
                otherSprings.end(),
                [](auto const & s1, auto const & s2)
                {
                    return std::get<1>(s1) < std::get<1>(s2);
                });

            // Pick winners
            for (size_t s = 0; s < startingArcCount - startingSprings.size() && s < otherSprings.size(); ++s)
            {
                startingSprings.emplace_back(std::get<0>(otherSprings[s]));
            }
        }
    }

    //
    // Electrify the starting springs and initialize visits
    //

    std::vector<SparkPointToVisit> currentPointsToVisit;

    auto const startingPointPosition = points.GetPosition(startingPointIndex);

    for (ElementIndex s : startingSprings)
    {
        ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, startingPointIndex);

        // Electrify
        newIsElectrified[s] = true;

        // Render
        mSparksToRender.emplace_back(
            startingPointIndex,
            // TODO: size
            1.0f,
            targetEndpointIndex,
            // TODO: size
            1.0f);

        // Next expansion
        float const equivalentPathLength = 1.0f; // TODO: material-based
        if (equivalentPathLength < MaxNumberOfExpansions)
        {
            currentPointsToVisit.emplace_back(
                targetEndpointIndex,
                (points.GetPosition(targetEndpointIndex) - startingPointPosition).normalise(),
                // TODO: size
                1.0f,
                equivalentPathLength,
                s);
        }
    }

    //
    // Expand
    //

    std::vector<SparkPointToVisit> nextPointsToVisit;

    while (!currentPointsToVisit.empty())
    {
        // Visit all points
        for (auto const & pv : currentPointsToVisit)
        {
            // TODOHERE
        }

        // Advance expansion
        std::swap(currentPointsToVisit, nextPointsToVisit);
        nextPointsToVisit.clear();
    }

    //
    // Finalize
    //

    // Swap IsElectrified buffers
    mIsArcElectrified.swap(mIsArcElectrifiedBackup);

    // Remember that we have populated electric sparks
    mAreSparksPopulatedBeforeNextUpdate = true;
}

}