/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-02-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipOverlays.h"

#include <cassert>

namespace Physics {

void ShipOverlays::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext)
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);
    auto const & viewModel = renderContext.GetViewModel();

    if (mIsCentersBufferDirty)
    {
        shipRenderContext.UploadCentersStart(mCenters.size());

        for (auto const & c : mCenters)
        {
            shipRenderContext.UploadCenter(
                c.Plane,
                c.Position,
                viewModel);
        }

        shipRenderContext.UploadCentersEnd();

        if (!mCenters.empty())
        {
            // Reset now for next time
            mCenters.clear();

            // We stay dirty so next time we'll upload emptyness
            assert(mIsCentersBufferDirty);
        }
        else
        {
            mIsCentersBufferDirty = false;
        }
    }

    if (mIsPointToPointArrowsBufferDirty)
    {
        shipRenderContext.UploadPointToPointArrowsStart(mPointToPointArrows.size());

        for (auto const & p : mPointToPointArrows)
        {
            shipRenderContext.UploadPointToPointArrow(
                p.Plane,
                p.StartPoint,
                p.EndPoint,
                p.Color);
        }

        shipRenderContext.UploadPointToPointArrowsEnd();

        if (!mPointToPointArrows.empty())
        {
            // Reset now for next time
            mPointToPointArrows.clear();

            // We stay dirty so next time we'll upload emptyness
            assert(mIsPointToPointArrowsBufferDirty);
        }
        else
        {
            mIsPointToPointArrowsBufferDirty = false;
        }
    }
}

}