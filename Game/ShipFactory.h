/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-04-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ShipDefinition.h"
#include "ShipFactoryTypes.h"
#include "ShipStrengthRandomizer.h"
#include "ShipTexturizer.h"

#include <GameCore/GameTypes.h>
#include <GameCore/TaskThreadPool.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

/*
 * This class contains all the logic for creating a ship out of a ShipDefinition, including
 * ship post-processing.
 */
class ShipFactory
{
public:

    static std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData> Create(
        ShipId shipId,
        Physics::World & parentWorld,
        ShipDefinition && shipDefinition,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        ShipStrengthRandomizer const & shipStrengthRandomizer,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::shared_ptr<TaskThreadPool> taskThreadPool,
        GameParameters const & gameParameters);

private:

    /////////////////////////////////////////////////////////////////
    // Building helpers
    /////////////////////////////////////////////////////////////////

    struct PointPair
    {
        ElementIndex Endpoint1Index;
        ElementIndex Endpoint2Index;

        PointPair(
            ElementIndex endpoint1Index,
            ElementIndex endpoint2Index)
            : Endpoint1Index(std::min(endpoint1Index, endpoint2Index))
            , Endpoint2Index(std::max(endpoint1Index, endpoint2Index))
        {}

        bool operator==(PointPair const & other) const
        {
            return this->Endpoint1Index == other.Endpoint1Index
                && this->Endpoint2Index == other.Endpoint2Index;
        }

        struct Hasher
        {
            size_t operator()(PointPair const & p) const
            {
                return p.Endpoint1Index * 23
                    + p.Endpoint2Index;
            }
        };
    };

    using PointPairToIndexMap = std::unordered_map<PointPair, ElementIndex, PointPair::Hasher>;

    static inline bool IsConnectedToNonRopePoints(
        ElementIndex pointIndex,
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1)
    {
        for (auto const springIndex1 : pointInfos1[pointIndex].ConnectedSprings1)
        {
            if (!pointInfos1[springInfos1[springIndex1].PointAIndex1].IsRope
                || !pointInfos1[springInfos1[springIndex1].PointBIndex1].IsRope)
            {
                return true;
            }
        }

        return false;
    }

    template <typename CoordType>
    static inline vec2f MakeTextureCoordinates(
        CoordType x,
        CoordType y,
        ShipSpaceSize const & shipSize)
    {
        float const deadCenterOffsetX = 0.5f / static_cast<float>(shipSize.width);
        float const deadCenterOffsetY = 0.5f / static_cast<float>(shipSize.height);

        return vec2f(
            static_cast<float>(x) / static_cast<float>(shipSize.width) + deadCenterOffsetX,
            static_cast<float>(y) / static_cast<float>(shipSize.height) + deadCenterOffsetY);
    }

    static void AppendRopes(
        std::vector<RopeElement> const & ropeElements,
        ShipSpaceSize const & shipSize,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map);

    static void CreateShipElementInfos(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map,
        std::vector<ShipFactoryTriangle> & triangleInfos1,
        size_t & leakingPointsCount);

    static std::vector<ShipFactoryTriangle> FilterOutRedundantTriangles(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1);

    static void ConnectPointsToTriangles(
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactoryTriangle> const & triangleInfos1);

    static std::vector<ShipFactoryFrontier> CreateShipFrontiers(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipFactoryPoint> const & pointInfos2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2);

    static std::vector<ElementIndex> PropagateFrontier(
        ElementIndex startPointIndex1,
        vec2i startPointCoordinates,
        Octant startOctant,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::set<ElementIndex> & frontierEdges2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2);

    static Physics::Points CreatePoints(
        std::vector<ShipFactoryPoint> const & pointInfos2,
        Physics::World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices,
        ShipPhysicsData const & physicsData);

    static void ConnectSpringsAndTriangles(
        std::vector<ShipFactorySpring> & springInfos2,
        std::vector<ShipFactoryTriangle> & triangleInfos2);

    static Physics::Springs CreateSprings(
        std::vector<ShipFactorySpring> const & springInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters);

    static Physics::Triangles CreateTriangles(
        std::vector<ShipFactoryTriangle> const & triangleInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap);

    static Physics::ElectricalElements CreateElectricalElements(
        Physics::Points const & points,
        Physics::Springs const & springs,
        std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
        ElectricalPanelMetadata const & panelMetadata,
        ShipId shipId,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters);

    static Physics::Frontiers CreateFrontiers(
        std::vector<ShipFactoryFrontier> const & shipFactoryFrontiers,
        Physics::Points const & points,
        Physics::Springs const & springs);

#ifdef _DEBUG
    static void VerifyShipInvariants(
        Physics::Points const & points,
        Physics::Springs const & springs,
        Physics::Triangles const & triangles);
#endif

private:

    using ReorderingResults = std::tuple<std::vector<ShipFactoryPoint>, std::vector<ElementIndex>, std::vector<ShipFactorySpring>, std::vector<ElementIndex>>;

    //
    // Reordering
    //

    template <int StripeLength>
    static ReorderingResults ReorderPointsAndSpringsOptimally_Stripes(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix);

    template <int StripeLength>
    static void ReorderPointsAndSpringsOptimally_Stripes_Stripe(
        int y,
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<bool> & reorderedPointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        std::vector<bool> & reorderedSpringInfos1,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ShipFactoryPoint> & pointInfos2,
        std::vector<ElementIndex> & pointIndexRemap,
        std::vector<ShipFactorySpring> & springInfos2,
        std::vector<ElementIndex> & springIndexRemap);

    static ReorderingResults ReorderPointsAndSpringsOptimally_Blocks(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix);

    static void ReorderPointsAndSpringsOptimally_Blocks_Row(
        int y,
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<bool> & reorderedPointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        std::vector<bool> & reorderedSpringInfos1,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ShipFactoryPoint> & pointInfos2,
        std::vector<ElementIndex> & pointIndexRemap,
        std::vector<ShipFactorySpring> & springInfos2,
        std::vector<ElementIndex> & springIndexRemap);

    template <int BlockSize>
    static ReorderingResults ReorderPointsAndSpringsOptimally_Tiling(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix);

    static std::vector<ShipFactorySpring> ReorderSpringsOptimally_TomForsyth(
        std::vector<ShipFactorySpring> const & springInfos1,
        size_t pointCount);

    static std::vector<ShipFactoryTriangle> ReorderTrianglesOptimally_ReuseOptimization(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        size_t pointCount);

    static std::vector<ShipFactoryTriangle> ReorderTrianglesOptimally_TomForsyth(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        size_t pointCount);

    static float CalculateACMR(std::vector<ShipFactorySpring> const & springInfos);

    static float CalculateACMR(std::vector<ShipFactoryTriangle> const & triangleInfos);

    static float CalculateVertexMissRatio(std::vector<ShipFactoryTriangle> const & triangleInfos);

private:

    /////////////////////////////////////////////////////////////////
    // Vertex cache optimization
    /////////////////////////////////////////////////////////////////

    // See Tom Forsyth's comments: using 32 is good enough; apparently 64 does not yield significant differences
    static size_t constexpr VertexCacheSize = 32;

    using ModelLRUVertexCache = std::list<size_t>;

    struct VertexData
    {
        int32_t CachePosition;                          // Position in cache; -1 if not in cache
        float CurrentScore;                             // Current score of the vertex
        std::vector<size_t> RemainingElementIndices;    // Indices of not yet drawn elements that use this vertex

        VertexData()
            : CachePosition(-1)
            , CurrentScore(0.0f)
            , RemainingElementIndices()
        {
        }
    };

    struct ElementData
    {
        bool HasBeenDrawn;                  // Set to true when the element has been drawn already
        float CurrentScore;                 // Current score of the element - sum of its vertices' scores
        std::vector<size_t> VertexIndices;  // Indices of vertices in this element

        ElementData()
            : HasBeenDrawn(false)
            , CurrentScore(0.0f)
            , VertexIndices()
        {
        }
    };

    template <size_t VerticesInElement>
    static std::vector<size_t> ReorderOptimally(
        std::vector<VertexData> & vertexData,
        std::vector<ElementData> & elementData);

    static void AddVertexToCache(
        size_t vertexIndex,
        ModelLRUVertexCache & cache);

    template <size_t VerticesInElement>
    static float CalculateVertexScore(VertexData const & vertexData);

    template <size_t Size>
    class TestLRUVertexCache
    {
    public:

        bool UseVertex(size_t vertexIndex);

        std::optional<size_t> GetCachePosition(size_t vertexIndex);

    private:

        std::list<size_t> mEntries;
    };
};
