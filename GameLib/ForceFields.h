/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "Physics.h"
#include "Vectors.h"

namespace Physics
{	

/*
 * This class represents an abstract force field that works on points.
 */
class ForceField
{
public:

    virtual ~ForceField()
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const = 0;
};

/*
 * A radial force field that attracts all points to a center point.
 */
class DrawForceField final : public ForceField
{
public:

    DrawForceField(
        vec2f const & centerPosition,
        float strength)
        : mCenterPosition(centerPosition)
        , mStrength(strength)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mStrength;
};

/*
 * Angular force field that rotates all points around a center point.
 */
class SwirlForceField final : public ForceField
{
public:

    SwirlForceField(
        vec2f const & centerPosition,
        float strength)
        : mCenterPosition(centerPosition)
        , mStrength(strength)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mStrength;
};

/*
 * Force field that simulates a blast around a center point.
 */
class BlastForceField final : public ForceField
{
public:

    BlastForceField(
        vec2f const & centerPosition,
        float blastRadius,
        float strength,
        ConnectedComponentId connectedComponentId,
        bool destroyPoint)
        : mCenterPosition(centerPosition)
        , mBlastRadius(blastRadius)
        , mStrength(strength)
        , mConnectedComponentId(connectedComponentId)
        , mDestroyPoint(destroyPoint)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mBlastRadius;
    float const mStrength;
    ConnectedComponentId const mConnectedComponentId;
    bool const mDestroyPoint;
};

/*
 * Force field that simulates a space warp along a circle around a center point.
 */
class RadialSpaceWarpForceField final : public ForceField
{
public:

    RadialSpaceWarpForceField(
        vec2f const & centerPosition,
        float radius,
        float radiusThickness,
        float strength)
        : mCenterPosition(centerPosition)
        , mRadius(radius)
        , mRadiusThickness(radiusThickness)
        , mStrength(strength)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mRadius;
    float const mRadiusThickness;
    float const mStrength;
};

/*
 * Force field that simulates a both angular and radial force sucking in all points towards
 * a center point.
 */
class ImplosionForceField final : public ForceField
{
public:

    ImplosionForceField(
        vec2f const & centerPosition,
        float strength)
        : mCenterPosition(centerPosition)        
        , mStrength(strength)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mStrength;
};

/*
 * Force field that simulates a radial explosion from a center point.
 */
class RadialExplosionForceField final : public ForceField
{
public:

    RadialExplosionForceField(
        vec2f const & centerPosition,
        float strength)
        : mCenterPosition(centerPosition)
        , mStrength(strength)
    {}

    virtual void Apply(
        Points & points,
        float currentSimulationTime,
        GameParameters const & gameParameters) const override;

private:

    vec2f const mCenterPosition;
    float const mStrength;
};

}
