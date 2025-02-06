/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <Core/GameMath.h>
#include <Core/Log.h>

#include <cassert>
#include <deque>

namespace Physics {

void Ship::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    SequenceNumber repairStepId,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    float const searchRadius =
        simulationParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    //
    // Pass 1: straighten one-spring and two-spring naked springs
    //

    // We store points in radius here in order to speedup subsequent passes
    std::vector<ElementIndex> pointsInRadius;

    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius)
        {
            StraightenOneSpringChains(pointIndex);

            StraightenTwoSpringChains(pointIndex);

            pointsInRadius.push_back(pointIndex);
        }
    }

    //
    // Pass 2: visit all points that had been attractors in the previous 2 step2
    //
    // This is to prevent attractors and attractees from flipping roles during a session;
    // an attractor will continue to be an attractor until it needs reparation
    //

    auto const previousStep = repairStepId.Previous();
    auto const previousPreviousStep = previousStep.Previous();
    for (auto const pointIndex : pointsInRadius)
    {
        if (mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId == previousStep
            || mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId == previousPreviousStep)
        {
            TryRepairAndPropagateFromPoint(
                pointIndex,
                targetPos,
                squareSearchRadius,
                repairStepId,
                currentSimulationTime,
                simulationParameters);
        }
    }

    //
    // Pass 3: visit all other points now, to give a chance to everyone else to be
    // an attractor
    //

    // Visit all (in-radius) non-ephemeral points
    for (auto const pointIndex : pointsInRadius)
    {
        TryRepairAndPropagateFromPoint(
            pointIndex,
            targetPos,
            squareSearchRadius,
            repairStepId,
            currentSimulationTime,
            simulationParameters);
    }

    //
    // Pass 4:
    //
    // a) Restore deleted _eligible_ triangles that were connected to each (in-radius) point
    //     at factory time
    //
    // A triangle is eligible for being restored if all of its subsprings are not deleted.
    //
    // We do this at global tool time as opposed to per-restored point, because there might
    // be triangles that have been deleted without their edge-springs having been deleted,
    // so the resurrection of triangles wouldn't be complete if we resurrected triangles
    // only when restoring a spring
    //
    // b) (Partially) restore (in-radius) springs' rest lengths
    //
    // c) Restore (in-radius) electrical elements of repaired points
    //

    // Visit all (in-radius) non-ephemeral points
    for (auto const pointIndex : pointsInRadius)
    {
        // a) Visit all deleted triangles, trying to restore them
        for (auto const fct : mPoints.GetFactoryConnectedTriangles(pointIndex).ConnectedTriangles)
        {
            if (mTriangles.IsDeleted(fct))
            {
                // Check if eligible
                bool hasDeletedSubsprings = false;
                for (auto const ss : mTriangles.GetSubSprings(fct).SpringIndices)
                    hasDeletedSubsprings |= mSprings.IsDeleted(ss);

                if (!hasDeletedSubsprings)
                {
                    // Restore it
                    mTriangles.Restore(fct);

                    // Attempt to restore all endpoints
                    AttemptPointRestore(mTriangles.GetPointAIndex(fct), currentSimulationTime);
                    AttemptPointRestore(mTriangles.GetPointBIndex(fct), currentSimulationTime);
                    AttemptPointRestore(mTriangles.GetPointCIndex(fct), currentSimulationTime);
                }
            }
        }

        // b) Visit all springs, trying to restore their rest lenghts
        for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
        {
            if (mSprings.GetRestLength(cs.SpringIndex) != mSprings.GetFactoryRestLength(cs.SpringIndex))
            {
                mSprings.SetRestLength(
                    cs.SpringIndex,
                    mSprings.GetFactoryRestLength(cs.SpringIndex)
                    + 0.97f * (mSprings.GetRestLength(cs.SpringIndex) - mSprings.GetFactoryRestLength(cs.SpringIndex)));

                // Check if may fully restore
                if (std::abs(mSprings.GetRestLength(cs.SpringIndex) - mSprings.GetFactoryRestLength(cs.SpringIndex)) < 0.05f) // Magic number
                {
                    mSprings.SetRestLength(
                        cs.SpringIndex,
                        mSprings.GetFactoryRestLength(cs.SpringIndex));
                }

                // Recalculate this spring's coefficients, now that we have changed its rest length
                mSprings.UpdateForRestLength(
                    cs.SpringIndex,
                    mPoints);
            }
        }

        // c) Restore electrical element - iff point is not damanged
        if (!mPoints.IsDamaged(pointIndex))
        {
            auto const electricalElementIndex = mPoints.GetElectricalElement(pointIndex);
            if (NoneElementIndex != electricalElementIndex
                && mElectricalElements.IsDeleted(electricalElementIndex))
            {
                mElectricalElements.Restore(electricalElementIndex);
            }
        }
    }

    //
    // Pass 5: make sure we don't destroy what we've repaired right away
    //

    // Reset dynamic forces
    mPoints.ResetDynamicForces();

    // Reset grace period
    mRepairGracePeriodMultiplier = 0.0f;
}

void Ship::StraightenOneSpringChains(ElementIndex pointIndex)
{
    //
    // Here we detect (currently) naked dead-end springs and forcefully move the lonely
    // opposite endpoint where it should be wrt the other springs attached to
    // this point.
    //
    //             O
    //            /
    //           /
    //          /
    // -   -   P   -   -
    //         |
    //         |
    //         |
    //         |

    auto const & connectedSprings = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings;
    if (connectedSprings.size() >= 2)
    {
        // Visit (currently) naked springs not connected to anything else
        for (auto const & nakedCs : connectedSprings)
        {
            ElementIndex const otherEndpointIndex = mSprings.GetOtherEndpointIndex(nakedCs.SpringIndex, pointIndex);
            if (mSprings.GetSuperTriangles(nakedCs.SpringIndex).empty() // Naked
                && mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size() == 1) // Other endpoint only has this naked spring
            {
                //
                // Move other endpoint where it should be wrt the (arbitrary) CCW spring
                // nearest to this spring
                //

                // The angle of the spring wrt this point
                // 0 = E, 1 = SE, ..., 7 = NE
                Octant const factoryPointSpringOctant = mSprings.GetFactoryEndpointOctant(
                    nakedCs.SpringIndex,
                    pointIndex);

                //
                // Find nearest CCW spring
                //

                int nearestCCWSpringIndex = -1;
                int nearestCCWSpringDeltaOctant = std::numeric_limits<int>::max();
                for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
                {
                    if (cs.SpringIndex != nakedCs.SpringIndex)
                    {
                        int cwDelta =
                            mSprings.GetFactoryEndpointOctant(cs.SpringIndex, pointIndex)
                            - factoryPointSpringOctant;

                        if (cwDelta < 0)
                            cwDelta += 8;

                        assert(cwDelta > 0 && cwDelta < 8);
                        int ccwDelta = 8 - cwDelta;
                        assert(ccwDelta > 0);

                        if (ccwDelta < nearestCCWSpringDeltaOctant)
                        {
                            nearestCCWSpringIndex = cs.SpringIndex;
                            nearestCCWSpringDeltaOctant = ccwDelta;
                        }
                    }
                }

                assert(nearestCCWSpringIndex >= 0);
                assert(nearestCCWSpringDeltaOctant > 0);

                //
                // Calculate this spring's world angle wrt nearest CCW
                //

                ElementIndex const ccwSpringOtherEndpointIndex =
                    mSprings.GetOtherEndpointIndex(nearestCCWSpringIndex, pointIndex);

                float const nearestCCWSpringWorldAngle = vec2f(1.0f, 0.0f).angleCw(mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex));

                //
                // Calculate target position for the other endpoint
                //

                // Target angle, in world coordinates, CW, 0 at E
                float const targetWorldAngleCw =
                    nearestCCWSpringWorldAngle
                    + 2.0f * Pi<float> / 8.0f * static_cast<float>(nearestCCWSpringDeltaOctant);

                vec2f const targetOtherEndpointPosition =
                    mPoints.GetPosition(pointIndex)
                    + vec2f::fromPolar(
                        (mPoints.GetPosition(otherEndpointIndex) - mPoints.GetPosition(pointIndex)).length(),
                        targetWorldAngleCw);

                //
                // Move the other endpoint
                //

                mPoints.SetPosition(
                    otherEndpointIndex,
                    targetOtherEndpointPosition.clamp(
                        -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth,
                        -SimulationParameters::HalfMaxWorldHeight, SimulationParameters::HalfMaxWorldHeight));
            }
        }
    }
}

void Ship::StraightenTwoSpringChains(ElementIndex pointIndex)
{
    //
    // Here we detect P (connected to S0 and S1 by naked springs) being on the
    // wrong side of S0S1, and flip it. We do this to supplement the CCW triangle
    // detection which won't work for traverse spring.
    //
    //       P              |
    //       O              |
    //      / \             |
    //     /   \            |
    //    /     \           |
    //   O       O          |
    //  S0       S1         |
    //

    auto const & connectedSprings = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings;

    if (connectedSprings.size() == 2
        && mSprings.GetSuperTriangles(connectedSprings[0].SpringIndex).empty()  // Naked at this moment
        && mSprings.GetSuperTriangles(connectedSprings[1].SpringIndex).empty()) // Naked at this moment
    {
        // The angles of the springs wrt P
        // 0 = E, 1 = SE, ..., 7 = NE

        Octant const spring0Octant = mSprings.GetFactoryEndpointOctant(
            connectedSprings[0].SpringIndex,
            pointIndex);

        Octant const spring1Octant = mSprings.GetFactoryEndpointOctant(
            connectedSprings[1].SpringIndex,
            pointIndex);

        Octant deltaOctant = spring1Octant - spring0Octant;
        if (deltaOctant < 0)
            deltaOctant += 8;

        assert(deltaOctant >= 0 && deltaOctant < 8);

        vec2f const & pPosition = mPoints.GetPosition(pointIndex);
        vec2f const & s0Position = mPoints.GetPosition(mSprings.GetOtherEndpointIndex(connectedSprings[0].SpringIndex, pointIndex));
        vec2f const & s1Position = mPoints.GetPosition(mSprings.GetOtherEndpointIndex(connectedSprings[1].SpringIndex, pointIndex));

        vec2f const ps0Vector = s0Position - pPosition;
        vec2f const ps1Vector = s1Position - pPosition;

        if ((deltaOctant < 4 && ps1Vector.cross(ps0Vector) < 0.0f) // Delta < 4: spring 1 must be to the R of spring 0
            || (deltaOctant > 4 && ps1Vector.cross(ps0Vector) > 0.0f)) // Delta > 4: spring 1 must be to the L of spring 0
        {
            // Reflect P onto the other side of the S0S1 vector: S0P' = PS0 - S0S1 * 2 * (PS0 dot S0S1) / |S0S1|^2
            vec2f const s0s1Vector = s0Position - s1Position;
            vec2f const newPPosition = s0Position + ps0Vector - s0s1Vector * 2.0f * (ps0Vector.dot(s0s1Vector)) / s0s1Vector.squareLength();

            // Set position
            mPoints.SetPosition(
                pointIndex,
                newPPosition.clamp(
                    -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth,
                    -SimulationParameters::HalfMaxWorldHeight, SimulationParameters::HalfMaxWorldHeight));
        }
    }
}

bool Ship::TryRepairAndPropagateFromPoint(
    ElementIndex startingPointIndex,
    vec2f const & targetPos,
    float squareSearchRadius,
    SequenceNumber repairStepId,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    bool hasRepairedAnything = false;

    // Conditions for a point to be an attactor:
    //  - is in radius
    //  - and has not already been an attractor in this step
    //  - and has not been an attractee in this step
    //  - and has not been an attractee in the *previous* step (so to prevent sudden role flips)
    //  - and needs reparation
    //  - and is not orphaned (we rely on existing springs in order to repair)
    //
    // After being an attractor, do a breadth-first visit from the point propagating
    // repair from directly-connected in-radius particles

    if (mPoints.GetRepairState(startingPointIndex).CurrentAttractorPropagationVisitStepId != repairStepId)
    {
        mPoints.GetRepairState(startingPointIndex).CurrentAttractorPropagationVisitStepId = repairStepId;

        std::deque<ElementIndex> pointsToVisit;

        for (ElementIndex pointIndex = startingPointIndex; ;)
        {
            //
            // Check if this point meets the conditions for being an attractor
            //

            if (mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId != repairStepId
                && mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId
                && mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId.Previous()
                && mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings.size() > mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() // Needs reparation
                && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0) // Not orphaned
            {
                //
                // This point has now taken the role of an attractor
                //

                // Calculate repair strength (1.0 at center and zero at border, fourth power)
                float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
                float const repairStrength =
                    (1.0f - (squareRadius / squareSearchRadius) * (squareRadius / squareSearchRadius))
                    * (simulationParameters.IsUltraViolentMode ? 10.0f : 1.0f);

                // Repair from this point
                bool const hasRepaired = RepairFromAttractor(
                    pointIndex,
                    repairStrength,
                    repairStepId,
                    currentSimulationTime,
                    simulationParameters);

                hasRepairedAnything |= hasRepaired;
            }

            //
            // Propagate to all of the in-radius, not-yet-visited immediately-connected points
            //

            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                ElementIndex const newPointIndex = cs.OtherEndpointIndex;

                if (mPoints.GetRepairState(newPointIndex).CurrentAttractorPropagationVisitStepId != repairStepId)
                {
                    mPoints.GetRepairState(newPointIndex).CurrentAttractorPropagationVisitStepId = repairStepId;

                    // See if it's in radius
                    if (float const squareRadius = (mPoints.GetPosition(newPointIndex) - targetPos).squareLength();
                        squareRadius <= squareSearchRadius)
                    {
                        pointsToVisit.push_back(newPointIndex);
                    }
                }
            }

            //
            // Visit next point
            //

            if (pointsToVisit.empty())
                break;

            pointIndex = pointsToVisit.front();
            pointsToVisit.pop_front();
        }
    }

    return hasRepairedAnything;
}

bool Ship::RepairFromAttractor(
    ElementIndex attractorPointIndex,
    float repairStrength,
    SequenceNumber repairStepId,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    // Tolerance to distance: the minimum distance between the endpoint
    // of a broken spring and its target position, below which we restore
    // the spring
    //
    // Note: a higher tolerance here causes springs to...spring into life
    // already stretched or compressed, generating an undesirable force impulse
    //
    // - Shipped 1.13 with 0.07 and up to 1.16.2 with 0.06
    float constexpr DisplacementTolerance = 0.05f;

    ////////////////////////////////////////////////////////////////////////////

    // This point hasn't taken any role yet in this step
    assert(mPoints.GetRepairState(attractorPointIndex).LastAttractorRepairStepId != repairStepId);
    assert(mPoints.GetRepairState(attractorPointIndex).LastAttracteeRepairStepId != repairStepId);

    // Remember that this point has taken over the role of attractor in this step
    mPoints.GetRepairState(attractorPointIndex).LastAttractorRepairStepId = repairStepId;

    //
    // (Attempt to) restore this point's deleted springs
    //

    bool hasAnySpringBeenRepaired = false;

    // Visit all the deleted springs that were connected at factory time
    for (auto const & fcs : mPoints.GetFactoryConnectedSprings(attractorPointIndex).ConnectedSprings)
    {
        if (mSprings.IsDeleted(fcs.SpringIndex))
        {
            auto const attracteePointIndex = fcs.OtherEndpointIndex;

            // Do not consider the spring if the other endpoint has already taken
            // the role of attractor in this step
            //
            // Note: we allow a point to be an attractee multiple times, as that helps it move better
            // into "multiple target places" at the same time
            if (mPoints.GetRepairState(attracteePointIndex).LastAttractorRepairStepId != repairStepId)
            {
                //
                // This point has taken over the role of attractee in this step
                //

                // Check if first time it became an actractee in this step
                if (mPoints.GetRepairState(attracteePointIndex).LastAttracteeRepairStepId != repairStepId)
                {
                    // Update its count of consecutive steps as an attractee
                    if (mPoints.GetRepairState(attracteePointIndex).LastAttracteeRepairStepId == repairStepId.Previous())
                        ++mPoints.GetRepairState(attracteePointIndex).CurrentAttracteeConsecutiveNumberOfSteps;
                    else
                        mPoints.GetRepairState(attracteePointIndex).CurrentAttracteeConsecutiveNumberOfSteps = 1;

                    // Remember it took on this role now
                    mPoints.GetRepairState(attracteePointIndex).LastAttracteeRepairStepId = repairStepId;
                }



                ////////////////////////////////////////////////////////
                //
                // Attempt to restore this spring by moving the other endpoint nearer
                //
                ////////////////////////////////////////////////////////

                //
                // The target position of the endpoint is on the circle whose radius
                // is the spring's rest length, and the angle is interpolated between
                // the two non-deleted springs immediately CW and CCW of this spring
                //

                // The angle of the spring wrt the attractor
                // 0 = E, 1 = SE, ..., 7 = NE
                Octant const factoryPointSpringOctant = mSprings.GetFactoryEndpointOctant(
                    fcs.SpringIndex,
                    attractorPointIndex);

                //
                // 1. Find nearest CW spring and nearest CCW spring
                // (which might end up being the same spring in case there's only one spring)
                //

                int nearestCWSpringIndex = -1;
                int nearestCWSpringDeltaOctant = std::numeric_limits<int>::max();
                int nearestCCWSpringIndex = -1;
                int nearestCCWSpringDeltaOctant = std::numeric_limits<int>::max();
                for (auto const & cs : mPoints.GetConnectedSprings(attractorPointIndex).ConnectedSprings)
                {
                    //
                    // CW
                    //

                    int cwDelta =
                        mSprings.GetFactoryEndpointOctant(cs.SpringIndex, attractorPointIndex)
                        - factoryPointSpringOctant;

                    if (cwDelta < 0)
                        cwDelta += 8;

                    if (cwDelta < nearestCWSpringDeltaOctant)
                    {
                        nearestCWSpringIndex = cs.SpringIndex;
                        nearestCWSpringDeltaOctant = cwDelta;
                    }

                    //
                    // CCW
                    //

                    assert(cwDelta > 0 && cwDelta < 8);
                    int ccwDelta = 8 - cwDelta;
                    assert(ccwDelta > 0);

                    if (ccwDelta < nearestCCWSpringDeltaOctant)
                    {
                        nearestCCWSpringIndex = cs.SpringIndex;
                        nearestCCWSpringDeltaOctant = ccwDelta;
                    }
                }

                assert(nearestCWSpringIndex >= 0);
                assert(nearestCWSpringDeltaOctant > 0);
                assert(nearestCCWSpringIndex >= 0);
                assert(nearestCCWSpringDeltaOctant > 0);

                //
                // 2. Calculate this spring's world angle by
                // interpolating among these two springs
                //

                ElementIndex const ccwSpringOtherEndpointIndex =
                    mSprings.GetOtherEndpointIndex(nearestCCWSpringIndex, attractorPointIndex);

                ElementIndex const cwSpringOtherEndpointIndex =
                    mSprings.GetOtherEndpointIndex(nearestCWSpringIndex, attractorPointIndex);

                // Angle between these two springs (internal angle)
                float neighborsAngleCw =
                    (ccwSpringOtherEndpointIndex == cwSpringOtherEndpointIndex)
                    ? 2.0f * Pi<float>
                    : (mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(attractorPointIndex))
                    .angleCw(mPoints.GetPosition(cwSpringOtherEndpointIndex) - mPoints.GetPosition(attractorPointIndex));

                if (neighborsAngleCw < 0.0f)
                    neighborsAngleCw += 2.0f * Pi<float>;

                // Interpolated angle - offset from CCW spring
                float const interpolatedAngleCwFromCCWSpring =
                    neighborsAngleCw
                    / static_cast<float>(nearestCWSpringDeltaOctant + nearestCCWSpringDeltaOctant) // Span between two springs, in octants
                    * static_cast<float>(nearestCCWSpringDeltaOctant);

                // And finally, the target world angle (world angle is 0 at E), by adding
                // interpolated CCW spring angle offset to world angle of CCW spring
                float const nearestCCWSpringWorldAngle = vec2f(1.0f, 0.0f).angleCw(mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(attractorPointIndex));
                // In world coordinates, CW, 0 at E
                float const targetWorldAngleCw =
                    nearestCCWSpringWorldAngle
                    + interpolatedAngleCwFromCCWSpring;

                //
                // Calculate target position for the attractee
                //

                vec2f const targetAttracteePosition =
                    mPoints.GetPosition(attractorPointIndex)
                    + vec2f::fromPolar(
                        mSprings.GetFactoryRestLength(fcs.SpringIndex),
                        targetWorldAngleCw);

                //
                // Check whether this spring with the endpoint at its calculated
                // target position would generate a CCW triangle; if so, we'll
                // ignore it as we want to avoid creating folded structures.
                // We rely on its particles to somehow acquire later their correct
                // positions
                //

                bool springWouldGenerateCCWTriangle = false;

                for (auto const testTriangleIndex : mSprings.GetFactorySuperTriangles(fcs.SpringIndex))
                {
                    vec2f vertexPositions[3];

                    // Edge A
                    vertexPositions[0] = (mTriangles.GetPointAIndex(testTriangleIndex) == attracteePointIndex)
                        ? targetAttracteePosition
                        : mPoints.GetPosition(mTriangles.GetPointAIndex(testTriangleIndex));

                    // Edge B
                    vertexPositions[1] = (mTriangles.GetPointBIndex(testTriangleIndex) == attracteePointIndex)
                        ? targetAttracteePosition
                        : mPoints.GetPosition(mTriangles.GetPointBIndex(testTriangleIndex));

                    // Edge C
                    vertexPositions[2] = (mTriangles.GetPointCIndex(testTriangleIndex) == attracteePointIndex)
                        ? targetAttracteePosition
                        : mPoints.GetPosition(mTriangles.GetPointCIndex(testTriangleIndex));

                    vec2f const edges[3]
                    {
                        vertexPositions[1] - vertexPositions[0],
                        vertexPositions[2] - vertexPositions[1],
                        vertexPositions[0] - vertexPositions[2]
                    };

                    if (edges[0].cross(edges[1]) > 0.0f
                        || edges[1].cross(edges[2]) > 0.0f
                        || edges[2].cross(edges[0]) > 0.0f)
                    {
                        springWouldGenerateCCWTriangle = true;
                        break;
                    }
                }

                if (springWouldGenerateCCWTriangle)
                {
                    // Skip this spring
                    continue;
                }

                ////mOverlays.AddPointToPointArrow(
                ////    mPoints.GetPlaneId(attracteePointIndex),
                ////    mPoints.GetPosition(attracteePointIndex),
                ////    targetAttracteePosition,
                ////    rgbColor(250, 40, 40));

                //
                // Check progress of attractee towards the target position
                //

                // Displacement vector (positive towards target)
                vec2f const displacementVector = targetAttracteePosition - mPoints.GetPosition(attracteePointIndex);

                // Distance
                float displacementMagnitude = displacementVector.length();

                // Check whether we are still further away than our tolerance,
                // and whether the attractee is free to move
                bool hasAttracteeBeenMoved = false;
                float displacementToleranceBoost = 1.0f;
                if (displacementMagnitude > DisplacementTolerance
                    && !mPoints.IsPinned(attracteePointIndex))
                {
                    //
                    // Endpoints are too far...
                    // ...move them closer by moving the attractee towards its target position
                    //

                    // Smooth movement:
                    // * Lonely particle: fast when far, slowing when getting closer
                    // * Connected particle: based on how long this point has been an attracted during
                    //   the current session - so to force detachment when particle is entangled with
                    //   something heavy
                    float movementSmoothing;
                    auto const attracteeDurationSteps = mPoints.GetRepairState(attracteePointIndex).CurrentAttracteeConsecutiveNumberOfSteps;
                    if (mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.empty())
                    {
                        // Orphan
                        //
                        // Slow down at small distances, but increase with insisting time to prevent lonely particles
                        // from getting frozen in mid-air

                        int constexpr MaxSimulatedFrames = 5 * 64; // 5 simulated seconds at 64fps

                        movementSmoothing =
                            SmoothStep(0.0f, 20.0f / simulationParameters.RepairSpeedAdjustment, displacementMagnitude)
                            * (0.15f + 0.35f * SmoothStep(0.0f, static_cast<float>(MaxSimulatedFrames) / simulationParameters.RepairSpeedAdjustment, static_cast<float>(attracteeDurationSteps)));

                        if (attracteeDurationSteps >= MaxSimulatedFrames
                            && (attracteeDurationSteps % 32) == 0)
                        {
                            // Hammer-boost
                            displacementToleranceBoost = 3.5f;
                        }
                    }
                    else
                    {
                        // Connected
                        //
                        // Ramp up forces over time

                        int constexpr MaxSimulatedFrames = 15 * 64; // 15 simulated seconds at 64fps

                        // Allow chains to move slower and thus have more chances to attach
                        float const timeAdjustment = (mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.size() == 1)
                            ? 1.6f
                            : 1.0f;

                        movementSmoothing = SmoothStep(
                            0.0f,
                            static_cast<float>(MaxSimulatedFrames) * timeAdjustment / simulationParameters.RepairSpeedAdjustment,
                            static_cast<float>(attracteeDurationSteps));

                        if (attracteeDurationSteps >= MaxSimulatedFrames
                            && (attracteeDurationSteps % 32) == 0)
                        {
                            // Hammer-boost
                            movementSmoothing *= 1.2f;

                            LogMessage("Repair: structure Hammer-Boost");
                        }
                    }

                    // Movement direction (positive towards this point)
                    vec2f const movementDir = displacementVector.normalise(displacementMagnitude);

                    // Movement magnitude
                    //
                    // The magnitude is multiplied with the point's repair smoothing, which goes
                    // from 0.0 at the moment the point is first engaged, to 1.0 later on.
                    //
                    // Note: here we calculate the movement based on the static positions
                    // of the two endpoints; however, if the two endpoints have a non-zero
                    // relative velocity, then this movement won't achieve the desired effect
                    // (it will undershoot or overshoot). I do think the end result is cool
                    // though, as you end up, for example, with points chasing a part of a ship
                    // that's moving away!
                    float const movementMagnitude =
                        displacementMagnitude
                        * movementSmoothing
                        * repairStrength;

                    // Move point, clamping to world boundaries
                    mPoints.SetPosition(attracteePointIndex,
                        (mPoints.GetPosition(attracteePointIndex) + movementDir * movementMagnitude)
                        .clamp(
                            -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth,
                            -SimulationParameters::HalfMaxWorldHeight, SimulationParameters::HalfMaxWorldHeight));

                    // Update displacement with move
                    displacementMagnitude -= movementMagnitude;

                    // Impart some non-linear inertia (smaller at higher displacements, higher at very low displacements),
                    // retaining a bit of the previous velocity
                    auto const displacementVelocity =
                        movementDir
                        * ((movementMagnitude < 0.0f) ? -1.0f : 1.0f)
                        * std::pow(std::abs(movementMagnitude), 0.2f)
                        / SimulationParameters::SimulationStepTimeDuration<float>
                        * 0.5f;
                    float constexpr InertialFraction = 0.65f;
                    mPoints.SetVelocity(attracteePointIndex,
                        (mPoints.GetVelocity(attracteePointIndex) * (1.0f - InertialFraction))
                        + (displacementVelocity * InertialFraction));

                    // Remember that we've acted on the attractee
                    hasAttracteeBeenMoved = true;
                }

                // Check whether we are now close enough to restore the spring
                if (displacementMagnitude <= DisplacementTolerance * displacementToleranceBoost)
                {
                    //
                    // The attractee is close enough to its target, implying that
                    // the spring length should be close to its rest length...
                    // ...we can restore the spring
                    //

                    // Restore the spring
                    mSprings.Restore(
                        fcs.SpringIndex,
                        simulationParameters,
                        mPoints);

                    assert(!mSprings.IsDeleted(fcs.SpringIndex));

                    // Forget that the attractee has been an attractee in this step, to allow it
                    // to soon take the role of attractor
                    mPoints.GetRepairState(attracteePointIndex).LastAttracteeRepairStepId = SequenceNumber::None();
                    mPoints.GetRepairState(attracteePointIndex).CurrentAttractorPropagationVisitStepId = SequenceNumber::None();

                    // Impart to the attractee the average velocity of all of its
                    // connected particles, including the attractor's
                    assert(mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.size() > 0);
                    vec2f const sumVelocity = std::accumulate(
                        mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.cbegin(),
                        mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.cend(),
                        vec2f::zero(),
                        [this](vec2f const & total, auto cs)
                        {
                            return total + mPoints.GetVelocity(cs.OtherEndpointIndex);
                        });
                    mPoints.SetVelocity(
                        attracteePointIndex,
                        sumVelocity / static_cast<float>(mPoints.GetConnectedSprings(attracteePointIndex).ConnectedSprings.size()));

                    // Halve the decay of both endpoints, to prevent newly-repaired
                    // rotten particles from crumbling again
                    float const attractorDecay = mPoints.GetDecay(attractorPointIndex);
                    mPoints.SetDecay(
                        attractorPointIndex,
                        attractorDecay + (1.0f - attractorDecay) / 2.0f);
                    float const attracteeDecay = mPoints.GetDecay(attracteePointIndex);
                    mPoints.SetDecay(
                        attracteePointIndex,
                        attracteeDecay + (1.0f - attracteeDecay) / 2.0f);

                    // Restore the spring's rest length to its factory value
                    mSprings.SetRestLength(
                        fcs.SpringIndex,
                        mSprings.GetFactoryRestLength(fcs.SpringIndex));

                    // Attempt to restore both endpoints
                    AttemptPointRestore(attractorPointIndex, currentSimulationTime);
                    AttemptPointRestore(attracteePointIndex, currentSimulationTime);

                    // Recalculate the spring's coefficients, since we have changed the
                    // spring's rest length
                    mSprings.UpdateForRestLength(
                        fcs.SpringIndex,
                        mPoints);

                    // Remember that we've acted on the attractee
                    hasAttracteeBeenMoved = true;

                    // Remember that we've repaired a spring
                    hasAnySpringBeenRepaired = true;
                }

                //
                // Dry the attractee, if we've messed with it
                //

                if (hasAttracteeBeenMoved)
                {
                    mPoints.SetWater(
                        attracteePointIndex,
                        mPoints.GetWater(attracteePointIndex) / 2.0f);
                }
            }
        }
    }

    return hasAnySpringBeenRepaired;
}

}