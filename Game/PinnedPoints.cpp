/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void PinnedPoints::OnPointDestroyed(ElementIndex pointElementIndex)
{
    //
    // If the point is pinned, unpin it
    //

    auto it = std::find(mCurrentPinnedPoints.begin(), mCurrentPinnedPoints.end(), pointElementIndex);
    if (it != mCurrentPinnedPoints.end())
    {
        // Unpin it
        assert(mShipPoints.IsPinned(*it));
        mShipPoints.Unpin(*it);

        // Remove from stack
        mCurrentPinnedPoints.erase(it);
    }
}

void PinnedPoints::OnSpringDestroyed(ElementIndex springElementIndex)
{
    //
    // If an endpoint was pinned and it has now lost all of its springs, then make
    // it unpinned
    //

    auto const pointAIndex = mShipSprings.GetPointAIndex(springElementIndex);
    auto const pointBIndex = mShipSprings.GetPointBIndex(springElementIndex);

    if (mShipPoints.IsPinned(pointAIndex)
        && mShipPoints.GetConnectedSprings(pointAIndex).empty())
    {
        // Unpin it
        mShipPoints.Unpin(pointAIndex);

        // Remove from set of pinned points
        mCurrentPinnedPoints.erase(pointAIndex);
    }

    if (mShipPoints.IsPinned(pointBIndex)
        && mShipPoints.GetConnectedSprings(pointBIndex).empty())
    {
        // Unpin it
        mShipPoints.Unpin(pointBIndex);

        // Remove from set of pinned points
        mCurrentPinnedPoints.erase(pointBIndex);
    }
}

void PinnedPoints::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (auto pinnedPointIndex : mCurrentPinnedPoints)
    {
        assert(!mShipPoints.IsDeleted(pinnedPointIndex));
        assert(mShipPoints.IsPinned(pinnedPointIndex));

        renderContext.UploadShipGenericTextureRenderSpecification(
            shipId,
            mShipPoints.GetPlaneId(pinnedPointIndex),
            TextureFrameId(TextureGroupType::PinnedPoint, 0),
            mShipPoints.GetPosition(pinnedPointIndex));
    }
}

}