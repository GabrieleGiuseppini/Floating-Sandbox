/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <Core/Colors.h>
#include <Core/GameTypes.h>

#include <cassert>

struct StructuralElement
{
    StructuralMaterial const * Material;

    StructuralElement()
        : Material(nullptr)
    {}

    explicit StructuralElement(StructuralMaterial const * material)
        : Material(material)
    {}

    bool operator==(StructuralElement const & other) const
    {
        return Material == other.Material;
    }
};

struct ElectricalElement
{
    ElectricalMaterial const * Material;
    ElectricalElementInstanceIndex InstanceIndex; // Different than None<->Material is instanced

    ElectricalElement()
        : Material(nullptr)
        , InstanceIndex(NoneElectricalElementInstanceIndex)
    {}

    ElectricalElement(
        ElectricalMaterial const * material,
        ElectricalElementInstanceIndex instanceIndex)
        : Material(material)
        , InstanceIndex(instanceIndex)
    {
        // Material<->InstanceIndex coherency
        assert(
            material == nullptr
            || (material->IsInstanced && instanceIndex != NoneElectricalElementInstanceIndex)
            || (!material->IsInstanced && instanceIndex == NoneElectricalElementInstanceIndex));
    }

    bool operator==(ElectricalElement const & other) const
    {
        return Material == other.Material
            && InstanceIndex == other.InstanceIndex;
    }
};

struct RopeElement
{
    ShipSpaceCoordinates StartCoords;
    ShipSpaceCoordinates EndCoords;
    StructuralMaterial const * Material;
    rgbaColor RenderColor;

    RopeElement()
        : StartCoords(0, 0)
        , EndCoords(0, 0)
        , Material(nullptr)
        , RenderColor()
    {}

    RopeElement(
        ShipSpaceCoordinates const & startCoords,
        ShipSpaceCoordinates const & endCoords,
        StructuralMaterial const * material,
        rgbaColor const & renderColor)
        : StartCoords(startCoords)
        , EndCoords(endCoords)
        , Material(material)
        , RenderColor(renderColor)
    {}

    bool operator==(RopeElement const & other) const
    {
        return StartCoords == other.StartCoords
            && EndCoords == other.EndCoords
            && Material == other.Material
            && RenderColor == other.RenderColor;
    }
};

