/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void PinnedPoints::OnEphemeralParticleDestroyed(ElementIndex pointElementIndex)
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

void PinnedPoints::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (auto pinnedPointIndex : mCurrentPinnedPoints)
    {
        assert(mShipPoints.IsPinned(pinnedPointIndex));

        renderContext.UploadShipGenericTextureRenderSpecification(
            shipId,
            mShipPoints.GetPlaneId(pinnedPointIndex),
            TextureFrameId(GenericTextureGroups::PinnedPoint, 0),
            mShipPoints.GetPosition(pinnedPointIndex));
    }
}

}