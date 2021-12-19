/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LayerElements.h"
#include "RopeBuffer.h"

#include <GameCore/Buffer2D.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <cassert>
#include <map>

template <LayerType TLayer>
struct LayerTypeTraits
{};

//////////////////////////////////////////////////////////////////
// Structural
//////////////////////////////////////////////////////////////////

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

struct RopesLayerData
{
    RopeBuffer Buffer;

    RopesLayerData()
        : Buffer()
    {}

    explicit RopesLayerData(RopeBuffer && buffer)
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
    using material_type = StructuralMaterial;
    using layer_data_type = RopesLayerData;
};

//////////////////////////////////////////////////////////////////
// Texture
//////////////////////////////////////////////////////////////////

struct TextureLayerData
{
    ImageData<rgbaColor> Buffer;

    explicit TextureLayerData(ImageData<rgbaColor> && buffer)
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