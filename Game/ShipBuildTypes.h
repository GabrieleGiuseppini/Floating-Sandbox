/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/GameTypes.h>
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
    std::optional<IntegralPoint> OriginalDefinitionCoordinates; // From any of the layers that provide points
    vec2f Position;
    vec2f TextureCoordinates;
    vec4f RenderColor;
    StructuralMaterial const& StructuralMtl;
    bool IsRope;
    bool IsLeaking;
    float Water;

    ElectricalMaterial const* ElectricalMtl;
    ElectricalElementInstanceIndex ElectricalElementInstanceIdx;
    std::vector<ElementIndex> ConnectedSprings;

    ShipBuildPoint(
        std::optional<IntegralPoint> originalDefinitionCoordinates,
        vec2f position,
        vec2f textureCoordinates,
        vec4f renderColor,
        StructuralMaterial const& structuralMtl,
        bool isRope,
        float water)
        : OriginalDefinitionCoordinates(originalDefinitionCoordinates)
        , Position(position)
        , TextureCoordinates(textureCoordinates)
        , RenderColor(renderColor)
        , StructuralMtl(structuralMtl)
        , IsRope(isRope)
        , IsLeaking(isRope ? true : false) // Ropes leak by default
        , Water(water)
        , ElectricalMtl(nullptr)
        , ElectricalElementInstanceIdx(NoneElectricalElementInstanceIndex)
        , ConnectedSprings()
    {
    }

    void AddConnectedSpring(ElementIndex springIndex)
    {
        assert(!ContainsConnectedSpring(springIndex));
        ConnectedSprings.push_back(springIndex);
    }

private:

    inline bool ContainsConnectedSpring(ElementIndex springIndex1) const
    {
        return std::find(
            ConnectedSprings.cbegin(),
            ConnectedSprings.cend(),
            springIndex1)
            != ConnectedSprings.cend();
    }
};
