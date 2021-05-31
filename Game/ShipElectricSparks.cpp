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
    : mIsSpringElectrifiedOld(springs.GetElementCount(), 0, false)
    , mIsSpringElectrifiedNew(springs.GetElementCount(), 0, false)
    , mPointElectrificationCounter(points.GetElementCount(), 0, std::numeric_limits<std::uint64_t>::max())
    , mAreSparksPopulatedBeforeNextUpdate(false)
    , mSparksToRender()
{
}

bool ShipElectricSparks::ApplySparkAt(
    vec2f const & targetPos,
    std::uint64_t counter,
    float /*progress*/,
    Points const & points,
    Springs const & springs,
    GameParameters const & /*gameParameters*/)
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

    size_t constexpr StartingArcsMin = 4;
    size_t constexpr StartingArcsMax = 6;
    float constexpr MaxEquivalentPathLength = 35.0f; // TODO: should this be based off total number of springs?

    // The information associated with a point that the next expansion will start from
    struct SparkPointToVisit
    {
        ElementIndex PointIndex;
        vec2f Direction; // Normalized direction that this arc started with
        float EquivalentPathLength; // Cumulative equivalent length of path so far, up to the point that the spark starts at
        ElementIndex IncomingSpringIndex; // The index of the spring that we traveled to reach this point

        SparkPointToVisit(
            ElementIndex pointIndex,
            vec2f const & direction,
            float equivalentPathLength,
            ElementIndex incomingSpringIndex)
            : PointIndex(pointIndex)
            , Direction(direction)
            , EquivalentPathLength(equivalentPathLength)
            , IncomingSpringIndex(incomingSpringIndex)
        {}
    };

    //
    // Initialize
    //

    // Prepare IsSpringElectrified buffer
    mIsSpringElectrifiedNew.fill(false);
    bool * const wasSpringElectrifiedInPreviousInteraction = mIsSpringElectrifiedOld.data();
    bool * const isSpringElectrifiedInThisInteraction = mIsSpringElectrifiedNew.data();

    // Only the starting point has been electrified for now
    if (counter == 0)
    {
        mPointElectrificationCounter.fill(std::numeric_limits<std::uint64_t>::max());
    }
    mPointElectrificationCounter[startingPointIndex] = counter;

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
    // 1. Jump-start: find the initial springs outgoing from the starting point
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
            assert(mPointElectrificationCounter[cs.OtherEndpointIndex] != counter);

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
    // 2. Electrify the starting springs and initialize expansions
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
            assert(mPointElectrificationCounter[targetEndpointIndex] != counter);
            mPointElectrificationCounter[targetEndpointIndex] = counter;

            // Render
            mSparksToRender.emplace_back(
                startingPointIndex,
                calculateSparkSize(0.0f),
                targetEndpointIndex,
                calculateSparkSize(equivalentPathLength));

            // Queue for next expansion
            if (equivalentPathLength < maxEquivalentPathLengthForThisInteraction)
            {
                currentPointsToVisit.emplace_back(
                    targetEndpointIndex,
                    (points.GetPosition(targetEndpointIndex) - startingPointPosition).normalise(),
                    equivalentPathLength,
                    s);
            }
        }
    }

    //
    // 3. Expand now
    //

    std::vector<SparkPointToVisit> nextPointsToVisit;

    std::vector<ElementIndex> nextSprings; // Allocated once for perf

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
            //  - Keep the top three that were not electrified in the previous interaction,
            //    ranking them on their alignment
            //      - We don't check beforehand if these will lead to an already-electrified
            //        point, so to allow for closing loops (which we won't electrify anyway)
            //

            nextSprings.clear();

            ElementIndex bestCandidateNewSpring1 = NoneElementIndex;
            float bestCandidateNewSpringAligment1 = -1.0f;
            ElementIndex bestCandidateNewSpring2 = NoneElementIndex;
            float bestCandidateNewSpringAligment2 = -1.0f;
            ElementIndex bestCandidateNewSpring3 = NoneElementIndex;
            float bestCandidateNewSpringAligment3 = -1.0f;

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (cs.SpringIndex != pv.IncomingSpringIndex)
                {
                    if (wasSpringElectrifiedInPreviousInteraction[cs.SpringIndex])
                    {
                        if (mPointElectrificationCounter[cs.OtherEndpointIndex] != counter
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
                        if (alignment > bestCandidateNewSpringAligment1)
                        {
                            bestCandidateNewSpring3 = bestCandidateNewSpring2;
                            bestCandidateNewSpringAligment3 = bestCandidateNewSpringAligment2;

                            bestCandidateNewSpring2 = bestCandidateNewSpring1;
                            bestCandidateNewSpringAligment2 = bestCandidateNewSpringAligment1;

                            bestCandidateNewSpring1 = cs.SpringIndex;
                            bestCandidateNewSpringAligment1 = alignment;
                        }
                        else if (alignment > bestCandidateNewSpringAligment2)
                        {
                            bestCandidateNewSpring3 = bestCandidateNewSpring2;
                            bestCandidateNewSpringAligment3 = bestCandidateNewSpringAligment2;

                            bestCandidateNewSpring2 = cs.SpringIndex;
                            bestCandidateNewSpringAligment2 = alignment;
                        }
                        else if (alignment > bestCandidateNewSpringAligment3)
                        {
                            bestCandidateNewSpring3 = cs.SpringIndex;
                            bestCandidateNewSpringAligment3 = alignment;
                        }
                    }
                }
            }

            if (bestCandidateNewSpring1 != NoneElementIndex)
            {
                if (nextSprings.empty())
                {
                    //
                    // Choose one spring out of the best three, with probabilities enforcing a nice zig-zag pattern
                    //

                    float const r = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();
                    if (r < 0.55f || bestCandidateNewSpring2 == NoneElementIndex)
                    {
                        nextSprings.emplace_back(bestCandidateNewSpring1);
                    }
                    else if (r < 0.85f || bestCandidateNewSpring3 == NoneElementIndex)
                    {
                        nextSprings.emplace_back(bestCandidateNewSpring2);
                    }
                    else
                    {
                        nextSprings.emplace_back(bestCandidateNewSpring3);
                    }
                }
                else if (nextSprings.size() == 1)
                {
                    //
                    // Decide whether we want to fork or re-route
                    //

                    if (// Fork more closer to theoretical end
                        && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.2f * std::pow(1.0f - distanceToTheoreticalMaxPathLength, 2.0f)))
                    {
                        // Fork
                        if (bestCandidateNewSpring3 != NoneElementIndex)
                        {
                            nextSprings[0]= bestCandidateNewSpring2;
                            nextSprings.emplace_back(bestCandidateNewSpring3);
                        }
                        else
                        {
                            nextSprings.emplace_back(bestCandidateNewSpring1);
                        }
                    }
                    else if (
                        // Reroute more closer to interaction end
                        GameRandomEngine::GetInstance().GenerateUniformBoolean(0.15f * std::pow(1.0f - distanceToInteractionMaxPathLength, 0.5f)))
                    {
                        // Reroute
                        if (bestCandidateNewSpring2 != NoneElementIndex
                            && GameRandomEngine::GetInstance().GenerateUniformBoolean(0.5f))
                        {
                            nextSprings[0] = bestCandidateNewSpring2;
                        }
                        else
                        {
                            nextSprings[0] = bestCandidateNewSpring1;
                        }
                    }
                }
            }

            //
            // Follow all of the new springs
            //

            for (auto const s : nextSprings)
            {
                ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, pv.PointIndex);

                float const startEquivalentPathLength = pv.EquivalentPathLength;
                float const equivalentStepLength = 1.0f; // TODO: material-based
                float const endEquivalentPathLength = startEquivalentPathLength + equivalentStepLength;

                // Render
                mSparksToRender.emplace_back(
                    pv.PointIndex,
                    calculateSparkSize(startEquivalentPathLength),
                    targetEndpointIndex,
                    calculateSparkSize(endEquivalentPathLength));

                if (mPointElectrificationCounter[targetEndpointIndex] != counter)
                {
                    // Electrify spring
                    isSpringElectrifiedInThisInteraction[s] = true;

                    // Electrify point
                    mPointElectrificationCounter[targetEndpointIndex] = counter;

                    // Next expansion
                    if (endEquivalentPathLength < maxEquivalentPathLengthForThisInteraction)
                    {
                        nextPointsToVisit.emplace_back(
                            targetEndpointIndex,
                            pv.Direction,
                            endEquivalentPathLength,
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
    mIsSpringElectrifiedNew.swap(mIsSpringElectrifiedOld);

    // Remember that we have populated electric sparks
    mAreSparksPopulatedBeforeNextUpdate = true;
}

}