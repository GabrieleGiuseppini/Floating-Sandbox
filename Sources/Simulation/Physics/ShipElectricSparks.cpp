/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-05-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>

#include <algorithm>
#include <cassert>

namespace Physics {

ShipElectricSparks::ShipElectricSparks(
    IShipPhysicsHandler & shipPhysicsHandler,
    Points const & points,
    Springs const & springs)
    : mShipPhysicsHandler(shipPhysicsHandler)
    , mIsSpringElectrifiedOld(springs.GetElementCount(), 0, false)
    , mIsSpringElectrifiedNew(springs.GetElementCount(), 0, false)
    , mPointElectrificationCounter(points.GetElementCount(), 0, std::numeric_limits<std::uint64_t>::max())
    , mAreSparksPopulatedBeforeNextUpdate(false)
    , mSparksToRender()
{
}

bool ShipElectricSparks::ApplySparkAt(
    vec2f const & targetPos,
    std::uint64_t counter,
    float lengthMultiplier,
    float currentSimulationTime,
    Points const & points,
    Springs const & springs,
    SimulationParameters const & simulationParameters)
{
    //
    // Find closest point, and check whether there _is_ actually a closest point
    //

    float nearestDistance = 2.0f;
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
            lengthMultiplier,
            currentSimulationTime,
            points,
            springs,
            simulationParameters);

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
    RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    shipRenderContext.UploadElectricSparksStart(mSparksToRender.size());

    for (auto const & electricSpark : mSparksToRender)
    {
        shipRenderContext.UploadElectricSpark(
            points.GetPlaneId(electricSpark.StartPointIndex),
            electricSpark.StartPointPosition,
            electricSpark.StartSize,
            electricSpark.EndPointPosition,
            electricSpark.EndSize,
            electricSpark.Direction,
            electricSpark.PreviousSparkIndex.has_value()
                ? mSparksToRender[*electricSpark.PreviousSparkIndex].Direction
                : electricSpark.Direction,
            electricSpark.NextSparkIndex.has_value()
                ? mSparksToRender[*electricSpark.NextSparkIndex].Direction
                : electricSpark.Direction);
    }

    shipRenderContext.UploadElectricSparksEnd();
}

/// //////////////////////////////////////////////////////////////

void ShipElectricSparks::PropagateSparks(
    ElementIndex const initialPointIndex,
    std::uint64_t counter,
    float lengthMultiplier,
    float currentSimulationTime,
    Points const & points,
    Springs const & springs,
    SimulationParameters const & simulationParameters)
{
    //
    // This algorithm works by running a number of "expansions" at each iteration,
    // with each expansion propagating sparks outwardly along springs
    //

    //
    // Constants
    //

    size_t constexpr InitialArcsMin = 4;
    size_t constexpr InitialArcsMax = 6;
    float constexpr ForkSpacingMin = 5.0f;
    float constexpr ForkSpacingMax = 10.0f;
    float const maxEquivalentPathLength =
        17.0f // Magic number: max length of arc without tool modifier and default settings
        * lengthMultiplier
        * (simulationParameters.IsUltraViolentMode ? 2.0f : 1.0f);

    // The information associated with a point that the next expansion will start from
    struct SparkPointToVisit
    {
        ElementIndex PointIndex;
        vec2f PreferredDirection; // Normalized direction that this arc started with
        float EquivalentPathLength; // Cumulative equivalent length of path so far, up to the point that the spark starts at
        ElementIndex IncomingSpringIndex; // The index of the spring that we traveled to reach this point
        size_t IncomingRenderableSparkIndex; // The index of the spark we traveled through to reach this point
        float EquivalentPathLengthToNextFork; // We'll fork when the equivalent path length is longer than this

        SparkPointToVisit(
            ElementIndex pointIndex,
            vec2f const & preferredDirection,
            float equivalentPathLength,
            ElementIndex incomingSpringIndex,
            size_t incomingRenderableSparkIndex,
            float equivalentPathLengthToNextFork)
            : PointIndex(pointIndex)
            , PreferredDirection(preferredDirection)
            , EquivalentPathLength(equivalentPathLength)
            , IncomingSpringIndex(incomingSpringIndex)
            , IncomingRenderableSparkIndex(incomingRenderableSparkIndex)
            , EquivalentPathLengthToNextFork(equivalentPathLengthToNextFork)
        {}
    };

    //
    // Initialize
    //

    // Prepare IsSpringElectrified buffer
    mIsSpringElectrifiedNew.fill(false);
    bool * const wasSpringElectrifiedInPreviousInteraction = mIsSpringElectrifiedOld.data();
    bool * const isSpringElectrifiedInThisInteraction = mIsSpringElectrifiedNew.data();

    // Prepare point electrification flag
    if (counter == 0)
    {
        mPointElectrificationCounter.fill(std::numeric_limits<std::uint64_t>::max());
    }

    // Clear the sparks that have to be rendered after this step
    mSparksToRender.clear();

    // Calculate max equivalent path length (total of single-step costs) for this interaction:
    // we won't create arcs longer than this at this interaction
    float const maxEquivalentPathLengthForThisInteraction = std::min(
        static_cast<float>(counter + 1),
        maxEquivalentPathLength);

    // Functor that calculates size of a spark, given its current path length and the distance of that path
    // length from the maximum for this interaction:
    //  - When we're at the end of the path for this interaction: small size
    //  - When we're at the beginning of the path for this interaction: large size
    auto const calculateSparkSize = [maxEquivalentPathLengthForThisInteraction](float equivalentPathLength)
    {
        return 0.05f + (1.0f - 0.05f) * (maxEquivalentPathLengthForThisInteraction - equivalentPathLength) / maxEquivalentPathLengthForThisInteraction;
    };

    //
    // 1. Electrify initial point
    //

    float const initialPointSize = calculateSparkSize(0.0f);

    mShipPhysicsHandler.HandleElectricSpark(
        initialPointIndex,
        initialPointSize, // strength
        currentSimulationTime,
        simulationParameters);

    mPointElectrificationCounter[initialPointIndex] = counter;

    //
    // 2. Jump-start: find the initial springs outgoing from the initial point
    //

    std::vector<ElementIndex> initialSprings;

    {
        // Decide number of initial springs for this interaction
        size_t const initialArcsCount = GameRandomEngine::GetInstance().GenerateUniformInteger(InitialArcsMin, InitialArcsMax);

        //
        // 1. Fetch all springs that were electrified in the previous iteration
        //

        std::vector<std::tuple<ElementIndex, float>> otherSprings;

        for (auto const & cs : points.GetConnectedSprings(initialPointIndex).ConnectedSprings)
        {
            assert(mPointElectrificationCounter[cs.OtherEndpointIndex] != counter);

            if (wasSpringElectrifiedInPreviousInteraction[cs.SpringIndex]
                && initialSprings.size() < initialArcsCount)
            {
                initialSprings.emplace_back(cs.SpringIndex);
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
        for (size_t s = 0; s < otherSprings.size() && initialSprings.size() < initialArcsCount; ++s)
        {
            initialSprings.emplace_back(std::get<0>(otherSprings[s]));
        }
    }

    //
    // 3. Electrify the initial springs and initialize expansions
    //

    std::vector<SparkPointToVisit> currentPointsToVisit;

    {
        auto const initialPointPosition = points.GetPosition(initialPointIndex);

        for (ElementIndex const s : initialSprings)
        {
            ElementIndex const targetEndpointIndex = springs.GetOtherEndpointIndex(s, initialPointIndex);
            vec2f const targetEndpointPosition = points.GetPosition(targetEndpointIndex);
            vec2f const direction = (targetEndpointPosition - initialPointPosition).normalise();

            float const equivalentPathLength = 1.0f; // TODO: material-based

            float const endSize = calculateSparkSize(0.0f + equivalentPathLength);

            // Note: we don't flag the initial springs as electrified, as they are the only ones who share
            // a point in common and thus if they're scooped up at the next interaction, they'll add
            // an N-way fork, which could even get compounded by being picked up at the next, and so on...

            // Electrify target point
            mShipPhysicsHandler.HandleElectricSpark(
                targetEndpointIndex,
                endSize, // strength
                currentSimulationTime,
                simulationParameters);

            // Remember the point is electrified now
            assert(mPointElectrificationCounter[targetEndpointIndex] != counter);
            mPointElectrificationCounter[targetEndpointIndex] = counter;

            // Queue for next expansion
            if (equivalentPathLength < maxEquivalentPathLengthForThisInteraction)
            {
                currentPointsToVisit.emplace_back(
                    targetEndpointIndex,
                    direction,
                    0.0f + equivalentPathLength,
                    s,
                    mSparksToRender.size(), // The arc we'll be pushing right now is the predecessor of this point we're pushing now
                    GameRandomEngine::GetInstance().GenerateUniformReal(ForkSpacingMin, ForkSpacingMax));
            }

            // Render
            mSparksToRender.emplace_back(
                initialPointIndex,
                initialPointPosition,
                initialPointSize,
                targetEndpointPosition,
                endSize,
                direction,
                std::nullopt); // No previous spark
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
            ElementIndex const startingPointIndex = pv.PointIndex;
            vec2f const startingPointPosition = points.GetPosition(startingPointIndex);

            // Initialize path length until next fork - we'll eventually reset it if we fork
            float equivalentPathLengthToNextFork = pv.EquivalentPathLengthToNextFork;

            // Calculate distance to the end of this path in this interaction
            float const distanceToInteractionMaxPathLength = (maxEquivalentPathLengthForThisInteraction - pv.EquivalentPathLength) / maxEquivalentPathLengthForThisInteraction;

            //
            // Of all the outgoing springs that are *not* the incoming spring:
            //  - Collect the first one that wa electrified in the previous interaction, does not
            //    lead to a point already electrified in this interaction (so to avoid forks),
            //    and agrees with alignment
            //  - Keep the others, ranking them on their alignment
            //      - We don't check beforehand if these will lead to an already-electrified
            //        point, so to allow for closing loops (which we won't electrify anyway)
            //

            nextSprings.clear();

            ElementIndex bestCandidateNewSpring1 = NoneElementIndex;
            float bestCandidateNewSpringAlignment1 = -1.0f;
            ElementIndex bestCandidateNewSpring2 = NoneElementIndex;
            float bestCandidateNewSpringAlignment2 = -1.0f;
            ElementIndex bestCandidateNewSpring3 = NoneElementIndex;
            float bestCandidateNewSpringAlignment3 = -1.0f;

            for (auto const & cs : points.GetConnectedSprings(pv.PointIndex).ConnectedSprings)
            {
                if (cs.SpringIndex != pv.IncomingSpringIndex)
                {
                    vec2f const springDirection = (points.GetPosition(cs.OtherEndpointIndex) - startingPointPosition).normalise();
                    float const springAlignment = springDirection.dot(pv.PreferredDirection);

                    if (nextSprings.empty() && wasSpringElectrifiedInPreviousInteraction[cs.SpringIndex])
                    {
                        if (mPointElectrificationCounter[cs.OtherEndpointIndex] != counter
                            && springAlignment > 0.0f)
                        {
                            // We take this one for sure
                            nextSprings.emplace_back(cs.SpringIndex);
                        }
                    }
                    else
                    {
                        // Rank based on alignment
                        if (springAlignment > bestCandidateNewSpringAlignment1)
                        {
                            bestCandidateNewSpring3 = bestCandidateNewSpring2;
                            bestCandidateNewSpringAlignment3 = bestCandidateNewSpringAlignment2;

                            bestCandidateNewSpring2 = bestCandidateNewSpring1;
                            bestCandidateNewSpringAlignment2 = bestCandidateNewSpringAlignment1;

                            bestCandidateNewSpring1 = cs.SpringIndex;
                            bestCandidateNewSpringAlignment1 = springAlignment;
                        }
                        else if (springAlignment > bestCandidateNewSpringAlignment2)
                        {
                            bestCandidateNewSpring3 = bestCandidateNewSpring2;
                            bestCandidateNewSpringAlignment3 = bestCandidateNewSpringAlignment2;

                            bestCandidateNewSpring2 = cs.SpringIndex;
                            bestCandidateNewSpringAlignment2 = springAlignment;
                        }
                        else if (springAlignment > bestCandidateNewSpringAlignment3)
                        {
                            bestCandidateNewSpring3 = cs.SpringIndex;
                            bestCandidateNewSpringAlignment3 = springAlignment;
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
                    // Ignore sign of alignment, if we're forced we'll even recoil back
                    //

                    float const r = GameRandomEngine::GetInstance().GenerateNormalizedUniformReal();
                    if (r < 0.25f || bestCandidateNewSpring2 == NoneElementIndex)
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
                else if (nextSprings.size() == 1 && bestCandidateNewSpringAlignment1 >= 0.0f)
                {
                    //
                    // Decide whether we want to fork or re-route, but always with a positive
                    // alignment
                    //

                    if (pv.EquivalentPathLength >= equivalentPathLengthToNextFork)
                    {
                        // Fork
                        if (bestCandidateNewSpringAlignment3 >= 0.0f)
                        {
                            // We have 3, choose second and third then
                            nextSprings[0] = bestCandidateNewSpring2;
                            nextSprings.emplace_back(bestCandidateNewSpring3);
                        }
                        else if (bestCandidateNewSpringAlignment2 >= 0.0f)
                        {
                            nextSprings.emplace_back(bestCandidateNewSpring2);
                        }
                        else
                        {
                            nextSprings.emplace_back(bestCandidateNewSpring1);
                        }

                        equivalentPathLengthToNextFork = pv.EquivalentPathLength + GameRandomEngine::GetInstance().GenerateUniformReal(ForkSpacingMin, ForkSpacingMax);
                    }
                    else if (
                        // Reroute when we're closer to interaction end
                        GameRandomEngine::GetInstance().GenerateUniformBoolean(0.15f * std::pow(1.0f - distanceToInteractionMaxPathLength, 0.5f)))
                    {
                        // Reroute
                        if (bestCandidateNewSpringAlignment2 >= 0.0f
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
                vec2f const targetEndpointPosition = points.GetPosition(targetEndpointIndex);
                vec2f const springDirection = (targetEndpointPosition - startingPointPosition).normalise();

                float const startEquivalentPathLength = pv.EquivalentPathLength;
                float const equivalentStepLength = 1.0f; // TODO: material-based
                float const endEquivalentPathLength = startEquivalentPathLength + equivalentStepLength;

                float const startSize = calculateSparkSize(startEquivalentPathLength);

                size_t const springSparkToRenderIndex = mSparksToRender.size(); // The arc we'll be pushing right now is the arc for this spring

                // Render
                mSparksToRender.emplace_back(
                    startingPointIndex,
                    startingPointPosition,
                    startSize,
                    targetEndpointPosition,
                    calculateSparkSize(endEquivalentPathLength),
                    springDirection,
                    pv.IncomingRenderableSparkIndex);

                // Connect this renderable spark to its predecessor, if this is the first one
                if (!mSparksToRender[pv.IncomingRenderableSparkIndex].NextSparkIndex.has_value())
                {
                    mSparksToRender[pv.IncomingRenderableSparkIndex].NextSparkIndex = springSparkToRenderIndex;
                }

                // Propagate visit
                if (mPointElectrificationCounter[targetEndpointIndex] != counter)
                {
                    // Electrify spring
                    isSpringElectrifiedInThisInteraction[s] = true;

                    // Electrify point
                    mShipPhysicsHandler.HandleElectricSpark(
                        targetEndpointIndex,
                        startSize, // strength
                        currentSimulationTime,
                        simulationParameters);

                    // Remember this point is not electrified
                    mPointElectrificationCounter[targetEndpointIndex] = counter;

                    // Next expansion
                    if (endEquivalentPathLength < maxEquivalentPathLengthForThisInteraction)
                    {
                        nextPointsToVisit.emplace_back(
                            targetEndpointIndex,
                            pv.PreferredDirection,
                            endEquivalentPathLength,
                            s,
                            springSparkToRenderIndex, // Predecessor
                            equivalentPathLengthToNextFork);
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