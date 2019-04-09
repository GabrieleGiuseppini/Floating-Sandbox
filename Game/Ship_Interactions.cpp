/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"


#include <GameCore/AABB.h>
#include <GameCore/GameDebug.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/Log.h>
#include <GameCore/Segment.h>

#include <algorithm>
#include <cassert>
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

void Ship::MoveBy(
    vec2f const & offset,
    GameParameters const & gameParameters)
{
    vec2f const velocity =
        offset
        * gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    size_t const count = mPoints.GetBufferElementCount();
    for (size_t p = 0; p < count; ++p)
    {
        positionBuffer[p] += offset;
        velocityBuffer[p] = velocity;
    }
}

void Ship::RotateBy(
    float angle,
    vec2f const & center,
    GameParameters const & gameParameters)
{
    float const inertia =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    vec2f * restrict positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f * restrict velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    size_t const count = mPoints.GetBufferElementCount();
    for (size_t p = 0; p < count; ++p)
    {
        vec2f pos = positionBuffer[p] - center;
        pos = vec2f(pos.dot(rotX), pos.dot(rotY)) + center;

        velocityBuffer[p] = (pos - positionBuffer[p]) * inertia;
        positionBuffer[p] = pos;
    }
}

void Ship::DestroyAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float const radius =
        gameParameters.DestroyRadius
        * radiusMultiplier
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    float const squareRadius = radius * radius;

    // Detach/destroy all active, attached points within the radius
    for (auto pointIndex : mPoints)
    {
        if (mPoints.IsActive(pointIndex)
            && (mPoints.GetPosition(pointIndex) - targetPos).squareLength() < squareRadius)
        {
            //
            // - Air bubble ephemeral points: destroy
            // - Non-ephemeral, attached points: detach
            //

            if (Points::EphemeralType::None == mPoints.GetEphemeralType(pointIndex)
                && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0)
            {
                // Choose a detach velocity - using the same distribution as Debris
                vec2f detachVelocity = GameRandomEngine::GetInstance().GenerateRandomRadialVector(
                    GameParameters::MinDebrisParticlesVelocity,
                    GameParameters::MaxDebrisParticlesVelocity);

                // Detach
                mPoints.Detach(
                    pointIndex,
                    detachVelocity,
                    Points::DetachOptions::GenerateDebris,
                    currentSimulationTime,
                    gameParameters);
            }
            else if (Points::EphemeralType::AirBubble == mPoints.GetEphemeralType(pointIndex))
            {
                // Destroy
                mPoints.DestroyEphemeralParticle(pointIndex);
            }
        }
    }
}

void Ship::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    float const searchRadius =
        gameParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    // Visit all non-ephemeral points
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        // Check if point is in radius and whether it's not orphaned
        //
        // If we were to attempt to restore also orphaned points, then two formerly-connected
        // orphaned points within the search radius would interact with each other and nullify
        // the effort put by the main structure's points
        float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (squareRadius <= squareSearchRadius
            && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0)
        {
            //
            // 1) (Attempt to) restore this point's delete springs
            //

            // Calculate tool strength (1.0 at center and zero at border, fourth power)
            float const toolStrength =
                (1.0f - (squareRadius / squareSearchRadius) * (squareRadius / squareSearchRadius))
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Visit all the springs that were connected at factory time
            for (auto const & fcs : mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings)
            {
                // Check if this spring is deleted
                if (mSprings.IsDeleted(fcs.SpringIndex))
                {
                    ////////////////////////////////////////////////////////
                    //
                    // Restore this spring or move the other endpoint nearer
                    //
                    ////////////////////////////////////////////////////////

                    //
                    // The target position of the endpoint is on the circle whose radius
                    // is the spring's rest length, and the angle is interpolated between
                    // the two non-deleted springs immediately CW and CCW of this spring
                    //

                    float targetWorldAngle; // In world coordinates, positive when CCW, 0 at E

                    // The angle of the spring wrt this point
                    // 0 = E, 1 = SE, ..., 7 = NE
                    int32_t const factoryPointSpringOctant = mSprings.GetFactoryEndpointOctant(
                        fcs.SpringIndex,
                        pointIndex);

                    size_t const connectedSpringsCount = mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size();
                    if (connectedSpringsCount == 0)
                    {
                        // No springs exist yet for this point...
                        // ...arbitrarily, use its factory octant as if it were a world angle
                        targetWorldAngle = Pi<float> / 4.0f * static_cast<float>(8.0f - factoryPointSpringOctant);
                    }
                    else
                    {
                        // One or more springs...

                        //
                        // 1. Find nearest spring CW and closest spring CCW
                        // (which might and up being the same spring in case there's only one spring)
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

                        // Angle between this two springs
                        float neighborsAngle =
                            (ccwSpringOtherEndpointIndex == cwSpringOtherEndpointIndex)
                            ? 2.0f * Pi<float>
                            : (mPoints.GetPosition(ccwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex))
                              .angle(mPoints.GetPosition(cwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex));

                        if (neighborsAngle < 0.0f)
                            neighborsAngle += 2.0f * Pi<float>;

                        // Interpolated angle from CW spring
                        float const interpolatedAngleFromCWSpring =
                            neighborsAngle
                            / static_cast<float>(nearestCWSpringDeltaOctant + nearestCCWSpringDeltaOctant)
                            * static_cast<float>(nearestCWSpringDeltaOctant);

                        // And finally, the target world angle (world angle is 0 at E)
                        targetWorldAngle =
                            (mPoints.GetPosition(cwSpringOtherEndpointIndex) - mPoints.GetPosition(pointIndex)).angle(vec2f(1.0f, 0.0f))
                            + interpolatedAngleFromCWSpring;
                    }

                    // Calculate target position
                    vec2f const targetOtherEndpointPosition =
                        mPoints.GetPosition(pointIndex)
                        + vec2f::fromPolar(
                            mSprings.GetRestLength(fcs.SpringIndex),
                            targetWorldAngle);


                    //
                    // Check progress of other endpoint towards the target position
                    //

                    // Displacement vector (positive towards target point)
                    vec2f const displacementVector = targetOtherEndpointPosition - mPoints.GetPosition(fcs.OtherEndpointIndex);

                    // Distance
                    float displacementMagnitude = displacementVector.length();

                    // Tolerance to distance
                    //
                    // Note: a higher tolerance here causes springs to...spring into life
                    // already stretches or compressed, generating an undesirable force impuls
                    float constexpr DisplacementTolerance = 0.025f;

                    // Check whether we are still further away than our tolerance,
                    // and whether this point is free to move
                    if (displacementMagnitude > DisplacementTolerance
                        && !mPoints.IsPinned(fcs.OtherEndpointIndex))
                    {
                        //
                        // Endpoints are too far...
                        // ...move them closer by moving the other endpoint towards its target position
                        //

                        // Fraction of the movement that we want to do in this step
                        // (which is once per SimulationStep)
                        //
                        // A higher value destroys the other point's springs too quickly;
                        // a lower value makes the other point follow a moving point forever
                        float constexpr MovementFraction =
                            4.0f // We want a point to cover the whole distance in 1/4th of a simulated second
                            * GameParameters::SimulationStepTimeDuration<float>;

                        // Movement direction (positive towards this point)
                        vec2f const movementDir = displacementVector.normalise(displacementMagnitude);

                        // Movement magnitude
                        //
                        // Note: here we calculate the movement based on the static positions
                        // of the two endpoints; however, if the two endpoints have a non-zero
                        // relative velocity, then this movement won't achieve the desired effect
                        // (it will undershoot or overshoot). I do think the end result is cool
                        // though, as you end up, for example, with points chasing a part of a ship
                        // that's moving away!
                        float const movementMagnitude =
                            displacementMagnitude
                            * MovementFraction
                            * toolStrength;

                        // Move point
                        mPoints.GetPosition(fcs.OtherEndpointIndex) +=
                            movementDir
                            * movementMagnitude;

                        // Adjust displacement
                        assert(movementMagnitude < displacementMagnitude);
                        displacementMagnitude -= movementMagnitude;

                        // Impart some non-linear inertia (smaller at higher displacements),
                        // just for better looks
                        // (note: last one that pulls this point wins)
                        mPoints.GetVelocity(fcs.OtherEndpointIndex) =
                            movementDir
                            * ((movementMagnitude < 0.0f) ? -1.0f : 1.0f)
                            * sqrtf(abs(movementMagnitude))
                            / GameParameters::GameParameters::SimulationStepTimeDuration<float>
                            * 0.5f;
                    }

                    // Check whether we are now close enough
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
                        mPoints.SetVelocity(fcs.OtherEndpointIndex, vec2f::zero());
                    }
                }
            }

            //
            // 2) Restore deleted _eligible_ triangles that were connected to this point at factory time
            //
            // A triangle is eligible for being restored if all of its subsprings are not deleted.
            //
            // We do this at tool time as there are triangles that have been deleted
			// without their edge-springs having been deleted, so the resurrection of triangles
			// wouldn't be complete if we resurrected triangles only when restoring a spring
            //

            // Visit all the triangles that were connected at factory time
            for (auto fct : mPoints.GetFactoryConnectedTriangles(pointIndex).ConnectedTriangles)
            {
                if (mTriangles.IsDeleted(fct))
                {
                    // Check if eligible
                    bool hasDeletedSubsprings = false;
                    for (auto fss : mTriangles.GetFactorySubSprings(fct))
                        hasDeletedSubsprings |= mSprings.IsDeleted(fss);

                    if (!hasDeletedSubsprings)
                    {
                        // Restore it
                        mTriangles.Restore(fct);
                    }
                }
            }


            //
            // 3) Restore eligible endpoints' IsLeaking
            //
            // Eligible endpoints are those that now have all of their factory springs
            //

            if (mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size()
                == mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings.size())
            {
                mPoints.RestoreFactoryIsLeaking(pointIndex);
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
            if (Geometry::Segment::ProperIntersectionTest(
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
                    GenerateSparkles(
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

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new DrawForceField(
            targetPos,
            strength * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f)));
}

void Ship::SwirlAt(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    // Store the force field
    mCurrentForceFields.emplace_back(
        new SwirlForceField(
            targetPos,
            strength * (gameParameters.IsUltraViolentMode ? 40.0f : 1.0f)));
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
    if (targetPos.y < mParentWorld.GetWaterHeightAt(targetPos.x))
    {
        GenerateAirBubbles(
            targetPos,
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
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        if (!mPoints.IsHull(pointIndex))
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
    return mBombs.ToggleAntiMatterBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleImpactBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleImpactBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleRCBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleRCBombAt(
        targetPos,
        gameParameters);
}

bool Ship::ToggleTimerBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    return mBombs.ToggleTimerBombAt(
        targetPos,
        gameParameters);
}

void Ship::DetonateRCBombs()
{
    mBombs.DetonateRCBombs();
}

void Ship::DetonateAntiMatterBombs()
{
    mBombs.DetonateAntiMatterBombs();
}

bool Ship::ScrubThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    float const scrubRadius = gameParameters.ScrubRadius;

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

    // Visit all points (excluding ephemerals, we don't want to scrub air bubbles)
    bool hasScrubbed = false;
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        auto const & pointPosition = mPoints.GetPosition(pointIndex);

        // First check whether the point is in the bounding box
        if (boundingBox.Contains(pointPosition))
        {
            // Distance = projection of (start->point) vector on segment normal
            float const distance = abs((pointPosition - startPos).dot(segmentNormal));

            // Check whether this point is in the radius
            if (distance <= scrubRadius)
            {
                //
                // Scrub this point, with magnitude dependent from distance
                //

                float newDecay =
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

ElementIndex Ship::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
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
    float const squareRadius = radius * radius;

    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
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
        return true;
    }

    return false;
}

}