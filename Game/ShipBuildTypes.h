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
 * Definitions of data structures related to ship building.
 *
 * These structures are shared between the ship builder and the ship post-processors.
 */

using ShipBuildPointIndexMatrix = Matrix2<std::optional<ElementIndex>>;

struct ShipBuildPoint
{
    std::optional<vec2i> OriginalDefinitionCoordinates; // In original image (y=0 @ bottom), from any of the layers that provide points
    std::optional<IntegralPoint> UserCoordinates; // For displaying messages to users
    vec2f Position;
    vec2f TextureCoordinates;
    vec4f RenderColor;
    StructuralMaterial const& StructuralMtl;
    bool IsRope;
    bool IsLeaking;
    float Strength;
    float Water;

    ElectricalMaterial const* ElectricalMtl;
    ElectricalElementInstanceIndex ElectricalElementInstanceIdx;
    std::vector<ElementIndex> ConnectedSprings1;
    std::vector<ElementIndex> ConnectedTriangles1;

    ShipBuildPoint(
        std::optional<vec2i> originalDefinitionCoordinates,
        std::optional<IntegralPoint> userCoordinates,
        vec2f position,
        vec2f textureCoordinates,
        vec4f renderColor,
        StructuralMaterial const& structuralMtl,
        bool isRope,
        float strength,
        float water)
        : OriginalDefinitionCoordinates(originalDefinitionCoordinates)
        , UserCoordinates(userCoordinates)
        , Position(position)
        , TextureCoordinates(textureCoordinates)
        , RenderColor(renderColor)
        , StructuralMtl(structuralMtl)
        , IsRope(isRope)
        , IsLeaking(isRope ? true : false) // Ropes leak by default
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
