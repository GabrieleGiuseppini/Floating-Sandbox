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

std::optional<ElementIndex> Ship::Pick(
    vec2f const & pickPosition,
    GameParameters const & gameParameters)
{
    //
    // Find closest non-ephemeral, non-orphaned point within the radius
    //

    float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

    float bestSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestPoint = NoneElementIndex;

    for (auto p : mPoints.NonEphemeralPoints())
    {
        if (!mPoints.GetConnectedSprings(p).ConnectedSprings.empty())
        {
            float const squareDistance = (mPoints.GetPosition(p) - pickPosition).squareLength();
            if (squareDistance < squareSearchRadius
                && squareDistance < bestSquareDistance)
            {
                bestSquareDistance = squareDistance;
                bestPoint = p;
            }
        }
    }

    if (bestPoint != NoneElementIndex)
        return bestPoint;
    else
        return std::nullopt;
}

void Ship::MoveBy(
    ElementIndex pointElementIndex,
    vec2f const & offset,
    GameParameters const & gameParameters)
{
    vec2f const velocity =
        offset
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
                mPoints.SetVelocity(p, velocity);
            }
        }

        TrimForWorldBounds(gameParameters);
    }
}

void Ship::MoveAllBy(
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

    TrimForWorldBounds(gameParameters);
}

void Ship::RotateBy(
    ElementIndex pointElementIndex,
    float angle,
    vec2f const & center,
    GameParameters const & gameParameters)
{
    float const inertia =
        gameParameters.MoveToolInertia
        * (gameParameters.IsUltraViolentMode ? 5.0f : 1.0f);

    vec2f const rotX(cos(angle), sin(angle));
    vec2f const rotY(-sin(angle), cos(angle));

    // Get connected component ID of the point
    auto connectedComponentId = mPoints.GetConnectedComponentId(pointElementIndex);
    if (connectedComponentId != NoneConnectedComponentId)
    {
        // Rotate all points (ephemeral and non-ephemeral) that belong to the same connected component
        for (auto p : mPoints)
        {
            if (mPoints.GetConnectedComponentId(p) == connectedComponentId)
            {
                vec2f pos = mPoints.GetPosition(p) - center;
                pos = vec2f(pos.dot(rotX), pos.dot(rotY)) + center;

                mPoints.SetVelocity(p, (pos - mPoints.GetPosition(p)) * inertia);
                mPoints.GetPosition(p) = pos;
            }
        }

        TrimForWorldBounds(gameParameters);
    }
}

void Ship::RotateAllBy(
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

    TrimForWorldBounds(gameParameters);
}

void Ship::DestroyAt(
    vec2f const & targetPos,
    float radiusFraction,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Destroy points probabilistically - probability is one at
    // distance = 0 and zero at distance = radius
    //

    float const radius =
        gameParameters.DestroyRadius
        * radiusFraction
        * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

    float const squareRadius = radius * radius;

    // Detach/destroy all active, attached points within the radius
    for (auto pointIndex : mPoints)
    {
        float const pointSquareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (mPoints.IsActive(pointIndex)
            && pointSquareDistance < squareRadius)
        {
            //
            // - Air bubble ephemeral points: destroy
            // - Non-ephemeral, attached points: detach probabilistically
            //

            if (Points::EphemeralType::None == mPoints.GetEphemeralType(pointIndex)
                && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0)
            {
                //
                // Calculate probability: 1.0 at distance = 0.0 and 0.0 at distance = radius;
                // however, we always destroy if we're in a very small fraction of the radius
                //

                float destroyProbability =
                    (squareRadius < 1.0f)
                    ? 1.0f
                    : (1.0f - (pointSquareDistance / squareRadius)) * (1.0f - (pointSquareDistance / squareRadius));

                if (GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() <= destroyProbability)
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
    RepairSessionId sessionId,
    RepairStepId stepId,
    float /*currentSimulationTime*/,
    GameParameters const & gameParameters)
{
    ///////////////////////////////////////////////////////

    // Rate at which the "other" endpoint of a spring accelerates towards the velocity
    // required for repairing
    float constexpr SmoothingAlpha = 0.01f;

    // Tolerance to distance
    //
    // Note: a higher tolerance here causes springs to...spring into life
    // already stretched or compressed, generating an undesirable force impulse
    float constexpr DisplacementTolerance = 0.1f;

    ///////////////////////////////////////////////////////

    float const searchRadius =
        gameParameters.RepairRadius
        * radiusMultiplier;

    float const squareSearchRadius = searchRadius * searchRadius;

    // Visit all non-ephemeral points
    for (auto pointIndex : mPoints.NonEphemeralPoints())
    {
        // Attempt to restore this point's springs if the point meets all these conditions:
        // - The point is in radius
        // - The point is not orphaned
        // - The point has not been acted upon already as an attracted in this step
        //
        // If we were to attempt to restore also orphaned points, then two formerly-connected
        // orphaned points within the search radius would interact with each other and nullify
        // the effort put by the main structure's points
        float const squareRadius = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
        if (squareRadius <= squareSearchRadius
            && mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.size() > 0
            && mPoints.GetRepairState(pointIndex).AttractedStepId != stepId)
        {
            // Remember that this point has taken the role of attractor in this step
            mPoints.GetRepairState(pointIndex).AttractorStepId = stepId;

            //
            // 1) (Attempt to) restore this point's delete springs
            //

            // Calculate tool strength (1.0 at center and zero at border, fourth power)
            float const toolStrength =
                (1.0f - (squareRadius / squareSearchRadius) * (squareRadius / squareSearchRadius))
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Visit all the deleted springs that were connected at factory time
            for (auto const & fcs : mPoints.GetFactoryConnectedSprings(pointIndex).ConnectedSprings)
            {
                auto const otherEndpointIndex = fcs.OtherEndpointIndex;

                // Consider this spring (and thus the other endpoint) iff:
                // - The spring is deleted, AND
                // - The other endpoint is out-of-range of the tool, OR it is in-range but not used
                //   already as attractor in this step
                bool considerSpring = mSprings.IsDeleted(fcs.SpringIndex);
                if (considerSpring)
                {
                    float otherEndpointSquareRadius = (mPoints.GetPosition(otherEndpointIndex) - targetPos).squareLength();
                    if (otherEndpointSquareRadius < squareSearchRadius
                        && mPoints.GetRepairState(otherEndpointIndex).AttractorStepId == stepId)
                    {
                        considerSpring = false;
                    }
                }

                if (considerSpring)
                {
                    ////////////////////////////////////////////////////////
                    //
                    // Restore this spring moving the other endpoint nearer
                    //
                    ////////////////////////////////////////////////////////

                    // Remember this point is being used as attracted in this session
                    mPoints.GetRepairState(otherEndpointIndex).AttractedStepId == stepId;

                    //
                    // Advance the other endpoint's repair smoothing
                    //

                    if (mPoints.GetRepairState(otherEndpointIndex).SmoothingSessionId != sessionId)
                    {
                        // First time this point is repaired in this session...
                        // ...make its smoothing start from zero
                        mPoints.GetRepairState(otherEndpointIndex).SmoothingSessionId = sessionId;
                        mPoints.GetRepairState(otherEndpointIndex).Smoothing = 0.0f;
                    }

                    if (mPoints.GetRepairState(otherEndpointIndex).SmoothingStepId != stepId)
                    {
                        // First time this point is considered in this session step...
                        // ...advance its smoothing
                        mPoints.GetRepairState(otherEndpointIndex).SmoothingStepId = stepId;
                        mPoints.GetRepairState(otherEndpointIndex).Smoothing +=
                            (1.0f - mPoints.GetRepairState(otherEndpointIndex).Smoothing)
                            * SmoothingAlpha;
                    }

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

                        // Fraction of the movement that we want to do in this step
                        // (which is once per SimulationStep)
                        //
                        // A higher value destroys the other point's (which might be already repaired) springs too quickly;
                        // a lower value makes the other point follow a moving point forever
                        float constexpr MovementFraction =
                            4.0f // We want a point to cover the whole distance in 1/4th of a simulated second
                            * GameParameters::SimulationStepTimeDuration<float>;

                        // Movement direction (positive towards this point)
                        vec2f const movementDir = displacementVector.normalise(displacementMagnitude);

                        // Movement magnitude
                        //
                        // The magnitude is multiplied with the point's repair smoothing, which goes
                        // from 0.0 at the moment the point is first engaged, to 1.0 later on
                        //
                        // Note: here we calculate the movement based on the static positions
                        // of the two endpoints; however, if the two endpoints have a non-zero
                        // relative velocity, then this movement won't achieve the desired effect
                        // (it will undershoot or overshoot). I do think the end result is cool
                        // though, as you end up, for example, with points chasing a part of a ship
                        // that's moving away!
                        float const movementMagnitude =
                            pow(displacementMagnitude, gameParameters.RepairStrengthAdjustment)
                            * MovementFraction
                            * toolStrength
                            * mPoints.GetRepairState(otherEndpointIndex).Smoothing;

                        // Move point
                        mPoints.GetPosition(otherEndpointIndex) +=
                            movementDir
                            * movementMagnitude;

                        // Adjust displacement
                        assert(movementMagnitude < displacementMagnitude);
                        displacementMagnitude -= movementMagnitude;

                        // Impart some non-linear inertia (smaller at higher displacements),
                        // retaining a bit of the previous velocity
                        // (note: last one that pulls this point wins)
                        auto const displacementVelocity =
                            movementDir
                            * ((movementMagnitude < 0.0f) ? -1.0f : 1.0f)
                            * pow(abs(movementMagnitude), 0.2f)
                            / GameParameters::SimulationStepTimeDuration<float>
                            * 0.5f;
                        mPoints.GetVelocity(otherEndpointIndex) =
                            (mPoints.GetVelocity(otherEndpointIndex) * 0.35f)
                            + (displacementVelocity * 0.65f);

                        // Remember that we've acted on the other endpoint
                        hasOtherEndpointPointBeenMoved = true;
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
                        mPoints.SetVelocity(otherEndpointIndex, vec2f::zero());

                        // Remember that we've acted on the other endpoint
                        hasOtherEndpointPointBeenMoved = true;
                    }


                    //
                    // Dry the ohter endpoint, if we've messed with it
                    //

                    if (hasOtherEndpointPointBeenMoved)
                    {
                        mPoints.GetWater(otherEndpointIndex) /= 2.0f;
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
    float strengthFraction,
    GameParameters const & gameParameters)
{
    float const strength =
        GameParameters::DrawForce
        * strengthFraction
        * (gameParameters.IsUltraViolentMode ? 20.0f : 1.0f);

    // Store the force field
    AddOrResetForceField<DrawForceField>(
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

    // Store the force field
    AddOrResetForceField<SwirlForceField>(
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