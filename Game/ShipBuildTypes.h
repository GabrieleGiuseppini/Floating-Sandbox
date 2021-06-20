/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Matrix.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <vector>

/*
 * Definitions of data structures related to ship building.
 *
 * These structures are shared between the ship builder and the ship texturizer.
 */

using ShipBuildPointIndexMatrix = std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]>;

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
