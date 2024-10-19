/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-09-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameDebug.h>

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <utility>

namespace Physics {

void Frontiers::AddFrontier(
    FrontierType type,
    std::vector<ElementIndex> edgeIndices,
    Springs const & springs)
{
    assert(edgeIndices.size() > 0);

    //
    // Create frontier
    //

    FrontierId const frontierIndex = CreateNewFrontier(
        type,
        edgeIndices[0],
        static_cast<ElementCount>(edgeIndices.size()));

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

        // Set point index
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
    Points const & points,
    Springs const & springs,
    Triangles const & triangles)
{
    // Take edge indices once and for all
    auto const edgeAIndex = triangles.GetSubSpringAIndex(triangleElementIndex);
    auto const edgeBIndex = triangles.GetSubSpringBIndex(triangleElementIndex);
    auto const edgeCIndex = triangles.GetSubSpringCIndex(triangleElementIndex);

    // Springs are already consistent with the removal of this triangle
    assert(springs.GetSuperTriangles(edgeAIndex).size() == 0
        || (springs.GetSuperTriangles(edgeAIndex).size() == 1 && springs.GetSuperTriangles(edgeAIndex)[0] != triangleElementIndex));
    assert(springs.GetSuperTriangles(edgeBIndex).size() == 0
        || (springs.GetSuperTriangles(edgeBIndex).size() == 1 && springs.GetSuperTriangles(edgeBIndex)[0] != triangleElementIndex));
    assert(springs.GetSuperTriangles(edgeCIndex).size() == 0
        || (springs.GetSuperTriangles(edgeCIndex).size() == 1 && springs.GetSuperTriangles(edgeCIndex)[0] != triangleElementIndex));

    // Count edges with frontiers
    size_t edgesWithFrontierCount = 0;
    ElementIndex lastEdgeWithFrontier = NoneElementIndex;
    int lastEdgeOrdinalWithFrontier = -1; // index (0..2) of the edge in the triangles's array
    for (int eOrd = 0; eOrd < static_cast<int>(triangles.GetSubSprings(triangleElementIndex).SpringIndices.size()); ++eOrd)
    {
        ElementIndex edgeIndex = triangles.GetSubSprings(triangleElementIndex).SpringIndices[eOrd];
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
        //
        // None of the edges has a frontier...
        // hence each edge of the triangle is connected to two triangles...
        //
        // ...so this triangle will generate a new internal frontier: C->B->A
        //  - The frontier edges will travel counterclockwise, but along triangles'
        //    edges it'll travel in the triangles' clockwise direction
        //

        //
        // Make frontier with edges: C->B->A(->C)
        //

        // Create frontier
        FrontierId const newFrontierId = CreateNewFrontier(
            FrontierType::Internal,
            edgeCIndex,
            3);

        // C->B
        mEdges[edgeCIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeCIndex].PointAIndex = triangles.GetPointAIndex(triangleElementIndex);
        mFrontierEdges[edgeCIndex].NextEdgeIndex = edgeBIndex;
        mFrontierEdges[edgeCIndex].PrevEdgeIndex = edgeAIndex;

        // B->A
        mEdges[edgeBIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeBIndex].PointAIndex = triangles.GetPointCIndex(triangleElementIndex);
        mFrontierEdges[edgeBIndex].NextEdgeIndex = edgeAIndex;
        mFrontierEdges[edgeBIndex].PrevEdgeIndex = edgeCIndex;

        // A->C
        mEdges[edgeAIndex].FrontierIndex = newFrontierId;
        mFrontierEdges[edgeAIndex].PointAIndex = triangles.GetPointBIndex(triangleElementIndex);
        mFrontierEdges[edgeAIndex].NextEdgeIndex = edgeCIndex;
        mFrontierEdges[edgeAIndex].PrevEdgeIndex = edgeBIndex;
    }
    else if (edgesWithFrontierCount == 1)
    {
        //
        // Only one edge has a frontier...
        // ...hence the other two edges are each connected to two triangles...
        //

        assert(lastEdgeWithFrontier != NoneElementIndex);
        assert(lastEdgeOrdinalWithFrontier != -1);

        //
        // ...we then propagate the frontier on the edge to the two other edges
        //

        //            lastEdgeWithFrontier
        //                X         Y
        // frontier: ---------------------->
        //
        //                  \     /
        //
        //                    \ /
        //                     Z
        //

        FrontierId const frontierId = mEdges[lastEdgeWithFrontier].FrontierIndex;

        int const edgeOrdXZ = PreviousEdgeOrdinal(lastEdgeOrdinalWithFrontier);
        ElementIndex const edgeXZ = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdXZ];

        int const edgeOrdZY = NextEdgeOrdinal(lastEdgeOrdinalWithFrontier);
        ElementIndex const edgeZY = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdZY];

        // X->Z
        mEdges[edgeXZ].FrontierIndex = frontierId;
        assert(triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithFrontier] == mFrontierEdges[lastEdgeWithFrontier].PointAIndex); // X
        mFrontierEdges[edgeXZ].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithFrontier]; // X
        mFrontierEdges[edgeXZ].NextEdgeIndex = edgeZY;
        mFrontierEdges[edgeXZ].PrevEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex;
        mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex].NextEdgeIndex = edgeXZ;

        // Z->Y
        mEdges[edgeZY].FrontierIndex = frontierId;
        mFrontierEdges[edgeZY].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdXZ]; // Z
        mFrontierEdges[edgeZY].NextEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex;
        mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex].PrevEdgeIndex = edgeZY;
        mFrontierEdges[edgeZY].PrevEdgeIndex = edgeXZ;

        // Clear X->Y
        mEdges[lastEdgeWithFrontier].FrontierIndex = NoneFrontierId;

        // Update frontier
        assert(mFrontiers[frontierId].has_value());
        mFrontiers[frontierId]->StartingEdgeIndex = edgeXZ; // Just to be safe, as we've nuked XY
        mFrontiers[frontierId]->Size += 1; // +2 - 1
    }
    else
    {
        assert(edgesWithFrontierCount == 2 || edgesWithFrontierCount == 3);

        //
        // Visit all cusps
        //

        bool const isABCusp = ProcessTriangleCuspDestroy<0, 1>(
            edgeAIndex,
            edgeBIndex,
            points,
            springs);

        bool const isBCCusp = ProcessTriangleCuspDestroy<1, 2>(
            edgeBIndex,
            edgeCIndex,
            points,
            springs);

        bool const isCACusp = ProcessTriangleCuspDestroy<2, 0>(
            edgeCIndex,
            edgeAIndex,
            points,
            springs);

        int const cuspCount =
            (isABCusp ? 1 : 0)
            + (isBCCusp ? 1 : 0)
            + (isCACusp ? 1 : 0);

        assert(cuspCount == 1 || cuspCount == 3);

        if (cuspCount == 1)
        {
            //
            // There is one and only one non-frontier edge
            //

            if (isABCusp)
            {
                ProcessTriangleOppositeCuspEdgeDestroy(
                    edgeCIndex,
                    edgeAIndex,
                    edgeBIndex);
            }
            else if (isBCCusp)
            {
                ProcessTriangleOppositeCuspEdgeDestroy(
                    edgeAIndex,
                    edgeBIndex,
                    edgeCIndex);
            }
            else
            {
                assert(isCACusp);

                ProcessTriangleOppositeCuspEdgeDestroy(
                    edgeBIndex,
                    edgeCIndex,
                    edgeAIndex);
            }
        }
        else
        {
            //
            // There are no non-frontier edges
            //

            assert(cuspCount == 3);

            // All the edges of the triangle have a frontier, and it's the same
            FrontierId const frontierId = mEdges[edgeAIndex].FrontierIndex;
            assert(frontierId != NoneFrontierId);
            assert(mEdges[edgeBIndex].FrontierIndex == frontierId);
            assert(mEdges[edgeCIndex].FrontierIndex == frontierId);

            // Destroy this frontier
            mEdges[edgeAIndex].FrontierIndex = NoneFrontierId;
            mEdges[edgeBIndex].FrontierIndex = NoneFrontierId;
            mEdges[edgeCIndex].FrontierIndex = NoneFrontierId;
            mFrontiers[frontierId]->Size = 0;
            DestroyFrontier(frontierId);
        }
    }

    ////////////////////////////////////////

    // Remember we are now dirty - frontiers need
    // to be re-uploaded
    mIsDirtyForRendering = true;
}

void Frontiers::HandleTriangleRestore(
    ElementIndex triangleElementIndex,
    Points const & points,
    Springs const & springs,
    Triangles const & triangles)
{
    // Take edge indices once and for all
    auto const edgeAIndex = triangles.GetSubSpringAIndex(triangleElementIndex);
    auto const edgeBIndex = triangles.GetSubSpringBIndex(triangleElementIndex);
    auto const edgeCIndex = triangles.GetSubSpringCIndex(triangleElementIndex);

    // Count edges with frontiers
    size_t edgesWithFrontierCount = 0;
    ElementIndex lastEdgeWithFrontier = NoneElementIndex;
    int lastEdgeOrdinalWithFrontier = -1; // index (0..2) of the edge in the triangles's array
    ElementIndex lastEdgeWithoutFrontier = NoneElementIndex;
    int lastEdgeOrdinalWithoutFrontier = -1; // index (0..2) of the edge in the triangles's array
    for (int eOrd = 0; eOrd < static_cast<int>(triangles.GetSubSprings(triangleElementIndex).SpringIndices.size()); ++eOrd)
    {
        ElementIndex const edgeIndex = triangles.GetSubSprings(triangleElementIndex).SpringIndices[eOrd];
        if (mEdges[edgeIndex].FrontierIndex != NoneFrontierId)
        {
            // If there's a frontier here, it's because of another triangle
            assert(springs.GetSuperTriangles(edgeIndex).size() == 1 && springs.GetSuperTriangles(edgeIndex)[0] != triangleElementIndex);

            ++edgesWithFrontierCount;
            lastEdgeWithFrontier = edgeIndex;
            lastEdgeOrdinalWithFrontier = eOrd;
        }
        else
        {
            // If there's no frontier here, it's because there are no triangles
            assert(springs.GetSuperTriangles(edgeIndex).size() == 0);

            lastEdgeWithoutFrontier = edgeIndex;
            lastEdgeOrdinalWithoutFrontier = eOrd;
        }
    }

    // Check cases
    if (edgesWithFrontierCount == 3)
    {
        //
        // This triangle is going to be restored onto a "hole" whose 3 edges
        // have frontiers...
        //

        // ...if there are 3 edges with frontiers, then it's the same frontier
        FrontierId const frontierId = mEdges[edgeAIndex].FrontierIndex;
        assert(mEdges[edgeBIndex].FrontierIndex == frontierId);
        assert(mEdges[edgeCIndex].FrontierIndex == frontierId);

        // ...if there are 3 edges with frontiers, then it's an internal frontier
        assert(mFrontiers[frontierId].has_value()
            && mFrontiers[frontierId]->Type == FrontierType::Internal);

        // ...if there are 3 edges with frontiers, then the edges are all connected
        assert(mFrontierEdges[edgeAIndex].NextEdgeIndex == edgeCIndex
            && mFrontierEdges[edgeCIndex].NextEdgeIndex == edgeBIndex
            && mFrontierEdges[edgeBIndex].NextEdgeIndex == edgeAIndex);

        //
        // ...simply destroy this frontier, then
        //

        mEdges[edgeAIndex].FrontierIndex = NoneFrontierId;
        mEdges[edgeBIndex].FrontierIndex = NoneFrontierId;
        mEdges[edgeCIndex].FrontierIndex = NoneFrontierId;
        mFrontiers[frontierId]->Size = 0;
        DestroyFrontier(frontierId);
    }
    else if (edgesWithFrontierCount == 2)
    {
        //
        // Two edges have a frontier and one doesn't...
        //

        assert(lastEdgeWithoutFrontier != NoneElementIndex);
        assert(lastEdgeOrdinalWithoutFrontier != -1);

        // ...hence the other (non-frontier) edge has zero triangles
        assert(springs.GetSuperTriangles(lastEdgeWithoutFrontier).size() == 0);

        //
        // ...we then move the frontier on the edges to the one edge without frontiers
        //

        //           lastEdgeWithoutFrontier
        //                X         Y
        // frontier: ------         -------->
        //                 \       /
        //                  \     /
        //                   \   /
        //                    \ /
        //                     Z
        //

        FrontierId const frontierId = mEdges[lastEdgeWithFrontier].FrontierIndex;

        int const edgeOrdXZ = PreviousEdgeOrdinal(lastEdgeOrdinalWithoutFrontier);
        ElementIndex const edgeXZ = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdXZ];

        int const edgeOrdZY = NextEdgeOrdinal(lastEdgeOrdinalWithoutFrontier);
        ElementIndex const edgeZY = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdZY];

        // XZ and ZY are connected
        assert(mEdges[edgeXZ].FrontierIndex == frontierId
            && mFrontierEdges[edgeXZ].NextEdgeIndex == edgeZY);
        assert(mEdges[edgeZY].FrontierIndex == frontierId
            && mFrontierEdges[edgeZY].PrevEdgeIndex == edgeXZ);

        // X->Y
        mEdges[lastEdgeWithoutFrontier].FrontierIndex = frontierId;
        assert(triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithoutFrontier] == mFrontierEdges[edgeXZ].PointAIndex); // X
        mFrontierEdges[lastEdgeWithoutFrontier].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[lastEdgeOrdinalWithoutFrontier]; // X
        mFrontierEdges[lastEdgeWithoutFrontier].NextEdgeIndex = mFrontierEdges[edgeZY].NextEdgeIndex;
        mFrontierEdges[mFrontierEdges[edgeZY].NextEdgeIndex].PrevEdgeIndex = lastEdgeWithoutFrontier;
        mFrontierEdges[lastEdgeWithoutFrontier].PrevEdgeIndex = mFrontierEdges[edgeXZ].PrevEdgeIndex;
        mFrontierEdges[mFrontierEdges[edgeXZ].PrevEdgeIndex].NextEdgeIndex = lastEdgeWithoutFrontier;

        // Clear X->Z
        mEdges[edgeXZ].FrontierIndex = NoneFrontierId;

        // Clear Z->Y
        mEdges[edgeZY].FrontierIndex = NoneFrontierId;

        // Update frontier
        assert(mFrontiers[frontierId].has_value());
        mFrontiers[frontierId]->StartingEdgeIndex = lastEdgeWithoutFrontier; // Just to be safe, as we've nuked the two
        mFrontiers[frontierId]->Size -= 1; // -2 + 1
    }
    else
    {
        assert(edgesWithFrontierCount == 0 || edgesWithFrontierCount == 1);

        //
        // 1) Propagate (eventual) frontier on one edge to whole triangle
        //

        if (edgesWithFrontierCount != 0)
        {
            //
            // One edge has a frontier and two don't...
            //

            assert(edgesWithFrontierCount == 1);
            assert(lastEdgeWithFrontier != NoneElementIndex);
            assert(lastEdgeOrdinalWithFrontier != -1);

            //            lastEdgeWithFrontier
            //                X         Y
            // frontier: <---------------------
            //
            //                  \     /
            //
            //                    \ /
            //                     Z
            //

            //
            // ...replace Y->X (which runs along another, old triangle) with Y->Z and Z->X (which will run along this new triangle)
            //

            FrontierId const frontierId = mEdges[lastEdgeWithFrontier].FrontierIndex;

            int const edgeOrdYZ = NextEdgeOrdinal(lastEdgeOrdinalWithFrontier);
            ElementIndex const edgeYZ = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdYZ];

            int const edgeOrdZX = PreviousEdgeOrdinal(lastEdgeOrdinalWithFrontier);
            ElementIndex const edgeZX = triangles.GetSubSprings(triangleElementIndex).SpringIndices[edgeOrdZX];

            ElementIndex const pointZIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdZX];

            // Y->Z
            mEdges[edgeYZ].FrontierIndex = frontierId;
            assert(triangles.GetPointIndices(triangleElementIndex)[edgeOrdYZ] == mFrontierEdges[lastEdgeWithFrontier].PointAIndex); // Y
            mFrontierEdges[edgeYZ].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[edgeOrdYZ]; // Y
            mFrontierEdges[edgeYZ].NextEdgeIndex = edgeZX;
            mFrontierEdges[edgeYZ].PrevEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex;
            mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].PrevEdgeIndex].NextEdgeIndex = edgeYZ;

            // Z->Y
            mEdges[edgeZX].FrontierIndex = frontierId;
            mFrontierEdges[edgeZX].PointAIndex = pointZIndex; // Z
            mFrontierEdges[edgeZX].NextEdgeIndex = mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex;
            mFrontierEdges[mFrontierEdges[lastEdgeWithFrontier].NextEdgeIndex].PrevEdgeIndex = edgeZX;
            mFrontierEdges[edgeZX].PrevEdgeIndex = edgeYZ;

            // Clear Y->X
            mEdges[lastEdgeWithFrontier].FrontierIndex = NoneFrontierId;

            // Update frontier
            assert(mFrontiers[frontierId].has_value());
            mFrontiers[frontierId]->StartingEdgeIndex = edgeYZ; // Just to be safe, as we've nuked YX
            mFrontiers[frontierId]->Size += 1; // +2 - 1
        }

        //
        // 2) Process each (future) cusp that is found on any vertex of the triangle
        //    which is NOT adjacent to the (one or zero) frontier edge
        //

        int cuspCount = 0;

        if (lastEdgeWithFrontier != edgeAIndex
            && lastEdgeWithFrontier != edgeBIndex)
        {
            if (ProcessTriangleCuspRestore<0, 1>(
                edgeAIndex,
                edgeBIndex,
                triangleElementIndex,
                points,
                springs,
                triangles))
            {
                ++cuspCount;
            }
        }

        if (lastEdgeWithFrontier != edgeBIndex
            && lastEdgeWithFrontier != edgeCIndex)
        {
            if (ProcessTriangleCuspRestore<1, 2>(
                edgeBIndex,
                edgeCIndex,
                triangleElementIndex,
                points,
                springs,
                triangles))
            {
                ++cuspCount;
            }
        }

        if (lastEdgeWithFrontier != edgeCIndex
            && lastEdgeWithFrontier != edgeAIndex)
        {
            if (ProcessTriangleCuspRestore<2, 0>(
                edgeCIndex,
                edgeAIndex,
                triangleElementIndex,
                points,
                springs,
                triangles))
            {
                ++cuspCount;
            }
        }

        //
        // 3) Process trivial, pathological case of isolated triangle being restored out of nothing
        //

        if (cuspCount == 0
            && edgesWithFrontierCount == 0)
        {
            //
            // This triangle will generate a new external frontier: A->B->C
            //

            // Create frontier
            FrontierId const newFrontierId = CreateNewFrontier(
                FrontierType::External,
                edgeAIndex,
                3);

            // A->B
            mEdges[edgeAIndex].FrontierIndex = newFrontierId;
            mFrontierEdges[edgeAIndex].PointAIndex = triangles.GetPointAIndex(triangleElementIndex);
            mFrontierEdges[edgeAIndex].NextEdgeIndex = edgeBIndex;
            mFrontierEdges[edgeAIndex].PrevEdgeIndex = edgeCIndex;

            // B->C
            mEdges[edgeBIndex].FrontierIndex = newFrontierId;
            mFrontierEdges[edgeBIndex].PointAIndex = triangles.GetPointBIndex(triangleElementIndex);
            mFrontierEdges[edgeBIndex].NextEdgeIndex = edgeCIndex;
            mFrontierEdges[edgeBIndex].PrevEdgeIndex = edgeAIndex;

            // C->A
            mEdges[edgeCIndex].FrontierIndex = newFrontierId;
            mFrontierEdges[edgeCIndex].PointAIndex = triangles.GetPointCIndex(triangleElementIndex);
            mFrontierEdges[edgeCIndex].NextEdgeIndex = edgeAIndex;
            mFrontierEdges[edgeCIndex].PrevEdgeIndex = edgeBIndex;
        }
    }

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
        renderContext.UploadShipPointFrontierColorsAsync(shipId, mPointColors.data());

        //
        // Upload frontier point indices
        //

        auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

        size_t const totalSize = std::accumulate(
            mFrontiers.cbegin(),
            mFrontiers.cend(),
            size_t(0),
            [](size_t total, auto const & f)
            {
                return total + (f.has_value() ? f->Size : 0);
            });

        shipRenderContext.UploadElementFrontierEdgesStart(totalSize);

        for (auto const & frontier : mFrontiers)
        {
            if (frontier.has_value())
            {
                assert(frontier->Size > 0);

                ElementIndex const startingEdgeIndex = frontier->StartingEdgeIndex;
                ElementIndex edgeIndex = startingEdgeIndex;

                do
                {
                    auto const nextEdgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex;

                    // Upload
                    shipRenderContext.UploadElementFrontierEdge(
                        mFrontierEdges[edgeIndex].PointAIndex,
                        mFrontierEdges[nextEdgeIndex].PointAIndex);

                    // Advance
                    edgeIndex = nextEdgeIndex;

                } while (edgeIndex != startingEdgeIndex);
            }
        }

        shipRenderContext.UploadElementFrontierEdgesEnd();

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
        size);

    // Add to frontier indices
    mFrontierIds.emplace_back(newFrontierId);

    return newFrontierId;
}

void Frontiers::DestroyFrontier(
    FrontierId frontierId)
{
    assert(mFrontiers[frontierId].has_value());
    assert(mFrontiers[frontierId]->Size == 0);

    mFrontiers[frontierId].reset();

    // Remove from frontier indices

    auto const indexIt = std::find(
        mFrontierIds.cbegin(),
        mFrontierIds.cend(),
        frontierId);

    assert(indexIt != mFrontierIds.cend());

    mFrontierIds.erase(indexIt);
}

FrontierId Frontiers::SplitIntoNewFrontier(
    ElementIndex const newFrontierStartEdgeIndex,
    ElementIndex const newFrontierEndEdgeIndex,
    FrontierId const oldFrontierId,
    FrontierType const newFrontierType,
    ElementIndex const cutEdgeIn,
    ElementIndex const cutEdgeOut)
{
    // Start and end currently belong to the old frontier
    assert(mEdges[newFrontierStartEdgeIndex].FrontierIndex == oldFrontierId);
    assert(mEdges[newFrontierEndEdgeIndex].FrontierIndex == oldFrontierId);

    //
    // New frontier
    //

    // Create new frontier
    FrontierId const newFrontierId = CreateNewFrontier(
        newFrontierType,
        newFrontierStartEdgeIndex,
        0);

    // Propagate new frontier along the soon-to-be-detached region
    ElementCount const newFrontierSize = PropagateFrontier(
        newFrontierStartEdgeIndex,
        newFrontierEndEdgeIndex,
        newFrontierId);

    // Connect first and last edges at cusp
    mFrontierEdges[newFrontierStartEdgeIndex].PrevEdgeIndex = newFrontierEndEdgeIndex;
    mFrontierEdges[newFrontierEndEdgeIndex].NextEdgeIndex = newFrontierStartEdgeIndex;

    // Update new frontier
    mFrontiers[newFrontierId]->Size = newFrontierSize;

    //
    // Old frontier
    //

    // Reconnect edges at cut
    mFrontierEdges[cutEdgeIn].NextEdgeIndex = cutEdgeOut;
    mFrontierEdges[cutEdgeOut].PrevEdgeIndex = cutEdgeIn;

    // Update old frontier
    mFrontiers[oldFrontierId]->StartingEdgeIndex = cutEdgeIn;  // Make sure the old frontier's was not starting with an edge that is now in the new frontier
    assert(mFrontiers[oldFrontierId]->Size >= newFrontierSize);
    mFrontiers[oldFrontierId]->Size -= newFrontierSize;

    return newFrontierId;
}

void Frontiers::ReplaceAndCutFrontier(
    ElementIndex const startEdgeIndex,
    ElementIndex const endEdgeIndex,
    FrontierId const oldFrontierId,
    FrontierId const newFrontierId,
    ElementIndex const cutEdgeIn,
    ElementIndex const cutEdgeOut)
{
    // It's not the same frontier
    assert(oldFrontierId != newFrontierId);

    // Start and end currently belong to the old frontier
    assert(mEdges[startEdgeIndex].FrontierIndex == oldFrontierId);
    assert(mEdges[endEdgeIndex].FrontierIndex == oldFrontierId);

    // Edges on the opposite (non-triangle) side of cusp
    ElementIndex const afterEdgeIn = mFrontierEdges[cutEdgeIn].NextEdgeIndex;
    ElementIndex const beforeEdgeOut = mFrontierEdges[cutEdgeOut].PrevEdgeIndex;

    // Propagate new frontier along the old frontier
    ElementCount const oldFrontierSize = PropagateFrontier(
        startEdgeIndex,
        endEdgeIndex,
        newFrontierId);

    // Cut: connect edges at triangle's side of cusp among themselves
    mFrontierEdges[cutEdgeIn].NextEdgeIndex = cutEdgeOut;
    mFrontierEdges[cutEdgeOut].PrevEdgeIndex = cutEdgeIn;

    // Cut: connect edges at opposite side of cusp among themselves
    mFrontierEdges[afterEdgeIn].PrevEdgeIndex = beforeEdgeOut;
    mFrontierEdges[beforeEdgeOut].NextEdgeIndex = afterEdgeIn;

    // Update new frontier
    mFrontiers[newFrontierId]->Size += oldFrontierSize;

    // Destroy old frontier
    assert(oldFrontierSize == mFrontiers[oldFrontierId]->Size);
    mFrontiers[oldFrontierId]->Size -= oldFrontierSize;
    DestroyFrontier(oldFrontierId);
}

void Frontiers::ReplaceAndJoinFrontier(
    ElementIndex const edgeIn,
    ElementIndex const edgeInOpposite,
    ElementIndex const edgeOutOpposite,
    ElementIndex const edgeOut,
    FrontierId const oldFrontierId,
    FrontierId const newFrontierId)
{
    // It's not the same frontier
    assert(oldFrontierId != newFrontierId);

    // Start and end currently belong to the old frontier
    assert(mEdges[edgeInOpposite].FrontierIndex == oldFrontierId);
    assert(mEdges[edgeOutOpposite].FrontierIndex == oldFrontierId);

    // EdgeIn and EdgeOut are currently connected to each other
    assert(mFrontierEdges[edgeIn].NextEdgeIndex == edgeOut);
    assert(mFrontierEdges[edgeOut].PrevEdgeIndex == edgeIn);

    // Propagate new frontier along the old frontier
    ElementCount const oldFrontierSize = PropagateFrontier(
        edgeInOpposite,
        edgeOutOpposite,
        newFrontierId);

    // Connect EdgeIn->EdgeInOpposite
    mFrontierEdges[edgeIn].NextEdgeIndex = edgeInOpposite;
    mFrontierEdges[edgeInOpposite].PrevEdgeIndex = edgeIn;

    // Connect EdgeOutOpposite->EdgeOut
    mFrontierEdges[edgeOutOpposite].NextEdgeIndex = edgeOut;
    mFrontierEdges[edgeOut].PrevEdgeIndex = edgeOutOpposite;

    // Update new frontier
    mFrontiers[newFrontierId]->Size += oldFrontierSize;

    // Destroy old frontier
    assert(oldFrontierSize == mFrontiers[oldFrontierId]->Size);
    mFrontiers[oldFrontierId]->Size -= oldFrontierSize;
    DestroyFrontier(oldFrontierId);
}

ElementCount Frontiers::PropagateFrontier(
    ElementIndex const startEdgeIndex,
    ElementIndex const endEdgeIndex,
    FrontierId const frontierId)
{
    ElementCount count = 1;
    for (ElementIndex edgeIndex = startEdgeIndex; ; ++count, edgeIndex = mFrontierEdges[edgeIndex].NextEdgeIndex)
    {
        mEdges[edgeIndex].FrontierIndex = frontierId;

        if (edgeIndex == endEdgeIndex)
            break;
    }

    return count;
}

template<int CuspEdgeInOrdinal, int CuspEdgeOutOrdinal>
inline bool Frontiers::ProcessTriangleCuspDestroy(
    ElementIndex const edgeIn,
    ElementIndex const edgeOut,
    Points const & points,
    Springs const & springs)
{
    //
    // Here we pretend to detach the cusp (which we don't know already as being a cusp)
    // from the (eventual) rest of the ship, adjusting frontiers in the process.
    //
    // On exit, both the triangle's edges entering the cusp and the (eventual)
    // edges leaving the cusp will be consistent.
    //
    // We return true if this was a cusp, false otherwise.
    //

    // The cusp edges are adjacent
    static_assert(
        (CuspEdgeInOrdinal <= 1 && CuspEdgeOutOrdinal == CuspEdgeInOrdinal + 1)
        || (CuspEdgeInOrdinal == 2 && CuspEdgeOutOrdinal == 0));

    //
    // We only care about cusps - i.e. with frontiers on both edges
    //

    FrontierId const frontierInId = mEdges[edgeIn].FrontierIndex;
    if (frontierInId == NoneFrontierId)
        return false;

    FrontierId const frontierOutId = mEdges[edgeOut].FrontierIndex;
    if (frontierOutId == NoneFrontierId)
        return false;

    // Springs are consistent
    assert(springs.GetSuperTriangles(edgeIn).size() == 0); // edgeIn has a frontier made of this triangle, which according to the springs is already gone
    assert(springs.GetSuperTriangles(edgeOut).size() == 0); // edgeOut has a frontier made of this triangle, which according to the springs is already gone
    (void)springs;

    //
    // Check the four different cases
    //

    assert(mFrontiers[frontierInId].has_value());
    assert(mFrontiers[frontierOutId].has_value());

    if (mFrontiers[frontierInId]->Type == FrontierType::External)
    {
        if (mFrontiers[frontierOutId]->Type == FrontierType::External)
        {
            // FrontierIn == External
            // FrontierOut == External

            // It's the same frontier
            assert(frontierInId == frontierOutId);

            // Check whether the edges are directly connected
            if (mFrontierEdges[edgeIn].NextEdgeIndex == edgeOut)
            {
                //
                // Edges are directly connected
                //

                assert(mFrontierEdges[edgeOut].PrevEdgeIndex == edgeIn); // Connected

                //
                // Nothing to do
                //
            }
            else
            {
                //
                // Edges are not directly connected
                //

                assert(mFrontierEdges[edgeOut].PrevEdgeIndex != edgeIn); // Not connected

                //
                // After coming into the cusp from edge1, the external frontier travels around
                // a region before returning back to the cusp and then away through edge2...
                //
                // ...quite arbitrarily, we decide that that region's frontier then becomes a
                // new external frontier (we could have chosen instead that that region's frontier
                // stays and the other portion becomes the new frontier)
                //

                SplitIntoNewFrontier(
                    mFrontierEdges[edgeIn].NextEdgeIndex,
                    mFrontierEdges[edgeOut].PrevEdgeIndex,
                    frontierInId,
                    FrontierType::External,
                    edgeIn,
                    edgeOut);
            }
        }
        else
        {
            // FrontierIn == External
            // FrontierOut == Internal

            assert(mFrontierEdges[edgeIn].NextEdgeIndex != edgeOut); // Not connected
            assert(mFrontierEdges[edgeOut].PrevEdgeIndex != edgeIn); // Not connected

            //
            // The external and internal frontiers are going to get merged into one,
            // single *external* frontier
            //

            // Replace the internal frontier (frontierOutId, connecting edgeOut to beforeEdgeOut)
            // with the external frontier (frontierInId)
            ReplaceAndCutFrontier(
                edgeOut, // Start
                mFrontierEdges[edgeOut].PrevEdgeIndex, // End
                frontierOutId, // Old
                frontierInId, // New
                edgeIn,
                edgeOut);
        }
    }
    else if (mFrontiers[frontierOutId]->Type == FrontierType::External)
    {
        // FrontierIn == Internal
        // FrontierOut == External

        assert(mFrontierEdges[edgeIn].NextEdgeIndex != edgeOut); // Not connected
        assert(mFrontierEdges[edgeOut].PrevEdgeIndex != edgeIn); // Not connected

        //
        // The internal and external frontiers are going to get merged into one,
        // single *external* frontier
        //

        // Replace the internal frontier (frontierInId, connecting afterEdgeIn to edgeIn)
        // with the external frontier (frontierOutId)
        ReplaceAndCutFrontier(
            mFrontierEdges[edgeIn].NextEdgeIndex, // Start
            edgeIn, // End
            frontierInId, // Old
            frontierOutId, // New
            edgeIn,
            edgeOut);
    }
    else
    {
        // FrontierIn == Internal
        // FrontierOut == Internal

        // Check whether it's the same frontier
        if (frontierInId == frontierOutId)
        {
            //
            // Same internal frontier
            //

            // Check whether the edges are directly connected
            if (mFrontierEdges[edgeIn].NextEdgeIndex == edgeOut)
            {
                //
                // Directly connected
                //

                assert(mFrontierEdges[edgeOut].PrevEdgeIndex == edgeIn); // Connected

                //
                // Nothing to do
                //
            }
            else
            {
                //
                // Not connected
                //

                assert(mFrontierEdges[edgeOut].PrevEdgeIndex != edgeIn); // Not connected

                //
                // This is a peculiar situation: we are separating two parts of a region,
                // along an internal frontier. This is the "ball forming in a hole" case.
                //
                // When an internal frontier gets split in two by means of a "cut", then
                // one of the two must become an *external frontier*
                // (though I don't have a hard proof that this must be the case...)
                //
                // Which of the two becomes the external? It's obviously the one with
                // a clockwise frontier.
                //

                // Start by splitting here, arbitrarily making the other one an internal frontier for the moment
                auto const newRegionStartEdgeIndex = mFrontierEdges[edgeIn].NextEdgeIndex;
                auto const newRegionEndEdgeIndex = mFrontierEdges[edgeOut].PrevEdgeIndex;
                FrontierId const newFrontierId = SplitIntoNewFrontier(
                    newRegionStartEdgeIndex,
                    newRegionEndEdgeIndex,
                    frontierInId,
                    FrontierType::Internal,
                    edgeIn,
                    edgeOut);

                // Now check whether the new region is counter-clockwise
                if (IsCounterClockwiseFrontier(
                    newRegionStartEdgeIndex,
                    newRegionEndEdgeIndex,
                    points))
                {
                    // The new frontier is counter-clockwise,
                    // hence internal...

                    // ...the old frontier is then external one
                    mFrontiers[frontierInId]->Type = FrontierType::External;
                }
                else
                {
                    // The new frontier is the external one
                    mFrontiers[newFrontierId]->Type = FrontierType::External;
                }
            }
        }
        else
        {
            //
            // Different internal frontiers
            //

            assert(mFrontierEdges[edgeIn].NextEdgeIndex != edgeOut); // Not connected
            assert(mFrontierEdges[edgeOut].PrevEdgeIndex != edgeIn); // Not connected

            //
            // Here we have two lobes inside a ship which will become one single, internal lobe
            //
            // It really is arbitrary which of the two frontiers takes over the other, so
            // for performance we let the longer take over the shorter
            //

            if (mFrontiers[frontierInId]->Size >= mFrontiers[frontierOutId]->Size)
            {
                // FrontierIn takes over FrontierOut

                ReplaceAndCutFrontier(
                    edgeOut, // Start
                    mFrontierEdges[edgeOut].PrevEdgeIndex, // End
                    frontierOutId, // Old
                    frontierInId, // New
                    edgeIn,
                    edgeOut);
            }
            else
            {
                // FrontierOut takes over FrontierIn

                ReplaceAndCutFrontier(
                    mFrontierEdges[edgeIn].NextEdgeIndex, // Start
                    edgeIn, // End
                    frontierInId, // Old
                    frontierOutId, // New
                    edgeIn,
                    edgeOut);
            }
        }
    }

    return true;
}

inline void Frontiers::ProcessTriangleOppositeCuspEdgeDestroy(
    ElementIndex const edge,
    ElementIndex const cuspEdgeIn,
    ElementIndex const cuspEdgeOut)
{
    assert(mEdges[edge].FrontierIndex == NoneFrontierId);
    assert(mFrontierEdges[cuspEdgeIn].NextEdgeIndex == cuspEdgeOut); // In is connected to Out
    assert(mFrontierEdges[cuspEdgeOut].PrevEdgeIndex == cuspEdgeIn); // In is connected to Out

    // Make this edge a frontier edge, undercutting the two cusp edges
    mFrontierEdges[edge].PointAIndex = mFrontierEdges[cuspEdgeIn].PointAIndex;
    mFrontierEdges[edge].PrevEdgeIndex = mFrontierEdges[cuspEdgeIn].PrevEdgeIndex;
    mFrontierEdges[mFrontierEdges[cuspEdgeIn].PrevEdgeIndex].NextEdgeIndex = edge;
    mFrontierEdges[edge].NextEdgeIndex = mFrontierEdges[cuspEdgeOut].NextEdgeIndex;
    mFrontierEdges[mFrontierEdges[cuspEdgeOut].NextEdgeIndex].PrevEdgeIndex = edge;

    assert(mEdges[cuspEdgeIn].FrontierIndex == mEdges[cuspEdgeOut].FrontierIndex); // Cusp edges' frontier is the same, by now
    auto const frontierId = mEdges[cuspEdgeIn].FrontierIndex;
    mEdges[edge].FrontierIndex = frontierId;

    // Clear cusp edges
    mEdges[cuspEdgeIn].FrontierIndex = NoneFrontierId;
    mEdges[cuspEdgeOut].FrontierIndex = NoneFrontierId;

    // Update frontier
    mFrontiers[frontierId]->StartingEdgeIndex = edge; // Just to be safe, as we've nuked the cusp edges
    assert(mFrontiers[frontierId]->Size >= 1);
    mFrontiers[frontierId]->Size -= 1; // -2 + 1
}

inline std::tuple<ElementIndex, ElementIndex> Frontiers::FindTriangleCuspOppositeFrontierEdges(
    ElementIndex const cuspPointIndex,
    ElementIndex const edgeIn,
    ElementIndex const edgeOut,
    Points const & points,
    Springs const & springs)
{
    auto const edgeInOctant = springs.GetFactoryEndpointOctant(edgeIn, cuspPointIndex);
    auto const edgeOutOctant = springs.GetFactoryEndpointOctant(edgeOut, cuspPointIndex);

    //
    // Among all the (frontier) springs connected to the cusp point, find the two closest
    // to the in and out edges, which are contained in the in^out sector
    //

    // in          |
    //  \      /   |
    //   \    /    |
    //    \  /     |
    // cp  >       |
    //    /  \     |
    //   /    \    |
    //  /      \   |
    // out         |

    // a) Find frontier spring connected to cusp point with lowest octant among all those
    //    with octant greater than edge in's
    // b) Find frontier spring connected to cusp point with highest octant among all those
    //    with octant lower than edge out's

    Octant bestNextEdgeInOctant = std::numeric_limits<Octant>::max();
    ElementIndex bestNextEdgeIn = NoneElementIndex;
    Octant bestNextEdgeOutOctant = std::numeric_limits<Octant>::min();
    ElementIndex bestNextEdgeOut = NoneElementIndex;

    Octant const octantDelta = (edgeOutOctant < edgeInOctant) ? 8 : 0;

    for (auto const & cs : points.GetFactoryConnectedSprings(cuspPointIndex).ConnectedSprings)
    {
        if (cs.SpringIndex != edgeIn
            && cs.SpringIndex != edgeOut
            && mEdges[cs.SpringIndex].FrontierIndex != NoneFrontierId)
        {
            Octant octant = springs.GetFactoryEndpointOctant(cs.SpringIndex, cuspPointIndex);
            if (octant < edgeOutOctant)
                octant += octantDelta;

            if (octant > edgeInOctant && octant < edgeOutOctant + octantDelta)
            {
                if (octant < bestNextEdgeInOctant)
                {
                    bestNextEdgeInOctant = octant;
                    bestNextEdgeIn = cs.SpringIndex;
                }

                if (octant > bestNextEdgeOutOctant)
                {
                    bestNextEdgeOutOctant = octant;
                    bestNextEdgeOut = cs.SpringIndex;
                }
            }
        }
    }

    if (bestNextEdgeIn != NoneElementIndex)
    {
        assert(bestNextEdgeOut != NoneElementIndex);
        assert(bestNextEdgeIn != bestNextEdgeOut);
    }
    else
    {
        assert(bestNextEdgeOut == NoneElementIndex);
    }

    return std::make_tuple(bestNextEdgeIn, bestNextEdgeOut);
}

template<int CuspEdgeInOrdinal, int CuspEdgeOutOrdinal>
inline bool Frontiers::ProcessTriangleCuspRestore(
    ElementIndex const edgeIn,
    ElementIndex const edgeOut,
    ElementIndex const triangleElementIndex,
    Points const & points,
    Springs const & springs,
    Triangles const & triangles)
{
    // The cusp edges are adjacent
    static_assert(
        (CuspEdgeInOrdinal <= 1 && CuspEdgeOutOrdinal == CuspEdgeInOrdinal + 1)
        || (CuspEdgeInOrdinal == 2 && CuspEdgeOutOrdinal == 0));

    // Either the cusp edges have no frontier, or they already have a frontier
    // (propagated because of an edge or a preceding cusp), in which case it's the same
    // frontier
    assert(mEdges[edgeIn].FrontierIndex == mEdges[edgeOut].FrontierIndex);

    //
    // Here we pretend to attach the cusp to an (eventual) frontier in front of it,
    // adjusting frontiers (both of the triangle and of the rest of the ship) in the
    // process.
    //
    // On exit, all of the triangle's edges will be consistent, (eventually) along with the
    // newly-connected edges.
    //
    // We return true if this was a real cusp, i.e. if the vertex became connected to a
    // frontier in front of it; false otherwise.
    //

    //
    // 1) Find frontier edges opposite to the cusp
    //

    auto [edgeInOpposite, edgeOutOpposite] = FindTriangleCuspOppositeFrontierEdges(
        triangles.GetPointIndices(triangleElementIndex)[CuspEdgeOutOrdinal],
        edgeIn,
        edgeOut,
        points,
        springs);

    if (edgeInOpposite == NoneElementIndex)
    {
        assert(edgeOutOpposite == NoneElementIndex);

        // Not a real cusp
        return false;
    }

    //
    // 2) Propagate frontiers
    //

    FrontierId const triangleFrontierId = mEdges[edgeIn].FrontierIndex;
    assert(mEdges[edgeOut].FrontierIndex == triangleFrontierId);

    FrontierId const oppositeFrontierId = mEdges[edgeInOpposite].FrontierIndex;
    assert(oppositeFrontierId != NoneElementIndex);
    assert(mEdges[edgeOutOpposite].FrontierIndex == oppositeFrontierId);

    if (triangleFrontierId == NoneElementIndex)
    {
        //
        // Triangle has no frontier...
        // ...propagate opposite frontier to triangle
        //

        int constexpr CuspEdgeMidOrdinal = (CuspEdgeOutOrdinal <= 1)
            ? CuspEdgeOutOrdinal + 1
            : 0;

        ElementIndex const edgeMid = triangles.GetSubSprings(triangleElementIndex).SpringIndices[CuspEdgeMidOrdinal];

        // OutOpposite->Out
        assert(mFrontierEdges[edgeOutOpposite].NextEdgeIndex == edgeInOpposite);
        mFrontierEdges[edgeOutOpposite].NextEdgeIndex = edgeOut;

        // Out
        mEdges[edgeOut].FrontierIndex = oppositeFrontierId;
        mFrontierEdges[edgeOut].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[CuspEdgeOutOrdinal];
        mFrontierEdges[edgeOut].NextEdgeIndex = edgeMid;
        mFrontierEdges[edgeOut].PrevEdgeIndex = edgeOutOpposite;

        // Mid
        mEdges[edgeMid].FrontierIndex = oppositeFrontierId;
        mFrontierEdges[edgeMid].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[CuspEdgeMidOrdinal];
        mFrontierEdges[edgeMid].NextEdgeIndex = edgeIn;
        mFrontierEdges[edgeMid].PrevEdgeIndex = edgeOut;

        // In
        mEdges[edgeIn].FrontierIndex = oppositeFrontierId;
        mFrontierEdges[edgeIn].PointAIndex = triangles.GetPointIndices(triangleElementIndex)[CuspEdgeInOrdinal];
        mFrontierEdges[edgeIn].NextEdgeIndex = edgeInOpposite;
        mFrontierEdges[edgeIn].PrevEdgeIndex = edgeMid;

        // In->InOpposite
        assert(mFrontierEdges[edgeInOpposite].PrevEdgeIndex == edgeOutOpposite);
        mFrontierEdges[edgeInOpposite].PrevEdgeIndex = edgeIn;

        // Update frontier
        assert(mFrontiers[oppositeFrontierId].has_value());
        mFrontiers[oppositeFrontierId]->Size += 3;
    }
    else
    {
        //
        // Triangle has a frontier (already)...
        //

        // ...running from edgeIn into edgeOut...
        assert(mFrontierEdges[edgeIn].NextEdgeIndex == edgeOut && mFrontierEdges[edgeOut].PrevEdgeIndex == edgeIn);

        // ...and obviously the same ID along the edges...
        assert(mEdges[edgeIn].FrontierIndex == mEdges[edgeOut].FrontierIndex);

        //
        // ...check all cases
        //

        if (mFrontiers[triangleFrontierId]->Type == FrontierType::Internal)
        {
            // A triangle along which there's an internal frontier may only end up touching
            // either a triangle of the same internal frontier, or an external frontier
            // (which would be the "ball-in-a-hole" case)
            assert(oppositeFrontierId == triangleFrontierId || mFrontiers[oppositeFrontierId]->Type == FrontierType::External);

            if (oppositeFrontierId == triangleFrontierId)
            {
                //
                // Triangle and cusp belong to same internal frontier...
                // ...the cusp joining separates that into *two* internal frontiers
                //

                SplitIntoNewFrontier(
                    edgeInOpposite,
                    edgeIn,
                    triangleFrontierId,
                    FrontierType::Internal,
                    edgeOutOpposite,
                    edgeOut);
            }
            else
            {
                //
                // Ball-in-a-hole case (internal frontier of a hole getting in touch with external
                // frontier of a ball in that hole)...
                // ...the external frontier of the ball gets fagocitated by the internal frontier
                //

                assert(mFrontiers[oppositeFrontierId]->Type == FrontierType::External);

                ReplaceAndJoinFrontier(
                    edgeIn,
                    edgeInOpposite,
                    edgeOutOpposite,
                    edgeOut,
                    oppositeFrontierId,
                    triangleFrontierId);
            }
        }
        else
        {
            // A triangle along which there's an external frontier may only end up touching
            // either a triangle with the same or different external frontier, or an internal frontier
            // (which would be the "ball-in-a-hole" case)

            if (mFrontiers[oppositeFrontierId]->Type == FrontierType::External)
            {
                if (oppositeFrontierId == triangleFrontierId)
                {
                    //
                    // Same external frontier...
                    //

                    //
                    //     /-------------\              |
                    //    /               \             |
                    //   /                 \            |
                    //  A-------- X --------B           |
                    //  __      /   \      __           |
                    //    \    /     \    /             |
                    //     \  /       \  /              |
                    //      \/         \/               |
                    //       C         D
                    //
                    // A->X = edgeIn
                    // X->C = edgeOut
                    //
                    // X->B = edgeInOpposite
                    // D->X = edgeOutOpposite
                    //

                    //
                    // ...one of the regions stays, the other one (the one
                    // CCW) becomes a new, internal region
                    //

                    if (IsCounterClockwiseFrontier(
                        edgeInOpposite,
                        edgeIn,
                        points))
                    {
                        // ..., A->X, X->B, ... becomes internal

                        SplitIntoNewFrontier(
                            edgeInOpposite, // start
                            edgeIn, // end
                            triangleFrontierId, // old frontier ID
                            FrontierType::Internal,
                            edgeOutOpposite,
                            edgeOut);
                    }
                    else
                    {
                        // ..., D->X, X->C, ... becomes internal

                        SplitIntoNewFrontier(
                            edgeOut, // start
                            edgeOutOpposite, // end
                            triangleFrontierId, // old frontier ID
                            FrontierType::Internal,
                            edgeIn,
                            edgeInOpposite);
                    }
                }
                else
                {
                    //
                    // Different external frontiers...
                    //

                    //
                    // ...just merge the two external frontiers; we arbitrarily choose
                    // one over the other
                    //

                    ReplaceAndJoinFrontier(
                        edgeIn,
                        edgeInOpposite,
                        edgeOutOpposite,
                        edgeOut,
                        oppositeFrontierId, // old frontier
                        triangleFrontierId); // new frontier
                }
            }
            else
            {
                //
                // Ball-in-a-hole case (internal frontier of a hole getting in touch with external
                // frontier of a ball in that hole)...
                // ...the external frontier of the ball gets fagocitated by the internal frontier
                //

                ReplaceAndJoinFrontier(
                    edgeOutOpposite,
                    edgeOut,
                    edgeIn,
                    edgeInOpposite,
                    triangleFrontierId,
                    oppositeFrontierId);
            }
        }
    }

    return true;
}

bool Frontiers::IsCounterClockwiseFrontier(
    ElementIndex startEdgeIndex,
    ElementIndex endEdgeIndex,
    Points const & points) const
{
    // Start and End belong to the same frontier
    // (but are *not* necessarily connected)
    assert(mEdges[startEdgeIndex].FrontierIndex != NoneFrontierId
        && mEdges[endEdgeIndex].FrontierIndex != NoneFrontierId
        && mEdges[startEdgeIndex].FrontierIndex == mEdges[endEdgeIndex].FrontierIndex);

    //
    // Sum (x2 − x1) * (y2 + y1) over the edges;
    //  - If the result is positive the curve is clockwise
    //  - If it's negative the curve is counter-clockwise
    //
    // Note how we use factory positions, as triangles
    // might get folded during the simulation
    //

    // end->start once, followed by start->...->end

    ElementIndex edgeIndex1 = endEdgeIndex;
    vec2f pos1 = points.GetFactoryPosition(mFrontierEdges[edgeIndex1].PointAIndex);
    ElementIndex edgeIndex2 = startEdgeIndex;
    vec2f pos2 = points.GetFactoryPosition(mFrontierEdges[edgeIndex2].PointAIndex);

    float totalArea = 0.0f;

    while (true)
    {
        totalArea += (pos2.x - pos1.x) * (pos2.y + pos1.y);

        if (edgeIndex2 == endEdgeIndex)
            break;

        // Advance
        edgeIndex1 = edgeIndex2;
        pos1 = pos2;
        edgeIndex2 = mFrontierEdges[edgeIndex2].NextEdgeIndex;
        pos2 = points.GetFactoryPosition(mFrontierEdges[edgeIndex2].PointAIndex);
    }

    return totalArea < 0.0f;
}

void Frontiers::RegeneratePointColors()
{
    static std::array<rgbColor, 4> const ExternalColors
    {
        rgbColor(0, 153, 0),
        rgbColor(26, 140, 255),
        rgbColor(26, 255, 83),
        rgbColor(26, 255, 255)
    };

    static std::array<rgbColor, 4> const InternalColors
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
        if (frontier.has_value())
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
        }
    }
}

#ifdef _DEBUG
void Frontiers::VerifyInvariants(
    Points const & points,
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
                    triangles.IsVertexSequenceInCwOrder(
                        triangleIndex,
                        mFrontierEdges[edgeIndex].PointAIndex,
                        mFrontierEdges[mFrontierEdges[edgeIndex].NextEdgeIndex].PointAIndex));

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

            Verify(
                std::find(
                    mFrontierIds.cbegin(),
                    mFrontierIds.cend(),
                    frontierIndex) != mFrontierIds.cend());

            if (frontier->Type == FrontierType::External)
            {
                Verify(
                    !IsCounterClockwiseFrontier(
                        frontier->StartingEdgeIndex,
                        mFrontierEdges[frontier->StartingEdgeIndex].PrevEdgeIndex,
                        points));
            }
            else
            {
                assert(frontier->Type == FrontierType::Internal);

                Verify(
                    IsCounterClockwiseFrontier(
                        frontier->StartingEdgeIndex,
                        mFrontierEdges[frontier->StartingEdgeIndex].PrevEdgeIndex,
                        points));
            }
        }
        else
        {
            Verify(
                std::find(
                    mFrontierIds.cbegin(),
                    mFrontierIds.cend(),
                    frontierIndex) == mFrontierIds.cend());
        }
    }

    //
    // Frontier IDs
    //

    for (auto frontierId : mFrontierIds)
    {
        Verify(frontierId < mFrontiers.size());

        Verify(mFrontiers[frontierId].has_value());
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
    // Springs
    //

    for (ElementIndex springIndex : springs)
    {
        //
        // Springs have one triangle iff they have a frontier edge
        //

        if (springs.GetSuperTriangles(springIndex).size() == 1)
        {
            Verify(mEdges[springIndex].FrontierIndex != NoneFrontierId);
        }
        else
        {
            Verify(mEdges[springIndex].FrontierIndex == NoneFrontierId);
        }
    }

    //
    // Triangles
    //

    for (ElementIndex triangleIndex : triangles)
    {
        //
        // Edges are in CCW order
        //

        auto const edgeAIndex = triangles.GetSubSprings(triangleIndex).SpringIndices[0];

        Verify(
            (triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointAIndex(edgeAIndex)
                && triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointBIndex(edgeAIndex))
            || (triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointBIndex(edgeAIndex)
                && triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointAIndex(edgeAIndex)));

        auto const edgeBIndex = triangles.GetSubSprings(triangleIndex).SpringIndices[1];

        Verify(
            (triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointAIndex(edgeBIndex)
                && triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointBIndex(edgeBIndex))
            || (triangles.GetPointBIndex(triangleIndex) == springs.GetEndpointBIndex(edgeBIndex)
                && triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointAIndex(edgeBIndex)));

        auto const edgeCIndex = triangles.GetSubSprings(triangleIndex).SpringIndices[2];

        Verify(
            (triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointAIndex(edgeCIndex)
                && triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointBIndex(edgeCIndex))
            || (triangles.GetPointCIndex(triangleIndex) == springs.GetEndpointBIndex(edgeCIndex)
                && triangles.GetPointAIndex(triangleIndex) == springs.GetEndpointAIndex(edgeCIndex)));

    }
}
#endif
}