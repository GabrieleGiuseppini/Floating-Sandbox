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

ShipElectricSparks::ShipElectricSparks(
    Points const & points,
    Springs const & springs)
    : mHasPointBeenVisited(points.GetElementCount(), 0, false)
    , mIsSpringElectrified(springs.GetElementCount(), 0, false)
    , mIsSpringElectrifiedBackup(springs.GetElementCount(), 0, false)
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
    LogMessage("TODOTEST:---------------------------------------------------------------------");

    //
    // Constants
    //

    size_t constexpr MedianNumberOfStartingArcs = 4;
    size_t constexpr MinNumberOfStartingArcs = 3;
    size_t constexpr MaxNumberOfStartingArcs = 6;

    //
    // The algorithm works by running a number of "expansions", each expansion
    // propagating the existing sparks one extra (or two) springs outwardly.
    //

    // The information associated with a point that the next expansion will start from
    struct SparkPointToVisit
    {
        ElementIndex PointIndex;
        vec2f Direction; // Normalized direction that we reached this point to from the origin
        float Size; // Cumulative size so far, at point
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

    // Prepare IsVisited buffer
    mHasPointBeenVisited.fill(false);

    // Prepare IsElectrified buffer
    mIsSpringElectrifiedBackup.fill(false);
    bool * const oldIsElectrified = mIsSpringElectrified.data();
    bool * const newIsElectrified = mIsSpringElectrifiedBackup.data();

    // Clear the sparks to render after this step
    mSparksToRender.clear();

    // Calculate max equivalent path length for this iteration - we won't create arcs longer than this
    // TODOTEST
    //float constexpr TODOLimit = 15.0f; // TODO: should this be based off total number of springs?
    float constexpr TODOLimit = 8.0f;
    float const maxEquivalentPathLengthForThisIteration = std::min(
        static_cast<float>(counter + 1),
        TODOLimit);

    LogMessage("TODOTEST: startPoint=", startingPointIndex);

    // Functor that calculates size of a sparkle, given its current path length and the distance of that path
    // length from the theoretical maximum
    auto const calculateSparkSize = [maxEquivalentPathLengthForThisIteration, TODOLimit](float pathLength)
    {
        return 0.2f + (1.0f - 0.2f) * (maxEquivalentPathLengthForThisIteration - pathLength) / TODOLimit;
    };

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
            if (oldIsElectrified[cs.SpringIndex])
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

        if (startingSprings.size() < MinNumberOfStartingArcs)
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

    LogMessage("TODOTEST: iter=", 0, " #startingSprings=", startingSprings.size());

    for (ElementIndex const s : startingSprings)
    {
        float const equivalentPathLength = 1.0f; // TODO: material-based

        ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, startingPointIndex);

        // Electrify
        newIsElectrified[s] = true;

        // Render
        float const targetSize = calculateSparkSize(equivalentPathLength);
        mSparksToRender.emplace_back(
            startingPointIndex,
            calculateSparkSize(0.0f),
            targetEndpointIndex,
            targetSize);

        // Next expansion
        if (equivalentPathLength < maxEquivalentPathLengthForThisIteration
            && !mHasPointBeenVisited[targetEndpointIndex])
        {
            currentPointsToVisit.emplace_back(
                targetEndpointIndex,
                (points.GetPosition(targetEndpointIndex) - startingPointPosition).normalise(),
                targetSize,
                equivalentPathLength,
                s);

            mHasPointBeenVisited[targetEndpointIndex] = true;
        }
    }

    //
    // Expand
    //

    std::vector<SparkPointToVisit> nextPointsToVisit;

    std::vector<ElementIndex> nextSprings;

    bool hasForkedInThisInteraction = false;

    // TODOTEST
    hasForkedInThisInteraction = true;

    // TODOTEST
    //while (!currentPointsToVisit.empty())
    int iter;
    for (iter = 1; !currentPointsToVisit.empty(); ++iter)
    {
        if (iter == 1)
            LogMessage("TODOTEST: iter=", iter, " #currentPointsToVisit=", currentPointsToVisit.size());

        assert(nextPointsToVisit.empty());

        // Visit all points
        for (auto const & pv : currentPointsToVisit)
        {
            vec2f const pointPosition = points.GetPosition(pv.PointIndex);

            //
            // Collect the outgoing springs that are *not* the incoming spring, and which
            // were previously electrified but not yet electrified in this interaction
            //

            nextSprings.clear();

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (oldIsElectrified[cs.SpringIndex]
                    && !newIsElectrified[cs.SpringIndex]
                    && cs.SpringIndex != pv.IncomingSpringIndex)
                {
                    nextSprings.emplace_back(cs.SpringIndex);
                }
            }

            //
            // Choose a new, not electrified outgoing spring under any of these conditions:
            //  - There are no already-electrified outgoing springs, and we choose to continue;
            //  - There is only one already-electrified outgoing spring, and we choose to fork while not having forked already in this iteration
            //  - There is only one already-electrified outgoing spring, and we choose to reroute
            //

            // Calculate how close we are to theoretical end
            float const distanceToTheoreticalMaxPathLength = (TODOLimit - pv.EquivalentPathLength) / TODOLimit;

            bool const doFindNewSpring =
                nextSprings.size() == 0
                // TODOTEST
                //&& GameRandomEngine::GetInstance().GenerateUniformBoolean(0.995f * distanceToTheoreticalMaxPathLength);
                ;

            bool const doFork =
                nextSprings.size() == 1
                && !hasForkedInThisInteraction
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.01f * (1.0f - distanceToTheoreticalMaxPathLength));

            bool const doReroute =
                nextSprings.size() == 1
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.2f * (1.0f - distanceToTheoreticalMaxPathLength) * (1.0f - distanceToTheoreticalMaxPathLength));

            if (doFindNewSpring || doFork || doReroute)
            {
                //
                // Find the top spring that is *not* the incoming spring, and which leads to a non-visited point;
                // ranking is based on alignment of direction with incoming direction,
                // we take second best - if possible - to impose a zig-zag pattern
                //

                ElementIndex bestSpring1 = NoneElementIndex;
                float bestSpringAligment1 = -1.0f;
                ElementIndex bestSpring2 = NoneElementIndex;
                float bestSpringAligment2 = -1.0f;

                for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
                {
                    if (!oldIsElectrified[cs.SpringIndex]
                        && cs.SpringIndex != pv.IncomingSpringIndex
                        && !mHasPointBeenVisited[cs.OtherEndpointIndex])
                    {
                        // Calculate alignment
                        float const alignment = (points.GetPosition(cs.OtherEndpointIndex) - pointPosition).normalise().dot(pv.Direction);
                        if (alignment > bestSpringAligment1)
                        {
                            bestSpring2 = bestSpring1;
                            bestSpringAligment2 = bestSpringAligment1;

                            bestSpring1 = cs.SpringIndex;
                            bestSpringAligment1 = alignment;
                        }
                        else if (alignment > bestSpringAligment2)
                        {
                            bestSpring2 = cs.SpringIndex;
                            bestSpringAligment2 = alignment;
                        }
                    }
                }

                if (bestSpring2 != NoneElementIndex
                    && bestSpringAligment2 >= 0.0f)
                {
                    nextSprings.emplace_back(bestSpring2);
                }
                else if (bestSpring1 != NoneElementIndex)
                {
                    nextSprings.emplace_back(bestSpring1);
                }
            }

            if (doFork)
            {
                hasForkedInThisInteraction = true;

                LogMessage("TODOHERE: HAS FORKED!");
            }

            if (doReroute)
            {
                assert(nextSprings.size() == 1 || nextSprings.size() == 2);
                if (nextSprings.size() == 2)
                {
                    nextSprings.erase(nextSprings.begin(), std::next(nextSprings.begin()));
                }
            }

            //
            // Follow all of these
            //

            for (auto const s : nextSprings)
            {
                float const equivalentStepLength = 1.0f; // TODO: material-based
                float const newEquivalentPathLength = pv.EquivalentPathLength + equivalentStepLength;

                ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, pv.PointIndex);

                // Electrify
                newIsElectrified[s] = true;

                // Render
                float const targetSize = calculateSparkSize(newEquivalentPathLength);
                mSparksToRender.emplace_back(
                    pv.PointIndex,
                    pv.Size,
                    targetEndpointIndex,
                    targetSize);

                // Next expansion
                if (newEquivalentPathLength < maxEquivalentPathLengthForThisIteration
                    && !mHasPointBeenVisited[targetEndpointIndex])
                {
                    nextPointsToVisit.emplace_back(
                        targetEndpointIndex,
                        (points.GetPosition(targetEndpointIndex) - pointPosition).normalise(),
                        targetSize,
                        newEquivalentPathLength,
                        s);

                    mHasPointBeenVisited[targetEndpointIndex] = true;
                }
            }
        }

        // Advance expansion
        std::swap(currentPointsToVisit, nextPointsToVisit);
        nextPointsToVisit.clear();
    }

    LogMessage("TODOTEST: enditer=", iter);

    //
    // Finalize
    //

    // Swap IsElectrified buffers
    mIsSpringElectrified.swap(mIsSpringElectrifiedBackup);

    // Remember that we have populated electric sparks
    mAreSparksPopulatedBeforeNextUpdate = true;
}

}