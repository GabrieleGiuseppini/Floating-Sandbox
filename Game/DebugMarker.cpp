/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-02-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "DebugMarker.h"

namespace Physics {

void DebugMarker::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    for (auto const & p : mPointToPointArrows)
    {
        shipRenderContext.UploadPointToPointArrow(
            p.Plane,
            p.StartPoint,
            p.EndPoint,
            p.Color);
    }
}

void DebugMarker::ClearPointToPointArrows()
{
    mPointToPointArrows.clear();
}

}