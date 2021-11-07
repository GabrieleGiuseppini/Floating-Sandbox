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

#include <cassert>
#include <map>
#include <memory>

template <LayerType TLayer>
struct LayerTypeTraits
{};

//////////////////////////////////////////////////////////////////
// Structural
//////////////////////////////////////////////////////////////////

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

struct StructuralLayerData
{
    Buffer2D<StructuralElement, struct ShipSpaceTag> Buffer;

    explicit StructuralLayerData(ShipSpaceSize shipSize)
        : Buffer(shipSize)
    {}

    explicit StructuralLayerData(Buffer2D<StructuralElement, struct ShipSpaceTag> && buffer)
        : Buffer(std::move(buffer))
    {}

    StructuralLayerData(
        ShipSpaceSize shipSize,
        StructuralElement fillElement)
        : Buffer(shipSize, fillElement)
    {}

    StructuralLayerData Clone() const
    {
        return StructuralLayerData(Buffer.Clone());
    }

    StructuralLayerData Clone(ShipSpaceRect const & region) const
    {
        return StructuralLayerData(Buffer.CloneRegion(region));
    }
};

template <>
struct LayerTypeTraits<LayerType::Structural>
{
    using material_type = StructuralMaterial;
    using layer_data_type = StructuralLayerData;
};

//////////////////////////////////////////////////////////////////
// Electrical
//////////////////////////////////////////////////////////////////

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

using ElectricalPanelMetadata = std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata>;

struct ElectricalLayerData
{
    Buffer2D<ElectricalElement, struct ShipSpaceTag> Buffer;
    ElectricalPanelMetadata Panel;

    explicit ElectricalLayerData(ShipSpaceSize shipSize)
        : Buffer(shipSize)
        , Panel()
    {}

    ElectricalLayerData(
        ShipSpaceSize shipSize,
        ElectricalPanelMetadata && panel)
        : Buffer(shipSize)
        , Panel(std::move(panel))
    {}

    ElectricalLayerData(
        Buffer2D<ElectricalElement, struct ShipSpaceTag> && buffer,
        ElectricalPanelMetadata && panel)
        : Buffer(std::move(buffer))
        , Panel(std::move(panel))
    {}

    ElectricalLayerData(
        ShipSpaceSize shipSize,
        ElectricalElement fillElement)
        : Buffer(shipSize, fillElement)
        , Panel()
    {}

    ElectricalLayerData Clone() const
    {
        ElectricalPanelMetadata panelClone = Panel;

        return ElectricalLayerData(
            Buffer.Clone(),
            std::move(panelClone));
    }

    ElectricalLayerData Clone(ShipSpaceRect const & region) const
    {
        ElectricalPanelMetadata panelClone = Panel;

        return ElectricalLayerData(
            Buffer.CloneRegion(region),
            std::move(panelClone));
    }
};

template <>
struct LayerTypeTraits<LayerType::Electrical>
{
    using material_type = ElectricalMaterial;
    using layer_data_type = ElectricalLayerData;
};

//////////////////////////////////////////////////////////////////
// Ropes
//////////////////////////////////////////////////////////////////

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

struct RopesLayerData
{
    Buffer2D<RopeElement, struct ShipSpaceTag> Buffer;

    explicit RopesLayerData(ShipSpaceSize shipSize)
        : Buffer(shipSize)
    {}

    explicit RopesLayerData(Buffer2D<RopeElement, struct ShipSpaceTag> && buffer)
        : Buffer(std::move(buffer))
    {}

    RopesLayerData Clone() const
    {
        return RopesLayerData(Buffer.Clone());
    }
};

template <>
struct LayerTypeTraits<LayerType::Ropes>
{
    using layer_data_type = RopesLayerData;
};

//////////////////////////////////////////////////////////////////
// Texture
//////////////////////////////////////////////////////////////////

struct TextureLayerData
{
    Buffer2D<rgbaColor, struct ImageTag> Buffer;

    explicit TextureLayerData(Buffer2D<rgbaColor, struct ImageTag> && buffer)
        : Buffer(std::move(buffer))
    {}

    TextureLayerData Clone() const
    {
        return TextureLayerData(Buffer.Clone());
    }
};

template <>
struct LayerTypeTraits<LayerType::Texture>
{
    using layer_data_type = TextureLayerData;
};