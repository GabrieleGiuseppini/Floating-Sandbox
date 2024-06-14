/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Matrix.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

/*
 * Definitions of data structures related to ship making.
 *
 * These structures are shared between the ship factory and the ship post-processors.
 */

using ShipFactoryPointIndexMatrix = Matrix2<std::optional<ElementIndex>>;

struct ShipFactoryPoint
{
    std::optional<ShipSpaceCoordinates> DefinitionCoordinates; // From any of the layers that provide points
    vec2f Position;
    vec2f TextureCoordinates;
    rgbaColor RenderColor;
    StructuralMaterial const& StructuralMtl;
    bool IsRope;
    bool IsLeaking;
    float Strength;
    float Water;

    ElectricalMaterial const* ElectricalMtl;
    ElectricalElementInstanceIndex ElectricalElementInstanceIdx;
    std::vector<ElementIndex> ConnectedSprings1;
    std::vector<ElementIndex> ConnectedTriangles1;

    ShipFactoryPoint(
        std::optional<ShipSpaceCoordinates> definitionCoordinates,
        vec2f position,
        vec2f textureCoordinates,
        rgbaColor renderColor,
        StructuralMaterial const& structuralMtl,
        bool isRope,
        bool isLeaking,
        float strength,
        float water)
        : DefinitionCoordinates(definitionCoordinates)
        , Position(position)
        , TextureCoordinates(textureCoordinates)
        , RenderColor(renderColor)
        , StructuralMtl(structuralMtl)
        , IsRope(isRope)
        , IsLeaking(isLeaking)
        , Strength(strength)
        , Water(water)
        , ElectricalMtl(nullptr)
        , ElectricalElementInstanceIdx(NoneElectricalElementInstanceIndex)
        , ConnectedSprings1()
        , ConnectedTriangles1()
    {
    }

    void AddConnectedSpring1(ElementIndex springIndex1)
    {
        assert(!ContainsConnectedSpring(springIndex1));
        ConnectedSprings1.push_back(springIndex1);
    }

private:

    inline bool ContainsConnectedSpring(ElementIndex springIndex1) const
    {
        return std::find(
            ConnectedSprings1.cbegin(),
            ConnectedSprings1.cend(),
            springIndex1) != ConnectedSprings1.cend();
    }
};

struct ShipFactorySpring
{
    ElementIndex PointAIndex;
    uint32_t PointAAngle;

    ElementIndex PointBIndex;
    uint32_t PointBAngle;

    FixedSizeVector<ElementIndex, 2> Triangles; // Triangles that have this spring as an edge

    ElementCount CoveringTrianglesCount; // Triangles that cover this spring, not necessarily having it as an edge

    ShipFactorySpring(
        ElementIndex pointAIndex,
        uint32_t pointAAngle,
        ElementIndex pointBIndex,
        uint32_t pointBAngle)
        : PointAIndex(pointAIndex)
        , PointAAngle(pointAAngle)
        , PointBIndex(pointBIndex)
        , PointBAngle(pointBAngle)
        , Triangles()
        , CoveringTrianglesCount(0)
    {
    }

    void SwapEndpoints()
    {
        std::swap(PointAIndex, PointBIndex);
        std::swap(PointAAngle, PointBAngle);
    }
};

struct ShipFactoryTriangle
{
    std::array<ElementIndex, 3> PointIndices1;

    FixedSizeVector<ElementIndex, 3> Springs2;

    std::optional<ElementIndex> CoveredTraverseSpringIndex2;

    ShipFactoryTriangle(
        std::array<ElementIndex, 3> const & pointIndices1)
        : PointIndices1(pointIndices1)
        , Springs2()
        , CoveredTraverseSpringIndex2()
    {
    }
};

struct ShipFactoryFrontier
{
    FrontierType Type;
    std::vector<ElementIndex> EdgeIndices2;

    ShipFactoryFrontier(
        FrontierType type,
        std::vector<ElementIndex> && edgeIndices2)
        : Type(type)
        , EdgeIndices2(std::move(edgeIndices2))
    {}
};

struct ShipFactoryPointPair
{
    ElementIndex Endpoint1Index;
    ElementIndex Endpoint2Index;

    ShipFactoryPointPair(
        ElementIndex endpoint1Index,
        ElementIndex endpoint2Index)
        : Endpoint1Index(std::min(endpoint1Index, endpoint2Index))
        , Endpoint2Index(std::max(endpoint1Index, endpoint2Index))
    {}

    bool operator==(ShipFactoryPointPair const & other) const
    {
        return this->Endpoint1Index == other.Endpoint1Index
            && this->Endpoint2Index == other.Endpoint2Index;
    }

    struct Hasher
    {
        size_t operator()(ShipFactoryPointPair const & p) const
        {
            return p.Endpoint1Index * 23
                + p.Endpoint2Index;
        }
    };
};

using ShipFactoryPointPairToIndexMap = std::unordered_map<ShipFactoryPointPair, ElementIndex, ShipFactoryPointPair::Hasher>;

struct ShipFactoryFloorInfo
{
    NpcFloorKindType FloorKind;
    NpcFloorGeometryType FloorGeometry;
    ElementIndex SpringIndex;

    ShipFactoryFloorInfo(
        NpcFloorKindType floorKind,
        NpcFloorGeometryType floorGeometry,
        ElementIndex springIndex)
        : FloorKind(floorKind)
        , FloorGeometry(floorGeometry)
        , SpringIndex(springIndex)
    {}
};

using ShipFactoryFloorPlan = std::unordered_map<ShipFactoryPointPair, ShipFactoryFloorInfo, ShipFactoryPointPair::Hasher>;
