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

    //
    // Add frontier head
    //

    mFrontiers.emplace_back(
        type,
        edgeIndices[0],
        static_cast<ElementIndex>(edgeIndices.size()));

    //
    // Concatenate all edges
    //

    // First off, find point in common between last and first edge; this point
    // will be the first point of the first edge

    ElementIndex previousEdgeIndex = edgeIndices[edgeIndices.size() - 1];

    ElementIndex edgeIndex = edgeIndices[0];

    ElementIndex point1Index =
        (springs.GetEndpointAIndex(edgeIndex) == springs.GetEndpointAIndex(previousEdgeIndex)
            || springs.GetEndpointAIndex(edgeIndex) == springs.GetEndpointBIndex(previousEdgeIndex))
        ? springs.GetEndpointAIndex(edgeIndex)
        : springs.GetEndpointBIndex(edgeIndex);

    for (size_t e = 0; e < edgeIndices.size(); ++e)
    {
        edgeIndex = edgeIndices[e];

        // point1Index is in common between these two edges
        assert((springs.GetEndpointAIndex(previousEdgeIndex) == point1Index || springs.GetEndpointBIndex(previousEdgeIndex) == point1Index)
            && (springs.GetEndpointAIndex(edgeIndex) == point1Index || springs.GetEndpointBIndex(edgeIndex) == point1Index));

        // Set point indices
        mFrontierEdges[previousEdgeIndex].PointBIndex = point1Index;
        mFrontierEdges[edgeIndex].PointAIndex = point1Index;

        // Concatenate edges
        mFrontierEdges[previousEdgeIndex].NextEdgeIndex = edgeIndex;

        // Advance
        previousEdgeIndex = edgeIndex;
    }

    // Concatenate last
    mFrontierEdges[edgeIndices[edgeIndices.size() - 2]].NextEdgeIndex = edgeIndices[edgeIndices.size() - 1];
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

            positionalProgress += std::min(positionalProgressDx, 1.0f);

            // Advance
            edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;

        } while (edgeIndex != startingEdgeIndex);
    }
}

}