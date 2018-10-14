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
    ElementIndex pointCIndex)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex, pointCIndex);
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
    int shipId,
    Render::RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i))
                && points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointCIndex(i)));

            renderContext.UploadShipElementTriangle(
                shipId,
                GetPointAIndex(i),
                GetPointBIndex(i),
                GetPointCIndex(i),
                points.GetConnectedComponentId(GetPointAIndex(i)));
        }
    }
}

}