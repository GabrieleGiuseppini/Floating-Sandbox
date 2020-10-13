/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-09-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"
#include "RenderTypes.h"

#include <GameCore/Buffer.h>

#include <array>
#include <optional>
#include <vector>

namespace Physics
{

/*
 * The frontiers in a ship.
 *
 * This class is initialized with external and internal frontiers detected during the ship
 * load process. After that, it maintains frontiers each time a triangle is destructed or
 * restored.
 */
class Frontiers
{
public:

    // Edge metadata that is only needed for physics
    struct FrontierEdge
    {
        ElementIndex PointAIndex; // In frontier's order
        ElementIndex PointBIndex; // In frontier's order

        ElementIndex NextEdgeIndex; // Next edge in frontier's order
        ElementIndex PrevEdgeIndex; // Previous edge in frontier's order

        FrontierEdge()
            : PointAIndex(NoneElementIndex)
            , PointBIndex(NoneElementIndex)
            , NextEdgeIndex(NoneElementIndex)
            , PrevEdgeIndex(NoneElementIndex)
        {}
    };

    // Frontier metadata
    struct Frontier
    {
        FrontierType Type;
        ElementIndex StartingEdgeIndex; // Arbitrary first edge in this frontier
        ElementCount Size; // Being a closed curve, this is both # of edges and # of points

        bool IsDirtyForRendering;

        Frontier(
            FrontierType type,
            ElementIndex startingEdgeIndex,
            ElementCount size,
            bool isDirtyForRendering)
            : Type(type)
            , StartingEdgeIndex(startingEdgeIndex)
            , Size(size)
            , IsDirtyForRendering(isDirtyForRendering)
        {}
    };

public:

    Frontiers(
        size_t pointCount,
        Springs const & springs)
        : mEdgeCount(springs.GetElementCount())
        , mEdges(mEdgeCount, 0, Edge())
        , mFrontierEdges(mEdgeCount, 0, FrontierEdge())
        , mFrontiers()
        , mPointColors(pointCount, 0, Render::FrontierColor(vec3f::zero(), 0.0f))
        , mCurrentVisitSequenceNumber()
        , mIsDirtyForRendering(true)
    {}

    void AddFrontier(
        FrontierType type,
        std::vector<ElementIndex> edgeIndices,
        Springs const & springs);

    /*
     * Maintains the frontiers consistent with the removal of the specified triangle.
     * springs and points: assumed to be already consistent with the removal of the triangle.
     */
    void HandleTriangleDestroy(
        ElementIndex triangleElementIndex,
        Points const & points,
        Springs const & springs,
        Triangles const & triangles);

    void HandleTriangleRestore(ElementIndex
        triangleElementIndex,
        Springs const & springs,
        Triangles const & triangles);

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext);

#ifdef _DEBUG
    void VerifyInvariants(
        Springs const & springs,
        Triangles const & triangles) const;
#endif

public:

    ElementCount GetElementCount() const
    {
        return static_cast<ElementCount>(mFrontiers.size());
    }

private:

    // Edge metadata for internal usage only
    struct Edge
    {
        // The ID that of the frontier that this edge belongs to,
        // or NoneFrontierId if the edge does not belong to a frontier.
        FrontierId FrontierIndex;

        // The last visit sequence number
        SequenceNumber LastVisitSequenceNumber;

        explicit Edge(FrontierId frontierIndex)
            : FrontierIndex(frontierIndex)
            , LastVisitSequenceNumber()
        {}

        Edge()
            : FrontierIndex(NoneFrontierId)
            , LastVisitSequenceNumber()
        {}
    };

private:

    FrontierId CreateNewFrontier(
        FrontierType type,
        ElementIndex startingEdgeIndex = NoneElementIndex,
        ElementCount size = 0);

    void DestroyFrontier(
        FrontierId frontierId);

    template<int CuspEdgeInOrdinal, int CuspEdgeOutOrdinal>
    inline bool ProcessTriangleCuspDestroy(
        ElementIndex const edgeIn,
        ElementIndex const edgeOut,
        ElementIndex const triangleElementIndex,
        Points const & points,
        Springs const & springs,
        Triangles const & triangles);

    inline void ProcessTriangleOppositeCuspEdgeDestroy(
        ElementIndex const edge,
        ElementIndex const cuspEdgeIn,
        ElementIndex const cuspEdgeOut);

    inline FrontierId SplitIntoNewFrontier(
        ElementIndex const newFrontierStartEdgeIndex,
        ElementIndex const newFrontierEndEdgeIndex,
        FrontierId const oldFrontierId,
        FrontierType const newFrontierType,
        ElementIndex const edgeIn,
        ElementIndex const edgeOut);

    inline void ReplaceFrontier(
        ElementIndex const startEdgeIndex,
        ElementIndex const endEdgeIndex,
        FrontierId const oldFrontierId,
        FrontierId const newFrontierId,
        ElementIndex const edgeIn,
        ElementIndex const edgeOut);

    inline ElementCount PropagateFrontier(
        ElementIndex const startEdgeIndex,
        ElementIndex const endEdgeIndex,
        FrontierId const frontierId);

    bool HasRegionFrontierOfType(
        FrontierType targetFrontierType,
        ElementIndex startingPointIndex,
        Points const & points,
        Springs const & springs);

    void RegeneratePointColors();

private:

    // The total number of edges (elements, not buffer)
    size_t const mEdgeCount;

    // All the edges in the ship.
    // Cardinality: edges (==springs)
    Buffer<Edge> mEdges;

    // All the edges in the ship; only those that belong to
    // a frontier have actual significance.
    // Cardinality: edges (==springs)
    Buffer<FrontierEdge> mFrontierEdges;

    // The frontiers, indexed by frontier indices.
    // Elements in this vector do not move around.
    // Cardinality: any.
    std::vector<std::optional<Frontier>> mFrontiers;

    // Frontier coloring info.
    // Cardinality: points
    Buffer<Render::FrontierColor> mPointColors;

    // The visit number used to mark edges as visited during the
    // region/frontier check
    SequenceNumber mCurrentVisitSequenceNumber;

    bool mIsDirtyForRendering; // When true, a change has occurred and thus needs to be re-uploaded
};

}
