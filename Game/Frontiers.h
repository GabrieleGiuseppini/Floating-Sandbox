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

    void HandleTriangleDestroy(ElementIndex triangleElementIndex);

    void HandleTriangleRestore(ElementIndex triangleElementIndex);

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

    void RegeneratePointColors() const;

private:

    // All the edges in the ship; only those that belong to
    // a frontier have actual significance.
    // Cardinality: edges (==springs)
    Buffer<FrontierEdge> mFrontierEdges;

    // The frontiers, indexed by frontier ID.
    // Cardinality: any.
    std::vector<Frontier> mFrontiers;

    // Frontier coloring info.
    // Cardinality: points
    Buffer<Render::FrontierColor> mutable mPointColors;

    bool mutable mIsDirtyForRendering; // When true, a change has occurred and thus needs to be re-uploaded
};

}
