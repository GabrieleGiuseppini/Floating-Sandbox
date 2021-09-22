/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/Buffer2D.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>

struct StructuralElement
{
    StructuralMaterial const * Material;
    rgbaColor RenderColor;

    StructuralElement()
        : Material(nullptr)
        , RenderColor()
    {}

    explicit StructuralElement(
        StructuralMaterial const * material,
        rgbaColor const & renderColor)
        : Material(material)
        , RenderColor(renderColor)
    {}
};

using StructuralLayerBuffer = Buffer2D<StructuralElement, struct ShipSpaceTag>;

struct ElectricalElement
{
    ElectricalMaterial const * Material;
    ElectricalElementInstanceIndex InstanceIndex;

    ElectricalElement()
        : Material(nullptr)
        , InstanceIndex(NoneElectricalElementInstanceIndex)
    {}

    ElectricalElement(
        ElectricalMaterial const * material,
        ElectricalElementInstanceIndex instanceIndex)
        : Material(material)
        , InstanceIndex(instanceIndex)
    {}
};

using ElectricalLayerBuffer = Buffer2D<ElectricalElement, struct ShipSpaceTag>;

struct RopeElement
{
    StructuralMaterial const * Material;
    RopeId Id;
    rgbaColor RenderColor;

    RopeElement()
        : Material(nullptr)
        , Id(NoneRopeId)
        , RenderColor()
    {}

    RopeElement(
        StructuralMaterial const * material,
        RopeId id,
        rgbaColor const & renderColor)
        : Material(material)
        , Id(id)
        , RenderColor(renderColor)
    {}
};

using RopesLayerBuffer = Buffer2D<RopeElement, struct ShipSpaceTag>;

using TextureLayerBuffer = Buffer2D<rgbaColor, struct ImageTag>;