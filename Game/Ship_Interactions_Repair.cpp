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

/*
void Ship::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    RepairSessionId sessionId,
    RepairSessionStepId sessionStepId,
    float currentSimulationTime,
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

    // Tolerance to rest length vs. factory rest length, before restoring it to factory
    float constexpr RestLengthDivergenceTolerance = 0.05f;

    ///////////////////////////////////////////////////////

    float const searchRadius =
        gameParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    // TODOTEST
    mDebugMarker.ClearPointToPointArrows();

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
                        ////// TODOTEST: attractor must have more springs attached than attractee
                        ////if (!(
                        ////    mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() >=
                        ////    mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size())
                        ////    && mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings.size() > mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size()
                        ////    && mPoints.GetFactoryConnectedSprings(otherEndpointIndex).ConnectedSprings.size() > mPoints.GetConnectedSprings(otherEndpointIndex).ConnectedSprings.size())
                        ////    continue;


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

                        // TODOTEST
                        mDebugMarker.AddPointToPointArrow(
                            mPoints.GetPlaneId(otherEndpointIndex),
                            mPoints.GetPosition(otherEndpointIndex),
                            targetOtherEndpointPosition,
                            rgbColor(250, 40, 40));

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

                            // TODOTEST: impatience check
                            ////if (mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps > 100)
                            ////{
                            ////    LogMessage("TODOHERE: impatient");

                            ////    mPoints.Detach(
                            ////        otherEndpointIndex,
                            ////        mPoints.GetVelocity(otherEndpointIndex),
                            ////        Points::DetachOptions::None,
                            ////        currentSimulationTime,
                            ////        gameParameters);

                            ////    mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps = 0;
                            ////}

                            ////// TODOTEST: connected component check
                            ////if (mPoints.GetConnectedComponentId(otherEndpointIndex) != mPoints.GetConnectedComponentId(pointIndex)
                            ////    && mPoints.GetRepairState(otherEndpointIndex).CurrentAttractedNumberOfSteps > 120) // TODOHERE: it seems we don't reset it to zero when the guy is not attracted
                            ////{
                            ////    mPoints.Detach(
                            ////        otherEndpointIndex,
                            ////        mPoints.GetVelocity(otherEndpointIndex),
                            ////        Points::DetachOptions::None,
                            ////        currentSimulationTime,
                            ////        gameParameters);
                            ////}

                            // Smoothing of the movement, based on how long this point has been an attracted
                            // in the current session
                            float const smoothing = LinearStep(
                                0.0f,
                                600.0f / gameParameters.RepairSpeedAdjustment, // Reach max in 10 seconds (at 60 fps)
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
*/

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

    // TODOTEST
    mDebugMarker.ClearPointToPointArrows();

    // TODOTEST
    LogMessage("TODOTEST: ---------------------------------------------");
    LogMessage("TODOTEST: ", repairStepId);

    //
    // Pass 1: visit first all points that had been attractors in the previous step
    //
    // This is to prevent attractors and attractees from flipping roles during a session;
    // an attractor will continue to be an attractor until it needs reparation
    //

    // Visit all (in-radius) non-ephemeral points that had been attractors in the previous step
    auto const previousStep = repairStepId.Previous();
    for (auto const pointIndex : mPoints.RawShipPoints())
    {
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

    //
    // Pass 2: visit all other points now, to give a chance to everyone else to be
    // an attractor
    //

    // Visit all (in-radius) non-ephemeral points
    for (auto const pointIndex : mPoints.RawShipPoints())
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

                        // TODOTEST: CCW triangle detection
                        vec2f edges[3]{
                            mPoints.GetPosition(mTriangles.GetPointBIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointAIndex(fct)),
                            mPoints.GetPosition(mTriangles.GetPointCIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointBIndex(fct)),
                            mPoints.GetPosition(mTriangles.GetPointAIndex(fct)) - mPoints.GetPosition(mTriangles.GetPointCIndex(fct))
                        };

                        if (edges[0].cross(edges[1]) > 0.0f
                            || edges[1].cross(edges[2]) > 0.0f
                            || edges[2].cross(edges[0]) > 0.0f)
                        {
                            LogMessage("TODOTEST2: CCW triangle restored!!! TriangleIndex=", fct);
                        }
                    }
                }
            }

            // b) Visit all springs, trying to restore their rest lenghts
            for (auto const cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
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
    //  - and has not been an attractor in this step
    //  - and has not been an attractee in this step
    //  - and needs reparation
    //  - and is not orphaned (to avoid orphan-oprhan attractions)
    //
    // After being an attractor, do a breadth-first visit from the point propagating
    // repair from directly-connected candidates

    std::deque<ElementIndex> pointsToVisit;

    for (ElementIndex pointIndex = startingPointIndex; ;)
    {
        //
        // Check if this point meets all the conditions
        //

        if (float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            squareRadius <= squareSearchRadius
            && mPoints.GetRepairState(pointIndex).LastAttractorRepairStepId != repairStepId
            && mPoints.GetRepairState(pointIndex).LastAttracteeRepairStepId != repairStepId
            && mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings.size() > mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size()
            && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0)
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

            //
            // Propagate to all of the immediately-connected points
            //

            for (auto const cs : mPoints.GetConnectedSprings(pointIndex).ConnectedSprings)
            {
                pointsToVisit.push_back(cs.OtherEndpointIndex);
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

    // TODOTEST
    LogMessage("TODOTEST: RepairFromPoint: ", pointIndex);

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
            bool doRecalculateSpringCoefficients = false;

            auto const otherEndpointIndex = fcs.OtherEndpointIndex;

            // Do not consider the spring if the other endpoint has already taken
            // the role of attractor in this step
            if (mPoints.GetRepairState(otherEndpointIndex).LastAttractorRepairStepId != repairStepId)
            {
                //
                // This point has taken over the role of attractee in this step
                //

                // Update its count of consecutive steps as an attractee
                if (mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId == repairStepId.Previous())
                    mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps++;
                else
                    mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps = 1;

                // Remember the role
                mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId = repairStepId;

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

                // TODOTEST
                mDebugMarker.AddPointToPointArrow(
                    mPoints.GetPlaneId(otherEndpointIndex),
                    mPoints.GetPosition(otherEndpointIndex),
                    targetOtherEndpointPosition,
                    rgbColor(250, 40, 40));

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
                    float const smoothing = LinearStep(
                        0.0f,
                        (6.0f * 64.0f) / gameParameters.RepairSpeedAdjustment, // Reach max in 6 seconds (at 64 fps)
                        static_cast<float>(mPoints.GetRepairState(otherEndpointIndex).CurrentAttracteeConsecutiveNumberOfSteps));

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
                        * repairStrength;

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

                    // Forget that the other endpoint has been an attractee in this step
                    mPoints.GetRepairState(otherEndpointIndex).LastAttracteeRepairStepId = SequenceNumber::None();

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
    }

    return hasAnySpringBeenRepaired;
}

}