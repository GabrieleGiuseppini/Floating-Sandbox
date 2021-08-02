/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-04-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ResourceLocator.h"
#include "ShipBuildTypes.h"
#include "ShipDefinition.h"
#include "ShipStrengthRandomizer.h"
#include "ShipTexturizer.h"

#include <GameCore/GameTypes.h>
#include <GameCore/ImageSize.h>
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
 * This class contains all the logic for building a ship out of a ShipDefinition, including
 * ship post-processing.
 */
class ShipBuilder
{
public:

    ShipBuilder(ResourceLocator const & resourceLocator);

    void VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const;

    std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData> Create(
        ShipId shipId,
        Physics::World & parentWorld,
        ShipDefinition && shipDefinition,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::shared_ptr<TaskThreadPool> taskThreadPool,
        GameParameters const & gameParameters) const;

    ShipAutoTexturizationSettings const & GetAutoTexturizationSharedSettings() const { return mShipTexturizer.GetSharedSettings(); }

    ShipAutoTexturizationSettings & GetAutoTexturizationSharedSettings() { return mShipTexturizer.GetSharedSettings(); }

    void SetAutoTexturizationSharedSettings(ShipAutoTexturizationSettings const & sharedSettings) { mShipTexturizer.SetSharedSettings(sharedSettings); }
    bool GetDoForceAutoTexturizationSharedSettingsOntoShipSettings() const { return mShipTexturizer.GetDoForceSharedSettingsOntoShipSettings(); }
    void SetDoForceAutoTexturizationSharedSettingsOntoShipSettings(bool value) { mShipTexturizer.SetDoForceSharedSettingsOntoShipSettings(value); }

    float GetShipStrengthRandomizationDensityAdjustment() const { return mShipStrengthRandomizer.GetDensityAdjustment(); }
    void SetShipStrengthRandomizationDensityAdjustment(float value) { mShipStrengthRandomizer.SetDensityAdjustment(value); }
    float GetShipStrengthRandomizationExtent() const { return mShipStrengthRandomizer.GetRandomizationExtent(); }
    void SetShipStrengthRandomizationExtent(float value) { mShipStrengthRandomizer.SetRandomizationExtent(value); }

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

    inline bool IsConnectedToNonRopePoints(
        ElementIndex pointIndex,
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1) const
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
    inline vec2f MakeTextureCoordinates(
        CoordType x,
        CoordType y,
        ImageSize const & imageSize) const
    {
        float const deadCenterOffsetX = 0.5f / static_cast<float>(imageSize.Width);
        float const deadCenterOffsetY = 0.5f / static_cast<float>(imageSize.Height);

        return vec2f(
            static_cast<float>(x) / static_cast<float>(imageSize.Width) + deadCenterOffsetX,
            static_cast<float>(y) / static_cast<float>(imageSize.Height) + deadCenterOffsetY);
    }

    void AppendRopeEndpoints(
        RgbImageData const & ropeLayerImage,
        std::map<MaterialDatabase::ColorKey, RopeSegment> & ropeSegments,
        std::vector<ShipBuildPoint> & pointInfos1,
        ShipBuildPointIndexMatrix & pointIndexMatrix,
        MaterialDatabase const & materialDatabase,
        vec2f const & shipOffset) const;

    void DecoratePointsWithElectricalMaterials(
        RgbImageData const & layerImage,
        std::vector<ShipBuildPoint> & pointInfos1,
        bool isDedicatedElectricalLayer,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        MaterialDatabase const & materialDatabase) const;

    void AppendRopes(
        std::map<MaterialDatabase::ColorKey, RopeSegment> const & ropeSegments,
        ImageSize const & structureImageSize,
        StructuralMaterial const & ropeMaterial,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map) const;

    void CreateShipElementInfos(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map,
        std::vector<ShipBuildTriangle> & triangleInfos1,
        size_t & leakingPointsCount) const;

    std::vector<ShipBuildTriangle> FilterOutRedundantTriangles(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1) const;

    void ConnectPointsToTriangles(
        std::vector<ShipBuildPoint> & pointInfos1,
        std::vector<ShipBuildTriangle> const & triangleInfos1) const;

    std::vector<ShipBuildFrontier> CreateShipFrontiers(
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipBuildPoint> const & pointInfos2,
        std::vector<ShipBuildSpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2) const;

    std::vector<ElementIndex> PropagateFrontier(
        ElementIndex startPointIndex1,
        vec2i startPointCoordinates,
        Octant startOctant,
        ShipBuildPointIndexMatrix const & pointIndexMatrix,
        std::set<ElementIndex> & frontierEdges2,
        std::vector<ShipBuildSpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2) const;

    Physics::Points CreatePoints(
        std::vector<ShipBuildPoint> const & pointInfos2,
        Physics::World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices,
        ShipPhysicsData const & physicsData) const;

    void ConnectSpringsAndTriangles(
        std::vector<ShipBuildSpring> & springInfos2,
        std::vector<ShipBuildTriangle> & triangleInfos2) const;

    Physics::Springs CreateSprings(
        std::vector<ShipBuildSpring> const & springInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters) const;

    Physics::Triangles CreateTriangles(
        std::vector<ShipBuildTriangle> const & triangleInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap) const;

    Physics::ElectricalElements CreateElectricalElements(
        Physics::Points const & points,
        Physics::Springs const & springs,
        std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
        std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> const & panelMetadata,
        ShipId shipId,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters) const;

    Physics::Frontiers CreateFrontiers(
        std::vector<ShipBuildFrontier> const & shipBuildFrontiers,
        Physics::Points const & points,
        Physics::Springs const & springs) const;

#ifdef _DEBUG
    void VerifyShipInvariants(
        Physics::Points const & points,
        Physics::Springs const & springs,
        Physics::Triangles const & triangles) const;
#endif

private:

    using ReorderingResults = std::tuple<std::vector<ShipBuildPoint>, std::vector<ElementIndex>, std::vector<ShipBuildSpring>, std::vector<ElementIndex>>;

    //
    // Reordering
    //

    template <int StripeLength>
    ReorderingResults ReorderPointsAndSpringsOptimally_Stripes(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipBuildPointIndexMatrix const & pointIndexMatrix) const;

    template <int StripeLength>
    void ReorderPointsAndSpringsOptimally_Stripes_Stripe(
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
        std::vector<ElementIndex> & springIndexRemap) const;

    ReorderingResults ReorderPointsAndSpringsOptimally_Blocks(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipBuildPointIndexMatrix const & pointIndexMatrix) const;

    void ReorderPointsAndSpringsOptimally_Blocks_Row(
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
        std::vector<ElementIndex> & springIndexRemap) const;

    template <int BlockSize>
    ReorderingResults ReorderPointsAndSpringsOptimally_Tiling(
        std::vector<ShipBuildPoint> const & pointInfos1,
        std::vector<ShipBuildSpring> const & springInfos1,
        ShipBuildPointIndexMatrix const & pointIndexMatrix) const;

    std::vector<ShipBuildSpring> ReorderSpringsOptimally_TomForsyth(
        std::vector<ShipBuildSpring> const & springInfos1,
        size_t pointCount) const;

    std::vector<ShipBuildTriangle> ReorderTrianglesOptimally_ReuseOptimization(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        size_t pointCount) const;

    std::vector<ShipBuildTriangle> ReorderTrianglesOptimally_TomForsyth(
        std::vector<ShipBuildTriangle> const & triangleInfos1,
        size_t pointCount) const;

    static float CalculateACMR(std::vector<ShipBuildSpring> const & springInfos);

    static float CalculateACMR(std::vector<ShipBuildTriangle> const & triangleInfos);

    static float CalculateVertexMissRatio(std::vector<ShipBuildTriangle> const & triangleInfos);

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
    std::vector<size_t> ReorderOptimally(
        std::vector<VertexData> & vertexData,
        std::vector<ElementData> & elementData) const;

    void AddVertexToCache(
        size_t vertexIndex,
        ModelLRUVertexCache & cache) const;

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

private:

    ShipStrengthRandomizer mShipStrengthRandomizer;
    ShipTexturizer mShipTexturizer;
};
