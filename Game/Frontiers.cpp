/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-09-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameDebug.h>

#include <algorithm>
#include <array>
#include <set>

namespace Physics {

Frontiers::Frontiers(
    size_t pointCount,
    Springs const & springs,
    Triangles const & triangles)
    : mEdgeCount(springs.GetElementCount())
    , mEdges(mEdgeCount, 0, Edge())
    , mFrontierEdges(mEdgeCount, 0, FrontierEdge())
    , mFrontiers()
    , mPointColors(pointCount, 0, Render::FrontierColor(vec3f::zero(), 0.0f))
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

    FrontierId const frontierIndex = static_cast<FrontierId>(mFrontiers.size());

    mFrontiers.emplace_back(
        Frontier(
            type,
            edgeIndices[0],
            static_cast<ElementIndex>(edgeIndices.size())));

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

        // Edge
        mEdges[edgeIndex].FrontierIndex = frontierIndex;

        // FrontierEdge

        // Set point indices
        mFrontierEdges[previousEdgeIndex].PointBIndex = point1Index;
        mFrontierEdges[edgeIndex].PointAIndex = point1Index;

        // Concatenate edges
        mFrontierEdges[previousEdgeIndex].NextEdgeIndex = edgeIndex;

        // Advance
        previousEdgeIndex = edgeIndex;
        point1Index = springs.GetOtherEndpointIndex(edgeIndex, point1Index); // The point that will be in common between this edge and the next
                                                                             // is the one that is not in common now with the previous edge
    }

    // Concatenate last
    mFrontierEdges[edgeIndices[edgeIndices.size() - 2]].NextEdgeIndex = edgeIndices[edgeIndices.size() - 1];
}

void Frontiers::HandleTriangleDestroy(
    ElementIndex triangleElementIndex,
    Triangles const & triangles)
{
    // TODOHERE

    ////////////////////////////////////////

    // Remember we are now dirty - frontiers need
    // to be re-uploaded
    mIsDirtyForRendering = true;
}

void Frontiers::HandleTriangleRestore(
    ElementIndex triangleElementIndex,
    Triangles const & triangles)
{
    // TODOHERE

    ////////////////////////////////////////

    // Remember we are now dirty - frontiers need
    // to be re-uploaded
    mIsDirtyForRendering = true;
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
                return total + (f.has_value() ? f->Size : 0);
            });

        renderContext.UploadShipElementFrontierEdgesStart(shipId, totalSize);

        for (auto const & frontier : mFrontiers)
        {
            if (frontier.has_value())
            {
                assert(frontier->Size > 0);

                ElementIndex const startingEdgeIndex = frontier->StartingEdgeIndex;
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
        }

        renderContext.UploadShipElementFrontierEdgesEnd(shipId);

        // We are not dirty anymore
        mIsDirtyForRendering = false;
    }
}

void Frontiers::RegeneratePointColors() const
{
    std::array<rgbColor, 4> const ExternalColors
    {
        rgbColor(0, 153, 0),
        rgbColor(26, 140, 255),
        rgbColor(26, 255, 83),
        rgbColor(26, 255, 255)
    };

    std::array<rgbColor, 4> const InternalColors
    {
        rgbColor(255, 0, 0),
        rgbColor(255, 255, 0),
        rgbColor(153, 0, 0),
        rgbColor(230, 184, 0)

    };

    size_t externalUsed = 0;
    size_t internalUsed = 0;

    for (auto const & frontier : mFrontiers)
    {
        if (frontier.has_value())
        {
            vec3f const baseColor = (frontier->Type == FrontierType::External)
                ? ExternalColors[(externalUsed++) % ExternalColors.size()].toVec3f()
                : InternalColors[(internalUsed++) % InternalColors.size()].toVec3f();

            ElementIndex const startingEdgeIndex = frontier->StartingEdgeIndex;
            ElementIndex edgeIndex = startingEdgeIndex;

            float positionalProgress = 0.0f;

            do
            {
                mPointColors[mFrontierEdges[edgeIndex].PointAIndex].frontierBaseColor = baseColor;
                mPointColors[mFrontierEdges[edgeIndex].PointAIndex].positionalProgress = positionalProgress;

                // Advance
                edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;
                positionalProgress += 1.0f;

            } while (edgeIndex != startingEdgeIndex);
        }
    }
}

#ifdef _DEBUG
void Frontiers::VerifyInvariants(
    Springs const & springs,
    Triangles const & triangles) const
{
    std::set<ElementIndex> edgesWithFrontiers;

    //
    // Frontiers
    //

    for (size_t frontierIndex = 0; frontierIndex < mFrontiers.size(); ++frontierIndex)
    {
        auto const & frontier = mFrontiers[frontierIndex];

        if (frontier.has_value())
        {
            Verify(frontier->Size >= 3);

            size_t frontierLen = 0;
            for (ElementIndex edgeIndex = frontier->StartingEdgeIndex; ++frontierLen;)
            {
                // There is a spring here
                Verify(!springs.IsDeleted(edgeIndex));

                // This spring has one and only one super triangle
                Verify(springs.GetSuperTriangles(edgeIndex).size() == 1);

                ElementIndex const triangleIndex = springs.GetSuperTriangles(edgeIndex)[0];

                // This edge is CW in the triangle
                Verify(
                    triangles.ArePointsInCwOrder(
                        triangleIndex,
                        mFrontierEdges[edgeIndex].PointAIndex,
                        mFrontierEdges[edgeIndex].PointBIndex));

                // Edges know about their frontier
                Verify(mEdges[edgeIndex].FrontierIndex == frontierIndex);

                // This edge only belongs to one frontier
                auto const [_, isInserted] = edgesWithFrontiers.insert(edgeIndex);
                Verify(isInserted);

                // Advance
                edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;
                if (edgeIndex == frontier->StartingEdgeIndex)
                    break;
            }

            Verify(frontierLen == frontier->Size);
        }
    }

    //
    // Edges
    //

    for (ElementIndex edgeIndex = 0; edgeIndex < mEdgeCount; ++edgeIndex)
    {
        Verify(edgesWithFrontiers.count(edgeIndex) == 1 || mEdges[edgeIndex].FrontierIndex == NoneFrontierId);
    }
}
#endif

}