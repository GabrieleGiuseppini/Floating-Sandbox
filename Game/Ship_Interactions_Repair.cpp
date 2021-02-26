/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <cassert>
#include <deque>

namespace Physics {

void Ship::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    SequenceNumber repairStepId,
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    float const searchRadius =
        gameParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    ////// TODOTEST
    ////mDebugMarker.ClearPointToPointArrows();

    //
    // Pass 1: straighten one-spring and two-spring naked springs
    //

    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius)
        {
            StraightenOneSpringChains(pointIndex);

            StraightenTwoSpringChains(pointIndex);
        }
    }

    //
    // Pass 2: visit all points that had been attractors in the previous step
    //
    // This is to prevent attractors and attractees from flipping roles during a session;
    // an attractor will continue to be an attractor until it needs reparation
    //

    // We store points in radius to speedup pass 2
    std::vector<ElementIndex> pointsInRadius;

    // Visit all (in-radius) non-ephemeral points that had been attractors in the previous step
    auto const previousStep = repairStepId.Previous();
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius)
        {
            pointsInRadius.push_back(pointIndex);

            if (mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId == previousStep)
            {
                TryRepairAndPropagateFromPoint(
                    pointIndex,
                    targetPos,
                    squareSearchRadius,
                    repairStepId,
                    gameParameters);
            }
        }
    }

    //
    // Pass 2: visit all other points now, to give a chance to everyone else to be
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
            gameParameters);
    }

    //
    // Pass 3:
    //  a) Restore deleted _eligible_ triangles that were connected to each (in-radius) point
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

    // Visit all (in-radius) non-ephemeral points
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius)
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
                        AttemptPointRestore(mTriangles.GetPointAIndex(fct));
                        AttemptPointRestore(mTriangles.GetPointBIndex(fct));
                        AttemptPointRestore(mTriangles.GetPointCIndex(fct));
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
        }
    }
}

void Ship::StraightenOneSpringChains(ElementIndex pointIndex)
{
    auto const & connectedSprings = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings;
    if (connectedSprings.size() >= 2)
    {
        // Visit naked springs not connected to anything else
        for (auto const & nakedCs : connectedSprings)
        {
            ElementIndex const otherEndpointIndex = mSprings.GetOtherEndpointIndex(nakedCs.SpringIndex, pointIndex);
            if (mSprings.GetSuperTriangles(nakedCs.SpringIndex).empty() // Naked
                && mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size() == 1) // Other endpoint only has this naked spring
            {
                // TODOTEST: first we try with CCW only, then might want to try with interpolation
                // between CW and CCW

                //
                // Move other endpoint where it should be wrt the CCW spring
                // nearest to this spring
                // (CCW arbitrarily)
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

                // In world coordinates, CW, 0 at E
                float const targetWorldAngleCw =
                    nearestCCWSpringWorldAngle
                    + 2.0f * Pi<float> / 8.0f * static_cast<float>(nearestCCWSpringDeltaOctant);

                //
                // Calculate target position for the other endpoint
                //

                vec2f const targetOtherEndpointPosition =
                    mPoints.GetPosition(pointIndex)
                    + vec2f::fromPolar(
                        (mPoints.GetPosition(otherEndpointIndex) - mPoints.GetPosition(pointIndex)).length(),
                        targetWorldAngleCw);

                //
                // Move the other endpoint
                //

                // TODOHERE: world bounds
                mPoints.SetPosition(otherEndpointIndex, targetOtherEndpointPosition);
            }
        }
    }
}

void Ship::StraightenTwoSpringChains(ElementIndex pointIndex)
{
    //
    // Here we detect P (connected to R and L by naked springs) being on the
    // wrong side of RL, and flip it
    //
    //     P
    //     O
    //    / \
    //   /   \
    //  /     \
    // O       O
    // R       L
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

        ElementIndex prSpring, plSpring;
        Octant prOctant, plOctant;

        if (spring1Octant == spring0Octant + 2
            || (spring1Octant < 2 && spring1Octant == spring0Octant + 2 - 8))
        {
            prSpring = connectedSprings[1].SpringIndex;
            prOctant = spring1Octant;
            plSpring = connectedSprings[0].SpringIndex;
            plOctant = spring0Octant;
        }
        else if (spring0Octant == spring1Octant + 2
            || (spring0Octant < 2 && spring0Octant == spring1Octant + 2 - 8))
        {
            prSpring = connectedSprings[0].SpringIndex;
            prOctant = spring0Octant;
            plSpring = connectedSprings[1].SpringIndex;
            plOctant = spring1Octant;
        }
        else
        {
            // Not under our jurisdiction
            // TODOHERE: do we really need to bail out here?
            return;
        }

        // Check if PR is still at the right or PL

        vec2f const & pPosition = mPoints.GetPosition(pointIndex);
        vec2f const & lPosition = mPoints.GetPosition(mSprings.GetOtherEndpointIndex(plSpring, pointIndex));
        vec2f const & rPosition = mPoints.GetPosition(mSprings.GetOtherEndpointIndex(prSpring, pointIndex));

        vec2f const prVector = rPosition - pPosition;
        vec2f const plVector = lPosition - pPosition;
        if (prVector.cross(plVector) < 0.0f)
        {
            //
            // This arc needs to be straightened
            //

            // Reflect P onto the other side of the RL vector: RP' = PR - RL * 2 * (PR dot RL) / |RL|^2
            vec2f const rlVector = lPosition - rPosition;
            vec2f const newPPosition = rPosition + prVector - rlVector * 2.0f * (prVector.dot(rlVector)) / rlVector.squareLength();

            // Set position
            // TODOHERE: world bounds
            mPoints.SetPosition(pointIndex, newPPosition);
        }
    }
}

bool Ship::TryRepairAndPropagateFromPoint(
    ElementIndex startingPointIndex,
    vec2f const & targetPos,
    float squareSearchRadius,
    SequenceNumber repairStepId,
    GameParameters const & gameParameters)
{
    bool hasRepairedAnything = false;

    // Conditions for a point to be an attactor:
    //  - is in radius
    //  - and has not already been an attractor in this step
    //  - and has not been an attractee in this step
    //  - and has not been an attractee in the *previous* step (so to prevent
    //    sudden role flipping)
    //  - and needs reparation
    //  - and is not orphaned (we rely on existing springs in order to repair)
    //
    // After being an attractor, do a breadth-first visit from the point propagating
    // repair from directly-connected candidates

    std::deque<ElementIndex> pointsToVisit;

    for (ElementIndex pointIndex = startingPointIndex; ;)
    {
        // Mark point as visited
        mPoints.GetRepairState(pointIndex).CurrentAttractorPropagationVisitStepId = repairStepId;

        //
        // Check if this point meets the conditions to propagate
        //

        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius
            && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0) // Not orphaned
        {
            //
            // Check if this point meets the remaining conditions for being an attractor
            //

            if (mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId != repairStepId
                && mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId
                && mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId.Previous()
                && mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings.size() > mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size()) // Needs reparation
            {
                //
                // This point has now taken the role of an attractor
                //

                // Calculate repair strength (1.0 at center and zero at border, fourth power)
                float const repairStrength =
                    (1.0f - (squareRadius / squareSearchRadius) * (squareRadius / squareSearchRadius))
                    * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

                // Repair from this point
                bool const hasRepaired = RepairFromAttractor(
                    pointIndex,
                    repairStrength,
                    repairStepId,
                    gameParameters);

                hasRepairedAnything |= hasRepaired;
            }

            //
            // Propagate to all of the no-yet-visited immediately-connected points
            //

            for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                if (mPoints.GetRepairState(cs.OtherEndpointIndex).CurrentAttractorPropagationVisitStepId != repairStepId)
                {
                    pointsToVisit.push_back(cs.OtherEndpointIndex);
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

    return hasRepairedAnything;
}

bool Ship::RepairFromAttractor(
    ElementIndex pointIndex,
    float repairStrength,
    SequenceNumber repairStepId,
    GameParameters const & gameParameters)
{
    // Tolerance to distance: the minimum distance between the endpoint
    // of a broken spring and its target position, below which we restore
    // the spring
    //
    // Note: a higher tolerance here causes springs to...spring into life
    // already stretched or compressed, generating an undesirable force impulse
    //
    // - Shipped 1.13 with 0.07
    float constexpr DisplacementTolerance = 0.06f;

    ////////////////////////////////////////////////////////////////////////////

    // This point hasn't taken any role yet in this step
    assert(mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId != repairStepId);
    assert(mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId);

    // Remember that this point has taken over the role of attractor in this step
    mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId = repairStepId;

    //
    // (Attempt to) restore this point's deleted springs
    //

    bool hasAnySpringBeenRepaired = false;

    // Visit all the deleted springs that were connected at factory time
    for (auto const & fcs : mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings)
    {
        if (mSprings.IsDeleted(fcs.SpringIndex))
        {
            auto const otherEndpointIndex = fcs.OtherEndpointIndex;

            // Do not consider the spring if the other endpoint has already taken
            // the role of attractor in this step
            //
            // Note: we allow a point to be an attractee multiple times, as that helps it move better
            // into "multiple target places" at the same time
            if (mPoints.GetRepairState(otherEndpointIndex).LastAttractorRepairStepId != repairStepId)
            {
                //
                // This point has taken over the role of attractee in this step
                //

                // Update its count of consecutive steps as an attractee, if this is its first time as an actractee
                // in this step
                if (mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId != repairStepId)
                {
                    if (mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId == repairStepId.Previous())
                        mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps++;
                    else
                        mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps = 1;
                }

                // Remember the role
                mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId = repairStepId;

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

                // The angle of the spring wrt this point
                // 0 = E, 1 = SE, ..., 7 = NE
                Octant const factoryPointSpringOctant = mSprings.GetFactoryEndpointOctant(
                    fcs.SpringIndex,
                    pointIndex);

                //
                // 1. Find nearest CW spring and nearest CCW spring
                // (which might end up being the same spring in case there's only one spring)
                //

                int nearestCWSpringIndex = -1;
                int nearestCWSpringDeltaOctant = std::numeric_limits<int>::max();
                int nearestCCWSpringIndex = -1;
                int nearestCCWSpringDeltaOctant = std::numeric_limits<int>::max();
                for (auto const & cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
                {
                    //
                    // CW
                    //

                    int cwDelta =
                        mSprings.GetFactoryEndpointOctant(cs.SpringIndex, pointIndex)
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
                    mSprings.GetOtherEndpointIndex(nearestCCWSpringIndex, pointIndex);

                ElementIndex const cwSpringOtherEndpointIndex =
                    mSprings.GetOtherEndpointIndex(nearestCWSpringIndex, pointIndex);

                // Angle between these two springs (internal angle)
                float neighborsAngleCw =
                    (ccwSpringOtherEndpointIndex == cwSpringOtherEndpointIndex)
                    ? 2.0f * Pi<float>
                    : (mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex))
                    .angleCw(mPoints.GetPosition(cwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex));

                if (neighborsAngleCw < 0.0f)
                    neighborsAngleCw += 2.0f * Pi<float>;

                // Interpolated angle - offset from CCW spring
                float const interpolatedAngleCwFromCCWSpring =
                    neighborsAngleCw
                    / static_cast<float>(nearestCWSpringDeltaOctant + nearestCCWSpringDeltaOctant) // Span between two springs, in octants
                    * static_cast<float>(nearestCCWSpringDeltaOctant);

                // And finally, the target world angle (world angle is 0 at E), by adding
                // interpolated CCW spring angle offset to world angle of CCW spring
                float const nearestCCWSpringWorldAngle = vec2f(1.0f, 0.0f).angleCw(mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex));
                // In world coordinates, CW, 0 at E
                float const targetWorldAngleCw =
                    nearestCCWSpringWorldAngle
                    + interpolatedAngleCwFromCCWSpring;

                //
                // Calculate target position for the other endpoint
                //

                vec2f const targetOtherEndpointPosition =
                    mPoints.GetPosition(pointIndex)
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
                    vertexPositions[0] = (mTriangles.GetPointAIndex(testTriangleIndex) == otherEndpointIndex)
                        ? targetOtherEndpointPosition
                        : mPoints.GetPosition(mTriangles.GetPointAIndex(testTriangleIndex));

                    // Edge B
                    vertexPositions[1] = (mTriangles.GetPointBIndex(testTriangleIndex) == otherEndpointIndex)
                        ? targetOtherEndpointPosition
                        : mPoints.GetPosition(mTriangles.GetPointBIndex(testTriangleIndex));

                    // Edge C
                    vertexPositions[2] = (mTriangles.GetPointCIndex(testTriangleIndex) == otherEndpointIndex)
                        ? targetOtherEndpointPosition
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

                ////// TODOTEST
                ////mDebugMarker.AddPointToPointArrow(
                ////    mPoints.GetPlaneId(otherEndpointIndex),
                ////    mPoints.GetPosition(otherEndpointIndex),
                ////    targetOtherEndpointPosition,
                ////    rgbColor(250, 40, 40));

                //
                // Check progress of other endpoint towards the target position
                //

                // Displacement vector (positive towards target point)
                vec2f const displacementVector = targetOtherEndpointPosition - mPoints.GetPosition(otherEndpointIndex);

                // Distance
                float displacementMagnitude = displacementVector.length();

                // Check whether we are still further away than our tolerance,
                // and whether this point is free to move
                bool hasOtherEndpointPointBeenMoved = false;
                if (displacementMagnitude > DisplacementTolerance
                    && !mPoints.IsPinned(otherEndpointIndex))
                {
                    //
                    // Endpoints are too far...
                    // ...move them closer by moving the other endpoint towards its target position
                    //

                    // Smooth movement:
                    // * Lonely particle: fast when far, slowing when getting closer
                    // * Connected particle: based on how long this point has been an attracted during
                    //   the current session - so to force detachment when particle is entangled with
                    //   something heavy
                    float movementSmoothing;
                    if (mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.empty())
                    {
                        movementSmoothing = SmoothStep(0.0f, 20.0f, displacementMagnitude)
                            * gameParameters.RepairSpeedAdjustment
                            * 0.15f;
                    }
                    else
                    {
                        movementSmoothing = SmoothStep(
                            0.0f,
                            (15.0f * 64.0f) / gameParameters.RepairSpeedAdjustment, // Reach max in 15 simulated seconds (at 64 fps)
                            static_cast<float>(mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps));
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
                    mPoints.SetPosition(otherEndpointIndex,
                        (mPoints.GetPosition(otherEndpointIndex) + movementDir * movementMagnitude)
                        .clamp(-GameParameters::HalfMaxWorldWidth,
                            GameParameters::HalfMaxWorldWidth,
                            -GameParameters::HalfMaxWorldHeight,
                            GameParameters::HalfMaxWorldHeight));

                    // Update displacement with move
                    assert(movementMagnitude < displacementMagnitude);
                    displacementMagnitude -= movementMagnitude;

                    // Impart some non-linear inertia (smaller at higher displacements),
                    // retaining a bit of the previous velocity
                    // (note: last one that pulls this point wins)
                    auto const displacementVelocity =
                        movementDir
                        * ((movementMagnitude < 0.0f) ? -1.0f : 1.0f)
                        * std::pow(std::abs(movementMagnitude), 0.2f)
                        / GameParameters::SimulationStepTimeDuration<float>
                        * 0.5f;
                    mPoints.SetVelocity(otherEndpointIndex,
                        (mPoints.GetVelocity(otherEndpointIndex) * 0.35f)
                        + (displacementVelocity * 0.65f));

                    // Remember that we've acted on the other endpoint
                    hasOtherEndpointPointBeenMoved = true;
                }

                // Check whether we are now close enough to restore the spring
                if (displacementMagnitude <= DisplacementTolerance)
                {
                    //
                    // The other endpoint is close enough to its target, implying that
                    // the spring length should be close to its rest length...
                    // ...we can restore the spring
                    //

                    // Restore the spring
                    mSprings.Restore(
                        fcs.SpringIndex,
                        gameParameters,
                        mPoints);

                    assert(!mSprings.IsDeleted(fcs.SpringIndex));

                    // Forget that the other endpoint has been an attractee in this step,
                    // so that it might soon take the role of attractor
                    mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId = SequenceNumber::None();

                    // Impart to the other endpoint the average velocity of all of its
                    // connected particles, including the new one
                    assert(mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size() > 0);
                    vec2f const sumVelocity = std::accumulate(
                        mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.cbegin(),
                        mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.cend(),
                        vec2f::zero(),
                        [this](vec2f const & total, auto cs)
                        {
                            return total + mPoints.GetVelocity(cs.OtherEndpointIndex);
                        });
                    mPoints.SetVelocity(
                        otherEndpointIndex,
                        sumVelocity / static_cast<float>(mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size()));

                    // Halve the decay of both endpoints, to prevent newly-repaired
                    // rotten particles from crumbling again
                    float const pointDecay = mPoints.GetDecay(pointIndex);
                    mPoints.SetDecay(
                        pointIndex,
                        pointDecay + (1.0f - pointDecay) / 2.0f);
                    float const otherPointDecay = mPoints.GetDecay(otherEndpointIndex);
                    mPoints.SetDecay(
                        otherEndpointIndex,
                        otherPointDecay + (1.0f - otherPointDecay) / 2.0f);

                    // Restore the spring's rest length to its factory value
                    mSprings.SetRestLength(
                        fcs.SpringIndex,
                        mSprings.GetFactoryRestLength(fcs.SpringIndex));

                    // Attempt to restore both endpoints
                    AttemptPointRestore(pointIndex);
                    AttemptPointRestore(otherEndpointIndex);

                    // Recalculate the spring's coefficients, since we have changed the
                    // spring's rest length
                    mSprings.UpdateForRestLength(
                        fcs.SpringIndex,
                        mPoints);

                    // Remember that we've acted on the other endpoint
                    hasOtherEndpointPointBeenMoved = true;

                    // Remember that we've repaired a spring
                    hasAnySpringBeenRepaired = true;
                }

                //
                // Dry the other endpoint, if we've messed with it
                //

                if (hasOtherEndpointPointBeenMoved)
                {
                    mPoints.GetWater(otherEndpointIndex) /= 2.0f;
                }
            }
        }
    }

    return hasAnySpringBeenRepaired;
}

}