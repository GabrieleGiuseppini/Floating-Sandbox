/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <algorithm>

namespace Physics {

void DrawForceField::Apply(
    Points & points,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/) const
{
    //
    // F = ForceStrength/sqrt(distance), along radius
    //

    for (auto pointIndex : points)
    {
        vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
        float forceMagnitude = mStrength / sqrtf(0.1f + displacement.length());

        points.GetForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

void SwirlForceField::Apply(
    Points & points,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/) const
{
    //
    // F = ForceStrength*radius/sqrt(distance), perpendicular to radius
    //

    for (auto pointIndex : points)
    {
        vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        float forceMagnitude = mStrength / sqrtf(0.1f + displacementLength);

        points.GetForce(pointIndex) += vec2f(-displacement.y, displacement.x) * forceMagnitude;
    }
}

void BlastForceField::Apply(
    Points & points,
    float currentSimulationTime,
    GameParameters const & gameParameters) const
{
    //
    // Go through all points and, for each point in radius:
    // - Keep non-ephemeral point that is closest to blast position; we'll Detach() it later
    //   (if this is the fist frame of the blast sequence)
    // - Flip over the point outside of the radius
    //

    float const squareBlastRadius = mBlastRadius * mBlastRadius;
    constexpr float DtSquared = GameParameters::SimulationStepTimeDuration<float> * GameParameters::SimulationStepTimeDuration<float>;

    float closestPointSquareDistance = std::numeric_limits<float>::max();
    ElementIndex closestPointIndex = NoneElementIndex;

    // Visit all (non-ephemeral) points (ephemerals would be blown immediately away otherwise)
    for (auto pointIndex : points.NonEphemeralPoints())
    {
        vec2f pointRadius = points.GetPosition(pointIndex) - mCenterPosition;
        float squarePointDistance = pointRadius.squareLength();
        if (squarePointDistance < squareBlastRadius)
        {
            // Check whether this point is the closest point
            if (squarePointDistance < closestPointSquareDistance)
            {
                closestPointSquareDistance = squarePointDistance;
                closestPointIndex = pointIndex;
            }

            // Create acceleration to flip the point
            vec2f flippedRadius = pointRadius.normalise() * (mBlastRadius + (mBlastRadius - pointRadius.length()));
            vec2f newPosition = mCenterPosition + flippedRadius;
            points.GetForce(pointIndex) +=
                (newPosition - points.GetPosition(pointIndex))
                / DtSquared
                * mStrength
                * points.GetMass(pointIndex);
        }
    }


    //
    // Eventually detach the closest point
    //

    if (mDetachPoint
        && NoneElementIndex != closestPointIndex)
    {
        // Choose a detach velocity - using the same distribution as Debris
        vec2f detachVelocity = GameRandomEngine::GetInstance().GenerateRandomRadialVector(
            GameParameters::MinDebrisParticlesVelocity,
            GameParameters::MaxDebrisParticlesVelocity);

        // Detach point
        points.Detach(
            closestPointIndex,
            detachVelocity,
            Points::DetachOptions::GenerateDebris
            | Points::DetachOptions::FireDestroyEvent,
            currentSimulationTime,
            gameParameters);
    }
}

void RadialSpaceWarpForceField::Apply(
    Points & points,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/) const
{
    for (auto pointIndex : points)
    {
        vec2f const pointRadius = points.GetPosition(pointIndex) - mCenterPosition;
        float const pointDistanceFromRadius = pointRadius.length() - mRadius;
        float const absolutePointDistanceFromRadius = std::abs(pointDistanceFromRadius);
        if (absolutePointDistanceFromRadius <= mRadiusThickness)
        {
            float const direction = pointDistanceFromRadius >= 0.0f ? 1.0f : -1.0f;

            float const strength = mStrength * (1.0f - absolutePointDistanceFromRadius / mRadiusThickness);

            points.GetForce(pointIndex) +=
                pointRadius.normalise()
                * strength
                * direction;
        }
    }
}

void ImplosionForceField::Apply(
    Points & points,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/) const
{
    for (auto pointIndex : points)
    {
        vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        vec2f normalizedDisplacement = displacement.normalise(displacementLength);

        // Make final acceleration somewhat independent from mass
        float const massNormalization = points.GetMass(pointIndex) / 50.0f;

        // Angular (constant)
        points.GetForce(pointIndex) +=
            vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
            * mStrength
            * massNormalization
            / 10.0f; // Magic number

        // Radial (stronger when closer)
        points.GetForce(pointIndex) +=
            normalizedDisplacement
            * mStrength
            / (0.2f + sqrt(displacementLength))
            * massNormalization
            * 10.0f; // Magic number
    }
}

void RadialExplosionForceField::Apply(
    Points & points,
    float /*currentSimulationTime*/,
    GameParameters const & /*gameParameters*/) const
{
    //
    // F = ForceStrength/sqrt(distance), along radius
    //

    for (auto pointIndex : points)
    {
        vec2f displacement = (points.GetPosition(pointIndex) - mCenterPosition);
        float forceMagnitude = mStrength / sqrtf(0.1f + displacement.length());

        points.GetForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

}