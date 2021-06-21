/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-04-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ShipBuildTypes.h"
#include "ShipDefinition.h"

#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageSize.h>
#include <GameCore/Matrix.h>
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
 * This class contains all the logic for building a ship out of a ShipDefinition.
 */
class ShipBuilder
{
public:

    static std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData> Create(
        ShipId shipId,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::shared_ptr<TaskThreadPool> taskThreadPool,
        ShipDefinition && shipDefinition,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        GameParameters const & gameParameters);

private:

    struct RopeSegment
    {
        ElementIndex PointAIndex1;
        ElementIndex PointBIndex1;

        MaterialDatabase::ColorKey RopeColorKey;

        RopeSegment()
            : PointAIndex1(NoneElementIndex)
            , PointBIndex1(NoneElementIndex)
            , RopeColorKey()
        {
        }

        bool SetEndpoint(
            ElementIndex pointIndex1,
            MaterialDatabase::ColorKey ropeColorKey)
        {
            if (NoneElementIndex == PointAIndex1)
            {
                PointAIndex1 = pointIndex1;
                RopeColorKey = ropeColorKey;
                return true;
            }
            else if (NoneElementIndex == PointBIndex1)
            {
                PointBIndex1 = pointIndex1;
                assert(RopeColorKey == ropeColorKey);
                return true;
            }
            else
            {
                // Too many
                return false;
            }
        }
    };

    struct ShipBuildSpring
    {
        ElementIndex PointAIndex1;
        uint32_t PointAAngle;

        ElementIndex PointBIndex1;
        uint32_t PointBAngle;

        FixedSizeVector<ElementIndex, 2> SuperTriangles2; // Triangles that have this spring as an edge

        ElementCount CoveringTrianglesCount; // Triangles that cover this spring, not necessarily having is as an edge

        ShipBuildSpring(
            ElementIndex pointAIndex1,
            uint32_t pointAAngle,
            ElementIndex pointBIndex1,
            uint32_t pointBAngle)
            : PointAIndex1(pointAIndex1)
            , PointAAngle(pointAAngle)
            , PointBIndex1(pointBIndex1)
            , PointBAngle(pointBAngle)
            , SuperTriangles2()
            , CoveringTrianglesCount(0)
        {
        }
    };

    struct ShipBuildTriangle
    {
        std::array<ElementIndex, 3> PointIndices1;

        FixedSizeVector<ElementIndex, 3> SubSprings2;

        std::optional<ElementIndex> CoveredTraverseSpringIndex2;

        ShipBuildTriangle(
            std::array<ElementIndex, 3> const & pointIndices1)
            : PointIndices1(pointIndices1)
            , SubSprings2()
            , CoveredTraverseSpringIndex2()
        {
        }
    };

    struct ShipBuildFrontier
    {
        FrontierType Type;
        std::vector<ElementIndex> EdgeIndices2;

        ShipBuildFrontier(
            FrontierType type,
            std::vector<ElementIndex> && edgeIndices2)
            : Type(type)
            , EdgeIndices2(std::move(edgeIndices2))
        {}
    };

    struct BatikPixel
    {
        float Distance;
        bool IsCrack;

        BatikPixel()
            : Distance(std::numeric_limits<float>::max())
            , IsCrack(false)
        {
        }
    };

    using BatikPixelMatrix = Matrix2<BatikPixel>;

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
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1)
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
        ImageSize const & imageSize)
    {
        float const deadCenterOffsetX = 0.5f / static_cast<float>(imageSize.Width);
        float const deadCenterOffsetY = 0.5f / static_cast<float>(imageSize.Height);

        return vec2f(
            static_cast<float>(x) / static_cast<float>(imageSize.Width) + deadCenterOffsetX,
            static_cast<float>(y) / static_cast<float>(imageSize.Height) + deadCenterOffsetY);
    }

    static void AppendRopeEndpoints(
        RgbImageData const & ropeLayerImage,
        std::map<MaterialDatabase::ColorKey, RopeSegment> & ropeSegments,
        std::vector<ShipBuildPoint> & pointInfos1,
        ShipBuildPointIndexMatrix & pointIndexMatrix,
        MaterialDatabase const & materialDatabase,
        vec2f const & shipOffset);

    static void DecoratePointsWithElectricalMaterials(
        RgbImageData const & layerImage,
        std::vector<ShipBuildPoint> & pointInfos1,
        bool isDedicatedElectricalLayer,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        MaterialDatabase const & materialDatabase);

    static void AppendRopes(
        std::map<MaterialDatabase::ColorKey, RopeSegment> const & ropeSegments,
        ImageSize const & structureImageSize,
        StructuralMaterial const & ropeMaterial,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map);

    static void CreateShipElementInfos(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map,
        std::vector<ShipBuildTriangle> & triangleInfos1,
        size_t & leakingPointsCount);

    static std::vector<ShipBuildTriangle> FilterOutRedundantTriangles(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1);

    static void ConnectPointsToTriangles(
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildTriangle> const & triangleInfos1);

    static std::vector<ShipBuildFrontier> CreateShipFrontiers(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipBuildPoint> const & pointInfos2,
        std::vector<ShipBuildSpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2);

    static std::vector<ElementIndex> PropagateFrontier(
        ElementIndex startPointIndex1,
        vec2i startPointCoordinates,
        Octant startOctant,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::set<ElementIndex> & frontierEdges2,
        std::vector<ShipBuildSpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2);

    static void RandomizeStrength_Perlin(std::vector<ShipBuildPoint> & pointInfos2);

    static void RandomizeStrength_Batik(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixRegionOrigin,
        vec2i const & pointIndexMatrixRegionSize,
        std::vector<ShipBuildPoint> & pointInfos2,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipBuildSpring> & springInfos2,
        std::vector<ShipBuildFrontier> const & shipBuildFrontiers);

    static void PropagateBatikCrack(
        vec2i const & startingPoint,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        vec2i const & pointIndexMatrixOffset,
        BatikPixelMatrix & pixelMatrix);

    static void UpdateBatikDistances(BatikPixelMatrix & pixelMatrix);

    static Physics::Points CreatePoints(
        std::vector<ShipBuildPoint> const & pointInfos2,
        Physics::World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices);

    static void ConnectSpringsAndTriangles(
        std::vector<ShipBuildSpring> & springInfos2,
        std::vector<ShipBuildTriangle> & triangleInfos2);

    static Physics::Springs CreateSprings(
        std::vector<ShipBuildSpring> const & springInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters);

    static Physics::Triangles CreateTriangles(
        std::vector<ShipBuildTriangle> const & triangleInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap);

    static Physics::ElectricalElements CreateElectricalElements(
        Physics::Points const & points,
        Physics::Springs const & springs,
        std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
        std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> const & panelMetadata,
        ShipId shipId,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters);

    static Physics::Frontiers CreateFrontiers(
        std::vector<ShipBuildFrontier> const & shipBuildFrontiers,
        Physics::Points const & points,
        Physics::Springs const & springs);

#ifdef _DEBUG
    static void VerifyShipInvariants(
        Physics::Points const & points,
        Physics::Springs const & springs,
        Physics::Triangles const & triangles);
#endif

private:

    using ReorderingResults = std::tuple<std::vector<ShipBuildPoint>, std::vector<ElementIndex>, std::vector<ShipBuildSpring>, std::vector<ElementIndex>>;

    //
    // Reordering
    //

    template <int StripeLength>
    static ReorderingResults ReorderPointsAndSpringsOptimally_Stripes(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipBuildPointIndexMatrix const & pointIndexMatrix);

    template <int StripeLength>
    static void ReorderPointsAndSpringsOptimally_Stripes_Stripe(
        int y,
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<bool> & reorderedPointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        std::vector<bool> & reorderedSpringInfos1,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ShipBuildPoint> & pointInfos2,
        std::vector<ElementIndex> & pointIndexRemap,
        std::vector<ShipBuildSpring> & springInfos2,
        std::vector<ElementIndex> & springIndexRemap);

    static ReorderingResults ReorderPointsAndSpringsOptimally_Blocks(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipBuildPointIndexMatrix const & pointIndexMatrix);

    static void ReorderPointsAndSpringsOptimally_Blocks_Row(
        int y,
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<bool> & reorderedPointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        std::vector<bool> & reorderedSpringInfos1,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ShipBuildPoint> & pointInfos2,
        std::vector<ElementIndex> & pointIndexRemap,
        std::vector<ShipBuildSpring> & springInfos2,
        std::vector<ElementIndex> & springIndexRemap);

    template <int BlockSize>
    static ReorderingResults ReorderPointsAndSpringsOptimally_Tiling(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        ShipBuildPointIndexMatrix const & pointIndexMatrix);

    static std::vector<ShipBuildSpring> ReorderSpringsOptimally_TomForsyth(
        std::vector<ShipBuildSpring> const & springInfos1,
        size_t pointCount);

    static std::vector<ShipBuildTriangle> ReorderTrianglesOptimally_ReuseOptimization(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        size_t pointCount);

    static std::vector<ShipBuildTriangle> ReorderTrianglesOptimally_TomForsyth(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        size_t pointCount);

    static float CalculateACMR(std::vector<ShipBuildSpring> const & springInfos);

    static float CalculateACMR(std::vector<ShipBuildTriangle> const & triangleInfos);

    static float CalculateVertexMissRatio(std::vector<ShipBuildTriangle> const & triangleInfos);

private:

    /////////////////////////////////////////////////////////////////
    // Vertex cache optimization
    /////////////////////////////////////////////////////////////////

    // See Tom Forsyth's comments: using 32 is good enough; apparently 64 does not yield significant differences
    static constexpr size_t VertexCacheSize = 32;

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
