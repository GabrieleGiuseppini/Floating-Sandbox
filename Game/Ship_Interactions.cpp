/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include <GameCore/AABB.h>
#include <GameCore/GameDebug.h>
#include <GameCore/GameGeometry.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

namespace Physics {

//   SSS    H     H  IIIIIII  PPPP
// SS   SS  H     H     I     P   PP
// S        H     H     I     P    PP
// SS       H     H     I     P   PP
//   SSS    HHHHHHH     I     PPPP
//      SS  H     H     I     P
//       S  H     H     I     P
// SS   SS  H     H     I     P
//   SSS    H     H  IIIIIII  P

std::optional<ElementIndex> Ship::PickPointToMove(
    vec2f const & pickPosition,
    GameParameters const & gameParameters) const
{
    //
    // Find closest non-ephemeral point within the radius
    //

    float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

    // Separate orphaned and non-orphaned points; we'll choose
    // orphaned when there are no non-orphaned
    float bestNonOrphanedSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestNonOrphanedPoint = NoneElementIndex;
    float bestOrphanedSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestOrphanedPoint = NoneElementIndex;

    for (auto p : mPoints.RawShipPoints())
    {
        float const squareDistance = (mPoints.GetPosition(p) - pickPosition).squareLength();
        if (squareDistance < squareSearchRadius)
        {
            if (!mPoints.GetConnectedSprings(p).ConnectedSprings.empty())
            {
                if (squareDistance < bestNonOrphanedSquareDistance)
                {
                    bestNonOrphanedSquareDistance = squareDistance;
                    bestNonOrphanedPoint = p;
                }
            }
            else
            {
                if (squareDistance < bestOrphanedSquareDistance)
                {
                    bestOrphanedSquareDistance = squareDistance;
                    bestOrphanedPoint = p;
                }
            }
        }
    }

    if (bestNonOrphanedPoint != NoneElementIndex)
        return bestNonOrphanedPoint;
    else if (bestOrphanedPoint != NoneElementIndex)
        return bestOrphanedPoint;
    else
        return std::nullopt;
}

void Ship::MoveBy(
    ElementIndex pointElementIndex,
    vec2f const & offset,
    vec2f const & inertialVelocity,
    GameParameters const & gameParameters)
{
    vec2f const actualInertialVelocity =
        inertialVelocity
        * gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    // Get connected component ID of the point
    auto connectedComponentId = mPoints.GetConnectedComponentId(pointElementIndex);
    if (connectedComponentId != NoneConnectedComponentId)
    {
        // Move all points (ephemeral and non-ephemeral) that belong to the same connected component
        for (auto p : mPoints)
        {
            if (mPoints.GetConnectedComponentId(p) == connectedComponentId)
            {
                mPoints.GetPosition(p) += offset;

                if (!mPoints.IsPinned(p))
                {
                    mPoints.SetVelocity(p, actualInertialVelocity);
                }
            }
        }

        TrimForWorldBounds(gameParameters);
    }
}

void Ship::MoveBy(
    vec2f const & offset,
    vec2f const & inertialVelocity,
    GameParameters const & gameParameters)
{
    vec2f const actualInertialVelocity =
        inertialVelocity
        * gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    for (auto p : mPoints.BufferElements())
    {
        positionBuffer[p] += offset;
        velocityBuffer[p] = actualInertialVelocity;
    }

    TrimForWorldBounds(gameParameters);
}

void Ship::RotateBy(
    ElementIndex pointElementIndex,
    float angle,
    vec2f const & center,
    float inertialAngle,
    GameParameters const & gameParameters)
{
    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    float const inertiaMagnitude =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const inertialRotX(cos(inertialAngle), sin(inertialAngle));
    vec2f const inertialRotY(-sin(inertialAngle), cos(inertialAngle));

    // Get connected component ID of the point
    auto connectedComponentId = mPoints.GetConnectedComponentId(pointElementIndex);
    if (connectedComponentId != NoneConnectedComponentId)
    {
        // Rotate all points (ephemeral and non-ephemeral) that belong to the same connected component
        for (auto p : mPoints)
        {
            if (mPoints.GetConnectedComponentId(p) == connectedComponentId)
            {
                vec2f const centeredPos = mPoints.GetPosition(p) - center;

                mPoints.GetPosition(p) = vec2f(centeredPos.dot(rotX), centeredPos.dot(rotY)) + center;

                if (!mPoints.IsPinned(p))
                {
                    mPoints.SetVelocity(
                        p,
                        (vec2f(centeredPos.dot(inertialRotX), centeredPos.dot(inertialRotY)) - centeredPos) * inertiaMagnitude);
                }
            }
        }

        TrimForWorldBounds(gameParameters);
    }
}

void Ship::RotateBy(
    float angle,
    vec2f const & center,
    float inertialAngle,
    GameParameters const & gameParameters)
{
    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    float const inertiaMagnitude =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const inertialRotX(cos(inertialAngle), sin(inertialAngle));
    vec2f const inertialRotY(-sin(inertialAngle), cos(inertialAngle));

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    for (auto p : mPoints.BufferElements())
    {
        vec2f const centeredPos = positionBuffer[p] - center;

        positionBuffer[p] = vec2f(centeredPos.dot(rotX), centeredPos.dot(rotY)) + center;
        velocityBuffer[p] =
            (vec2f(centeredPos.dot(inertialRotX), centeredPos.dot(inertialRotY)) - centeredPos) * inertiaMagnitude;
    }

    TrimForWorldBounds(gameParameters);
}

std::optional<ElementIndex> Ship::PickObjectForPickAndPull(
    vec2f const & pickPosition,
    GameParameters const & /*gameParameters*/)
{
    //
    // Find closest point - of any type - within the search radius
    //

    float constexpr SearchRadius = 0.75f; // Magic number
    float constexpr SquareSearchRadius = SearchRadius * SearchRadius;

    float bestSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestPoint = NoneElementIndex;

    for (auto p : mPoints)
    {
        float const squareDistance = (mPoints.GetPosition(p) - pickPosition).squareLength();
        if (squareDistance < SquareSearchRadius
            && squareDistance < bestSquareDistance
            && mPoints.IsActive(p)
            && !mPoints.IsPinned(p))
        {
            bestSquareDistance = squareDistance;
            bestPoint = p;
        }
    }

    if (bestPoint != NoneElementIndex)
        return bestPoint;
    else
        return std::nullopt;
}

void Ship::Pull(
    ElementIndex pointElementIndex,
    vec2f const & target,
    GameParameters const & gameParameters)
{
    //
    //
    // Exhert a pull on the specified particle, according to a Hookean force
    //
    //


    //
    // In order to ensure stability, we choose a stiffness equal to the maximum stiffness
    // that keeps the system stable. This is the stiffness that generates a force such
    // that its integration in a simulation step (DT) produces a delta position
    // equal (and not greater) than the "spring"'s displacement itself.
    // In a regime where the particle velocity is zeroed at every simulation - like we do
    // here - and thus it only exists during the N mechanical sub-iterations, the delta
    // position after i mechanical sub-iterations of a force F is:
    //      Dp(i) = T(i) * F/m * dt^2
    // Where T(n) is the triangular coefficient, and dt is the sub-iteration delta-time
    // (i.e. DT/N)
    //

    float const triangularCoeff =
        (gameParameters.NumMechanicalDynamicsIterations<float>() * (gameParameters.NumMechanicalDynamicsIterations<float>() + 1.0f))
        / 2.0f;

    float const forceStiffness =
        mPoints.GetMass(pointElementIndex)
        / (gameParameters.MechanicalSimulationStepTimeDuration<float>() * gameParameters.MechanicalSimulationStepTimeDuration<float>())
        / triangularCoeff
        * (gameParameters.IsUltraViolentMode ? 4.0f : 1.0f);

    //
    // Now calculate Hookean force
    //

    vec2f const displacement = target - mPoints.GetPosition(pointElementIndex);
    float const displacementLength = displacement.length();
    vec2f const dir = displacement.normalise(displacementLength);

    mPoints.GetNonSpringForce(pointElementIndex) +=
        dir
        * (displacementLength * forceStiffness);

    //
    // Zero velocity: this it a bit unpolite, but it prevents the "classic"
    // Hookean force/Euler instability; also, prevents orbit forming which would
    // occur if we were to dump velocities along the point->target direction only
    //

    mPoints.SetVelocity(pointElementIndex, vec2f::zero());

    ////////////////////////////////////////////////////////////

    //
    // Highlight element
    //

    // The "strength" of the highlight depends on the displacement magnitude,
    // going asymptotically to 1.0 for length = 200
    float const highlightStrength = 1.0f - std::exp(-displacementLength / 10.0f);

    mPoints.StartCircleHighlight(
        pointElementIndex,
        rgbColor(
            Mix(
                vec3f(0.0f, 0.0f, 0.0f),
                vec3f(1.0f, 0.1f, 0.1f),
                highlightStrength)));
}

bool Ship::DestroyAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    bool hasDestroyed = false;

    auto const doDestroyPoint =
        [&](ElementIndex pointIndex)
        {
            // Choose a detach velocity - using the same distribution as Debris
            vec2f const detachVelocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
                GameParameters::MinDebrisParticlesVelocity,
                GameParameters::MaxDebrisParticlesVelocity);

            // Detach
            DetachPointForDestroy(
                pointIndex,
                detachVelocity,
                currentSimulationTime,
                gameParameters);

            // Record event, if requested to
            if (mEventRecorder != nullptr)
            {
                mEventRecorder->RecordEvent<RecordedPointDetachForDestroyEvent>(
                    pointIndex,
                    detachVelocity,
                    currentSimulationTime);
            }
        };

    //
    // Destroy points probabilistically - probability is one at
    // distance = 0 and zero at distance = radius
    //

    float const radius =
        gameParameters.DestroyRadius
        * radiusMultiplier
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    float const squareRadius = radius * radius;

    // Nearest point in a radius that guarantees the presence of a particle
    float constexpr FallbackSquareRadius = 0.75f;
    ElementIndex nearestFallbackPointInRadiusIndex = NoneElementIndex;
    float nearestFallbackPointRadius = std::numeric_limits<float>::max();

    float const largerSearchSquareRadius = std::max(squareRadius, FallbackSquareRadius);

    // Detach/destroy all active, attached points within the radius
    for (auto const pointIndex : mPoints)
    {
        float const pointSquareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();

        if (mPoints.IsActive(pointIndex)
            && pointSquareDistance < largerSearchSquareRadius)
        {
            //
            // - Air bubble ephemeral points: destroy
            // - Non-ephemeral, attached points: detach probabilistically
            //

            if (Points::EphemeralType::None == mPoints.GetEphemeralType(pointIndex)
                && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0)
            {
                if (pointSquareDistance < squareRadius)
                {
                    //
                    // Calculate probability: 1.0 at distance = 0.0 and 0.0 at distance = radius;
                    // however, we always destroy if we're in a very small fraction of the radius
                    //

                    float destroyProbability =
                        (pointSquareDistance < 1.0f)
                        ? 1.0f
                        : (1.0f - (pointSquareDistance / squareRadius)) * (1.0f - (pointSquareDistance / squareRadius));

                    if (GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() <= destroyProbability)
                    {
                        doDestroyPoint(pointIndex);

                        hasDestroyed = true;
                    }
                }

                if (pointSquareDistance < nearestFallbackPointRadius)
                {
                    nearestFallbackPointInRadiusIndex = pointIndex;
                    nearestFallbackPointRadius = pointSquareDistance;
                }
            }
            else if (Points::EphemeralType::AirBubble == mPoints.GetEphemeralType(pointIndex)
                && pointSquareDistance < squareRadius)
            {
                // Destroy
                mPoints.DestroyEphemeralParticle(pointIndex);

                hasDestroyed = true;
            }
        }
    }

    // Make sure we always destroy something, if we had a particle in-radius
    if (!hasDestroyed && NoneElementIndex != nearestFallbackPointInRadiusIndex)
    {
        doDestroyPoint(nearestFallbackPointInRadiusIndex);

        hasDestroyed = true;
    }

    return hasDestroyed;
}

void Ship::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    RepairSessionId sessionId,
    RepairSessionStepId sessionStepId,
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    ///////////////////////////////////////////////////////

    // Tolerance to distance: the minimum distance between the endpoint
    // of a broken spring and its target position, below which we restore
    // the spring
    //
    // Note: a higher tolerance here causes springs to...spring into life
    // already stretched or compressed, generating an undesirable force impulse
    //
    // - Shipped 1.13 with 0.07
    float constexpr DisplacementTolerance = 0.06f;

    // Tolerance to rest length vs. factory rest length, before restoring to it factory
    float constexpr RestLengthDivergenceTolerance = 0.05f;

    ///////////////////////////////////////////////////////

    float const searchRadius =
        gameParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    // Visit all non-ephemeral points
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        // Attempt to restore this point's springs if the point meets all these conditions:
        // - The point is in radius
        // - The point is not orphaned
        // - The point has not taken already the role of attracted in the last two session steps
        //
        // Note: if we were to attempt to restore also orphaned points, then two formerly-connected
        // orphaned points within the search radius would interact with each other and nullify
        // the effort put by the main structure's points

        float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (squareRadius > squareSearchRadius)
            continue;

        if (mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0
            && (mPoints.GetRepairState(pointIndex).LastAttractedSessionId != sessionId
                 || mPoints.GetRepairState(pointIndex).LastAttractedSessionStepId + 1 < sessionStepId))
        {
            // Remember that this point has taken over the role of attractor in this step
            mPoints.GetRepairState(pointIndex).LastAttractorSessionId = sessionId;
            mPoints.GetRepairState(pointIndex).LastAttractorSessionStepId = sessionStepId;

            // Calculate tool strength (1.0 at center and zero at border, fourth power)
            float const toolStrength =
                (1.0f - (squareRadius / squareSearchRadius) * (squareRadius / squareSearchRadius))
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            //
            // 1) (Attempt to) restore this point's deleted springs
            //

            // Visit all the deleted springs that were connected at factory time
            for (auto const & fcs : mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings)
            {
                bool doRecalculateSpringCoefficients = false;

                //
                // Restore the spring itself, if it's deleted
                //

                if (mSprings.IsDeleted(fcs.SpringIndex))
                {
                    auto const otherEndpointIndex = fcs.OtherEndpointIndex;

                    // Do not consider the spring if the other endpoint has already taken
                    // the role of attractor in this or in the previous step
                    if (mPoints.GetRepairState(otherEndpointIndex).LastAttractorSessionId != sessionId
                        || mPoints.GetRepairState(otherEndpointIndex).LastAttractorSessionStepId + 1 < sessionStepId)
                    {
                        //
                        // This point has taken over the role of attracted in this step
                        //

                        // Update its number of consecutive steps
                        if (mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionId == sessionId
                            && mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionStepId + 1 == sessionStepId)
                        {
                            ++mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps;
                        }
                        else if (mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionId != sessionId
                            || mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionStepId + 1 < sessionStepId)
                        {
                            mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps = 0;
                        }

                        // Remember this point has taken over the role of attracted in this step
                        mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionId = sessionId;
                        mPoints.GetRepairState(otherEndpointIndex).LastAttractedSessionStepId = sessionStepId;


                        ////////////////////////////////////////////////////////
                        //
                        // Restore this spring by moving the other endpoint nearer
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
                        // Check whether restoring this spring with the endpoint at its
                        // calculated target position would generate by mistake a CCW triangle
                        //

                        bool springWouldGenerateCCWTriangle = false;

                        for (auto const testTriangleIndex : mSprings.GetFactorySuperTriangles(fcs.SpringIndex))
                        {
                            vec2f vertexPositions[3];

                            // Edge A
                            if (mSprings.IsDeleted(mTriangles.GetSubSpringAIndex(testTriangleIndex)) && mTriangles.GetSubSpringAIndex(testTriangleIndex) != fcs.SpringIndex)
                                continue;
                            vertexPositions[0] = (mTriangles.GetPointAIndex(testTriangleIndex) == otherEndpointIndex)
                                ? targetOtherEndpointPosition
                                : mPoints.GetPosition(mTriangles.GetPointAIndex(testTriangleIndex));

                            // Edge B
                            if (mSprings.IsDeleted(mTriangles.GetSubSpringBIndex(testTriangleIndex)) && mTriangles.GetSubSpringBIndex(testTriangleIndex) != fcs.SpringIndex)
                                continue;
                            vertexPositions[1] = (mTriangles.GetPointBIndex(testTriangleIndex) == otherEndpointIndex)
                                ? targetOtherEndpointPosition
                                : mPoints.GetPosition(mTriangles.GetPointBIndex(testTriangleIndex));

                            // Edge C
                            if (mSprings.IsDeleted(mTriangles.GetSubSpringCIndex(testTriangleIndex)) && mTriangles.GetSubSpringCIndex(testTriangleIndex) != fcs.SpringIndex)
                                continue;
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
                            // Skip
                            continue;
                        }

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

                            // Smoothing of the movement, based on how long this point has been an attracted
                            // in the current session
                            float const smoothing = SmoothStep(
                                0.0f,
                                10.0f * 30.0f / gameParameters.RepairSpeedAdjustment, // Reach max in 5 seconds (at 60 fps)
                                static_cast<float>(mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps));

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
                                * smoothing
                                * toolStrength;

                            // Move point
                            mPoints.SetPosition(otherEndpointIndex,
                                (mPoints.GetPosition(otherEndpointIndex) + movementDir * movementMagnitude)
                                .clamp(-GameParameters::HalfMaxWorldWidth,
                                        GameParameters::HalfMaxWorldWidth,
                                        -GameParameters::HalfMaxWorldHeight,
                                        GameParameters::HalfMaxWorldHeight));

                            // Adjust displacement
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

                            // Brake the other endpoint
                            // Note: imparting the same velocity of the attractor to the attracted
                            // seems to make things worse
                            mPoints.SetVelocity(otherEndpointIndex, vec2f::zero());

                            // Halve the decay of both endpoints
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

                            // Remember to recalculate the spring's coefficients, now that we
                            // have changed its rest length
                            doRecalculateSpringCoefficients = true;

                            // Remember that we've acted on the other endpoint
                            hasOtherEndpointPointBeenMoved = true;
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
                else
                {
                    //
                    // (Partially) restore this spring's rest length
                    //

                    if (mSprings.GetRestLength(fcs.SpringIndex) != mSprings.GetFactoryRestLength(fcs.SpringIndex))
                    {
                        mSprings.SetRestLength(
                            fcs.SpringIndex,
                            mSprings.GetFactoryRestLength(fcs.SpringIndex)
                            + 0.97f * (mSprings.GetRestLength(fcs.SpringIndex) - mSprings.GetFactoryRestLength(fcs.SpringIndex)));

                        // Check if may fully restore
                        if (std::abs(mSprings.GetRestLength(fcs.SpringIndex) - mSprings.GetFactoryRestLength(fcs.SpringIndex)) < RestLengthDivergenceTolerance)
                        {
                            mSprings.SetRestLength(
                                fcs.SpringIndex,
                                mSprings.GetFactoryRestLength(fcs.SpringIndex));
                        }

                        // Remember to recalculate the spring's coefficients, now that we have changed its rest length
                        doRecalculateSpringCoefficients = true;
                    }
                }


                //
                // Recalculate the spring's coefficients, if we have messed with it
                //

                if (doRecalculateSpringCoefficients)
                {
                    mSprings.UpdateForRestLength(
                        fcs.SpringIndex,
                        mPoints);
                }
            }

            //
            // 2) Restore deleted _eligible_ triangles that were connected to this point at factory time
            //
            // A triangle is eligible for being restored if all of its subsprings are not deleted.
            //
            // We do this at global tool time as opposed to per-restored point, because there might
            // be triangles that have been deleted without their edge-springs having been deleted,
            // so the resurrection of triangles wouldn't be complete if we resurrected triangles
            // only when restoring a spring
            //

            // Visit all the triangles that were connected at factory time
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

                        // TODOTEST: CCW triangle detection
                        ////vec2f edges[3]{
                        ////    mPoints.GetPosition(mTriangles.GetPointBIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointAIndex(fct)),
                        ////    mPoints.GetPosition(mTriangles.GetPointCIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointBIndex(fct)),
                        ////    mPoints.GetPosition(mTriangles.GetPointAIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointCIndex(fct))
                        ////};

                        ////if (edges[0].cross(edges[1]) > 0.0f
                        ////    || edges[1].cross(edges[2]) > 0.0f
                        ////    || edges[2].cross(edges[0]) > 0.0f)
                        ////{
                        ////    LogMessage("TODOTEST2: triangle=", fct);
                        ////}
                    }
                }
            }
        }
    }
}

void Ship::SawThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Find all springs that intersect the saw segment
    //

    unsigned int metalsSawed = 0;
    unsigned int nonMetalsSawed = 0;

    for (auto springIndex : mSprings)
    {
        if (!mSprings.IsDeleted(springIndex))
        {
            if (Segment::ProperIntersectionTest(
                startPos,
                endPos,
                mSprings.GetEndpointAPosition(springIndex, mPoints),
                mSprings.GetEndpointBPosition(springIndex, mPoints)))
            {
                // Destroy spring
                mSprings.Destroy(
                    springIndex,
                    Springs::DestroyOptions::FireBreakEvent
                    | Springs::DestroyOptions::DestroyOnlyConnectedTriangle,
                    gameParameters,
                    mPoints);

                bool const isMetal =
                    mSprings.GetBaseStructuralMaterial(springIndex).MaterialSound == StructuralMaterial::MaterialSoundType::Metal;

                if (isMetal)
                {
                    // Emit sparkles
                    GenerateSparklesForCut(
                        springIndex,
                        startPos,
                        endPos,
                        currentSimulationTime,
                        gameParameters);
                }

                // Remember we have sawed this material
                if (isMetal)
                    metalsSawed++;
                else
                    nonMetalsSawed++;
            }
        }
    }

    // Notify (including zero)
    mGameEventHandler->OnSawed(true, metalsSawed);
    mGameEventHandler->OnSawed(false, nonMetalsSawed);
}

bool Ship::ApplyHeatBlasterAt(
    vec2f const & targetPos,
    HeatBlasterActionType action,
    float radius,
    GameParameters const & gameParameters)
{
    // Q = q*dt
    float const heatBlasterHeat =
        gameParameters.HeatBlasterHeatFlow * 1000.0f // KJoule->Joule
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f)
        * GameParameters::SimulationStepTimeDuration<float>
        * (action == HeatBlasterActionType::Cool ? -1.0f : 1.0f); // Heat vs. Cool

    float const squareRadius = radius * radius;

    // Search all points within the radius
    //
    // We also do ephemeral points in order to change buoyancy of air bubbles
    bool atLeastOnePointFound = false;
    for (auto const pointIndex : mPoints)
    {
        float const pointSquareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (pointSquareDistance < squareRadius
            && mPoints.IsActive(pointIndex))
        {
            //
            // Inject/remove heat at this point
            //

            // Smooth heat out for radius
            float const smoothing = 1.0f - SmoothStep(
                0.0f,
                radius,
                sqrt(pointSquareDistance));

            // Calc temperature delta
            // T = Q/HeatCapacity
            float deltaT =
                heatBlasterHeat * smoothing
                * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

            // Increase/lower temperature
            mPoints.SetTemperature(
                pointIndex,
                std::max(mPoints.GetTemperature(pointIndex) + deltaT, 0.1f)); // 3rd principle of thermodynamics

            // Remember we've found a point
            atLeastOnePointFound = true;
        }
    }

    return atLeastOnePointFound;
}

bool Ship::ExtinguishFireAt(
    vec2f const & targetPos,
    float radius,
    GameParameters const & gameParameters)
{
    float const squareRadius =
        radius * radius
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    // Search for burning points within the radius
    //
    // No real reason to ignore ephemeral points, other than they're currently
    // not expected to burn
    bool atLeastOnePointFound = false;
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        float const pointSquareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (pointSquareDistance < squareRadius)
        {
            // Check if the point is in a state in which we can smother its combustion
            if (mPoints.IsBurningForSmothering(pointIndex))
            {
                //
                // Extinguish point - fake it's with water
                //

                mPoints.SmotherCombustion(pointIndex, true);

                //
                // Also lower the point's temperature, or else it'll start burning
                // right away
                //

                float const strength = 1.0f - SmoothStep(
                    squareRadius / 2.0f,
                    squareRadius,
                    pointSquareDistance);

                mPoints.AddHeat(
                    pointIndex,
                    -950000.0f * strength);
            }

            // Remember we've found a point
            atLeastOnePointFound = true;
        }
    }

    return atLeastOnePointFound;
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strengthFraction,
    GameParameters const & gameParameters)
{
    float const strength =
        GameParameters::DrawForce
        * strengthFraction
        * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    // Apply the force field
    ApplyDrawForceField(
        targetPos,
        strength);
}

void Ship::SwirlAt(
    vec2f const & targetPos,
    float strengthFraction,
    GameParameters const & gameParameters)
{
    float const strength =
        GameParameters::SwirlForce
        * strengthFraction
        * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    // Apply the force field
    ApplySwirlForceField(
        targetPos,
        strength);
}

bool Ship::TogglePinAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mPinnedPoints.ToggleAt(
        targetPos,
        gameParameters);
}

bool Ship::InjectBubblesAt(
    vec2f const & targetPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (targetPos.y < mParentWorld.GetOceanSurfaceHeightAt(targetPos.x))
    {
        GenerateAirBubbles(
            targetPos,
            GameParameters::Temperature0,
            currentSimulationTime,
            mMaxMaxPlaneId,
            gameParameters);

        return true;
    }
    else
    {
        return false;
    }
}

bool Ship::FloodAt(
    vec2f const & targetPos,
    float waterQuantityMultiplier,
    GameParameters const & gameParameters)
{
    float const searchRadius = gameParameters.FloodRadius;

    float const quantityOfWater =
        gameParameters.FloodQuantity
        * waterQuantityMultiplier
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    //
    // Find the (non-ephemeral) non-hull points in the radius
    //

    float const searchSquareRadius = searchRadius * searchRadius;

    bool anyHasFlooded = false;
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        if (!mPoints.GetIsHull(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < searchSquareRadius)
            {
                if (quantityOfWater >= 0.0f)
                    mPoints.GetWater(pointIndex) += quantityOfWater;
                else
                    mPoints.GetWater(pointIndex) -= std::min(-quantityOfWater, mPoints.GetWater(pointIndex));

                anyHasFlooded = true;
            }
        }
    }

    return anyHasFlooded;
}

bool Ship::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mGadgets.ToggleAntiMatterBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleImpactBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mGadgets.ToggleImpactBombAt(
        targetPos,
        gameParameters);
}

std::optional<bool> Ship::TogglePhysicsProbeAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mGadgets.TogglePhysicsProbeAt(
        targetPos,
        gameParameters);
}

void Ship::RemovePhysicsProbe()
{
    return mGadgets.RemovePhysicsProbe();
}

bool Ship::ToggleRCBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mGadgets.ToggleRCBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleTimerBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mGadgets.ToggleTimerBombAt(
        targetPos,
        gameParameters);
}

void Ship::DetonateRCBombs()
{
    mGadgets.DetonateRCBombs();
}

void Ship::DetonateAntiMatterBombs()
{
    mGadgets.DetonateAntiMatterBombs();
}

bool Ship::ScrubThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    float const scrubRadius = gameParameters.ScrubRotRadius;

    //
    // Find all points in the radius of the segment
    //

    // Calculate normal to the segment (doesn't really matter which orientation)
    vec2f normalizedSegment = (endPos - startPos).normalise();
    vec2f segmentNormal = vec2f(-normalizedSegment.y, normalizedSegment.x);

    // Calculate bounding box for segment *and* search radius
    Geometry::AABB boundingBox(
        std::min(startPos.x, endPos.x) - scrubRadius,   // Left
        std::max(startPos.x, endPos.x) + scrubRadius,   // Right
        std::max(startPos.y, endPos.y) + scrubRadius,   // Top
        std::min(startPos.y, endPos.y) - scrubRadius);  // Bottom

    // Visit all points (excluding ephemerals, they don't rot and
    // thus we don't need to scrub them!)
    bool hasScrubbed = false;
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        auto const & pointPosition = mPoints.GetPosition(pointIndex);

        // First check whether the point is in the bounding box
        if (boundingBox.Contains(pointPosition))
        {
            // Distance = projection of (start->point) vector on segment normal
            float const distance = std::abs((pointPosition - startPos).dot(segmentNormal));

            // Check whether this point is in the radius
            if (distance <= scrubRadius)
            {
                //
                // Scrub this point, with magnitude dependent from distance
                //

                float const newDecay =
                    mPoints.GetDecay(pointIndex)
                    + 0.5f * (1.0f - mPoints.GetDecay(pointIndex)) * (scrubRadius - distance) / scrubRadius;

                mPoints.SetDecay(pointIndex, newDecay);

                // Remember at least one point has been scrubbed
                hasScrubbed |= true;
            }
        }
    }

    if (hasScrubbed)
    {
        // Make sure the decay buffer gets uploaded again
        mPoints.MarkDecayBufferAsDirty();
    }

    return hasScrubbed;
}

bool Ship::RotThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    float const rotRadius = gameParameters.ScrubRotRadius; // Yes, using the same for symmetry

    //
    // Find all points in the radius of the segment
    //

    // Calculate normal to the segment (doesn't really matter which orientation)
    vec2f normalizedSegment = (endPos - startPos).normalise();
    vec2f segmentNormal = vec2f(-normalizedSegment.y, normalizedSegment.x);

    // Calculate bounding box for segment *and* search radius
    Geometry::AABB boundingBox(
        std::min(startPos.x, endPos.x) - rotRadius,   // Left
        std::max(startPos.x, endPos.x) + rotRadius,   // Right
        std::max(startPos.y, endPos.y) + rotRadius,   // Top
        std::min(startPos.y, endPos.y) - rotRadius);  // Bottom

    // Visit all points (excluding ephemerals, they don't rot and
    // thus we don't need to rot them!)
    bool hasRotted = false;
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        auto const & pointPosition = mPoints.GetPosition(pointIndex);

        // First check whether the point is in the bounding box
        if (boundingBox.Contains(pointPosition))
        {
            // Distance = projection of (start->point) vector on segment normal
            float const distance = std::abs((pointPosition - startPos).dot(segmentNormal));

            // Check whether this point is in the radius
            if (distance <= rotRadius)
            {
                //
                // Rot this point, with magnitude dependent from distance,
                // and more pronounced when the point is underwater or has water
                //

                float const decayCoeff = (mParentWorld.IsUnderwater(pointPosition) || mPoints.GetWater(pointIndex) >= 1.0f)
                    ? 0.0175f
                    : 0.010f;

                float const newDecay =
                    mPoints.GetDecay(pointIndex)
                    * (1.0f - decayCoeff * (rotRadius - distance) / rotRadius);

                mPoints.SetDecay(pointIndex, newDecay);

                // Remember at least one point has been rotted
                hasRotted |= true;
            }
        }
    }

    if (hasRotted)
    {
        // Make sure the decay buffer gets uploaded again
        mPoints.MarkDecayBufferAsDirty();
    }

    return hasRotted;
}

void Ship::ApplyThanosSnap(
    float centerX,
    float /*radius*/,
    float leftFrontX,
    float rightFrontX,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Calculate direction
    float leftX, rightX;
    float direction;
    if (rightFrontX <= centerX)
    {
        // Left wave front

        assert(leftFrontX < centerX);
        leftX = leftFrontX;
        rightX = centerX;
        direction = -1.0f;
    }
    else
    {
        // Right wave front

        assert(leftFrontX >= centerX);
        leftX = centerX;
        rightX = rightFrontX;
        direction = 1.0f;
    }

    // Visit all points (excluding ephemerals, there's nothing to detach there)
    bool atLeastOneDetached = false;
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        auto const x = mPoints.GetPosition(pointIndex).x;
        if (leftX <= x
            && x <= rightX
            && !mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty())
        {
            //
            // Detach this point
            //

            // Choose a detach velocity
            vec2f detachVelocity = vec2f(
                direction * GameRandomEngine::GetInstance().GenerateUniformReal(35.0f, 60.0f),
                GameRandomEngine::GetInstance().GenerateUniformReal(-3.0f, 18.0f));

            // Detach
            mPoints.Detach(
                pointIndex,
                mPoints.GetVelocity(pointIndex) + detachVelocity,
                Points::DetachOptions::None,
                currentSimulationTime,
                gameParameters);

            // Set decay to min, so that debris gets darkened
            mPoints.SetDecay(pointIndex, 0.0f);

            atLeastOneDetached = true;
        }
    }

    if (atLeastOneDetached)
    {
        // We've changed the decay buffer, need to upload it next then!
        mPoints.MarkDecayBufferAsDirty();
    }
}

ElementIndex Ship::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const pointIndex : mPoints)
    {
        if (mPoints.IsActive(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointIndex;
}

bool Ship::QueryNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    //
    // Find point
    //

    bool pointWasFound = false;

    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const pointIndex : mPoints)
    {
        if (mPoints.IsActive(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    if (NoneElementIndex != bestPointIndex)
    {
        mPoints.Query(bestPointIndex);
        pointWasFound = true;
    }


    //
    // Find triangle enclosing target - if any
    //

    ElementIndex enclosingTriangleIndex = NoneElementIndex;
    for (auto const triangleIndex : mTriangles)
    {
        if ((mPoints.GetPosition(mTriangles.GetPointBIndex(triangleIndex)) - mPoints.GetPosition(mTriangles.GetPointAIndex(triangleIndex)))
            .cross(targetPos - mPoints.GetPosition(mTriangles.GetPointAIndex(triangleIndex))) < 0
            &&
            (mPoints.GetPosition(mTriangles.GetPointCIndex(triangleIndex)) - mPoints.GetPosition(mTriangles.GetPointBIndex(triangleIndex)))
            .cross(targetPos - mPoints.GetPosition(mTriangles.GetPointBIndex(triangleIndex))) < 0
            &&
            (mPoints.GetPosition(mTriangles.GetPointAIndex(triangleIndex)) - mPoints.GetPosition(mTriangles.GetPointCIndex(triangleIndex)))
            .cross(targetPos - mPoints.GetPosition(mTriangles.GetPointCIndex(triangleIndex))) < 0)
        {
            enclosingTriangleIndex = triangleIndex;
            break;
        }
    }

    if (NoneElementIndex != enclosingTriangleIndex)
    {
        LogMessage("TriangleIndex: ", enclosingTriangleIndex);
    }

    return pointWasFound;
}

std::optional<vec2f> Ship::FindSuitableLightningTarget() const
{
    //
    // Find top N points
    //

    constexpr size_t MaxCandidates = 4;

    // Sorted by y, largest first
    std::vector<vec2f> candidatePositions;

    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        // Non-deleted, non-orphaned point
        if (mPoints.IsActive(pointIndex)
            && !mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty())
        {
            auto const & pos = mPoints.GetPosition(pointIndex);

            if (!mParentWorld.IsUnderwater(pos))
            {
                candidatePositions.insert(
                    std::upper_bound(
                        candidatePositions.begin(),
                        candidatePositions.end(),
                        pos,
                        [](auto const & candidatePos, auto const & pos)
                        {
                            // Height of to-be-inserted point is augmented based on distance from the point
                            float const distance = (candidatePos - pos).length();
                            float const actualPosY = pos.y + std::max(std::floor(distance / 3.0f), 5.0f);
                            return candidatePos.y > actualPosY;
                        }),
                    pos);

                if (candidatePositions.size() > MaxCandidates)
                {
                    candidatePositions.pop_back();
                    assert(candidatePositions.size() == MaxCandidates);
                }
            }
        }
    }

    if (candidatePositions.empty())
        return std::nullopt;

    //
    // Choose
    //

    return candidatePositions[GameRandomEngine::GetInstance().Choose(candidatePositions.size())];
}

void Ship::ApplyLightning(
    vec2f const & targetPos,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float const searchRadius =
        gameParameters.LightningBlastRadius
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    // Note: we don't consider the simulation dt here as the lightning touch-down
    // happens in one frame only, rather than being splattered across multiple frames
    float const lightningHeat =
        gameParameters.LightningBlastHeat * 1000.0f // KJoule->Joule
        * (gameParameters.IsUltraViolentMode ? 8.0f : 1.0f);

    //
    // Find the (non-ephemeral) points in the radius
    //

    float const searchSquareRadius = searchRadius * searchRadius;
    float const searchSquareRadiusBlast = searchSquareRadius / 2.0f;
    float const searchSquareRadiusHeat = searchSquareRadius;

    for (auto const pointIndex : mPoints.RawShipPoints())
    {
        float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();

        bool wasDestroyed = false;

        if (squareDistance < searchSquareRadiusBlast)
        {
            //
            // Calculate destroy probability: 1.0 at distance = 0.0 and 0.0 at distance = radius;
            // however, we always destroy if we're in a very small fraction of the radius
            //

            float destroyProbability =
                (searchSquareRadiusBlast < 1.0f)
                ? 1.0f
                : (1.0f - (squareDistance / searchSquareRadiusBlast)) * (1.0f - (squareDistance / searchSquareRadiusBlast));

            if (GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() <= destroyProbability)
            {
                //
                // Destroy
                //

                // Choose a detach velocity - using the same distribution as Debris
                vec2f detachVelocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
                    GameParameters::MinDebrisParticlesVelocity,
                    GameParameters::MaxDebrisParticlesVelocity);

                // Detach
                mPoints.Detach(
                    pointIndex,
                    detachVelocity,
                    Points::DetachOptions::GenerateDebris,
                    currentSimulationTime,
                    gameParameters);

                // Generate sparkles
                GenerateSparklesForLightning(
                    pointIndex,
                    currentSimulationTime,
                    gameParameters);

                // Notify
                mGameEventHandler->OnLightningHit(mPoints.GetStructuralMaterial(pointIndex));

                wasDestroyed = true;
            }
        }

        if (!wasDestroyed
            && squareDistance < searchSquareRadiusHeat)
        {
            //
            // Apply heat
            //

            // Smooth heat out for radius
            float const smoothing = 1.0f - SmoothStep(
                searchSquareRadiusHeat * 3.0f / 4.0f,
                searchSquareRadiusHeat,
                squareDistance);

            // Calc temperature delta
            // T = Q/HeatCapacity
            float deltaT =
                lightningHeat * smoothing
                * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

            // Increase/lower temperature
            mPoints.SetTemperature(
                pointIndex,
                std::max(mPoints.GetTemperature(pointIndex) + deltaT, 0.1f)); // 3rd principle of thermodynamics
        }
    }
}

void Ship::HighlightElectricalElement(ElectricalElementId electricalElementId)
{
    assert(electricalElementId.GetShipId() == mId);

    mElectricalElements.HighlightElectricalElement(
        electricalElementId,
        mPoints);
}

void Ship::SetSwitchState(
    ElectricalElementId electricalElementId,
    ElectricalState switchState,
    GameParameters const & gameParameters)
{
    assert(electricalElementId.GetShipId() == mId);

    mElectricalElements.SetSwitchState(
        electricalElementId,
        switchState,
        mPoints,
        gameParameters);
}

void Ship::SetEngineControllerState(
    ElectricalElementId electricalElementId,
    int telegraphValue,
    GameParameters const & gameParameters)
{
    assert(electricalElementId.GetShipId() == mId);

    mElectricalElements.SetEngineControllerState(
        electricalElementId,
        telegraphValue,
        gameParameters);
}

bool Ship::DestroyTriangle(ElementIndex triangleIndex)
{
    if (triangleIndex < mTriangles.GetElementCount()
        && !mTriangles.IsDeleted(triangleIndex))
    {
        mTriangles.Destroy(triangleIndex);
        return true;
    }
    else
    {
        return false;
    }
}

bool Ship::RestoreTriangle(ElementIndex triangleIndex)
{
    if (triangleIndex < mTriangles.GetElementCount()
        && mTriangles.IsDeleted(triangleIndex))
    {
        mTriangles.Restore(triangleIndex);
        return true;
    }
    else
    {
        return false;
    }
}

}