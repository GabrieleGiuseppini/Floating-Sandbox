/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameMath.h>

#include <algorithm>

namespace Physics {

void Ship::ApplyDrawForceField(
    vec2f const & centerPosition,
    float strength)
{
    //
    // F = ForceStrength/sqrt(distance), along radius
    //

    for (auto pointIndex : mPoints)
    {
        vec2f displacement = (centerPosition - mPoints.GetPosition(pointIndex));
        float forceMagnitude = strength / sqrtf(0.1f + displacement.length());

        mPoints.GetNonSpringForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

void Ship::ApplySwirlForceField(
    vec2f const & centerPosition,
    float strength)
{
    //
    // F = ForceStrength*radius/sqrt(distance), perpendicular to radius
    //

    for (auto pointIndex : mPoints)
    {
        vec2f displacement = (centerPosition - mPoints.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        float forceMagnitude = strength / sqrtf(0.1f + displacementLength);

        mPoints.GetNonSpringForce(pointIndex) += vec2f(-displacement.y, displacement.x) * forceMagnitude;
    }
}

void Ship::ApplyBlastForceField(
    vec2f const & centerPosition,
    float blastRadius,
    float blastForce,
    bool doDetachPoint,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Go through all points and, for each point in radius:
    // - Keep non-ephemeral point that is closest to blast position; we'll Detach() it later
    //   (if this is the fist frame of the blast sequence)
    // - Flip over the point outside of the radius
    //

    float const squareBlastRadius = blastRadius * blastRadius;

    float closestPointSquareDistance = std::numeric_limits<float>::max();
    ElementIndex closestPointIndex = NoneElementIndex;

    // Visit all points
    for (auto pointIndex : mPoints)
    {
        vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
        float const squarePointDistance = pointRadius.squareLength();
        if (squarePointDistance < squareBlastRadius)
        {
            // Apply blast force (inversely proportional to distance,
            // not second power as one would expect though)
            float const pointRadiusLength = std::sqrt(squarePointDistance);
            mPoints.GetNonSpringForce(pointIndex) +=
                pointRadius.normalise(pointRadiusLength)
                / std::max(pointRadiusLength, 1.0f)
                * blastForce;

            // Check whether this point is the closest point
            if (squarePointDistance < closestPointSquareDistance)
            {
                closestPointSquareDistance = squarePointDistance;
                closestPointIndex = pointIndex;
            }
        }
    }


    //
    // Eventually detach the closest point
    //

    if (doDetachPoint
        && NoneElementIndex != closestPointIndex)
    {
        // Choose a detach velocity - using the same distribution as Debris
        vec2f const detachVelocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
            GameParameters::MinDebrisParticlesVelocity,
            GameParameters::MaxDebrisParticlesVelocity);

        // Detach point
        mPoints.Detach(
            closestPointIndex,
            detachVelocity,
            Points::DetachOptions::GenerateDebris
            | Points::DetachOptions::FireDestroyEvent,
            currentSimulationTime,
            gameParameters);
    }
}

void Ship::ApplyRadialSpaceWarpForceField(
    vec2f const & centerPosition,
    float radius,
    float radiusThickness,
    float strength)
{
    for (auto pointIndex : mPoints)
    {
        vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
        float const pointDistanceFromRadius = pointRadius.length() - radius;
        float const absolutePointDistanceFromRadius = std::abs(pointDistanceFromRadius);
        if (absolutePointDistanceFromRadius <= radiusThickness)
        {
            float const forceDirection = pointDistanceFromRadius >= 0.0f ? 1.0f : -1.0f;

            float const forceStrength = strength * (1.0f - absolutePointDistanceFromRadius / radiusThickness);

            mPoints.GetNonSpringForce(pointIndex) +=
                pointRadius.normalise()
                * forceStrength
                * forceDirection;
        }
    }
}

void Ship::ApplyImplosionForceField(
    vec2f const & centerPosition,
    float strength)
{
    for (auto pointIndex : mPoints)
    {
        vec2f displacement = (centerPosition - mPoints.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        vec2f normalizedDisplacement = displacement.normalise(displacementLength);

        // Make final acceleration somewhat independent from mass
        float const massNormalization = mPoints.GetMass(pointIndex) / 50.0f;

        // Angular (constant)
        mPoints.GetNonSpringForce(pointIndex) +=
            vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
            * strength
            * massNormalization
            / 10.0f; // Magic number

        // Radial (stronger when closer)
        mPoints.GetNonSpringForce(pointIndex) +=
            normalizedDisplacement
            * strength
            / (0.2f + sqrt(displacementLength))
            * massNormalization
            * 10.0f; // Magic number
    }
}

void Ship::ApplyRadialExplosionForceField(
    vec2f const & centerPosition,
    float strength)
{
    //
    // F = ForceStrength/sqrt(distance), along radius
    //

    for (auto pointIndex : mPoints)
    {
        vec2f displacement = (mPoints.GetPosition(pointIndex) - centerPosition);
        float forceMagnitude = strength / sqrtf(0.1f + displacement.length());

        mPoints.GetNonSpringForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

}