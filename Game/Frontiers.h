/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-09-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
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

        FrontierEdge()
            : PointAIndex(NoneElementIndex)
            , PointBIndex(NoneElementIndex)
            , NextEdgeIndex(NoneElementIndex)
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
        Springs const & springs,
        Triangles const & triangles);

    void AddFrontier(
        FrontierType type,
        std::vector<ElementIndex> edgeIndices,
        Springs const & springs);

    void HandleTriangleDestroy(
        ElementIndex triangleElementIndex,
        Springs const & springs,
        Triangles const & triangles);

    void HandleTriangleRestore(ElementIndex
        triangleElementIndex,
        Springs const & springs,
        Triangles const & triangles);

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

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

    struct Edge
    {
        // The ID that of the frontier that this edge belongs to,
        // or NoneFrontierId if the edge does not belong to a frontier.
        FrontierId FrontierIndex;

        explicit Edge(FrontierId frontierIndex)
            : FrontierIndex(frontierIndex)
        {}

        Edge()
            : FrontierIndex(NoneFrontierId)
        {}
    };

    struct Triangle
    {
        std::array<ElementIndex, 3u> EdgeIndices; // CW order

        Triangle(
            ElementIndex edgeAIndex,
            ElementIndex edgeBIndex,
            ElementIndex edgeCIndex)
            : EdgeIndices{ edgeAIndex , edgeBIndex , edgeCIndex }
        {
        }
    };

private:

    FrontierId CreateNewFrontier(FrontierType type);

    void RegeneratePointColors() const;

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

    // All the triangles in the ship.
    // Cardinality: triangles
    Buffer<Triangle> mTriangles;

    // The frontiers, indexed by frontier indices.
    // Elements in this vector do not move around.
    // Cardinality: any.
    std::vector<std::optional<Frontier>> mFrontiers;

    // Frontier coloring info.
    // Cardinality: points
    Buffer<Render::FrontierColor> mutable mPointColors;

    bool mutable mIsDirtyForRendering; // When true, a change has occurred and thus needs to be re-uploaded
};

}
