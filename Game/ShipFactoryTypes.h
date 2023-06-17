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

    FixedSizeVector<ElementIndex, 2> SuperTriangles; // Triangles that have this spring as an edge

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
        , SuperTriangles()
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

    FixedSizeVector<ElementIndex, 3> SubSprings2;

    std::optional<ElementIndex> CoveredTraverseSpringIndex2;

    ShipFactoryTriangle(
        std::array<ElementIndex, 3> const & pointIndices1)
        : PointIndices1(pointIndices1)
        , SubSprings2()
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
