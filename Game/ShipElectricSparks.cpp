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
    Points const & points, // TODO: see if needed
    Springs const & springs)
    : mIsSpringElectrified(springs.GetElementCount(), 0, false)
    , mIsSpringElectrifiedBackup(springs.GetElementCount(), 0, false)
    , mIsPointElectrified(points.GetElementCount(), 0, false)
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
    // This algorithm works by running a number of "expansions" at each iteration,
    // with each expansion propagating sparks outwardly along springs
    //

    //
    // Constants
    //

    size_t constexpr StartingArcsMin = 3;
    size_t constexpr StartingArcsMax = 5;
    float constexpr MaxEquivalentPathLength = 35.0f; // TODO: should this be based off total number of springs?

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

    // Prepare IsSpringElectrified buffer
    mIsSpringElectrifiedBackup.fill(false);
    bool * const wasSpringElectrifiedInPreviousInteraction = mIsSpringElectrified.data();
    bool * const isSpringElectrifiedInThisInteraction = mIsSpringElectrifiedBackup.data();

    // Only the starting point has been electrified for now
    mIsPointElectrified.fill(false);
    mIsPointElectrified[startingPointIndex] = true;

    // Clear the sparks that have to be rendered after this step
    mSparksToRender.clear();

    // Calculate max equivalent path length (total of single-step costs) for this interaction:
    // we won't create arcs longer than this at this interaction
    float const maxEquivalentPathLengthForThisInteraction = std::min(
        static_cast<float>(counter + 1),
        MaxEquivalentPathLength);

    // Functor that calculates size of a sparkle, given its current path length and the distance of that path
    // length from the maximum for this interaction:
    //  - When we're at the end of the path for this interaction: small size
    //  - When we're at the beginning of the path for this interaction: large size
    auto const calculateSparkSize = [maxEquivalentPathLengthForThisInteraction](float equivalentPathLength)
    {
        return 0.2f + (1.0f - 0.2f) * (maxEquivalentPathLengthForThisInteraction - equivalentPathLength) / maxEquivalentPathLengthForThisInteraction;
    };

    //
    // Jump-start: find the initial springs outgoing from the starting point
    //

    std::vector<ElementIndex> startingSprings;

    {
        // Decide number of starting springs for this interaction
        size_t const startingArcsCount = GameRandomEngine::GetInstance().GenerateUniformInteger(StartingArcsMin, StartingArcsMax);

        //
        // 1. Fetch all springs that were electrified in the previous iteration
        //

        std::vector<std::tuple<ElementIndex, float>> otherSprings;

        for (auto const & cs : points.GetConnectedSprings(startingPointIndex).ConnectedSprings)
        {
            assert(!mIsPointElectrified[cs.OtherEndpointIndex]);

            if (wasSpringElectrifiedInPreviousInteraction[cs.SpringIndex]
                && startingSprings.size() < startingArcsCount)
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
        for (size_t s = 0; s < otherSprings.size() && startingSprings.size() < startingArcsCount; ++s)
        {
            startingSprings.emplace_back(std::get<0>(otherSprings[s]));
        }
    }

    //
    // Electrify the starting springs and initialize expansions
    //

    std::vector<SparkPointToVisit> currentPointsToVisit;

    {
        auto const startingPointPosition = points.GetPosition(startingPointIndex);

        for (ElementIndex const s : startingSprings)
        {
            float const equivalentPathLength = 1.0f; // TODO: material-based

            ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, startingPointIndex);

            // Note: we don't flag the starting springs as electrieid, as they are the only ones who share
            // a point in common and thus if they're scooped up at the next interaction, they'll add
            // an N-way fork, which could even get compounded by being picked up at the next, and so on

            // Electrify target point
            assert(!mIsPointElectrified[targetEndpointIndex]);
            mIsPointElectrified[targetEndpointIndex] = true;

            // Render
            float const sourceSize = calculateSparkSize(0.0f);
            float const targetSize = calculateSparkSize(equivalentPathLength);
            mSparksToRender.emplace_back(
                startingPointIndex,
                sourceSize,
                targetEndpointIndex,
                targetSize);

            // Queue for next expansion
            if (equivalentPathLength < maxEquivalentPathLengthForThisInteraction)
            {
                currentPointsToVisit.emplace_back(
                    targetEndpointIndex,
                    (points.GetPosition(targetEndpointIndex) - startingPointPosition).normalise(),
                    targetSize,
                    equivalentPathLength,
                    s);
            }
        }
    }

    //
    // Expand now
    //

    std::vector<SparkPointToVisit> nextPointsToVisit;

    std::vector<ElementIndex> nextSprings; // Allocated once for perf

    // Flag to limit forks to only once per interaction
    bool hasForkedInThisInteraction = false;

    while (!currentPointsToVisit.empty())
    {
        assert(nextPointsToVisit.empty());

        // Visit all points awaiting expansion
        for (auto const & pv : currentPointsToVisit)
        {
            vec2f const pointPosition = points.GetPosition(pv.PointIndex);

            // Calculate distance to the theoretical end of its path
            float const distanceToTheoreticalMaxPathLength = (MaxEquivalentPathLength - pv.EquivalentPathLength) / MaxEquivalentPathLength;

            // Calculate distance to the end of this path in this interaction
            float const distanceToInteractionMaxPathLength = (maxEquivalentPathLengthForThisInteraction - pv.EquivalentPathLength) / maxEquivalentPathLengthForThisInteraction;

            //
            // Of all the outgoing springs that are *not* the incoming spring:
            //  - Collect those that were electrified in the previous interaction, do not
            //    lead to a point already electrified in this interaction (so to avoid forks),
            //    and agree with alignment
            //  - Keep the top two that were not electrified in the previous interaction,
            //    ranking them on their alignment
            //      - We don't check beforehand if these will lead to an already-electrified
            //        point, so to allow for closing loops (which we won't electrify anyway)
            //

            nextSprings.clear();

            ElementIndex bestSpring1 = NoneElementIndex;
            float bestSpringAligment1 = -1.0f;
            ElementIndex bestSpring2 = NoneElementIndex;
            float bestSpringAligment2 = -1.0f;

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (cs.SpringIndex != pv.IncomingSpringIndex)
                {
                    if (wasSpringElectrifiedInPreviousInteraction[cs.SpringIndex])
                    {
                        if (!mIsPointElectrified[cs.OtherEndpointIndex]
                            && (points.GetPosition(cs.OtherEndpointIndex) - pointPosition).normalise().dot(pv.Direction) > 0.0f)
                        {
                            // We take this one for sure
                            nextSprings.emplace_back(cs.SpringIndex);
                        }
                    }
                    else
                    {
                        // Rank based on alignment
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
            }

            //
            // Choose a new, not electrified spring under any of these conditions:
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
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.3f * std::pow(1.0f - distanceToTheoreticalMaxPathLength, 4.0f));

            bool const doReroute =
                nextSprings.size() == 1
                // Reroute more closer to interaction end
                && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.15f * std::pow(1.0f - distanceToInteractionMaxPathLength, 2.0f));

            if (doFindNewSpring || doFork || doReroute)
            {
                // Pick second best if possible, to impose a zig-zag pattern
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
            // Follow all of the new springs
            //

            for (auto const s : nextSprings)
            {
                ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, pv.PointIndex);

                float const equivalentStepLength = 1.0f; // TODO: material-based
                float const newEquivalentPathLength = pv.EquivalentPathLength + equivalentStepLength;

                // Render
                float const targetSize = calculateSparkSize(newEquivalentPathLength);
                mSparksToRender.emplace_back(
                    pv.PointIndex,
                    pv.Size,
                    targetEndpointIndex,
                    targetSize);

                if (!mIsPointElectrified[targetEndpointIndex])
                {
                    // Electrify spring
                    isSpringElectrifiedInThisInteraction[s] = true;

                    // Electrify point
                    mIsPointElectrified[targetEndpointIndex] = true;

                    // Next expansion
                    if (newEquivalentPathLength < maxEquivalentPathLengthForThisInteraction)
                    {
                        nextPointsToVisit.emplace_back(
                            targetEndpointIndex,
                            (points.GetPosition(targetEndpointIndex) - pointPosition).normalise(),
                            targetSize,
                            newEquivalentPathLength,
                            s);
                    }
                }
            }
        }

        // Advance expansion
        std::swap(currentPointsToVisit, nextPointsToVisit);
        nextPointsToVisit.clear();
    }

    //
    // Finalize
    //

    // Swap IsElectrified buffers
    mIsSpringElectrified.swap(mIsSpringElectrifiedBackup);

    // Remember that we have populated electric sparks
    mAreSparksPopulatedBeforeNextUpdate = true;
}

}