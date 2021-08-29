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
#include "ShipDefinition.h"
#include "ShipFactoryTypes.h"
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
 * This class contains all the logic for creating a ship out of a ShipDefinition, including
 * ship post-processing.
 */
class ShipFactory
{
public:

    ShipFactory(
        MaterialDatabase const & materialDatabase,
        ResourceLocator const & resourceLocator);

    std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData> Create(
        ShipId shipId,
        Physics::World & parentWorld,
        ShipDefinition && shipDefinition,
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
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1) const
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
        std::vector<ShipFactoryPoint> & pointInfos1,
        ShipFactoryPointIndexMatrix & pointIndexMatrix,
        MaterialDatabase const & materialDatabase,
        vec2f const & shipOffset) const;

    void DecoratePointsWithElectricalMaterials(
        RgbImageData const & layerImage,
        std::vector<ShipFactoryPoint> & pointInfos1,
        bool isDedicatedElectricalLayer,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        MaterialDatabase const & materialDatabase) const;

    void AppendRopes(
        std::map<MaterialDatabase::ColorKey, RopeSegment> const & ropeSegments,
        ImageSize const & structureImageSize,
        StructuralMaterial const & ropeMaterial,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map) const;

    void CreateShipElementInfos(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> & springInfos1,
        PointPairToIndexMap & pointPairToSpringIndex1Map,
        std::vector<ShipFactoryTriangle> & triangleInfos1,
        size_t & leakingPointsCount) const;

    std::vector<ShipFactoryTriangle> FilterOutRedundantTriangles(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1) const;

    void ConnectPointsToTriangles(
        std::vector<ShipFactoryPoint> & pointInfos1,
        std::vector<ShipFactoryTriangle> const & triangleInfos1) const;

    std::vector<ShipFactoryFrontier> CreateShipFrontiers(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ElementIndex> const & pointIndexRemap2,
        std::vector<ShipFactoryPoint> const & pointInfos2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2) const;

    std::vector<ElementIndex> PropagateFrontier(
        ElementIndex startPointIndex1,
        vec2i startPointCoordinates,
        Octant startOctant,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::set<ElementIndex> & frontierEdges2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        std::vector<ElementIndex> const & springIndexRemap2) const;

    Physics::Points CreatePoints(
        std::vector<ShipFactoryPoint> const & pointInfos2,
        Physics::World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices,
        ShipPhysicsData const & physicsData) const;

    void ConnectSpringsAndTriangles(
        std::vector<ShipFactorySpring> & springInfos2,
        std::vector<ShipFactoryTriangle> & triangleInfos2) const;

    Physics::Springs CreateSprings(
        std::vector<ShipFactorySpring> const & springInfos2,
        Physics::Points & points,
        std::vector<ElementIndex> const & pointIndexRemap,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters) const;

    Physics::Triangles CreateTriangles(
        std::vector<ShipFactoryTriangle> const & triangleInfos2,
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
        std::vector<ShipFactoryFrontier> const & shipFactoryFrontiers,
        Physics::Points const & points,
        Physics::Springs const & springs) const;

#ifdef _DEBUG
    void VerifyShipInvariants(
        Physics::Points const & points,
        Physics::Springs const & springs,
        Physics::Triangles const & triangles) const;
#endif

private:

    using ReorderingResults = std::tuple<std::vector<ShipFactoryPoint>, std::vector<ElementIndex>, std::vector<ShipFactorySpring>, std::vector<ElementIndex>>;

    //
    // Reordering
    //

    template <int StripeLength>
    ReorderingResults ReorderPointsAndSpringsOptimally_Stripes(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix) const;

    template <int StripeLength>
    void ReorderPointsAndSpringsOptimally_Stripes_Stripe(
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
        std::vector<ElementIndex> & springIndexRemap) const;

    ReorderingResults ReorderPointsAndSpringsOptimally_Blocks(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix) const;

    void ReorderPointsAndSpringsOptimally_Blocks_Row(
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
        std::vector<ElementIndex> & springIndexRemap) const;

    template <int BlockSize>
    ReorderingResults ReorderPointsAndSpringsOptimally_Tiling(
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix) const;

    std::vector<ShipFactorySpring> ReorderSpringsOptimally_TomForsyth(
        std::vector<ShipFactorySpring> const & springInfos1,
        size_t pointCount) const;

    std::vector<ShipFactoryTriangle> ReorderTrianglesOptimally_ReuseOptimization(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        size_t pointCount) const;

    std::vector<ShipFactoryTriangle> ReorderTrianglesOptimally_TomForsyth(
        std::vector<ShipFactoryTriangle> const & triangleInfos1,
        size_t pointCount) const;

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

    MaterialDatabase const & mMaterialDatabase;
    ShipStrengthRandomizer mShipStrengthRandomizer;
    ShipTexturizer mShipTexturizer;
};
