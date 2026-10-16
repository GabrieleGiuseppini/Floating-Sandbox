/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFormatDeSerializer.h"

#include "SimulationParameters.h" // For limits

#include <Core/Colors.h>
#include <Core/GameExceptions.h>
#include <Core/GameTypes.h>
#include <Core/ImageTools.h>
#include <Core/Log.h>
#include <Core/MemoryStreams.h>
#include <Core/PngTools.h>
#include <Core/PortableTimepoint.h>
#include <Core/UserGameException.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <utility>

namespace {

unsigned char const HeaderTitle[] = "FLOATING SANDBOX SHIP\x1a\x00\x00";

uint8_t constexpr LatestFileFormatVersion = 2; // Bump whenever we add a mandatory tag

}

ShipDefinition ShipDefinitionFormatDeSerializer::Load(
    BinaryReadStream & shipDefinitionInputStream,
    DefaultMaterialDatabase const & defaultMaterialDatabase)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read and process sections
    //

    std::optional<ShipAttributes> shipAttributes;
    std::optional<ShipMetadata> shipMetadata;
    ShipPhysicsData shipPhysicsData;
    std::optional<ShipAutoTexturizationSettings> shipAutoTexturizationSettings;
    std::unique_ptr<StructuralLayerData> structuralLayer;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type const>> CustomStructuralLayerMaterials;
    std::unique_ptr<ElectricalLayerData> electricalLayer;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type const>> CustomElectricalLayerMaterials;
    std::unique_ptr<RopesLayerData> ropesLayer;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type const>> CustomRopesLayerMaterials;
    std::unique_ptr<TextureLayerData> textureLayer;
    bool hasSeenTail = false;

    Parse(
        shipDefinitionInputStream,
        [&](SectionHeader const & sectionHeader, BinaryReadStream & inputStream) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::ShipAttributes):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipAttributes = ReadShipAttributes(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Metadata):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipMetadata = ReadMetadata(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::PhysicsData):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipPhysicsData = ReadPhysicsData(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::AutoTexturizationSettings):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipAutoTexturizationSettings = ReadAutoTexturizationSettings(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::StructuralLayer_V1):
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    ReadStructuralLayer_V1(
                        buffer,
                        *shipAttributes,
                        defaultMaterialDatabase.GetStructuralMaterialColorMap(),
                        structuralLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::StructuralLayer_V2):
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    CustomStructuralLayerMaterials = ReadStructuralLayer_V2(
                        buffer,
                        *shipAttributes,
                        defaultMaterialDatabase.GetStructuralMaterialColorMap(),
                        structuralLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::ElectricalLayer_V1):
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    ReadElectricalLayer_V1(
                        buffer,
                        *shipAttributes,
                        defaultMaterialDatabase.GetElectricalMaterialColorMap(),
                        electricalLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::RopesLayer_V1):
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    ReadRopesLayer_V1(
                        buffer,
                        *shipAttributes,
                        defaultMaterialDatabase.GetStructuralMaterialColorMap(),
                        ropesLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::TextureLayer_PNG):
                {
                    RgbaImageData image = ReadPngImage(inputStream, sectionHeader.SectionBodySize);

                    // Make texture out of this image
                    textureLayer = std::make_unique<TextureLayerData>(std::move(image));

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Tail):
                {
                    hasSeenTail = true;

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Preview_PNG):
                {
                    // Ignore and skip section
                    size_t const szSkipped = inputStream.Skip(sectionHeader.SectionBodySize);
                    if (szSkipped != sectionHeader.SectionBodySize)
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    break;
                }

                default:
                {
                    // Unrecognized tag
                    LogMessage("WARNING: Unrecognized main section tag ", sectionHeader.Tag);

                    // Skip section
                    size_t const szSkipped = inputStream.Skip(sectionHeader.SectionBodySize);
                    if (szSkipped != sectionHeader.SectionBodySize)
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    break;
                }
            }

            // Keep parsing until the end
            return false;
        });

    //
    // Ensure all the required sections have been seen
    //

    if (!shipAttributes.has_value() || !shipMetadata.has_value() || !structuralLayer || !hasSeenTail)
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }

    return ShipDefinition(
        ShipLayers(
            shipAttributes->ShipSize,
            std::move(structuralLayer),
            std::move(electricalLayer),
            std::move(ropesLayer),
            std::move(textureLayer),
            nullptr), // TODO: InteriorLayer
        *shipMetadata,
        shipPhysicsData,
        shipAutoTexturizationSettings,
        CustomMaterialsPod(
            std::move(CustomStructuralLayerMaterials),
            std::move(CustomElectricalLayerMaterials),
            std::move(CustomRopesLayerMaterials)));
}

ShipPreviewData ShipDefinitionFormatDeSerializer::LoadPreviewData(BinaryReadStream & shipDefinitionInputStream)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read and process sections
    //

    std::optional<ShipAttributes> shipAttributes;
    std::optional<ShipMetadata> shipMetadata;

    Parse(
        shipDefinitionInputStream,
        [&](SectionHeader const & sectionHeader, BinaryReadStream & inputStream) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::ShipAttributes):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipAttributes = ReadShipAttributes(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Metadata):
                {
                    ReadIntoBuffer(inputStream, buffer, sectionHeader.SectionBodySize);
                    shipMetadata = ReadMetadata(buffer);

                    break;
                }

                default:
                {
                    // Skip section
                    size_t const szSkipped = inputStream.Skip(sectionHeader.SectionBodySize);
                    if (szSkipped != sectionHeader.SectionBodySize)
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    break;
                }
            }

            return shipAttributes.has_value() && shipMetadata.has_value();
        });

    if (!shipAttributes.has_value() || !shipMetadata.has_value())
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }

    bool const isHD = shipAttributes->HasTextureLayer && !shipMetadata->DoHideHDInPreview;
    bool const hasElectricals = shipAttributes->HasElectricalLayer && !shipMetadata->DoHideElectricalsInPreview;

    return ShipPreviewData(
        shipAttributes->ShipSize,
        *shipMetadata,
        isHD,
        hasElectricals);
}

RgbaImageData ShipDefinitionFormatDeSerializer::LoadPreviewImage(
    BinaryReadStream & shipDefinitionInputStream,
    ImageSize const & maxSize)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read until we find a suitable preview
    //

    std::optional<RgbaImageData> previewImage;

    Parse(
        shipDefinitionInputStream,
        [&](SectionHeader const & sectionHeader, BinaryReadStream & inputStream) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::TextureLayer_PNG):
                {
                    previewImage.emplace(ReadPngImageAndMakePreview(inputStream, sectionHeader.SectionBodySize, maxSize));

                    LogMessage("ShipDefinitionFormatDeSerializer: returning preview from texture layer section");

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Preview_PNG):
                {
                    previewImage.emplace(ReadPngImageAndMakePreview(inputStream, sectionHeader.SectionBodySize, maxSize));

                    LogMessage("ShipDefinitionFormatDeSerializer: returning preview from preview section");

                    break;
                }

                default:
                {
                    // Skip section
                    size_t const szSkipped = inputStream.Skip(sectionHeader.SectionBodySize);
                    if (szSkipped != sectionHeader.SectionBodySize)
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    break;
                }
            }

            return previewImage.has_value();
        });

    if (!previewImage.has_value())
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }

    return std::move(*previewImage);
}

void ShipDefinitionFormatDeSerializer::Save(
    ShipDefinition const & shipDefinition,
    Version const & currentGameVersion,
    BinaryWriteStream & shipDefinitionOutputStream)
{
    // We just don't expect these to be populated
    assert(shipDefinition.CustomMaterials.IsEmpty());

    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Decide file format version
    //

    CustomMaterialOverridesMap<StructuralMaterial> structuralLayerCustomMaterialOverridesMap;
    if (shipDefinition.Layers.StructuralLayer)
    {
        structuralLayerCustomMaterialOverridesMap = ExtractCustomMaterialOverridesMap(*shipDefinition.Layers.StructuralLayer);
    }

    CustomMaterialOverridesMap<ElectricalMaterial> electricalLayerCustomMaterialOverridesMap;
    if (shipDefinition.Layers.ElectricalLayer)
    {
        electricalLayerCustomMaterialOverridesMap = ExtractCustomMaterialOverridesMap(*shipDefinition.Layers.ElectricalLayer);
    }

    CustomMaterialOverridesMap<StructuralMaterial> ropesLayerCustomMaterialOverridesMap;
    if (shipDefinition.Layers.RopesLayer)
    {
        ropesLayerCustomMaterialOverridesMap = ExtractCustomMaterialOverridesMap(*shipDefinition.Layers.RopesLayer);
    }

    std::uint16_t fileFormatVersion =
        (structuralLayerCustomMaterialOverridesMap.empty() && electricalLayerCustomMaterialOverridesMap.empty() && ropesLayerCustomMaterialOverridesMap.empty())
        ? 1 : 2;

    assert(fileFormatVersion <= LatestFileFormatVersion);

    //
    // Write header
    //

    AppendFileHeader(fileFormatVersion, shipDefinitionOutputStream, buffer);

    //
    // Write ship attributes
    //

    ShipAttributes const shipAttributes = ShipAttributes(
        currentGameVersion,
        shipDefinition.Layers.Size,
        (bool)shipDefinition.Layers.ExteriorTextureLayer,
        (bool)shipDefinition.Layers.ElectricalLayer);

    AppendSection(
        shipDefinitionOutputStream,
        static_cast<std::uint32_t>(MainSectionTagType::ShipAttributes),
        [&]() { return AppendShipAttributes(shipAttributes, buffer); },
        buffer);

    //
    // Write metadata
    //

    AppendSection(
        shipDefinitionOutputStream,
        static_cast<std::uint32_t>(MainSectionTagType::Metadata),
        [&]() { return AppendMetadata(shipDefinition.Metadata, buffer); },
        buffer);

    if (shipDefinition.Layers.ExteriorTextureLayer)
    {
        //
        // Write texture
        //

        AppendSection(
            shipDefinitionOutputStream,
            static_cast<std::uint32_t>(MainSectionTagType::TextureLayer_PNG),
            [&]() { return AppendPngImage(shipDefinition.Layers.ExteriorTextureLayer->Buffer, buffer); },
            buffer);
    }
    else if (shipDefinition.Layers.StructuralLayer)
    {
        //
        // Make and write a preview image
        //

        AppendSection(
            shipDefinitionOutputStream,
            static_cast<std::uint32_t>(MainSectionTagType::Preview_PNG),
            [&]() { return AppendPngPreview(*shipDefinition.Layers.StructuralLayer, buffer); },
            buffer);
    }
    else
    {
        // Note: no preview image - but won't happen in reality that there's no structural layer
    }

    //
    // Write structural layer
    //

    if (shipDefinition.Layers.StructuralLayer)
    {
        if (structuralLayerCustomMaterialOverridesMap.empty())
        {
            // V1 Structural Layer
            AppendSection(
                shipDefinitionOutputStream,
                static_cast<std::uint32_t>(MainSectionTagType::StructuralLayer_V1),
                [&]() { return AppendStructuralLayer_V1(*shipDefinition.Layers.StructuralLayer, buffer); },
                buffer);
        }
        else
        {
            // V2 Structural Layer
            AppendSection(
                shipDefinitionOutputStream,
                static_cast<std::uint32_t>(MainSectionTagType::StructuralLayer_V2),
                [&]() { return AppendStructuralLayer_V2(*shipDefinition.Layers.StructuralLayer, structuralLayerCustomMaterialOverridesMap, buffer); },
                buffer);
        }
    }

    //
    // Write electrical layer
    //

    if (shipDefinition.Layers.ElectricalLayer)
    {
        if (electricalLayerCustomMaterialOverridesMap.empty())
        {
            // V1 Electrical Layer
            AppendSection(
                shipDefinitionOutputStream,
                static_cast<std::uint32_t>(MainSectionTagType::ElectricalLayer_V1),
                [&]() { return AppendElectricalLayer_V1(*shipDefinition.Layers.ElectricalLayer, buffer); },
                buffer);
        }
        else
        {
            // V2 Electrical Layer
            AppendSection(
                shipDefinitionOutputStream,
                static_cast<std::uint32_t>(MainSectionTagType::ElectricalLayer_V2),
                [&]() { return AppendElectricalLayer_V2(*shipDefinition.Layers.ElectricalLayer, electricalLayerCustomMaterialOverridesMap, buffer); },
                buffer);
        }
    }

    //
    // Write ropes layer
    //

    if (shipDefinition.Layers.RopesLayer)
    {
        AppendSection(
            shipDefinitionOutputStream,
            static_cast<std::uint32_t>(MainSectionTagType::RopesLayer_V1),
            [&]() { return AppendRopesLayer_V1(*shipDefinition.Layers.RopesLayer, buffer); },
            buffer);
    }

    //
    // Write physics data
    //

    AppendSection(
        shipDefinitionOutputStream,
        static_cast<std::uint32_t>(MainSectionTagType::PhysicsData),
        [&]() { return AppendPhysicsData(shipDefinition.PhysicsData, buffer); },
        buffer);

    //
    // Write auto-texturization settings
    //

    if (shipDefinition.AutoTexturizationSettings.has_value())
    {
        AppendSection(
            shipDefinitionOutputStream,
            static_cast<std::uint32_t>(MainSectionTagType::AutoTexturizationSettings),
            [&]() { return AppendAutoTexturizationSettings(*shipDefinition.AutoTexturizationSettings, buffer); },
            buffer);
    }

    //
    // Write tail
    //

    AppendSection(
        shipDefinitionOutputStream,
        static_cast<std::uint32_t>(MainSectionTagType::Tail),
        [&]() { return 0; },
        buffer);
}

PasswordHash ShipDefinitionFormatDeSerializer::CalculatePasswordHash(std::string const & password)
{
    return static_cast<PasswordHash>(std::hash<std::string>{}(password + "fs_salt_0$%"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Write

template<typename TSectionBodyAppender>
void ShipDefinitionFormatDeSerializer::AppendSection(
    BinaryWriteStream & shipDefinitionOutputStream,
    std::uint32_t tag,
    TSectionBodyAppender const & sectionBodyAppender,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    // Tag
    buffer.Append(tag);

    // SectionBodySize
    size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

    // SectionBody
    size_t const sectionBodySize = sectionBodyAppender();

    // SectionBodySize, again
    buffer.WriteAt(static_cast<std::uint32_t>(sectionBodySize), sectionBodySizeIndex);

    // Serialize
    shipDefinitionOutputStream.Write(buffer.GetData(), buffer.GetSize());
}

size_t ShipDefinitionFormatDeSerializer::AppendPngImage(
    RgbaImageData const & rawImageData,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    auto encodedImageOutputStream = MemoryBinaryWriteStream();
    PngTools::EncodeImage(rawImageData, encodedImageOutputStream);

    size_t const byteSize = encodedImageOutputStream.GetSize();
    void * targetBuf = reinterpret_cast<void *>(buffer.Receive(byteSize));
    std::memcpy(targetBuf, encodedImageOutputStream.GetData(), byteSize);

    return byteSize;
}

void ShipDefinitionFormatDeSerializer::AppendFileHeader(
    std::uint16_t fileFormatVersion,
    BinaryWriteStream & shipDefinitionOutputStream,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    AppendFileHeader(fileFormatVersion, buffer);
    shipDefinitionOutputStream.Write(buffer.GetData(), buffer.GetSize());
}

void ShipDefinitionFormatDeSerializer::AppendFileHeader(
    std::uint16_t fileFormatVersion,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    static_assert(sizeof(HeaderTitle) == 24 + 1);

    std::memcpy(
        buffer.Receive(24),
        HeaderTitle,
        24);

    buffer.Append<std::uint16_t>(fileFormatVersion);
    // Padding
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);

    assert(buffer.GetSize() == sizeof(FileHeader));
}

size_t ShipDefinitionFormatDeSerializer::AppendShipAttributes(
    ShipAttributes const & shipAttributes,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    // FS version
    {
        // Tag and size
        buffer.Append(static_cast<std::uint32_t>(ShipAttributesTagType::FSVersion2));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        size_t valueSize = 0;

        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersion.GetMajor()));
        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersion.GetMinor()));
        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersion.GetPatch()));
        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersion.GetBuild()));

        // Store size
        buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

        sectionBodySize += sizeof(std::uint32_t) + sizeof(std::uint32_t) + valueSize;
    }

    // Ship size
    {
        // Tag and size
        buffer.Append(static_cast<std::uint32_t>(ShipAttributesTagType::ShipSize));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        size_t valueSize = 0;

        valueSize += buffer.Append(static_cast<std::uint32_t>(shipAttributes.ShipSize.width));
        valueSize += buffer.Append(static_cast<std::uint32_t>(shipAttributes.ShipSize.height));

        // Store size
        buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

        sectionBodySize += sizeof(std::uint32_t) + sizeof(std::uint32_t) + valueSize;
    }

    // Has texture layer
    {
        sectionBodySize += AppendShipAttributesEntry(
            ShipAttributesTagType::HasTextureLayer,
            shipAttributes.HasTextureLayer,
            buffer);
    }

    // Has electrical layer
    {
        sectionBodySize += AppendShipAttributesEntry(
            ShipAttributesTagType::HasElectricalLayer,
            shipAttributes.HasElectricalLayer,
            buffer);
    }

    // Last write time
    {
        // Legacy since multi-platform port
        PortableTimepoint now = PortableTimepoint::Now();
        sectionBodySize += AppendShipAttributesEntry(
            ShipAttributesTagType::LastWriteTime,
            now.Value(),
            buffer);
    }

    // Tail
    {
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(ShipAttributesTagType::Tail));
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(0));
        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    }

    return sectionBodySize;
}

template<typename T>
size_t ShipDefinitionFormatDeSerializer::AppendShipAttributesEntry(
    ShipDefinitionFormatDeSerializer::ShipAttributesTagType tag,
    T const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(value);
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendMetadata(
    ShipMetadata const & metadata,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::ShipName,
            metadata.ShipName,
            buffer);
    }

    if (metadata.Author.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::Author,
            *metadata.Author,
            buffer);
    }

    if (metadata.ArtCredits.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::ArtCredits,
            *metadata.ArtCredits,
            buffer);
    }

    if (metadata.YearBuilt.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::YearBuilt,
            *metadata.YearBuilt,
            buffer);
    }

    if (metadata.Category.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::Category,
            static_cast<std::uint64_t>(*metadata.Category),
            buffer);
    }

    if (metadata.Description.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::Description,
            *metadata.Description,
            buffer);
    }

    {
        buffer.Append(static_cast<std::uint32_t>(MetadataTagType::Scale));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        size_t valueSize = buffer.Append(metadata.Scale.inputUnits);
        valueSize += buffer.Append(metadata.Scale.outputUnits);

        buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader) + valueSize;
    }

    if (metadata.Password.has_value())
    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::Password,
            static_cast<std::uint64_t>(*metadata.Password),
            buffer);
    }

    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::DoHideElectricalsInPreview,
            metadata.DoHideElectricalsInPreview,
            buffer);
    }

    {
        sectionBodySize += AppendMetadataEntry(
            MetadataTagType::DoHideHDInPreview,
            metadata.DoHideHDInPreview,
            buffer);
    }

    // Tail
    {
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(MetadataTagType::Tail));
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(0));
        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    }

    return sectionBodySize;
}

template<typename T>
size_t ShipDefinitionFormatDeSerializer::AppendMetadataEntry(
    ShipDefinitionFormatDeSerializer::MetadataTagType tag,
    T const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(value);
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendPhysicsData(
    ShipPhysicsData const & physicsData,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    {
        sectionBodySize += AppendPhysicsDataEntry(
            PhysicsDataTagType::OffsetX,
            physicsData.Offset.x,
            buffer);
    }

    {
        sectionBodySize += AppendPhysicsDataEntry(
            PhysicsDataTagType::OffsetY,
            physicsData.Offset.y,
            buffer);
    }

    {
        sectionBodySize += AppendPhysicsDataEntry(
            PhysicsDataTagType::InternalAirPressure,
            physicsData.InternalAirPressure,
            buffer);
    }

    // Tail
    {
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(PhysicsDataTagType::Tail));
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(0));
        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    }

    return sectionBodySize;
}

template<typename T>
size_t ShipDefinitionFormatDeSerializer::AppendPhysicsDataEntry(
    ShipDefinitionFormatDeSerializer::PhysicsDataTagType tag,
    T const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(value);
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendAutoTexturizationSettings(
    ShipAutoTexturizationSettings const & autoTexturizationSettings,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    {
        sectionBodySize += AppendAutoTexturizationSettingsEntry(
            AutoTexturizationSettingsTagType::Mode,
            static_cast<std::uint32_t>(autoTexturizationSettings.Mode),
            buffer);
    }

    {
        sectionBodySize += AppendAutoTexturizationSettingsEntry(
            AutoTexturizationSettingsTagType::MaterialTextureMagnification,
            autoTexturizationSettings.MaterialTextureMagnification,
            buffer);
    }

    {
        sectionBodySize += AppendAutoTexturizationSettingsEntry(
            AutoTexturizationSettingsTagType::MaterialTextureTransparency,
            autoTexturizationSettings.MaterialTextureTransparency,
            buffer);
    }

    // Tail
    {
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(AutoTexturizationSettingsTagType::Tail));
        sectionBodySize += buffer.Append(static_cast<std::uint32_t>(0));
        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    }

    return sectionBodySize;
}

template<typename TMaterial>
static size_t ShipDefinitionFormatDeSerializer::AppendCustomMaterialMap(
    CustomMaterialOverridesMap<TMaterial> const & customMaterialOverridesMap,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    // Number of entries

    assert(customMaterialOverridesMap.size() <= SimulationParameters::MaxCustomMaterialPerShip);
    std::uint16_t count = static_cast<std::uint16_t>(customMaterialOverridesMap.size());
    subSectionBodySize += buffer.Append(count);

    // Entries

    // Transform map into vector, by index
    std::vector<CustomMaterialOverridesKey<TMaterial> const *> customMaterialOverridesVector(customMaterialOverridesMap.size(), nullptr);
    for (auto const & it : customMaterialOverridesMap)
    {
        assert(it.second < customMaterialOverridesVector.size());
        customMaterialOverridesVector[it.second] = &(it.first);
    }

    // List of material overrides
    for (CustomMaterialOverridesKey<TMaterial> const * mk : customMaterialOverridesVector)
    {
        assert(mk != nullptr);

        size_t entryBodySize = 0;

        // Serialize RGB base color key
        entryBodySize += buffer.Append(reinterpret_cast<unsigned char const *>(&(mk->BaseColorKey)), sizeof(MaterialColorKey));

        // Serialize variant overrides
        entryBodySize += AppendCustomMaterialVariantOverrides(mk->VariantOverrides, buffer);

        subSectionBodySize += entryBodySize;
    }

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendCustomMaterialVariantOverrides(
    StructuralMaterial::VariantOverridesType const & variantOverrides,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t overridesSize = 0;

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::Name,
        variantOverrides.Name,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::RenderColor,
        variantOverrides.RenderColor,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::Strength,
        variantOverrides.Strength,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::Density,
        variantOverrides.Density,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::IgnitionTemperature,
        variantOverrides.IgnitionTemperature,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::MeltingTemperature,
        variantOverrides.MeltingTemperature,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::RotReceptivity,
        variantOverrides.RotReceptivity,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::RustReceptivity,
        variantOverrides.RustReceptivity,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::WaterSolubility,
        variantOverrides.WaterSolubility,
        buffer);

    // Tail
    overridesSize += buffer.Append(static_cast<std::uint32_t>(CustomMaterialMapEntryTagType::Tail));
    overridesSize += buffer.Append(static_cast<std::uint32_t>(0));
    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));

    return overridesSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendCustomMaterialVariantOverrides(
    ElectricalMaterial::VariantOverridesType const & variantOverrides,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t overridesSize = 0;

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::Name,
        variantOverrides.Name,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::HeatGenerated,
        variantOverrides.HeatGenerated,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::Luminiscence,
        variantOverrides.Luminiscence,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::LightSpread,
        variantOverrides.LightSpread,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::EnginePower,
        variantOverrides.EnginePower,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::WaterPumpNominalForce,
        variantOverrides.WaterPumpNominalForce,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::TimerDurationSeconds,
        variantOverrides.TimerDurationSeconds,
        buffer);

    overridesSize += AppendCustomMaterialVariantOverrideEntry(
        CustomMaterialMapEntryTagType::ThermalSwitchTransitionTemperature,
        variantOverrides.ThermalSwitchTransitionTemperature,
        buffer);

    // Tail
    overridesSize += buffer.Append(static_cast<std::uint32_t>(CustomMaterialMapEntryTagType::Tail));
    overridesSize += buffer.Append(static_cast<std::uint32_t>(0));
    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));

    return overridesSize;
}

template<typename T>
size_t ShipDefinitionFormatDeSerializer::AppendCustomMaterialVariantOverrideEntry(
    ShipDefinitionFormatDeSerializer::CustomMaterialMapEntryTagType tag,
    T const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(value);
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendCustomMaterialVariantOverrideEntry(
    ShipDefinitionFormatDeSerializer::CustomMaterialMapEntryTagType tag,
    rgbaColor const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(reinterpret_cast<unsigned char const *>(&value), sizeof(rgbaColor));
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

template<typename T>
size_t ShipDefinitionFormatDeSerializer::AppendAutoTexturizationSettingsEntry(
    ShipDefinitionFormatDeSerializer::AutoTexturizationSettingsTagType tag,
    T const & value,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(tag));
    size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();
    size_t const valueSize = buffer.Append(value);
    buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    return sizeof(SectionHeader) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayer_V1(
    StructuralLayerData const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType_V1::Buffer));
        size_t const subSectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendStructuralLayerBuffer_V1(
            structuralLayer.Buffer,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), subSectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Tail
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType_V1::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayerBuffer_V1(
    Buffer2D<StructuralElement, struct ShipSpaceTag> const & structuralLayerBuffer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    //
    // Encode layer with RLE of RGB color key buffer
    //

    size_t const layerLinearSize = structuralLayerBuffer.Size.GetLinearSize();
    DeSerializationBuffer<BigEndianess> rleBuffer(layerLinearSize * sizeof(MaterialColorKey)); // Upper bound

    StructuralElement const * structuralElementBuffer = structuralLayerBuffer.Data.get();
    for (size_t i = 0; i < layerLinearSize; /*incremented in loop*/)
    {
        // Get the element at this position
        StructuralElement const & structuralElement = structuralElementBuffer[i];
        ++i;

        // Count consecutive identical values
        std::uint16_t materialCount = 1;
        for (;
            i < layerLinearSize
            && structuralElementBuffer[i] == structuralElement
            && materialCount < std::numeric_limits<var_uint16_t>::max().value();
            ++i, ++materialCount);

        // Serialize count
        rleBuffer.Append<var_uint16_t>(var_uint16_t(materialCount));

        // Serialize RGB color key
        MaterialColorKey const colorKey = (structuralElement.Material == nullptr) ? EmptyMaterialColorKey : structuralElement.Material->ColorKey;
        rleBuffer.Append(reinterpret_cast<unsigned char const *>(&colorKey), sizeof(MaterialColorKey));
    }

    //
    // Serialize RLE buffer
    //

    subSectionBodySize += buffer.Append(
        rleBuffer.GetData(),
        rleBuffer.GetSize());

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayer_V2(
    StructuralLayerData const & structuralLayer,
    CustomMaterialOverridesMap<StructuralMaterial> const & customMaterialOverridesMap,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Custom Material Map
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType_V2::CustomMaterialsMap));
        size_t const subSectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        size_t const subSectionBodySize = AppendCustomMaterialMap(
            customMaterialOverridesMap,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), subSectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;    }

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType_V2::Buffer));
        size_t const subSectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendStructuralLayerBuffer_V2(
            structuralLayer.Buffer,
            customMaterialOverridesMap,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), subSectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Tail
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType_V2::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayerBuffer_V2(
    Buffer2D<StructuralElement, struct ShipSpaceTag> const & structuralLayerBuffer,
    CustomMaterialOverridesMap<StructuralMaterial> const & customMaterialOverridesMap,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    //
    // Encode layer with RLE of [uint16 custom material index [RGB color key]]
    //

    size_t const layerLinearSize = structuralLayerBuffer.Size.GetLinearSize();
    DeSerializationBuffer<BigEndianess> rleBuffer(layerLinearSize * sizeof(MaterialColorKey)); // Upper bound
    static_assert(sizeof(std::uint16_t) <= sizeof(MaterialColorKey)); // Or else not upper bound

    StructuralElement const * structuralElementBuffer = structuralLayerBuffer.Data.get();
    for (size_t i = 0; i < layerLinearSize; /*incremented in loop*/)
    {
        // Get the element at this position
        StructuralElement const & structuralElement = structuralElementBuffer[i];
        ++i;

        // Count consecutive identical values
        std::uint16_t materialCount = 1;
        for (;
            i < layerLinearSize
            && structuralElementBuffer[i] == structuralElement
            && materialCount < std::numeric_limits<var_uint16_t>::max().value();
            ++i, ++materialCount);

        // Serialize count
        rleBuffer.Append<var_uint16_t>(var_uint16_t(materialCount));

        // Serialize material
        if (structuralElement.Material != nullptr && structuralElement.Material->VariantOverrides.has_value())
        {
            // Custom material

            // Look it up
            CustomMaterialOverridesKey<StructuralMaterial> customMaterialKey(
                structuralElement.Material->ColorKey,
                *structuralElement.Material->VariantOverrides);
            assert(customMaterialOverridesMap.count(customMaterialKey) == 1);
            std::uint16_t const customMaterialIndex = customMaterialOverridesMap.at(customMaterialKey);

            // Serialize custom material index
            rleBuffer.Append<std::uint16_t>(customMaterialIndex);
        }
        else
        {
            // Default material

            // Serialize custom material index
            rleBuffer.Append<std::uint16_t>(SimulationParameters::MaxCustomMaterialPerShip);

            // Serialize RGB color key
            MaterialColorKey const colorKey = (structuralElement.Material == nullptr) ? EmptyMaterialColorKey : structuralElement.Material->ColorKey;
            rleBuffer.Append(reinterpret_cast<unsigned char const *>(&colorKey), sizeof(MaterialColorKey));
        }
    }

    //
    // Serialize RLE buffer
    //

    subSectionBodySize += buffer.Append(
        rleBuffer.GetData(),
        rleBuffer.GetSize());

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayer_V1(
    ElectricalLayerData const & electricalLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType_V1::Buffer));
        size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendElectricalLayerBuffer_V1(
            electricalLayer.Buffer,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Electrical panel
    //

    if (!electricalLayer.Panel.IsEmpty())
    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType_V1::Panel));
        size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendElectricalLayerPanel(
            electricalLayer.Panel,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Tail
    //

    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType_V1::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayerBuffer_V1(
    Buffer2D<ElectricalElement, struct ShipSpaceTag> const & electricalLayerBuffer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    //
    // Encode layer with RLE of <RGB color key, instance ID> buffer
    //

    size_t const layerLinearSize = electricalLayerBuffer.Size.GetLinearSize();
    DeSerializationBuffer<BigEndianess> rleBuffer(layerLinearSize * (sizeof(MaterialColorKey) + sizeof(std::uint16_t))); // Upper bound

    ElectricalElement const * electricalElementBuffer = electricalLayerBuffer.Data.get();
    for (size_t i = 0; i < layerLinearSize; /*incremented in loop*/)
    {
        // Get the element at this position
        ElectricalElement const & electricalElement = electricalElementBuffer[i];
        ++i;

        // Count consecutive identical values
        std::uint16_t materialCount = 1;
        for (;
            i < layerLinearSize
            && electricalElementBuffer[i] == electricalElement
            && materialCount < std::numeric_limits<var_uint16_t>::max().value();
            ++i, ++materialCount);

        // Serialize count
        rleBuffer.Append<var_uint16_t>(var_uint16_t(materialCount));

        // Serialize RGB key
        MaterialColorKey const colorKey = (electricalElement.Material == nullptr) ? EmptyMaterialColorKey : electricalElement.Material->ColorKey;
        rleBuffer.Append(reinterpret_cast<unsigned char const *>(&colorKey), sizeof(MaterialColorKey));

        // Serialize instance index - only if instanced
        if (electricalElement.Material != nullptr
            && electricalElement.Material->IsInstanced)
        {
            static_assert(sizeof(ElectricalElementInstanceIndex) <= sizeof(std::uint16_t));
            rleBuffer.Append<var_uint16_t>(var_uint16_t(electricalElement.InstanceIndex));
        }
    }

    //
    // Serialize RLE buffer
    //

    subSectionBodySize += buffer.Append(
        rleBuffer.GetData(),
        rleBuffer.GetSize());

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayer_V2(
    ElectricalLayerData const & electricalLayer,
    CustomMaterialOverridesMap<ElectricalMaterial> const & customMaterialOverridesMap,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Custom Material Map
    //

    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType_V2::CustomMaterialsMap));
        size_t const subSectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        size_t const subSectionBodySize = AppendCustomMaterialMap(
            customMaterialOverridesMap,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), subSectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    // TODOHERE
    (void)electricalLayer;

    ////
    //// Buffer
    ////

    //{
    //    buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Buffer));
    //    size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

    //    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    //    sectionBodySize += sizeof(SectionHeader);

    //    // Body
    //    size_t const subSectionBodySize = AppendElectricalLayerBuffer_V2(
    //        electricalLayer.Buffer,
    //        buffer);

    //    buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

    //    sectionBodySize += subSectionBodySize;
    //}

    ////
    //// Electrical panel
    ////

    //if (!electricalLayer.Panel.IsEmpty())
    //{
    //    buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Panel));
    //    size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

    //    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    //    sectionBodySize += sizeof(SectionHeader);

    //    // Body
    //    size_t const subSectionBodySize = AppendElectricalLayerPanel(
    //        electricalLayer.Panel,
    //        buffer);

    //    buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

    //    sectionBodySize += subSectionBodySize;
    //}

    ////
    //// Tail
    ////

    //{
    //    buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Tail));
    //    buffer.Append<std::uint32_t>(0);

    //    static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
    //    sectionBodySize += sizeof(SectionHeader);
    //}

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayerPanel(
    ElectricalPanel const & electricalPanel,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    // Number of entries
    std::uint16_t count = static_cast<std::uint16_t>(electricalPanel.GetSize());
    subSectionBodySize += buffer.Append(count);

    // Entries
    for (auto const & entry : electricalPanel)
    {
        subSectionBodySize += buffer.Append(static_cast<std::uint32_t>(entry.first));

        subSectionBodySize += buffer.Append(entry.second.PanelCoordinates.has_value());
        if (entry.second.PanelCoordinates.has_value())
        {
            subSectionBodySize += buffer.Append(static_cast<std::int32_t>(entry.second.PanelCoordinates->x));
            subSectionBodySize += buffer.Append(static_cast<std::int32_t>(entry.second.PanelCoordinates->y));
        }

        subSectionBodySize += buffer.Append(entry.second.Label.has_value());
        if (entry.second.Label.has_value())
        {
            subSectionBodySize += buffer.Append(*entry.second.Label);
        }

        subSectionBodySize += buffer.Append(entry.second.IsHidden);
    }

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendRopesLayer_V1(
    RopesLayerData const & ropesLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(RopesLayerTagType::Buffer));
        size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendRopesLayerBuffer_V1(
            ropesLayer.Buffer,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Tail
    //

    {
        buffer.Append(static_cast<uint32_t>(RopesLayerTagType::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendRopesLayerBuffer_V1(
    RopeBuffer const & ropesLayerBuffer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    // Number of entries
    std::uint32_t count = static_cast<std::uint32_t>(ropesLayerBuffer.GetElementCount());
    subSectionBodySize += buffer.Append(count);

    // Entries
    for (auto const & element : ropesLayerBuffer)
    {
        // Start coords
        subSectionBodySize += buffer.Append(static_cast<std::int32_t>(element.StartCoords.x));
        subSectionBodySize += buffer.Append(static_cast<std::int32_t>(element.StartCoords.y));

        // End coords
        subSectionBodySize += buffer.Append(static_cast<std::int32_t>(element.EndCoords.x));
        subSectionBodySize += buffer.Append(static_cast<std::int32_t>(element.EndCoords.y));

        // Material
        assert(element.Material != nullptr);
        subSectionBodySize += buffer.Append(reinterpret_cast<unsigned char const *>(&element.Material->ColorKey), sizeof(MaterialColorKey));

        // RenderColor
        subSectionBodySize += buffer.Append(reinterpret_cast<unsigned char const *>(&element.RenderColor), sizeof(element.RenderColor));
    }

    return subSectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendPngPreview(
    StructuralLayerData const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    //
    // Calculate trimmed quad
    //

    auto const bufferSize = structuralLayer.Buffer.Size;

    int minY;
    {
        bool hasData = false;
        for (minY = 0; !hasData && minY < bufferSize.height; ++minY)
        {
            for (int x = 0; !hasData && x < bufferSize.width; ++x)
            {
                hasData = structuralLayer.Buffer[{x, minY}].Material != nullptr;
            }

            if (hasData)
                break;
        }
    }

    int maxY;
    {
        bool hasData = false;
        for (maxY = bufferSize.height - 1; !hasData && maxY > minY; --maxY)
        {
            for (int x = 0; !hasData && x < bufferSize.width; ++x)
            {
                hasData = structuralLayer.Buffer[{x, maxY}].Material != nullptr;
            }

            if (hasData)
                break;
        }
    }

    int minX;
    {
        bool hasData = false;
        for (minX = 0; minX < bufferSize.width; ++minX)
        {
            for (int y = 0; !hasData && y < bufferSize.height; ++y)
            {
                hasData = structuralLayer.Buffer[{minX, y}].Material != nullptr;
            }

            if (hasData)
                break;
        }
    }

    int maxX;
    {
        bool hasData = false;
        for (maxX = bufferSize.width - 1; maxX > minX; --maxX)
        {
            for (int y = 0; !hasData && y < bufferSize.height; ++y)
            {
                hasData = structuralLayer.Buffer[{maxX, y}].Material != nullptr;
            }

            if (hasData)
                break;
        }
    }

    assert(minY <= bufferSize.height && maxY >= 0 && minX <= bufferSize.width && maxX >= 0);

    ImageSize const trimmedSize = ImageSize(
        std::max(maxX - minX + 1, 0),
        std::max(maxY - minY + 1, 0));

    //
    // Make preview
    //

    RgbaImageData previewRawData = RgbaImageData(trimmedSize);

    for (int y = 0; y < trimmedSize.height; ++y)
    {
        for (int x = 0; x < trimmedSize.width; ++x)
        {
            StructuralElement const & element = structuralLayer.Buffer[{x + minX, y + minY}];
            previewRawData[{x, y}] = element.Material != nullptr
                ? element.Material->RenderColor
                : rgbaColor(EmptyMaterialColorKey, 0);
        }
    }

    //
    // Append preview
    //

    return AppendPngImage(
        previewRawData,
        buffer);
}

ShipDefinitionFormatDeSerializer::CustomMaterialOverridesMap<StructuralMaterial> ShipDefinitionFormatDeSerializer::ExtractCustomMaterialOverridesMap(StructuralLayerData const & layerData)
{
    CustomMaterialOverridesMap<StructuralMaterial> customMaterialOverridesMap;

    size_t const layerLinearSize = layerData.Buffer.GetLinearSize();
    StructuralElement const * structuralElementBuffer = layerData.Buffer.Data.get();
    for (size_t i = 0; i < layerLinearSize; ++i)
    {
        StructuralMaterial const * material = structuralElementBuffer[i].Material;
        if (material != nullptr
            && material->VariantOverrides.has_value())
        {
            // Calc its index
            size_t customMaterialIndex = customMaterialOverridesMap.size();
            if (customMaterialIndex >= SimulationParameters::MaxCustomMaterialPerShip)
            {
                throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
            }

            // Add to map
            auto const [_, isInserted] = customMaterialOverridesMap.try_emplace(
                CustomMaterialOverridesKey<StructuralMaterial>(
                    material->ColorKey,
                    *(material->VariantOverrides)),
                static_cast<std::uint16_t>(customMaterialIndex));
        }
    }

    return customMaterialOverridesMap;
}

ShipDefinitionFormatDeSerializer::CustomMaterialOverridesMap<ElectricalMaterial> ShipDefinitionFormatDeSerializer::ExtractCustomMaterialOverridesMap(ElectricalLayerData const & layerData)
{
    CustomMaterialOverridesMap<ElectricalMaterial> customMaterialOverridesMap;

    size_t const layerLinearSize = layerData.Buffer.GetLinearSize();
    ElectricalElement const * electricalElementBuffer = layerData.Buffer.Data.get();
    for (size_t i = 0; i < layerLinearSize; ++i)
    {
        ElectricalMaterial const * material = electricalElementBuffer[i].Material;
        if (material != nullptr
            && material->VariantOverrides.has_value())
        {
            // Calc its index
            size_t customMaterialIndex = customMaterialOverridesMap.size();
            if (customMaterialIndex >= SimulationParameters::MaxCustomMaterialPerShip)
            {
                throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
            }

            // Add to map
            auto const [_, isInserted] = customMaterialOverridesMap.try_emplace(
                CustomMaterialOverridesKey<ElectricalMaterial>(
                    material->ColorKey,
                    *(material->VariantOverrides)),
                static_cast<std::uint16_t>(customMaterialIndex));
        }
    }

    return customMaterialOverridesMap;
}

ShipDefinitionFormatDeSerializer::CustomMaterialOverridesMap<StructuralMaterial> ShipDefinitionFormatDeSerializer::ExtractCustomMaterialOverridesMap(RopesLayerData const & layerData)
{
    // TODO
    (void)layerData;
    return CustomMaterialOverridesMap<StructuralMaterial>();
}

// Read

template<typename SectionHandler>
void ShipDefinitionFormatDeSerializer::Parse(
    BinaryReadStream & shipDefinitionInputStream,
    SectionHandler const & sectionHandler)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read header
    //

    ReadFileHeader(shipDefinitionInputStream, buffer);

    //
    // Read and process sections
    //

    while (true)
    {
        // Read section header
        SectionHeader const sectionHeader = ReadSectionHeader(shipDefinitionInputStream, buffer);

        // Handle section
        if (sectionHandler(sectionHeader, shipDefinitionInputStream))
        {
            // We're done
            break;
        }

        // Exit when we see the tail
        if (sectionHeader.Tag == static_cast<uint32_t>(MainSectionTagType::Tail))
        {
            // We're done
            break;
        }
    }
}

void ShipDefinitionFormatDeSerializer::ThrowMaterialNotFound(ShipAttributes const & shipAttributes)
{
    throw UserGameException(
        UserGameException::MessageIdType::LoadShipMaterialNotFound,
        { shipAttributes.FileFSVersion.ToMajorMinorPatchString() });
}

void ShipDefinitionFormatDeSerializer::ReadIntoBuffer(
    BinaryReadStream & shipDefinitionInputStream,
    DeSerializationBuffer<BigEndianess> & buffer,
    size_t size)
{
    buffer.Reset();

    size_t const szRead = shipDefinitionInputStream.Read(buffer.Receive(size), size);
    if (szRead != size)
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }
}

ShipDefinitionFormatDeSerializer::SectionHeader ShipDefinitionFormatDeSerializer::ReadSectionHeader(
    BinaryReadStream & shipDefinitionInputStream,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    ReadIntoBuffer(shipDefinitionInputStream, buffer, sizeof(SectionHeader));
    return ReadSectionHeader(buffer, 0);
}

ShipDefinitionFormatDeSerializer::SectionHeader ShipDefinitionFormatDeSerializer::ReadSectionHeader(
    DeSerializationBuffer<BigEndianess> const & buffer,
    size_t offset)
{
    std::uint32_t tag;
    size_t const sz1 = buffer.ReadAt<std::uint32_t>(offset, tag);

    std::uint32_t sectionBodySize;
    buffer.ReadAt<std::uint32_t>(offset + sz1, sectionBodySize);

    return SectionHeader{
        tag,
        sectionBodySize
    };
}

RgbaImageData ShipDefinitionFormatDeSerializer::ReadPngImage(
    BinaryReadStream & shipDefinitionInputStream,
    size_t imageDataSize)
{
    auto const startPos = shipDefinitionInputStream.GetCurrentPosition();

    RgbaImageData image = PngTools::DecodeImageRgba(shipDefinitionInputStream);

    if (shipDefinitionInputStream.GetCurrentPosition() < startPos + imageDataSize)
    {
        shipDefinitionInputStream.Skip(startPos + imageDataSize - shipDefinitionInputStream.GetCurrentPosition());
    }
    else if (shipDefinitionInputStream.GetCurrentPosition() > startPos + imageDataSize)
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }

    return image;
}

RgbaImageData ShipDefinitionFormatDeSerializer::ReadPngImageAndMakePreview(
    BinaryReadStream & shipDefinitionInputStream,
    size_t imageDataSize,
    ImageSize const & maxSize)
{
    RgbaImageData trimmed = ImageTools::TrimTransparent(
        ReadPngImage(shipDefinitionInputStream, imageDataSize));
    return ImageTools::Resize(
        trimmed,
        trimmed.Size.ShrinkToFit(maxSize));
}

void ShipDefinitionFormatDeSerializer::ReadFileHeader(
    BinaryReadStream & shipDefinitionInputStream,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    size_t const szRead = shipDefinitionInputStream.Read(buffer.Receive(sizeof(FileHeader)), sizeof(FileHeader));
    if (szRead != sizeof(FileHeader))
    {
        throw UserGameException(UserGameException::MessageIdType::UnrecognizedShipFile);
    }

    return ReadFileHeader(buffer);
}

void ShipDefinitionFormatDeSerializer::ReadFileHeader(DeSerializationBuffer<BigEndianess> & buffer)
{
    if (std::memcmp(buffer.GetData(), HeaderTitle, sizeof(FileHeader::Title)))
    {
        throw UserGameException(UserGameException::MessageIdType::UnrecognizedShipFile);
    }

    std::uint16_t fileFormatVersion;
    buffer.ReadAt<std::uint16_t>(offsetof(FileHeader, FileFormatVersion), fileFormatVersion);
    if (fileFormatVersion > LatestFileFormatVersion)
    {
        throw UserGameException(UserGameException::MessageIdType::UnsupportedShipFile);
    }
}

ShipDefinitionFormatDeSerializer::ShipAttributes ShipDefinitionFormatDeSerializer::ReadShipAttributes(DeSerializationBuffer<BigEndianess> const & buffer)
{
    std::optional<Version> fsVersion;
    std::optional<ShipSpaceSize> shipSize;
    std::optional<bool> hasTextureLayer;
    std::optional<bool> hasElectricalLayer;

    // Read all tags
    for (size_t offset = 0;;)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
        offset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(ShipAttributesTagType::FSVersion1): // Obsolete
            {
                std::uint16_t versionMajor;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMajor);

                std::uint16_t versionMinor;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMinor);

                fsVersion.emplace(versionMajor, versionMinor, 0, 0);

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::FSVersion2):
            {
                std::uint16_t versionMajor;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMajor);

                std::uint16_t versionMinor;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMinor);

                std::uint16_t versionPatch;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionPatch);

                std::uint16_t versionBuild;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionBuild);

                fsVersion.emplace(versionMajor, versionMinor, versionPatch, versionBuild);

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::ShipSize):
            {
                std::uint32_t width;
                offset += buffer.ReadAt<std::uint32_t>(offset, width);

                std::uint32_t height;
                offset += buffer.ReadAt<std::uint32_t>(offset, height);

                shipSize.emplace(static_cast<ShipSpaceSize::integral_type>(width), static_cast<ShipSpaceSize::integral_type>(height));

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::HasTextureLayer):
            {
                bool hasTextureLayerValue;
                offset += buffer.ReadAt<bool>(offset, hasTextureLayerValue);

                hasTextureLayer.emplace(hasTextureLayerValue);

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::HasElectricalLayer):
            {
                bool hasElectricalLayerValue;
                offset += buffer.ReadAt<bool>(offset, hasElectricalLayerValue);

                hasElectricalLayer.emplace(hasElectricalLayerValue);

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::LastWriteTime):
            {
                PortableTimepoint::value_type value;
                offset += buffer.ReadAt<PortableTimepoint::value_type>(offset, value);

                break;
            }

            case static_cast<uint32_t>(ShipAttributesTagType::Tail):
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized ship attributes tag ", sectionHeader.Tag);

                // Skip it
                offset += sectionHeader.SectionBodySize;

                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(ShipAttributesTagType::Tail))
        {
            // We're done
            break;
        }
    }

    if (!fsVersion.has_value() || !shipSize.has_value() || !hasTextureLayer.has_value() || !hasElectricalLayer.has_value())
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }

    return ShipAttributes(
        *fsVersion,
        *shipSize,
        *hasTextureLayer,
        *hasElectricalLayer);
}

ShipMetadata ShipDefinitionFormatDeSerializer::ReadMetadata(DeSerializationBuffer<BigEndianess> const & buffer)
{
    ShipMetadata metadata("Unknown");

    // Read all tags
    for (size_t offset = 0;;)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
        offset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(MetadataTagType::ArtCredits):
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.ArtCredits = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Author):
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.Author = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Category):
            {
                std::uint64_t category;
                buffer.ReadAt<std::uint64_t>(offset, category);
                metadata.Category = static_cast<ShipCategoryType>(category);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Description):
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.Description = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideElectricalsInPreview):
            {
                buffer.ReadAt<bool>(offset, metadata.DoHideElectricalsInPreview);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideHDInPreview):
            {
                buffer.ReadAt<bool>(offset, metadata.DoHideHDInPreview);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Password):
            {
                std::uint64_t password;
                buffer.ReadAt<std::uint64_t>(offset, password);
                metadata.Password = static_cast<PasswordHash>(password);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Scale):
            {
                float inputUnits;
                size_t readOffset = buffer.ReadAt<float>(offset, inputUnits);

                float outputUnits;
                buffer.ReadAt<float>(offset + readOffset, outputUnits);

                metadata.Scale = ShipSpaceToWorldSpaceCoordsRatio(inputUnits, outputUnits);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::ShipName):
            {
                buffer.ReadAt<std::string>(offset, metadata.ShipName);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::YearBuilt):
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.YearBuilt = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Tail):
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized metadata tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(MetadataTagType::Tail))
        {
            // We're done
            break;
        }

        offset += sectionHeader.SectionBodySize;
    }

    return metadata;
}

ShipPhysicsData ShipDefinitionFormatDeSerializer::ReadPhysicsData(DeSerializationBuffer<BigEndianess> const & buffer)
{
    ShipPhysicsData physicsData;

    // Read all tags
    for (size_t offset = 0;;)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
        offset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(PhysicsDataTagType::OffsetX):
            {
                buffer.ReadAt<float>(offset, physicsData.Offset.x);

                break;
            }

            case static_cast<uint32_t>(PhysicsDataTagType::OffsetY) :
            {
                buffer.ReadAt<float>(offset, physicsData.Offset.y);

                break;
            }

            case static_cast<uint32_t>(PhysicsDataTagType::InternalAirPressure) :
            {
                buffer.ReadAt<float>(offset, physicsData.InternalAirPressure);

                break;
            }

            case static_cast<uint32_t>(PhysicsDataTagType::Tail) :
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized physics data tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(PhysicsDataTagType::Tail))
        {
            // We're done
            break;
        }

        offset += sectionHeader.SectionBodySize;
    }

    return physicsData;
}

ShipAutoTexturizationSettings ShipDefinitionFormatDeSerializer::ReadAutoTexturizationSettings(DeSerializationBuffer<BigEndianess> const & buffer)
{
    ShipAutoTexturizationSettings autoTexturizationSettings;

    // Read all tags
    for (size_t offset = 0;;)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
        offset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(AutoTexturizationSettingsTagType::Mode) :
            {
                std::uint32_t modeValue;
                buffer.ReadAt<std::uint32_t>(offset, modeValue);

                autoTexturizationSettings.Mode = static_cast<ShipAutoTexturizationModeType>(modeValue);

                break;
            }

            case static_cast<uint32_t>(AutoTexturizationSettingsTagType::MaterialTextureMagnification) :
            {
                buffer.ReadAt<float>(offset, autoTexturizationSettings.MaterialTextureMagnification);

                break;
            }

            case static_cast<uint32_t>(AutoTexturizationSettingsTagType::MaterialTextureTransparency) :
            {
                buffer.ReadAt<float>(offset, autoTexturizationSettings.MaterialTextureTransparency);

                break;
            }

            case static_cast<uint32_t>(AutoTexturizationSettingsTagType::Tail) :
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized auto-texturization settings tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(AutoTexturizationSettingsTagType::Tail))
        {
            // We're done
            break;
        }

        offset += sectionHeader.SectionBodySize;
    }

    return autoTexturizationSettings;
}

std::vector<std::unique_ptr<StructuralMaterial const>> ShipDefinitionFormatDeSerializer::ReadCustomMaterialMap(
    DeSerializationBuffer<BigEndianess> const & buffer,
    size_t bufferReadOffset,
    MaterialColorMap<StructuralMaterial> const & baseMaterialColorMap)
{
    size_t offset = bufferReadOffset;

    // Number of entries

    std::uint16_t count;
    offset += buffer.ReadAt<std::uint16_t>(offset, count);

    // Entries

    std::vector<std::unique_ptr<StructuralMaterial const>> customMaterials;

    for (size_t i = 0; i < static_cast<size_t>(count); ++i)
    {
        // Deserialize RGB base color key
        MaterialColorKey baseColorKey;
        offset += buffer.ReadAt(offset, reinterpret_cast<unsigned char *>(&baseColorKey), sizeof(baseColorKey));

        auto const baseSrchIt = baseMaterialColorMap.find(baseColorKey);
        assert(baseSrchIt != baseMaterialColorMap.end());
        if (baseSrchIt == baseMaterialColorMap.end())
        {
            throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
        }

        // Make base variant override, which we'll override with what we find here
        StructuralMaterial::VariantOverridesType variantOverrides = baseSrchIt->second.MakeStartingVariantOverrides();

        // Deserialize variant overrides
        for (;;)
        {
            SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
            offset += sizeof(SectionHeader);

            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Name):
                {
                    buffer.ReadAt<std::string>(offset, variantOverrides.Name);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::RenderColor):
                {
                    buffer.ReadAt(offset, reinterpret_cast<unsigned char *>(&variantOverrides.RenderColor), sizeof(variantOverrides.RenderColor));

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Strength):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.Strength);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Density):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.Density);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::IgnitionTemperature):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.IgnitionTemperature);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::MeltingTemperature):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.MeltingTemperature);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::RotReceptivity):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.RotReceptivity);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::RustReceptivity):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.RustReceptivity);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::WaterSolubility):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.WaterSolubility);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Tail):
                {
                    // We're done
                    break;
                }

                default:
                {
                    // Unrecognized tag
                    LogMessage("WARNING: Unrecognized physics data tag ", sectionHeader.Tag);
                    break;
                }
            }

            if (sectionHeader.Tag == static_cast<uint32_t>(CustomMaterialMapEntryTagType::Tail))
            {
                // We're done
                break;
            }

            offset += sectionHeader.SectionBodySize;
        }

        // Make material
        customMaterials.emplace_back(
            baseSrchIt->second.MakeCustomMaterial(
                variantOverrides));
    }

    return customMaterials;
}

std::vector<std::unique_ptr<ElectricalMaterial const>> ShipDefinitionFormatDeSerializer::ReadCustomMaterialMap(
    DeSerializationBuffer<BigEndianess> const & buffer,
    size_t bufferReadOffset,
    MaterialColorMap<ElectricalMaterial> const & baseMaterialColorMap)
{
    size_t offset = bufferReadOffset;

    // Number of entries

    std::uint16_t count;
    offset += buffer.ReadAt<std::uint16_t>(offset, count);

    // Entries

    std::vector<std::unique_ptr<ElectricalMaterial const>> customMaterials;

    for (size_t i = 0; i < static_cast<size_t>(count); ++i)
    {
        // Deserialize RGB base color key
        MaterialColorKey baseColorKey;
        offset += buffer.ReadAt(offset, reinterpret_cast<unsigned char *>(&baseColorKey), sizeof(baseColorKey));

        auto const baseSrchIt = baseMaterialColorMap.find(baseColorKey);
        assert(baseSrchIt != baseMaterialColorMap.end());
        if (baseSrchIt == baseMaterialColorMap.end())
        {
            throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
        }

        // Make base variant override, which we'll override with what we find here
        ElectricalMaterial::VariantOverridesType variantOverrides = baseSrchIt->second.MakeStartingVariantOverrides();

        // Deserialize variant overrides
        for (;;)
        {
            SectionHeader const sectionHeader = ReadSectionHeader(buffer, offset);
            offset += sizeof(SectionHeader);

            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Name):
                {
                    buffer.ReadAt<std::string>(offset, variantOverrides.Name);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::HeatGenerated):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.HeatGenerated);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Luminiscence):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.Luminiscence);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::LightSpread):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.LightSpread);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::EnginePower):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.EnginePower);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::WaterPumpNominalForce):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.WaterPumpNominalForce);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::TimerDurationSeconds):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.TimerDurationSeconds);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::ThermalSwitchTransitionTemperature):
                {
                    buffer.ReadAt<float>(offset, variantOverrides.ThermalSwitchTransitionTemperature);

                    break;
                }

                case static_cast<uint32_t>(CustomMaterialMapEntryTagType::Tail):
                {
                    // We're done
                    break;
                }

                default:
                {
                    // Unrecognized tag
                    LogMessage("WARNING: Unrecognized physics data tag ", sectionHeader.Tag);
                    break;
                }
            }

            if (sectionHeader.Tag == static_cast<uint32_t>(CustomMaterialMapEntryTagType::Tail))
            {
                // We're done
                break;
            }

            offset += sectionHeader.SectionBodySize;
        }

        // Make material
        customMaterials.emplace_back(
            baseSrchIt->second.MakeCustomMaterial(
                variantOverrides));
    }

    return customMaterials;
}

void ShipDefinitionFormatDeSerializer::ReadStructuralLayer_V1(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
    std::unique_ptr<StructuralLayerData> & structuralLayer)
{
    size_t readOffset = 0;

    // Allocate buffer
    structuralLayer.reset(new StructuralLayerData(shipAttributes.ShipSize));

    // Read all tags
    while (true)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, readOffset);
        readOffset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(StructuralLayerTagType_V1::Buffer) :
            {
                // Decode RLE buffer
                size_t writeOffset = 0;
                StructuralElement * structuralLayerWrite = structuralLayer->Buffer.Data.get();
                for (size_t bufferReadOffset = 0; bufferReadOffset < sectionHeader.SectionBodySize; /*incremented in loop*/)
                {
                    // Deserialize count
                    var_uint16_t count;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, count);

                    // Deserialize colorKey value
                    MaterialColorKey colorKey;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));

                    // Lookup material
                    StructuralMaterial const * material;
                    if (colorKey == EmptyMaterialColorKey)
                    {
                        material = nullptr;
                    }
                    else
                    {
                        auto const materialIt = defaultMaterialColorMap.find(colorKey);
                        if (materialIt == defaultMaterialColorMap.cend())
                        {
                            ThrowMaterialNotFound(shipAttributes);
                        }

                        material = &(materialIt->second);
                    }

                    // Fill material
                    std::fill_n(
                        structuralLayerWrite + writeOffset,
                        count.value(),
                        StructuralElement(material));

                    // Advance
                    writeOffset += count.value();
                }

                assert(writeOffset == shipAttributes.ShipSize.GetLinearSize());

                break;
            }

            case static_cast<uint32_t>(StructuralLayerTagType_V1::Tail) :
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized structural tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(StructuralLayerTagType_V1::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }
}

std::vector<std::unique_ptr<StructuralMaterial const>> ShipDefinitionFormatDeSerializer::ReadStructuralLayer_V2(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
    std::unique_ptr<StructuralLayerData> & structuralLayer)
{
    std::vector<std::unique_ptr<StructuralMaterial const>> customMaterials;

    size_t readOffset = 0;

    // Allocate buffer
    structuralLayer.reset(new StructuralLayerData(shipAttributes.ShipSize));

    // Read all tags
    while (true)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, readOffset);
        readOffset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(StructuralLayerTagType_V2::CustomMaterialsMap):
            {
                customMaterials = ReadCustomMaterialMap(
                    buffer,
                    readOffset,
                    defaultMaterialColorMap);

                break;
            }

            case static_cast<uint32_t>(StructuralLayerTagType_V2::Buffer):
            {
                // Decode RLE buffer
                size_t writeOffset = 0;
                StructuralElement * structuralLayerWrite = structuralLayer->Buffer.Data.get();
                for (size_t bufferReadOffset = 0; bufferReadOffset < sectionHeader.SectionBodySize; /*incremented in loop*/)
                {
                    // Deserialize count
                    var_uint16_t count;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, count);

                    // Deserialize custom material index
                    std::uint16_t customMaterialIndex;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, customMaterialIndex);

                    StructuralMaterial const * material;
                    if (customMaterialIndex != SimulationParameters::MaxCustomMaterialPerShip)
                    {
                        // Custom material
                        if (customMaterialIndex >= customMaterials.size())
                        {
                            throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                        }

                        material = customMaterials[customMaterialIndex].get();
                    }
                    else
                    {
                        // Deserialize colorKey value
                        MaterialColorKey colorKey;
                        bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));

                        // Lookup material
                        if (colorKey == EmptyMaterialColorKey)
                        {
                            material = nullptr;
                        }
                        else
                        {
                            auto const materialIt = defaultMaterialColorMap.find(colorKey);
                            if (materialIt == defaultMaterialColorMap.cend())
                            {
                                ThrowMaterialNotFound(shipAttributes);
                            }

                            material = &(materialIt->second);
                        }
                    }

                    // Fill material
                    std::fill_n(
                        structuralLayerWrite + writeOffset,
                        count.value(),
                        StructuralElement(material));

                    // Advance
                    writeOffset += count.value();
                }

                assert(writeOffset == shipAttributes.ShipSize.GetLinearSize());

                break;
            }

            case static_cast<uint32_t>(StructuralLayerTagType_V2::Tail):
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized structural tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(StructuralLayerTagType_V2::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }

    return customMaterials;
}

void ShipDefinitionFormatDeSerializer::ReadElectricalLayer_V1(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialColorMap<ElectricalMaterial> const & defaultMaterialColorMap,
    std::unique_ptr<ElectricalLayerData> & electricalLayer)
{
    size_t readOffset = 0;

    // Allocate buffer
    electricalLayer.reset(new ElectricalLayerData(shipAttributes.ShipSize));

    // Read all tags
    while (true)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, readOffset);
        readOffset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(ElectricalLayerTagType_V1::Buffer) :
            {
                // Decode RLE buffer
                size_t writeOffset = 0;
                ElectricalElement * electricalLayerWrite = electricalLayer->Buffer.Data.get();
                for (size_t bufferReadOffset = 0; bufferReadOffset < sectionHeader.SectionBodySize; /*incremented in loop*/)
                {
                    // Deserialize count
                    var_uint16_t count;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, count);

                    // Deserialize colorKey value
                    MaterialColorKey colorKey;
                    bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));

                    // Lookup material
                    ElectricalMaterial const * material;
                    if (colorKey == EmptyMaterialColorKey)
                    {
                        material = nullptr;
                    }
                    else
                    {
                        auto const materialIt = defaultMaterialColorMap.find(colorKey);
                        if (materialIt == defaultMaterialColorMap.cend())
                        {
                            ThrowMaterialNotFound(shipAttributes);
                        }

                        material = &(materialIt->second);
                    }

                    // Deserialize instanceID - only if instanced
                    ElectricalElementInstanceIndex instanceId;
                    if (material != nullptr
                        && material->IsInstanced)
                    {
                        var_uint16_t instanceIdTmp;
                        bufferReadOffset += buffer.ReadAt(readOffset + bufferReadOffset, instanceIdTmp);
                        instanceId = static_cast<ElectricalElementInstanceIndex>(instanceIdTmp.value());
                    }
                    else
                    {
                        instanceId = NoneElectricalElementInstanceIndex;
                    }

                    // Fill material
                    std::fill_n(
                        electricalLayerWrite + writeOffset,
                        count.value(),
                        ElectricalElement(material, instanceId));

                    // Advance
                    writeOffset += count.value();
                }

                assert(writeOffset == shipAttributes.ShipSize.GetLinearSize());

                break;
            }

            case static_cast<uint32_t>(ElectricalLayerTagType_V1::Panel) :
            {
                electricalLayer->Panel.Clear();

                size_t elecPanelReadOffset = readOffset;

                // Number of entries
                std::uint16_t entryCount;
                elecPanelReadOffset += buffer.ReadAt<std::uint16_t>(elecPanelReadOffset, entryCount);

                // Entries
                for (std::uint16_t i = 0; i < entryCount; ++i)
                {
                    std::uint32_t instanceIndex;
                    elecPanelReadOffset += buffer.ReadAt<std::uint32_t>(elecPanelReadOffset, instanceIndex);

                    std::optional<IntegralCoordinates> panelCoordinates;
                    bool panelCoordsHasValue;
                    elecPanelReadOffset += buffer.ReadAt<bool>(elecPanelReadOffset, panelCoordsHasValue);
                    if (panelCoordsHasValue)
                    {
                        int32_t x;
                        elecPanelReadOffset += buffer.ReadAt<std::int32_t>(elecPanelReadOffset, x);
                        int32_t y;
                        elecPanelReadOffset += buffer.ReadAt<std::int32_t>(elecPanelReadOffset, y);

                        panelCoordinates = IntegralCoordinates(static_cast<int>(x), static_cast<int>(y));
                    }

                    std::optional<std::string> label;
                    bool labelHasValue;
                    elecPanelReadOffset += buffer.ReadAt<bool>(elecPanelReadOffset, labelHasValue);
                    if (labelHasValue)
                    {
                        std::string labelStr;
                        elecPanelReadOffset += buffer.ReadAt<std::string>(elecPanelReadOffset, labelStr);
                        label = labelStr;
                    }

                    bool isHidden;
                    elecPanelReadOffset += buffer.ReadAt<bool>(elecPanelReadOffset, isHidden);

                    auto const [_, isAdded] = electricalLayer->Panel.TryAdd(
                        static_cast<ElectricalElementInstanceIndex>(instanceIndex),
                        ElectricalPanel::ElementMetadata(
                            panelCoordinates,
                            label,
                            isHidden));

                    if (!isAdded)
                    {
                        LogMessage("WARNING: Duplicate electrical element instance index \"", instanceIndex, "\"");
                    }
                }

                break;
            }

            case static_cast<uint32_t>(ElectricalLayerTagType_V1::Tail) :
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized electrical tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(ElectricalLayerTagType_V1::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }
}

void ShipDefinitionFormatDeSerializer::ReadRopesLayer_V1(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialColorMap<StructuralMaterial> const & defaultMaterialColorMap,
    std::unique_ptr<RopesLayerData> & ropesLayer)
{
    size_t readOffset = 0;

    // Allocate layer
    ropesLayer.reset(new RopesLayerData(shipAttributes.ShipSize));

    // Read all tags
    while (true)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(buffer, readOffset);
        readOffset += sizeof(SectionHeader);

        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(RopesLayerTagType::Buffer) :
            {
                size_t bufferReadOffset = readOffset;

                // Number of entries
                std::uint32_t entryCount;
                bufferReadOffset += buffer.ReadAt<std::uint32_t>(bufferReadOffset, entryCount);

                // Entries
                for (std::uint32_t i = 0; i < entryCount; ++i)
                {
                    // Start coords
                    int32_t startX;
                    bufferReadOffset += buffer.ReadAt<std::int32_t>(bufferReadOffset, startX);
                    int32_t startY;
                    bufferReadOffset += buffer.ReadAt<std::int32_t>(bufferReadOffset, startY);

                    // End coords
                    int32_t endX;
                    bufferReadOffset += buffer.ReadAt<std::int32_t>(bufferReadOffset, endX);
                    int32_t endY;
                    bufferReadOffset += buffer.ReadAt<std::int32_t>(bufferReadOffset, endY);

                    // Deserialize material colorKey value
                    MaterialColorKey colorKey;
                    bufferReadOffset += buffer.ReadAt(bufferReadOffset, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));

                    // Lookup material
                    auto const materialIt = defaultMaterialColorMap.find(colorKey);
                    if (materialIt == defaultMaterialColorMap.cend())
                    {
                        ThrowMaterialNotFound(shipAttributes);
                    }

                    // RenderColor
                    rgbaColor renderColor;
                    bufferReadOffset += buffer.ReadAt(bufferReadOffset, reinterpret_cast<unsigned char *>(&renderColor), sizeof(renderColor));

                    ropesLayer->Buffer.EmplaceBack(
                        ShipSpaceCoordinates(startX, startY),
                        ShipSpaceCoordinates(endX, endY),
                        &(materialIt->second),
                        renderColor);
                }

                break;
            }

            case static_cast<uint32_t>(RopesLayerTagType::Tail) :
            {
                // We're done
                break;
            }

            default:
            {
                // Unrecognized tag
                LogMessage("WARNING: Unrecognized ropes tag ", sectionHeader.Tag);
                break;
            }
        }

        if (sectionHeader.Tag == static_cast<uint32_t>(RopesLayerTagType::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }
}