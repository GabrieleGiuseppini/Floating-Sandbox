/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameMath.h>

#include <algorithm>

namespace Physics {

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

            mPoints.AddStaticForce(
                pointIndex,
                pointRadius.normalise() * forceStrength * forceDirection);
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
        mPoints.AddStaticForce(
            pointIndex,
            vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
                * strength
                * massNormalization
                / 10.0f); // Magic number

        // Radial (stronger when closer)
        mPoints.AddStaticForce(
            pointIndex,
            normalizedDisplacement
                * strength
                / (0.2f + 0.5f * sqrt(displacementLength))
                * massNormalization
                * 10.0f); // Magic number
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

        mPoints.AddStaticForce(
            pointIndex,
            displacement.normalise() * forceMagnitude);
    }
}

}