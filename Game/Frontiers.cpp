/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-09-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

Frontiers::Frontiers(
    size_t pointCount,
    Springs const & springs,
    Triangles const & triangles)
    : mFrontierEdges(springs.GetElementCount())
    , mFrontiers()
    , mPointColors(pointCount)
    , mIsDirtyForRendering(true)
{
    // TODO
}

void Frontiers::AddFrontier(
    FrontierType type,
    std::vector<ElementIndex> edgeIndices)
{
    // TODO
}

void Frontiers::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    if (renderContext.GetShowFrontiers()
        && mIsDirtyForRendering)
    {
        //
        // Upload frontier point colors
        //

        // Generate point colors
        RegeneratePointColors();

        // Upload point colors
        renderContext.UploadShipPointFrontierColors(shipId, mPointColors.data());

        //
        // Upload frontier point indices
        //

        // TODO: precalc size?
        renderContext.UploadShipElementFrontierEdgesStart(shipId);

        for (auto const & frontier : mFrontiers)
        {
            // TODO
        }

        renderContext.UploadShipElementFrontierEdgesEnd(shipId);

        mIsDirtyForRendering = false;
    }
}

void Frontiers::RegeneratePointColors() const
{
    // TODO
}

}