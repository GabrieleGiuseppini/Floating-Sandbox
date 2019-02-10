/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Triangles::Add(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    ElementIndex pointCIndex,
    SubSpringsVector const & subSprings)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex, pointCIndex);

    mSubSpringsBuffer.emplace_back(subSprings);
}

void Triangles::Destroy(ElementIndex triangleElementIndex)
{
    assert(triangleElementIndex < mElementCount);
    assert(!IsDeleted(triangleElementIndex));

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(triangleElementIndex);
    }

    // Flag ourselves as deleted
    mIsDeletedBuffer[triangleElementIndex] = true;
}

void Triangles::UploadElements(
    ShipId shipId,
    Render::RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            // TODO: this will go
            assert(points.GetPlaneId(GetPointAIndex(i)) == points.GetPlaneId(GetPointBIndex(i))
                && points.GetPlaneId(GetPointAIndex(i)) == points.GetPlaneId(GetPointCIndex(i)));

            renderContext.UploadShipElementTriangle(
                shipId,
                GetPointAIndex(i),
                GetPointBIndex(i),
                GetPointCIndex(i),
                points.GetPlaneId(GetPointAIndex(i)));
        }
    }
}

}