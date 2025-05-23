/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElectricalPanel.h"
#include "LayerElements.h"
#include "RopeBuffer.h"

#include <Core/Buffer2D.h>
#include <Core/Colors.h>
#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <cassert>
#include <memory>

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

    StructuralLayerData CloneRegion(ShipSpaceRect const & region) const
    {
        return StructuralLayerData(Buffer.CloneRegion(region));
    }

    StructuralLayerData MakeRegionBackup(ShipSpaceRect const & region) const
    {
        return StructuralLayerData(Buffer.CloneRegion(region));
    }

    void RestoreRegionBackup(
        StructuralLayerData && sourceRegionBackup,
        ShipSpaceCoordinates const & position)
    {
        Buffer.BlitFromRegion(
            sourceRegionBackup.Buffer,
            ShipSpaceRect(
                ShipSpaceCoordinates(0, 0),
                sourceRegionBackup.Buffer.Size),
            position);
    }

    void Trim(ShipSpaceRect const & rect)
    {
        Buffer.Trim(rect);
    }

    StructuralLayerData MakeReframed(
        ShipSpaceSize const & newSize, // Final size
        ShipSpaceCoordinates const & originOffset, // Position in final buffer of original {0, 0}
        StructuralElement const & fillerValue) const;
};

template <>
struct LayerTypeTraits<LayerType::Structural>
{
    using material_type = StructuralMaterial;
    using buffer_type = Buffer2D<StructuralElement, struct ShipSpaceTag>;
    using layer_data_type = StructuralLayerData;
};

//////////////////////////////////////////////////////////////////
// Electrical
//////////////////////////////////////////////////////////////////

struct ElectricalLayerData
{
    Buffer2D<ElectricalElement, struct ShipSpaceTag> Buffer;
    ElectricalPanel Panel;

    explicit ElectricalLayerData(ShipSpaceSize shipSize)
        : Buffer(shipSize)
        , Panel()
    {}

    ElectricalLayerData(
        ShipSpaceSize shipSize,
        ElectricalPanel && panel)
        : Buffer(shipSize)
        , Panel(std::move(panel))
    {}

    ElectricalLayerData(
        Buffer2D<ElectricalElement, struct ShipSpaceTag> && buffer,
        ElectricalPanel && panel)
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
        ElectricalPanel panelClone = Panel;

        return ElectricalLayerData(
            Buffer.Clone(),
            std::move(panelClone));
    }

    ElectricalLayerData CloneRegion(ShipSpaceRect const & region) const
    {
        return ElectricalLayerData(
            Buffer.CloneRegion(region),
            MakeTrimmedPanel(Panel, region));
    }

    ElectricalLayerData MakeRegionBackup(ShipSpaceRect const & region) const
    {
        ElectricalPanel panelClone = Panel;

        return ElectricalLayerData(
            Buffer.CloneRegion(region),
            std::move(panelClone)); // Panel is whole
    }

    void RestoreRegionBackup(
        ElectricalLayerData && sourceRegionBackup,
        ShipSpaceCoordinates const & position)
    {
        Buffer.BlitFromRegion(
            sourceRegionBackup.Buffer,
            ShipSpaceRect(
                ShipSpaceCoordinates(0, 0),
                sourceRegionBackup.Buffer.Size),
            position);

        Panel = std::move(sourceRegionBackup.Panel); // Panel is whole
    }

    void Trim(ShipSpaceRect const & rect)
    {
        Panel = MakeTrimmedPanel(Panel, rect);
        Buffer.Trim(rect);
    }

    ElectricalLayerData MakeReframed(
        ShipSpaceSize const & newSize, // Final size
        ShipSpaceCoordinates const & originOffset, // Position in final buffer of original {0, 0}
        ElectricalElement const & fillerValue) const;

private:

    ElectricalPanel MakeTrimmedPanel(
        ElectricalPanel const & panel,
        ShipSpaceRect const & rect) const;
};

template <>
struct LayerTypeTraits<LayerType::Electrical>
{
    using material_type = ElectricalMaterial;
    using buffer_type = Buffer2D<ElectricalElement, struct ShipSpaceTag>;
    using layer_data_type = ElectricalLayerData;
};

//////////////////////////////////////////////////////////////////
// Ropes
//////////////////////////////////////////////////////////////////

struct RopesLayerData
{
    RopeBuffer Buffer;

    explicit RopesLayerData(ShipSpaceSize shipSize)
        : Buffer(shipSize)
    {}

    explicit RopesLayerData(RopeBuffer && buffer)
        : Buffer(std::move(buffer))
    {}

    RopesLayerData Clone() const
    {
        return RopesLayerData(Buffer.Clone());
    }

    RopesLayerData CloneRegion(ShipSpaceRect const & region) const
    {
        return RopesLayerData(Buffer.CloneRegion(region));
    }

    RopesLayerData MakeRegionBackup(ShipSpaceRect const & region) const
    {
        (void)region;
        return RopesLayerData(Buffer.Clone()); // Buffer is whole
    }

    void RestoreRegionBackup(
        RopesLayerData && sourceRegionBackup,
        ShipSpaceCoordinates const & position)
    {
        (void)position;
        Buffer = std::move(sourceRegionBackup.Buffer); // Buffer is whole
    }

    void Trim(ShipSpaceRect const & rect)
    {
        Buffer.Trim(rect.origin, rect.size);
    }

    RopesLayerData MakeReframed(
        ShipSpaceSize const & newSize, // Final size
        ShipSpaceCoordinates const & originOffset) const; // Position in final buffer of original {0, 0}
};

template <>
struct LayerTypeTraits<LayerType::Ropes>
{
    using material_type = StructuralMaterial;
    using buffer_type = RopeBuffer;
    using layer_data_type = RopesLayerData;
};

//////////////////////////////////////////////////////////////////
// Texture (X 2)
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

    TextureLayerData CloneRegion(ImageRect const & region) const
    {
        return TextureLayerData(Buffer.CloneRegion(region));
    }

    TextureLayerData MakeRegionBackup(ImageRect const & region) const
    {
        return TextureLayerData(Buffer.CloneRegion(region));
    }

    void RestoreRegionBackup(
        TextureLayerData && sourceRegionBackup,
        ImageCoordinates const & position)
    {
        Buffer.BlitFromRegion(
            sourceRegionBackup.Buffer,
            ImageRect(
                ImageCoordinates(0, 0),
                sourceRegionBackup.Buffer.Size),
            position);
    }

    void Trim(ImageRect const & rect)
    {
        Buffer.Trim(rect);
    }

    TextureLayerData MakeReframed(
        ImageSize const & newSize, // Final size
        ImageCoordinates const & originOffset, // Position in final buffer of original {0, 0}
        rgbaColor const & fillerValue) const;
};

template <>
struct LayerTypeTraits<LayerType::ExteriorTexture>
{
    using buffer_type = ImageData<rgbaColor>;
    using layer_data_type = TextureLayerData;
};

template <>
struct LayerTypeTraits<LayerType::InteriorTexture>
{
    using buffer_type = ImageData<rgbaColor>;
    using layer_data_type = TextureLayerData;
};

//////////////////////////////////////////////////////////////////
// All Layers
//////////////////////////////////////////////////////////////////

/*
 * Container of (regions of) layers, for each layer type.
 */
struct ShipLayers
{
    ShipSpaceSize Size;
    std::unique_ptr<StructuralLayerData> StructuralLayer;
    std::unique_ptr<ElectricalLayerData> ElectricalLayer;
    std::unique_ptr<RopesLayerData> RopesLayer;
    std::unique_ptr<TextureLayerData> ExteriorTextureLayer;
    std::unique_ptr<TextureLayerData> InteriorTextureLayer;

    ShipLayers(
        ShipSpaceSize const & size,
        std::unique_ptr<StructuralLayerData> && structuralLayer,
        std::unique_ptr<ElectricalLayerData> && electricalLayer,
        std::unique_ptr<RopesLayerData> && ropesLayer,
        std::unique_ptr<TextureLayerData> && exteriorTextureLayer,
        std::unique_ptr<TextureLayerData> && interiorTextureLayer)
        : Size(size)
        , StructuralLayer(std::move(structuralLayer))
        , ElectricalLayer(std::move(electricalLayer))
        , RopesLayer(std::move(ropesLayer))
        , ExteriorTextureLayer(std::move(exteriorTextureLayer))
        , InteriorTextureLayer(std::move(interiorTextureLayer))
    {}

    ShipLayers Clone() const
    {
        return ShipLayers(
            Size,
            StructuralLayer ? std::make_unique<StructuralLayerData>(StructuralLayer->Clone()) : nullptr,
            ElectricalLayer ? std::make_unique<ElectricalLayerData>(ElectricalLayer->Clone()) : nullptr,
            RopesLayer ? std::make_unique<RopesLayerData>(RopesLayer->Clone()) : nullptr,
            ExteriorTextureLayer ? std::make_unique<TextureLayerData>(ExteriorTextureLayer->Clone()) : nullptr,
            InteriorTextureLayer ? std::make_unique<TextureLayerData>(InteriorTextureLayer->Clone()) : nullptr);
    }

    void Flip(DirectionType direction);

    void Rotate90(RotationDirectionType direction);
};