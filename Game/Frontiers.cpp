/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-09-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <algorithm>
#include <array>

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
}

void Frontiers::AddFrontier(
    FrontierType type,
    std::vector<ElementIndex> edgeIndices,
    Springs const & springs)
{
    assert(edgeIndices.size() > 0);

    ElementIndex const firstEdgeIndex = edgeIndices[0];

    // Add frontier head
    mFrontiers.emplace_back(
        type,
        firstEdgeIndex,
        static_cast<ElementIndex>(edgeIndices.size()));

    /* TODOHERE: this is broken
    // Concatenate all edges
    ElementIndex previousEdgeIndex = firstEdgeIndex;
    for (size_t e = 1; e < edgeIndices.size(); ++e)
    {
        ElementIndex const edgeIndex = edgeIndices[e];

        mFrontierEdges[e - 1].NextEdgeIndex = edgeIndex;

        // Point 1 of this edge is the endpoint that is common with the previous edge
        ElementIndex point1Index =
            (springs.GetEndpointAIndex(edgeIndex) == springs.GetEndpointAIndex(previousEdgeIndex)
                || springs.GetEndpointAIndex(edgeIndex) == springs.GetEndpointBIndex(previousEdgeIndex))
            ? springs.GetEndpointAIndex(edgeIndex)
            : springs.GetEndpointBIndex(edgeIndex);


        // TODOHERE
    }

    // Last
    mFrontierEdges[edgeIndices[edgeIndices.size() - 1]].NextEdgeIndex = firstEdgeIndex;
    */
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

        size_t const totalSize = std::accumulate(
            mFrontiers.cbegin(),
            mFrontiers.cend(),
            size_t(0),
            [](size_t total, auto const & f)
            {
                return total + f.Size;
            });

        renderContext.UploadShipElementFrontierEdgesStart(shipId, totalSize);

        for (auto const & frontier : mFrontiers)
        {
            assert(frontier.Size > 0);

            ElementIndex const startingEdgeIndex = frontier.StartingEdgeIndex;
            ElementIndex edgeIndex = startingEdgeIndex;

            do
            {
                // Upload
                renderContext.UploadShipElementFrontierEdge(
                    shipId,
                    mFrontierEdges[edgeIndex].PointAIndex,
                    mFrontierEdges[edgeIndex].PointBIndex);

                // Advance
                edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;

            } while (edgeIndex != startingEdgeIndex);
        }

        renderContext.UploadShipElementFrontierEdgesEnd(shipId);

        mIsDirtyForRendering = false;
    }
}

void Frontiers::RegeneratePointColors() const
{
    std::array<rgbColor, 4> const ExternalColors
    {
        rgbColor(0, 153, 0),
        rgbColor(0, 51, 204),
        rgbColor(51, 153, 51),
        rgbColor(0, 0, 204)
    };

    std::array<rgbColor, 4> const InternalColors
    {
        rgbColor(204, 51, 0),
        rgbColor(255, 204, 0),
        rgbColor(255, 0, 0),
        rgbColor(255, 255, 0)
    };

    size_t externalUsed = 0;
    size_t internalUsed = 0;

    for (auto const & frontier : mFrontiers)
    {
        vec3f const baseColor = (frontier.Type == FrontierType::External)
            ? ExternalColors[(externalUsed++) % ExternalColors.size()].toVec3f()
            : InternalColors[(internalUsed++) % InternalColors.size()].toVec3f();

        ElementIndex const startingEdgeIndex = frontier.StartingEdgeIndex;
        ElementIndex edgeIndex = startingEdgeIndex;


        float const positionalProgressDx = 1.0f / static_cast<float>(frontier.Size);
        float positionalProgress = positionalProgressDx;

        do
        {
            mPointColors[mFrontierEdges[edgeIndex].PointAIndex].frontierBaseColor = baseColor;
            mPointColors[mFrontierEdges[edgeIndex].PointAIndex].positionalProgress = positionalProgress;

            positionalProgress += positionalProgressDx;

            // Advance
            edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;

        } while (edgeIndex != startingEdgeIndex);
    }
}

}