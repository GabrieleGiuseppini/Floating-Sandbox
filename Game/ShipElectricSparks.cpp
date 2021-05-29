/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-05-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cassert>

namespace Physics {

ShipElectricSparks::ShipElectricSparks(Springs const & springs)
    : mSpringCount(springs.GetElementCount())
    , mIsArcElectrified(mSpringCount, 0, false)
    , mIsArcElectrifiedBackup(mSpringCount, 0, false)
    , mAreSparksPopulatedBeforeNextUpdate(false)
    , mSparksToRender()
{
}

bool ShipElectricSparks::ApplySparkAt(
    vec2f const & targetPos,
    std::uint64_t counter,
    float progress,
    Points const & points,
    Springs const & springs,
    GameParameters const & gameParameters)
{
    //
    // Find closest point, and check whether there _is_ actually a closest point
    //

    float nearestDistance = 1.5f;
    ElementIndex nearestPointIndex = NoneElementIndex;

    for (auto pointIndex : points.RawShipPoints()) // No point in visiting ephemeral points
    {
        vec2f const pointRadius = points.GetPosition(pointIndex) - targetPos;
        float const squarePointDistance = pointRadius.squareLength();
        if (squarePointDistance < nearestDistance)
        {
            nearestDistance = squarePointDistance;
            nearestPointIndex = pointIndex;
        }
    }

    if (nearestPointIndex != NoneElementIndex)
    {
        PropagateSparks(
            nearestPointIndex,
            counter,
            progress,
            points,
            springs);

        return true;
    }
    else
    {
        // No luck
        return false;
    }
}

void ShipElectricSparks::Update()
{
    if (!mAreSparksPopulatedBeforeNextUpdate)
    {
        mSparksToRender.clear();
    }

    mAreSparksPopulatedBeforeNextUpdate = false;
}

void ShipElectricSparks::Upload(
    Points const & points,
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    shipRenderContext.UploadElectricSparksStart(mSparksToRender.size());

    for (auto const & electricSpark : mSparksToRender)
    {
        shipRenderContext.UploadElectricSpark(
            points.GetPlaneId(electricSpark.StartPointIndex),
            points.GetPosition(electricSpark.StartPointIndex),
            electricSpark.StartSize,
            points.GetPosition(electricSpark.EndPointIndex),
            electricSpark.EndSize);
    }

    shipRenderContext.UploadElectricSparksEnd();
}

void ShipElectricSparks::PropagateSparks(
    ElementIndex startingPointIndex,
    std::uint64_t counter,
    float progress,
    // TODO: see if both needed
    Points const & points,
    Springs const & springs)
{
    //
    // Clear the sparks to render in the next step
    //

    mSparksToRender.clear();


    // TODOTEST
    if (points.GetConnectedSprings(startingPointIndex).ConnectedSprings.size() > 0)
    {
        auto const spring1 = points.GetConnectedSprings(startingPointIndex).ConnectedSprings[0].SpringIndex;
        auto const point2 = points.GetConnectedSprings(startingPointIndex).ConnectedSprings[0].OtherEndpointIndex;

        mSparksToRender.emplace_back(
            startingPointIndex,
            1.0f,
            point2,
            1.0f);

        if (points.GetConnectedSprings(point2).ConnectedSprings.size() > 1)
        {
            auto const spring2 =
                points.GetConnectedSprings(point2).ConnectedSprings[0].SpringIndex != spring1
                ? points.GetConnectedSprings(point2).ConnectedSprings[0].SpringIndex
                : points.GetConnectedSprings(point2).ConnectedSprings[1].SpringIndex;

            mSparksToRender.emplace_back(
                point2,
                1.0f,
                springs.GetOtherEndpointIndex(spring2, point2),
                0.2f);
        }
    }

    //
    // Remember that we have populated electric sparks
    //

    mAreSparksPopulatedBeforeNextUpdate = true;
}

}