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

template <LayerType TLayer>
struct LayerTypeTraits
{};

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

template <>
struct LayerTypeTraits<LayerType::Structural>
{
    using buffer_type = StructuralLayerBuffer;
};

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

template <>
struct LayerTypeTraits<LayerType::Electrical>
{
    using buffer_type = ElectricalLayerBuffer;
};

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

template <>
struct LayerTypeTraits<LayerType::Ropes>
{
    using buffer_type = RopesLayerBuffer;
};

using TextureLayerBuffer = Buffer2D<rgbaColor, struct ImageTag>;

template <>
struct LayerTypeTraits<LayerType::Texture>
{
    using buffer_type = TextureLayerBuffer;
};