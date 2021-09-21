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

    StructuralElement()
        : Material(nullptr)
    {}

    explicit StructuralElement(StructuralMaterial const * material)
        : Material(material)
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

    RopeElement()
        : Material(nullptr)
        , Id(NoneRopeId)
    {}

    RopeElement(
        StructuralMaterial const * material,
        RopeId id)
        : Material(material)
        , Id(id)
    {}
};

using RopesLayerBuffer = Buffer2D<RopeElement, struct ShipSpaceTag>;

using TextureLayerBuffer = Buffer2D<rgbaColor, struct ImageTag>;