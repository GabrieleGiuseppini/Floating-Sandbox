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
    , mTriangles(triangles.GetElementCount())
    , mFrontiers()
    , mPointColors(pointCount, 0, Render::FrontierColor(vec3f::zero(), 0.0f))
    , mIsDirtyForRendering(true)
{
    //
    // Populate triangles
    //

    for (auto triangleIndex : triangles)
    {
        // Find subsprings with triangle's edges
        ElementIndex edgeAIndex = NoneElementIndex;
        ElementIndex edgeBIndex = NoneElementIndex;
        ElementIndex edgeCIndex = NoneElementIndex;
        for (ElementIndex edgeIndex : triangles.GetSubSprings(triangleIndex)) // Also contains ropes
        {
            if ((springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointAIndex(triangleIndex) && springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointBIndex(triangleIndex))
                || (springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointAIndex(triangleIndex) && springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointBIndex(triangleIndex)))
            {
                // Edge A
                assert(edgeAIndex == NoneElementIndex);
                edgeAIndex = edgeIndex;
            }
            else if ((springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointBIndex(triangleIndex) && springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointCIndex(triangleIndex))
                || (springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointBIndex(triangleIndex) && springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointCIndex(triangleIndex)))
            {
                // Edge B
                assert(edgeBIndex == NoneElementIndex);
                edgeBIndex = edgeIndex;
            }
            else if ((springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointCIndex(triangleIndex) && springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointAIndex(triangleIndex))
                || (springs.GetEndpointBIndex(edgeIndex) == triangles.GetPointCIndex(triangleIndex) && springs.GetEndpointAIndex(edgeIndex) == triangles.GetPointAIndex(triangleIndex)))
            {
                // Edge C
                assert(edgeCIndex == NoneElementIndex);
                edgeCIndex = edgeIndex;
            }
        }

        assert(edgeAIndex != NoneElementIndex && edgeBIndex != NoneElementIndex && edgeCIndex != NoneElementIndex);

        mTriangles.emplace_back(edgeAIndex, edgeBIndex, edgeCIndex);
    }
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
            static_cast<ElementIndex>(edgeIndices.size()),
            true)); // IsDirtyForRendering

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
        mFrontierEdges[edgeIndex].PrevEdgeIndex = previousEdgeIndex;

        // Advance
        previousEdgeIndex = edgeIndex;
        point1Index = springs.GetOtherEndpointIndex(edgeIndex, point1Index); // The point that will be in common between this edge and the next
                                                                             // is the one that is not in common now with the previous edge
    }
}

void Frontiers::HandleTriangleDestroy(
    ElementIndex triangleElementIndex,
    Springs const & springs,
    Triangles const & triangles)
{
    // Count edges with frontiers
    size_t edgesWithFrontierCount = 0;
    ElementIndex lastEdgeWithFrontier = NoneElementIndex;
    int lastEdgeOrdinalWithFrontier = -1; // index (0..2) of the edge in the triangles's array
    for (int eOrd = 0; eOrd < mTriangles[triangleElementIndex].EdgeIndices.size(); ++eOrd)
    {
        ElementIndex edgeIndex = mTriangles[triangleElementIndex].EdgeIndices[eOrd];
        if (mEdges[edgeIndex].FrontierIndex != NoneFrontierId)
        {
            ++edgesWithFrontierCount;
            lastEdgeWithFrontier = edgeIndex;
            lastEdgeOrdinalWithFrontier = eOrd;
        }
    }

    // Check cases
    if (edgesWithFrontierCount == 0)
    {
        LogMessage("TODOTEST: CASE 0 (t_idx=", triangleElementIndex, ")");

        //
        // None of the edges has a frontier...
        // hence each edge of the triangle is connected to two triangles...
        //

#ifdef _DEBUG
        for (int e = 0; e < 3; ++e)
            assert(springs.GetSuperTriangles(mTriangles[triangleElementIndex].EdgeIndices[e]).size() == 2);
#endif

        //
        // ...so this triangle will generate a new internal frontier: C->B->A
        //  - The frontier edges will travel counterclockwise, but along triangles'
        //    edges it'll travel in the triangles' clockwise direction
        //

        //
        // Make frontier with edges: C->B->A(->C)
        //

        auto const edgeAIndex = mTriangles[triangleElementIndex].EdgeIndices[0];
        auto const edgeBIndex = mTriangles[triangleElementIndex].EdgeIndices[1];
        auto const edgeCIndex = mTriangles[triangleElementIndex].EdgeIndices[2];

        // Create frontier
        FrontierId const newFrontierId = CreateNewFrontier(
            FrontierType::Internal,
            edgeCIndex,
            3);

        // C->B
        mEdges[edgeCIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeCIndex].PointAIndex = triangles.GetPointAIndex(triangleElementIndex);
        mFrontierEdges[edgeCIndex].PointBIndex = triangles.GetPointCIndex(triangleElementIndex);
        mFrontierEdges[edgeCIndex].NextEdgeIndex = edgeBIndex;
        mFrontierEdges[edgeCIndex].PrevEdgeIndex = edgeAIndex;

        // B->A
        mEdges[edgeBIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeBIndex].PointAIndex = triangles.GetPointCIndex(triangleElementIndex);
        mFrontierEdges[edgeBIndex].PointBIndex = triangles.GetPointBIndex(triangleElementIndex);
        mFrontierEdges[edgeBIndex].NextEdgeIndex = edgeAIndex;
        mFrontierEdges[edgeBIndex].PrevEdgeIndex = edgeCIndex;

        // A->C
        mEdges[edgeAIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeAIndex].PointAIndex = triangles.GetPointBIndex(triangleElementIndex);
        mFrontierEdges[edgeAIndex].PointBIndex = triangles.GetPointAIndex(triangleElementIndex);
        mFrontierEdges[edgeAIndex].NextEdgeIndex = edgeCIndex;
        mFrontierEdges[edgeAIndex].PrevEdgeIndex = edgeBIndex;
    }
    else if (edgesWithFrontierCount == 1)
    {
        LogMessage("TODOTEST: CASE 1 (t_idx=", triangleElementIndex, ")");

        //
        // Only one edge has a frontier...
        // ...hence the other two edges are each connected to two triangles...
        //

        assert(lastEdgeWithFrontier != NoneElementIndex);
        assert(lastEdgeOrdinalWithFrontier != -1);

#ifdef _DEBUG
        for (int e = 0; e < 3; ++e)
            assert(
                (mTriangles[triangleElementIndex].EdgeIndices[e] == lastEdgeWithFrontier && springs.GetSuperTriangles(mTriangles[triangleElementIndex].EdgeIndices[e]).size() == 1)
                || (mTriangles[triangleElementIndex].EdgeIndices[e] != lastEdgeWithFrontier && springs.GetSuperTriangles(mTriangles[triangleElementIndex].EdgeIndices[e]).size() == 2));
#endif

        //
        // ...we then propagate the frontier on the edge to the two other edges
        //

        //            lastEdgeWithFrontier
        //                X         Y
        // frontier: ---------------------->
        //                 \       /
        //                  \     /
        //                   \   /
        //                    \ /
        //                     Z
        //

        FrontierId const frontierId = mEdges[lastEdgeWithFrontier].FrontierIndex;

        int edgeOrdXZ = lastEdgeOrdinalWithFrontier - 1;
        if (edgeOrdXZ < 0)
            edgeOrdXZ += 3;

        ElementIndex const edgeXZ = mTriangles[triangleElementIndex].EdgeIndices[edgeOrdXZ];

        int edgeOrdZY = lastEdgeOrdinalWithFrontier + 1;
        if (edgeOrdZY >= 3)
            edgeOrdZY -= 3;

        ElementIndex const edgeZY = mTriangles[triangleElementIndex].EdgeIndices[edgeOrdZY];

        // X->Z
        mEdges[edgeXZ].FrontierIndex = frontierId;
        assert(triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithFrontier] == mFrontierEdges[lastEdgeWithFrontier].PointAIndex); // X
        mFrontierEdges[edgeXZ].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithFrontier]; // X
        mFrontierEdges[edgeXZ].PointBIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdXZ]; // Z
        mFrontierEdges[edgeXZ].NextEdgeIndex = edgeZY;
        mFrontierEdges[edgeXZ].PrevEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex;
        mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex].NextEdgeIndex = edgeXZ;

        // Z->Y
        mEdges[edgeZY].FrontierIndex = frontierId;
        assert(triangles.GetPointIndices(triangleElementIndex)[edgeOrdZY] == mFrontierEdges[lastEdgeWithFrontier].PointBIndex); // Y
        mFrontierEdges[edgeZY].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdXZ]; // Z
        mFrontierEdges[edgeZY].PointBIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdZY]; // Y
        mFrontierEdges[edgeZY].NextEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex;
        mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex].PrevEdgeIndex = edgeZY;
        mFrontierEdges[edgeZY].PrevEdgeIndex = edgeXZ;

        // Clear X->Y
        mEdges[lastEdgeWithFrontier].FrontierIndex = NoneFrontierId;

        // Update frontier
        assert(mFrontiers[frontierId].has_value());
        mFrontiers[frontierId]->StartingEdgeIndex = edgeXZ; // Just to be safe, as we've nuked XY
        mFrontiers[frontierId]->Size += 1; // +2 - 1
        mFrontiers[frontierId]->IsDirtyForRendering = true;
    }
    else
    {
        LogMessage("TODOTEST: CASE 2/3 (t_idx=", triangleElementIndex, ")");

        assert(edgesWithFrontierCount == 2 || edgesWithFrontierCount == 3);

        //
        // Visit all cusps
        //

        ProcessTriangleCuspDestroy<0, 1>(
            triangleElementIndex,
            springs,
            triangles);

        ProcessTriangleCuspDestroy<1, 2>(
            triangleElementIndex,
            springs,
            triangles);

        ProcessTriangleCuspDestroy<2, 0>(
            triangleElementIndex,
            springs,
            triangles);

        //
        // Visit all non-frontier edges
        //


        // TODO: ...

        // TODO: if this triangle is left with a frontier all for itself, destroy the frontier
    }

    ////////////////////////////////////////

    // Remember we are now dirty - frontiers need
    // to be re-uploaded
    mIsDirtyForRendering = true;
}

void Frontiers::HandleTriangleRestore(
    ElementIndex triangleElementIndex,
    Springs const & springs,
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
    Render::RenderContext & renderContext)
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////

FrontierId Frontiers::CreateNewFrontier(
    FrontierType type,
    ElementIndex startingEdgeIndex,
    ElementCount size)
{
    // Check if we may find an unused slot
    FrontierId newFrontierId;
    for (newFrontierId = 0; newFrontierId < mFrontiers.size() && mFrontiers[newFrontierId].has_value(); ++newFrontierId);

    if (newFrontierId == mFrontiers.size())
    {
        // Create new slot
        mFrontiers.emplace_back();
    }

    assert(newFrontierId < mFrontiers.size());

    mFrontiers[newFrontierId].emplace(
        type,
        startingEdgeIndex,
        size,
        true); // IsDirtyForRendering

    return newFrontierId;
}

template<int CuspEdge1Ordinal, int CuspEdge2Ordinal>
inline bool Frontiers::ProcessTriangleCuspDestroy(
    ElementIndex triangleElementIndex,
    Springs const & springs,
    Triangles const & triangles)
{
    //
    // Here we pretend to detach the cusp from the (eventual) rest of the ship,
    // adjusting frontiers in the process.
    //
    // On exit, both the traingle's edges entering the cusp and the (eventual)
    // edges leaving the cusp will be consistent.
    //
    // We return true if this was a cusp, false otherwise.
    //

    // The cusp edges are adjacent
    static_assert(
        (CuspEdge1Ordinal <= 1 && CuspEdge2Ordinal == CuspEdge1Ordinal + 1)
        || (CuspEdge1Ordinal == 2 && CuspEdge2Ordinal == 0));

    //
    // We only care about cusps with frontiers on both edges
    //

    ElementIndex const edge1Index = mTriangles[triangleElementIndex].EdgeIndices[CuspEdge1Ordinal];
    FrontierId const frontier1Id = mEdges[edge1Index].FrontierIndex;
    if (frontier1Id == NoneFrontierId)
        return false;

    ElementIndex const edge2Index = mTriangles[triangleElementIndex].EdgeIndices[CuspEdge2Ordinal];
    FrontierId const frontier2Id = mEdges[edge2Index].FrontierIndex;
    if (frontier2Id == NoneFrontierId)
        return false;

    //
    // - The frontier on edge 1 is entering the cusp
    // - The frontier on edge 2 is leaving the cusp
    //

#ifdef  _DEBUG
    ElementIndex const cuspPointIndex = triangles.GetPointIndices(triangleElementIndex)[CuspEdge2Ordinal];

    assert(mFrontierEdges[edge1Index].PointBIndex == cuspPointIndex);
    assert(mFrontierEdges[edge2Index].PointAIndex == cuspPointIndex);
#endif

    //
    // Check the four different cases
    //

    assert(mFrontiers[frontier1Id].has_value());
    assert(mFrontiers[frontier2Id].has_value());

    if (mFrontiers[frontier1Id]->Type == FrontierType::External)
    {
        if (mFrontiers[frontier2Id]->Type == FrontierType::External)
        {
            // Frontier1 == External
            // Frontier2 == External

            LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): F1=Ext, F2=Ext");

            // It's the same frontier
            assert(frontier1Id == frontier2Id);

            // Check whether the edges are directly connected
            if (mFrontierEdges[edge1Index].NextEdgeIndex == edge2Index)
            {
                //
                // Nothing to do
                //

                assert(mFrontierEdges[edge2Index].PrevEdgeIndex == edge1Index);

                LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): Connected, NOP");

                return false;
            }

            //
            // After coming into the cusp from edge1, the external frontier travels around
            // a region before returning back to the cusp and then away through edge2
            //
            // ...that region's frontier then becomes a new external frontier
            //

            auto const newFrontierEdge1Index = mFrontierEdges[edge1Index].NextEdgeIndex;
            auto const newFrontierEdge2Index = mFrontierEdges[edge2Index].PrevEdgeIndex;

            // Count edges/points along the soon-to-be-detached region
            ElementCount const newRegionSize = CountFrontierEdges(newFrontierEdge1Index, newFrontierEdge2Index);

            // Create new external frontier
            FrontierId const newFrontierId = CreateNewFrontier(
                FrontierType::External,
                newFrontierEdge1Index,
                newRegionSize);

            // Connect first and last edge of new frontier
            mFrontierEdges[newFrontierEdge1Index].PrevEdgeIndex = newFrontierEdge2Index;
            mFrontierEdges[newFrontierEdge2Index].NextEdgeIndex = newFrontierEdge1Index;

            // Connect old frontier's edges
            mFrontierEdges[edge1Index].NextEdgeIndex = edge2Index;
            mFrontierEdges[edge2Index].PrevEdgeIndex = edge1Index;

            // Update old frontier head
            mFrontiers[frontier1Id]->StartingEdgeIndex = edge1Index;  // Make sure the old frontier's was not starting with an edge that is now in the new frontier
            assert(mFrontiers[frontier1Id]->Size >= newRegionSize);
            mFrontiers[frontier1Id]->Size -= newRegionSize;
            mFrontiers[frontier1Id]->IsDirtyForRendering = true;

            return true;
        }
        else
        {
            // Frontier1 == External
            // Frontier2 == Internal

            LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): F1=Ext, F2=Int");

            // It's not the same frontier
            assert(frontier1Id != frontier2Id);

            // TODO
            return true;
        }
    }
    else if (mFrontiers[frontier2Id]->Type == FrontierType::External)
    {
        // Frontier1 == Internal
        // Frontier2 == External

        LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): F1=Int, F2=Ext");

        // It's not the same frontier
        assert(frontier1Id != frontier2Id);

        // TODO
        return true;
    }
    else
    {
        // Frontier1 == Internal
        // Frontier2 == Internal

        LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): F1=Int, F2=Int");

        // Check whether it's the same frontier
        if (frontier1Id == frontier2Id)
        {
            //
            // Same internal frontier
            //

            // Check whether the edges are directly connected
            if (mFrontierEdges[edge1Index].NextEdgeIndex == edge2Index)
            {
                //
                // Nothing to do
                //

                assert(mFrontierEdges[edge2Index].PrevEdgeIndex == edge1Index);

                LogMessage("TODOTEST: ProcessCusp(", CuspEdge1Ordinal, ", ", CuspEdge2Ordinal, "): Connected, NOP");

                return false;
            }

            // TODO: one of these will become external

            // TODO
            return true;
        }
        else
        {
            //
            // Different internal frontiers
            //

            // TODO
            return true;
        }
    }
}

void Frontiers::RegeneratePointColors()
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

    for (auto & frontier : mFrontiers)
    {
        if (frontier.has_value()
            && frontier->IsDirtyForRendering)
        {
            //
            // Propagate color and positional progress for this frontier
            //

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

            frontier->IsDirtyForRendering = false;
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

                // Frontier links are correct
                Verify(mFrontierEdges[mFrontierEdges[edgeIndex].PrevEdgeIndex].NextEdgeIndex == edgeIndex);
                Verify(mFrontierEdges[mFrontierEdges[edgeIndex].NextEdgeIndex].PrevEdgeIndex == edgeIndex);

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
        //
        // Edges without a frontier have no frontier ID
        //

        Verify(edgesWithFrontiers.count(edgeIndex) == 1 || mEdges[edgeIndex].FrontierIndex == NoneFrontierId);
    }

    //
    // Triangles
    //

    for (ElementIndex triangleIndex : triangles)
    {
        //
        // Edges are in CCW order
        //

        auto const edgeAIndex = mTriangles[triangleIndex].EdgeIndices[0];

        Verify(
            (triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointAIndex(edgeAIndex)
                && triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointBIndex(edgeAIndex))
            || (triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointBIndex(edgeAIndex)
                && triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointAIndex(edgeAIndex)));

        auto const edgeBIndex = mTriangles[triangleIndex].EdgeIndices[1];

        Verify(
            (triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointAIndex(edgeBIndex)
                && triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointBIndex(edgeBIndex))
            || (triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointBIndex(edgeBIndex)
                && triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointAIndex(edgeBIndex)));

        auto const edgeCIndex = mTriangles[triangleIndex].EdgeIndices[2];

        Verify(
            (triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointAIndex(edgeCIndex)
                && triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointBIndex(edgeCIndex))
            || (triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointBIndex(edgeCIndex)
                && triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointAIndex(edgeCIndex)));

    }
}
#endif

}