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
        ElementIndex PointAIndex; // First of two points in frontier's order

        ElementIndex NextEdgeIndex; // Next edge in frontier's order
        ElementIndex PrevEdgeIndex; // Previous edge in frontier's order

        FrontierEdge()
            : PointAIndex(NoneElementIndex)
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

        Frontier(
            FrontierType type,
            ElementIndex startingEdgeIndex,
            ElementCount size)
            : Type(type)
            , StartingEdgeIndex(startingEdgeIndex)
            , Size(size)
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
     * Springs and points: assumed to be already consistent with the removal of the triangle.
     */
    void HandleTriangleDestroy(
        ElementIndex triangleElementIndex,
        Points const & points,
        Springs const & springs,
        Triangles const & triangles);

    /*
     * Maintains the frontiers consistent with the restoration of the specified triangle.
     * Springs and points: assumed to be not yet consistent with the restoration of the triangle.
     */
    void HandleTriangleRestore(
        ElementIndex triangleElementIndex,
        Points const & points,
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

    static inline int PreviousEdgeOrdinal(int edgeOrdinal)
    {
        int edgeOrd = edgeOrdinal - 1;
        if (edgeOrd < 0)
            edgeOrd += 3;

        return edgeOrd;
    }

    static inline int NextEdgeOrdinal(int edgeOrdinal)
    {
        int edgeOrd = edgeOrdinal + 1;
        if (edgeOrd >= 3)
            edgeOrd -= 3;

        return edgeOrd;
    }

    FrontierId CreateNewFrontier(
        FrontierType type,
        ElementIndex startingEdgeIndex = NoneElementIndex,
        ElementCount size = 0);

    void DestroyFrontier(
        FrontierId frontierId);

    inline FrontierId SplitIntoNewFrontier(
        ElementIndex const newFrontierStartEdgeIndex,
        ElementIndex const newFrontierEndEdgeIndex,
        FrontierId const oldFrontierId,
        FrontierType const newFrontierType,
        ElementIndex const edgeIn,
        ElementIndex const edgeOut);

    inline void ReplaceAndCutFrontier(
        ElementIndex const startEdgeIndex,
        ElementIndex const endEdgeIndex,
        FrontierId const oldFrontierId,
        FrontierId const newFrontierId,
        ElementIndex const edgeIn,
        ElementIndex const edgeOut);

    inline void ReplaceAndJoinFrontier(
        ElementIndex const edgeIn,
        ElementIndex const edgeInOpposite,
        ElementIndex const edgeOutOpposite,
        ElementIndex const edgeOut,
        FrontierId const oldFrontierId,
        FrontierId const newFrontierId);

    inline ElementCount PropagateFrontier(
        ElementIndex const startEdgeIndex,
        ElementIndex const endEdgeIndex,
        FrontierId const frontierId);

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

    inline std::tuple<ElementIndex, ElementIndex> FindTriangleCuspOppositeFrontierEdges(
        ElementIndex const cuspPointIndex,
        ElementIndex const edgeIn,
        ElementIndex const edgeOut,
        Points const & points,
        Springs const & springs);

    template<int CuspEdgeInOrdinal, int CuspEdgeOutOrdinal>
    inline bool ProcessTriangleCuspRestore(
        ElementIndex const edgeIn,
        ElementIndex const edgeOut,
        ElementIndex const triangleElementIndex,
        Points const & points,
        Springs const & springs,
        Triangles const & triangles);

    bool IsCounterClockwiseFrontier(
        ElementIndex startEdgeIndex,
        ElementIndex endEdgeIndex,
        Points const & points);

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

    bool mIsDirtyForRendering; // When true, a change has occurred and thus all frontiers need to be re-uploaded
};

}
