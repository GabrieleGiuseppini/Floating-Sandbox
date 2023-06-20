/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-04-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElectricalPanel.h"
#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ShipDefinition.h"
#include "ShipFactoryTypes.h"
#include "ShipLoadOptions.h"
#include "ShipStrengthRandomizer.h"
#include "ShipTexturizer.h"

#include <GameCore/GameTypes.h>
#include <GameCore/IndexRemap.h>

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
        ShipLoadOptions const & shipLoadOptions,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        ShipStrengthRandomizer const & shipStrengthRandomizer,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
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
            if (!pointInfos1[springInfos1[springIndex1].PointAIndex].IsRope
                || !pointInfos1[springInfos1[springIndex1].PointBIndex].IsRope)
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
        //
        // Important: we offset the texture coords by half of a "ship pixel" (which is multiple texture pixels),
        // so that the texture for a particle at ship coords (x, y) is sampled at the center of the
        // texture's quad for that particle.
        //
        // In other words, the texture is still mapped onto the whole ship space (i.e. ship_width x ship_height),
        // but given that of the ship mesh only the portion anchored at the _center_ of its corner quads is
        // visible (i.e. the (0.5 -> width-0.5) X (0.5 -> height-0.5) portion), the texture ends up with a small
        // portion of its outermost border cut off.
        //
        // With this offset, the domain of the texture coordinates is thus:
        //  Ship (0, 0) -> Texture (o, o)
        //  Ship (SW-1, SH-1) -> Texture (1.0-o, 1.0-o)
        //
        // Where (SW, SH) are the ship dimensions, and o is the offset (which is the number of pixels - in texture space - in half of a ship square).
        //

        float const sampleOffsetX = 0.5f / static_cast<float>(shipSize.width);
        float const sampleOffsetY = 0.5f / static_cast<float>(shipSize.height);

        return vec2f(
            static_cast<float>(x) / static_cast<float>(shipSize.width) + sampleOffsetX,
            static_cast<float>(y) / static_cast<float>(shipSize.height) + sampleOffsetY);
    }

    static void AppendRopes(
        RopeBuffer const & ropeBuffer,
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

    using LayoutOptimizationResults = std::tuple<std::vector<ShipFactoryPoint>, IndexRemap, std::vector<ShipFactorySpring>, IndexRemap, ElementCount>;

    static LayoutOptimizationResults OptimizeLayout(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::vector<ShipFactoryPoint> const & pointInfos1,
        std::vector<ShipFactorySpring> const & springInfos1);

    static void ConnectSpringsAndTriangles(
        std::vector<ShipFactorySpring> & springInfos2,
        std::vector<ShipFactoryTriangle> & triangleInfos2,
        IndexRemap const & pointIndexRemap);

    static std::vector<ShipFactoryFrontier> CreateShipFrontiers(
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        IndexRemap const & pointIndexRemap,
        std::vector<ShipFactoryPoint> const & pointInfos2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        IndexRemap const & springIndexRemap);

    static std::vector<ElementIndex> PropagateFrontier(
        ElementIndex startPointIndex1,
        vec2i startPointCoordinates,
        Octant startOctant,
        ShipFactoryPointIndexMatrix const & pointIndexMatrix,
        std::set<ElementIndex> & frontierEdges2,
        std::vector<ShipFactorySpring> const & springInfos2,
        PointPairToIndexMap const & pointPairToSpringIndex1Map,
        IndexRemap const & springIndexRemap);

    static Physics::Points CreatePoints(
        std::vector<ShipFactoryPoint> const & pointInfos2,
        Physics::World & parentWorld,
        MaterialDatabase const & materialDatabase,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters,
        std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices,
        ShipPhysicsData const & physicsData);

    static Physics::Springs CreateSprings(
        std::vector<ShipFactorySpring> const & springInfos2,
        ElementCount perfectSquareCount,
        Physics::Points & points,
        Physics::World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters);

    static Physics::Triangles CreateTriangles(
        std::vector<ShipFactoryTriangle> const & triangleInfos2,
        Physics::Points & points,
        IndexRemap const & pointIndexRemap);

    static Physics::ElectricalElements CreateElectricalElements(
        Physics::Points const & points,
        std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
        ElectricalPanel const & electricalPanel,
        bool flipH,
        bool flipV,
        bool rotate90CW,
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

    /////////////////////////////////////////////////////////////////
    // Vertex cache optimization
    /////////////////////////////////////////////////////////////////

    static float CalculateACMR(std::vector<ShipFactorySpring> const & springInfos);

    static float CalculateACMR(std::vector<ShipFactoryTriangle> const & triangleInfos);

    // See Tom Forsyth's comments: using 32 is good enough; apparently 64 does not yield significant differences
    static size_t constexpr VertexCacheSize = 32;

    template <size_t Size>
    class TestLRUVertexCache
    {
    public:

        bool UseVertex(size_t vertexIndex);

    private:

        std::list<size_t> mEntries;
    };
};
