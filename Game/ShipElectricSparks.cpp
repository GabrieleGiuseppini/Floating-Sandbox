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

    size_t constexpr StartingArcs = 6;
    float constexpr MaxPathLength = 30.0f; // TODO: should this be based off total number of springs?

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

    // Calculate max equivalent path length for this interaction - we won't create arcs longer than this at this interaction
    float const maxEquivalentPathLengthForThisInteraction = std::min(
        static_cast<float>(counter + 1),
        MaxPathLength);

    // Functor that calculates size of a sparkle, given its current path length and the distance of that path
    // length from the maximum for this interaction
    auto const calculateSparkSize = [maxEquivalentPathLengthForThisInteraction](float pathLength)
    {
        return 0.2f + (1.0f - 0.2f) * (maxEquivalentPathLengthForThisInteraction - pathLength) / maxEquivalentPathLengthForThisInteraction;
    };

    //
    // Jump-start: find the initial springs outgoing from the starting point
    //

    std::vector<ElementIndex> startingSprings;

    {
        //
        // 1. Fetch all springs already electrified, and collect the remaining springs
        //

        std::vector<std::tuple<ElementIndex, float>> otherSprings;

        for (auto const & cs : points.GetConnectedSprings(startingPointIndex).ConnectedSprings)
        {
            if (oldIsElectrified[cs.SpringIndex]
                && startingSprings.size() < StartingArcs)
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

        // Sort remaining by random seed
        std::sort(
            otherSprings.begin(),
            otherSprings.end(),
            [](auto const & s1, auto const & s2)
            {
                return std::get<1>(s1) < std::get<1>(s2);
            });

        // Pick winners
        for (size_t s = 0; s < otherSprings.size() && startingSprings.size() < StartingArcs; ++s)
        {
            startingSprings.emplace_back(std::get<0>(otherSprings[s]));
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
        float const sourceSize = calculateSparkSize(0.0f);
        float const targetSize = calculateSparkSize(equivalentPathLength);
        mSparksToRender.emplace_back(
            startingPointIndex,
            sourceSize,
            targetEndpointIndex,
            targetSize);

        // Next expansion
        if (equivalentPathLength < maxEquivalentPathLengthForThisInteraction
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
    size_t TODOElectrifiedSpringsCount = 0;

    // TODOTEST
    //while (!currentPointsToVisit.empty())
    int iter;
    for (iter = 1; !currentPointsToVisit.empty(); ++iter)
    {
        LogMessage("TODOTEST: iter=", iter, " #currentPointsToVisit=", currentPointsToVisit.size());

        assert(nextPointsToVisit.empty());

        // Visit all points
        for (auto const & pv : currentPointsToVisit)
        {
            vec2f const pointPosition = points.GetPosition(pv.PointIndex);

            // Calculate distance to the theoretical end of its path
            float const distanceToTheoreticalMaxPathLength = (MaxPathLength - pv.EquivalentPathLength) / MaxPathLength;

            // Calculate distance to the end of this path in this interaction
            float const distanceToInteractionMaxPathLength = (maxEquivalentPathLengthForThisInteraction - pv.EquivalentPathLength) / maxEquivalentPathLengthForThisInteraction;

            /*
            //
            // Find the topmost three candidates for continuing the incoming arc; ranking and selection
            // criteria are:
            //  - Not the incoming spring
            //  - Not yet electrified in this interaction
            //  - Aligned with incoming arc (though if we can choose, we'll choose the second best one
            //    to enforce a zig-zag pattern)
            //  - Between a previously-electrified with positive alignment and a non-previously-electrified
            //    with positive alignment, we'll choose the previously-electrified one
            //

            nextSprings.clear();

            ElementIndex bestSpring1 = NoneElementIndex;
            float bestSpringAligment1 = -1.0f;
            ElementIndex bestSpring2 = NoneElementIndex;
            float bestSpringAligment2 = -1.0f;
            ElementIndex bestSpring3 = NoneElementIndex;
            float bestSpringAligment3 = -1.0f;

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (cs.SpringIndex != pv.IncomingSpringIndex
                    && !newIsElectrified[cs.SpringIndex])
                {
                    // Calculate alignment
                    float const alignment = (points.GetPosition(cs.OtherEndpointIndex) - pointPosition).normalise().dot(pv.Direction);
                    if (alignment > bestSpringAligment1)
                    {
                        bestSpring3 = bestSpring2;
                        bestSpringAligment3 = bestSpringAligment2;

                        bestSpring2 = bestSpring1;
                        bestSpringAligment2 = bestSpringAligment1;

                        bestSpring1 = cs.SpringIndex;
                        bestSpringAligment1 = alignment;
                    }
                    else if (alignment > bestSpringAligment2)
                    {
                        bestSpring3 = bestSpring2;
                        bestSpringAligment3 = bestSpringAligment2;

                        bestSpring2 = cs.SpringIndex;
                        bestSpringAligment2 = alignment;
                    }
                    else if (alignment > bestSpringAligment3)
                    {
                        bestSpring3 = cs.SpringIndex;
                        bestSpringAligment3 = alignment;
                    }
                }
            }

            // TODOTEST: not taking previous electrification into consideration
            if (bestSpring2 != NoneElementIndex
                && bestSpringAligment2 >= 0.0f)
            {
                nextSprings.emplace_back(bestSpring2);

                // See if we want to fork
                if (!hasForkedInThisInteraction
                    && bestSpring3 != NoneElementIndex
                    && bestSpringAligment3 >= 0.0f
                    && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.01f * (1.0f - distanceToTheoreticalMaxPathLength)))
                {
                    // Fork!
                    nextSprings.emplace_back(bestSpring3);

                    hasForkedInThisInteraction = true;
                }
            }
            else if (bestSpring1 != NoneElementIndex)
            {
                nextSprings.emplace_back(bestSpring1);
            }
            */

            //
            // Collect the outgoing springs that are *not* the incoming spring, which
            // were previously electrified but not yet electrified in this interaction,
            // and which are aligned with our incoming direction
            //

            nextSprings.clear();

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (oldIsElectrified[cs.SpringIndex]
                    && !newIsElectrified[cs.SpringIndex]
                    && cs.SpringIndex != pv.IncomingSpringIndex
                    && (points.GetPosition(cs.OtherEndpointIndex) - pointPosition).normalise().dot(pv.Direction) > 0.0f)
                {
                    // TODOTEST: enforcing max one; NOTE: this makes the algo forget all forks
                    if (!nextSprings.empty())
                        continue;

                    nextSprings.emplace_back(cs.SpringIndex);
                }
            }

            //
            // Choose a new, not electrified outgoing spring under any of these conditions:
            //  - There are no already-electrified outgoing springs, and we choose to continue;
            //  - There is only one already-electrified outgoing spring, and we choose to fork while not having forked already in this iteration
            //  - There is only one already-electrified outgoing spring, and we choose to reroute
            //

            bool const doFindNewSpring =
                nextSprings.size() == 0
                // TODOTEST
                //&& GameRandomEngine::GetInstance().GenerateUniformBoolean(0.995f * distanceToTheoreticalMaxPathLength);
                ;

            bool const doFork =
                nextSprings.size() == 1
                && !hasForkedInThisInteraction
                // Fork more closer to theoretical end
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.01f * (1.0f - distanceToTheoreticalMaxPathLength) * (1.0f - distanceToTheoreticalMaxPathLength));

            bool const doReroute =
                nextSprings.size() == 1
                // Reroute more closer to interaction end
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.2f * (1.0f - distanceToInteractionMaxPathLength) * (1.0f - distanceToInteractionMaxPathLength));

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

                LogMessage("TODOTEST: HasForked!");
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
                if (newEquivalentPathLength < maxEquivalentPathLengthForThisInteraction
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

                ++TODOElectrifiedSpringsCount;
            }
        }

        // Advance expansion
        std::swap(currentPointsToVisit, nextPointsToVisit);
        nextPointsToVisit.clear();
    }

    LogMessage("TODOTEST: enditer=", iter, " #electrified springs=", TODOElectrifiedSpringsCount);

    //
    // Finalize
    //

    // Swap IsElectrified buffers
    mIsSpringElectrified.swap(mIsSpringElectrifiedBackup);

    // Remember that we have populated electric sparks
    mAreSparksPopulatedBeforeNextUpdate = true;
}

}