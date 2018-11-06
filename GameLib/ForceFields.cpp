/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void DrawForceField::Apply(Points & points) const
{
    for (auto pointIndex : points)
    {
        // F = ForceStrength/sqrt(distance), along radius
        vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
        float forceMagnitude = mStrength / sqrtf(0.1f + displacement.length());

        points.GetForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

void SwirlForceField::Apply(Points & points) const
{
    for (auto pointIndex : points)
    {
        // F = ForceStrength/sqrt(distance), perpendicular to radius
        vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
        float const displacementLength = displacement.length();
        float forceMagnitude = mStrength / sqrtf(0.1f + displacementLength);

        points.GetForce(pointIndex) += vec2f(-displacement.y, displacement.x) * forceMagnitude;
    }
}

void BlastForceField::Apply(Points & points) const
{
    for (auto pointIndex : points)
    {
        // TODO
    }
}

void RadialSpaceWarpForceField::Apply(Points & points) const
{
    for (auto pointIndex : points)
    {
        if (!points.IsDeleted(pointIndex))
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
}

void ImplosionForceField::Apply(Points & points) const
{
    for (auto pointIndex : points)
    {
        if (!points.IsDeleted(pointIndex))
        {            
            vec2f displacement = (mCenterPosition - points.GetPosition(pointIndex));
            float const displacementLength = displacement.length();
            vec2f normalizedDisplacement = displacement.normalise(displacementLength);

            // Angular - constant
            points.GetForce(pointIndex) +=
                vec2f(-normalizedDisplacement.y, normalizedDisplacement.x)
                * mStrength
                / 2.0f
                ;

            // Radial - stronger when closer
            points.GetForce(pointIndex) += 
                normalizedDisplacement 
                * mStrength
                / (1.0f + sqrt(displacementLength))
                * 10.0f
                ;
        }
    }
}

}