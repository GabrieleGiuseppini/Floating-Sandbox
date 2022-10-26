/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFormatDeSerializer.h"

#include "ImageFileTools.h"

#include <GameCore/Colors.h>
#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Log.h>
#include <GameCore/UserGameException.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace {

unsigned char const HeaderTitle[] = "FLOATING SANDBOX SHIP\x1a\x00\x00";

uint8_t constexpr CurrentFileFormatVersion = 1;

}

ShipDefinition ShipDefinitionFormatDeSerializer::Load(
    std::filesystem::path const & shipFilePath,
    MaterialDatabase const & materialDatabase)
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
    std::unique_ptr<ElectricalLayerData> electricalLayer;
    std::unique_ptr<RopesLayerData> ropesLayer;
    std::unique_ptr<TextureLayerData> textureLayer;
    bool hasSeenTail = false;

    Parse(
        shipFilePath,
        [&](SectionHeader const & sectionHeader, std::ifstream & inputFile) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::ShipAttributes) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipAttributes = ReadShipAttributes(shipFilePath, buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Metadata) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipMetadata = ReadMetadata(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::PhysicsData) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipPhysicsData = ReadPhysicsData(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::AutoTexturizationSettings) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipAutoTexturizationSettings = ReadAutoTexturizationSettings(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::StructuralLayer) :
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    ReadStructuralLayer(
                        buffer,
                        *shipAttributes,
                        materialDatabase.GetStructuralMaterialMap(),
                        structuralLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::ElectricalLayer) :
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    ReadElectricalLayer(
                        buffer,
                        *shipAttributes,
                        materialDatabase.GetElectricalMaterialMap(),
                        electricalLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::RopesLayer) :
                {
                    // Make sure we've already gotten the ship attributes
                    if (!shipAttributes.has_value())
                    {
                        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
                    }

                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    ReadRopesLayer(
                        buffer,
                        *shipAttributes,
                        materialDatabase.GetStructuralMaterialMap(),
                        ropesLayer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::TextureLayer_PNG) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    RgbaImageData image = ReadPngImage(buffer);

                    // Make texture out of this image
                    textureLayer = std::make_unique<TextureLayerData>(std::move(image));

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Tail) :
                {
                    hasSeenTail = true;

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Preview_PNG) :
                {
                    // Ignore and skip section
                    inputFile.seekg(sectionHeader.SectionBodySize, std::ios_base::cur);

                    break;
                }

                default:
                {
                    // Unrecognized tag
                    LogMessage("WARNING: Unrecognized main section tag ", sectionHeader.Tag);

                    // Skip section
                    inputFile.seekg(sectionHeader.SectionBodySize, std::ios_base::cur);

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
            std::move(textureLayer)),
        *shipMetadata,
        shipPhysicsData,
        shipAutoTexturizationSettings);
}

ShipPreviewData ShipDefinitionFormatDeSerializer::LoadPreviewData(std::filesystem::path const & shipFilePath)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read and process sections
    //

    std::optional<ShipAttributes> shipAttributes;
    std::optional<ShipMetadata> shipMetadata;

    Parse(
        shipFilePath,
        [&](SectionHeader const & sectionHeader, std::ifstream & inputFile) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::ShipAttributes):
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipAttributes = ReadShipAttributes(shipFilePath, buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Metadata):
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipMetadata = ReadMetadata(buffer);

                    break;
                }

                default:
                {
                    // Skip section
                    inputFile.seekg(sectionHeader.SectionBodySize, std::ios_base::cur);

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
        shipFilePath,
        shipAttributes->ShipSize,
        *shipMetadata,
        isHD,
        hasElectricals,
        shipAttributes->LastWriteTime);
}

RgbaImageData ShipDefinitionFormatDeSerializer::LoadPreviewImage(
    std::filesystem::path const & previewFilePath,
    ImageSize const & maxSize)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Read until we find a suitable preview
    //

    std::optional<RgbaImageData> previewImage;

    Parse(
        previewFilePath,
        [&](SectionHeader const & sectionHeader, std::ifstream & inputFile) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::TextureLayer_PNG):
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    previewImage.emplace(ReadPngImageAndResize(buffer, maxSize));

                    LogMessage("ShipDefinitionFormatDeSerializer: returning preview from texture layer section");

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Preview_PNG):
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    previewImage.emplace(ReadPngImageAndResize(buffer, maxSize));

                    LogMessage("ShipDefinitionFormatDeSerializer: returning preview from preview section");

                    break;
                }

                default:
                {
                    // Skip section
                    inputFile.seekg(sectionHeader.SectionBodySize, std::ios_base::cur);

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
    std::filesystem::path const & shipFilePath)
{
    DeSerializationBuffer<BigEndianess> buffer(256);    

    //
    // Open file
    //

    std::ofstream outputFile = std::ofstream(
        shipFilePath,
        std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    //
    // Write header
    //

    AppendFileHeader(outputFile, buffer);

    //
    // Write ship attributes
    //

    ShipAttributes const shipAttributes = ShipAttributes(
        Version::CurrentVersion(),
        shipDefinition.Layers.Size,
        shipDefinition.Layers.HasTextureLayer(),
        shipDefinition.Layers.HasElectricalLayer(),
        PortableTimepoint::Now());

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::ShipAttributes),
        [&]() { return AppendShipAttributes(shipAttributes, buffer); },
        buffer);

    //
    // Write metadata
    //

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::Metadata),
        [&]() { return AppendMetadata(shipDefinition.Metadata, buffer); },
        buffer);

    if (shipDefinition.Layers.HasTextureLayer())
    {
        //
        // Write texture
        //

        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::TextureLayer_PNG),
            [&]() { return AppendPngImage(shipDefinition.Layers.TextureLayer->Buffer, buffer); },
            buffer);
    }
    else if (shipDefinition.Layers.HasStructuralLayer())
    {
        //
        // Make and write a preview image
        //

        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::Preview_PNG),
            [&]() { return AppendPngPreview(*shipDefinition.Layers.StructuralLayer, buffer); },
            buffer);
    }

    //
    // Write structural layer
    //

    if (shipDefinition.Layers.HasStructuralLayer())
    {
        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::StructuralLayer),
            [&]() { return AppendStructuralLayer(*shipDefinition.Layers.StructuralLayer, buffer); },
            buffer);
    }

    //
    // Write electrical layer
    //

    if (shipDefinition.Layers.ElectricalLayer)
    {
        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::ElectricalLayer),
            [&]() { return AppendElectricalLayer(*shipDefinition.Layers.ElectricalLayer, buffer); },
            buffer);
    }

    //
    // Write ropes layer
    //

    if (shipDefinition.Layers.RopesLayer)
    {
        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::RopesLayer),
            [&]() { return AppendRopesLayer(*shipDefinition.Layers.RopesLayer, buffer); },
            buffer);
    }

    //
    // Write physics data
    //

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::PhysicsData),
        [&]() { return AppendPhysicsData(shipDefinition.PhysicsData, buffer); },
        buffer);

    //
    // Write auto-texturization settings
    //

    if (shipDefinition.AutoTexturizationSettings.has_value())
    {
        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::AutoTexturizationSettings),
            [&]() { return AppendAutoTexturizationSettings(*shipDefinition.AutoTexturizationSettings, buffer); },
            buffer);
    }

    //
    // Write tail
    //

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::Tail),
        [&]() { return 0; },
        buffer);

    //
    // Close file
    //

    outputFile.flush();
    outputFile.close();
}

PasswordHash ShipDefinitionFormatDeSerializer::CalculatePasswordHash(std::string const & password)
{
    return static_cast<PasswordHash>(std::hash<std::string>{}(password + "fs_salt_0$%"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Write

template<typename TSectionBodyAppender>
void ShipDefinitionFormatDeSerializer::AppendSection(
    std::ofstream & outputFile,
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
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());
}

size_t ShipDefinitionFormatDeSerializer::AppendPngImage(
    RgbaImageData const & rawImageData,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    return ImageFileTools::EncodePngImage(
        rawImageData,
        buffer);
}

void ShipDefinitionFormatDeSerializer::AppendFileHeader(
    std::ofstream & outputFile,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    AppendFileHeader(buffer);
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());
}

void ShipDefinitionFormatDeSerializer::AppendFileHeader(DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    static_assert(sizeof(HeaderTitle) == 24 + 1);

    std::memcpy(
        buffer.Receive(24),
        HeaderTitle,
        24);

    buffer.Append<std::uint16_t>(CurrentFileFormatVersion);
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
        sectionBodySize += AppendShipAttributesEntry(
            ShipAttributesTagType::LastWriteTime,
            shipAttributes.LastWriteTime.Value(),
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
            PhysicsDataTagType::InternalPressure,
            physicsData.InternalPressure,
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

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayer(
    StructuralLayerData const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType::Buffer));
        size_t const subSectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendStructuralLayerBuffer(
            structuralLayer.Buffer,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), subSectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Tail
    //

    {
        buffer.Append(static_cast<uint32_t>(StructuralLayerTagType::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayerBuffer(
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

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayer(
    ElectricalLayerData const & electricalLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Buffer
    //

    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Buffer));
        size_t const sectionBodySizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);

        // Body
        size_t const subSectionBodySize = AppendElectricalLayerBuffer(
            electricalLayer.Buffer,
            buffer);

        buffer.WriteAt(static_cast<std::uint32_t>(subSectionBodySize), sectionBodySizeIndex);

        sectionBodySize += subSectionBodySize;
    }

    //
    // Electrical panel
    //

    if (!electricalLayer.Panel.empty())
    {
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Panel));
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
        buffer.Append(static_cast<uint32_t>(ElectricalLayerTagType::Tail));
        buffer.Append<std::uint32_t>(0);

        static_assert(sizeof(SectionHeader) == sizeof(std::uint32_t) + sizeof(std::uint32_t));
        sectionBodySize += sizeof(SectionHeader);
    }

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayerBuffer(
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

size_t ShipDefinitionFormatDeSerializer::AppendElectricalLayerPanel(
    ElectricalPanelMetadata const & electricalPanel,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    // Number of entries
    std::uint16_t count = static_cast<std::uint16_t>(electricalPanel.size());
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

size_t ShipDefinitionFormatDeSerializer::AppendRopesLayer(
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
        size_t const subSectionBodySize = AppendRopesLayerBuffer(
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

size_t ShipDefinitionFormatDeSerializer::AppendRopesLayerBuffer(
    RopeBuffer const & ropesLayerBuffer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t subSectionBodySize = 0;

    // Number of entries
    std::uint32_t count = static_cast<std::uint32_t>(ropesLayerBuffer.GetSize());
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
                : rgbaColor(EmptyMaterialColorKey, 255);
        }
    }

    //
    // Append preview
    //

    return AppendPngImage(
        previewRawData,
        buffer);
}

// Read

template<typename SectionHandler>
void ShipDefinitionFormatDeSerializer::Parse(
    std::filesystem::path const & shipFilePath,
    SectionHandler const & sectionHandler)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Open file
    //

    std::ifstream inputFile = OpenFileForRead(shipFilePath);

    //
    // Read header
    //

    ReadFileHeader(inputFile, buffer);

    //
    // Read and process sections
    //

    while (true)
    {
        // Read section header
        SectionHeader const sectionHeader = ReadSectionHeader(inputFile, buffer);

        // Handle section
        if (sectionHandler(sectionHeader, inputFile))
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

    // Close file
    inputFile.close();
}

std::ifstream ShipDefinitionFormatDeSerializer::OpenFileForRead(std::filesystem::path const & shipFilePath)
{
    return std::ifstream(
        shipFilePath,
        std::ios_base::in | std::ios_base::binary);
}

void ShipDefinitionFormatDeSerializer::ThrowMaterialNotFound(ShipAttributes const & shipAttributes)
{
    auto const currentVersion = Version::CurrentVersion();
    if (currentVersion < shipAttributes.FileFSVersion)
    {
        // File was created with newer version
        throw UserGameException(
            UserGameException::MessageIdType::LoadShipMaterialNotFoundLaterVersion,
            { shipAttributes.FileFSVersion.ToMajorMinorPatchString() });
    }
    else
    {
        throw UserGameException(UserGameException::MessageIdType::LoadShipMaterialNotFoundSameVersion);
    }
}

void ShipDefinitionFormatDeSerializer::ReadIntoBuffer(
    std::ifstream & inputFile,
    DeSerializationBuffer<BigEndianess> & buffer,
    size_t size)
{
    buffer.Reset();

    inputFile.read(
        reinterpret_cast<char *>(buffer.Receive(size)),
        size);

    if (!inputFile)
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }
}

ShipDefinitionFormatDeSerializer::SectionHeader ShipDefinitionFormatDeSerializer::ReadSectionHeader(
    std::ifstream & inputFile,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    ReadIntoBuffer(inputFile, buffer, sizeof(SectionHeader));
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

RgbaImageData ShipDefinitionFormatDeSerializer::ReadPngImage(DeSerializationBuffer<BigEndianess> & buffer)
{
    return ImageFileTools::DecodePngImage(buffer);
}

RgbaImageData ShipDefinitionFormatDeSerializer::ReadPngImageAndResize(
    DeSerializationBuffer<BigEndianess> & buffer,
    ImageSize const & maxSize)
{
    return ImageFileTools::DecodePngImageAndResize(buffer, maxSize);
}

void ShipDefinitionFormatDeSerializer::ReadFileHeader(
    std::ifstream & inputFile,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    inputFile.read(
        reinterpret_cast<char *>(buffer.Receive(sizeof(FileHeader))),
        sizeof(FileHeader));

    if (!inputFile)
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
    if (fileFormatVersion > CurrentFileFormatVersion)
    {
        throw UserGameException(UserGameException::MessageIdType::UnsupportedShipFile);
    }
}

ShipDefinitionFormatDeSerializer::ShipAttributes ShipDefinitionFormatDeSerializer::ReadShipAttributes(
    std::filesystem::path const & shipFilePath,
    DeSerializationBuffer<BigEndianess> const & buffer)
{
    std::optional<Version> fsVersion;
    std::optional<ShipSpaceSize> shipSize;
    std::optional<bool> hasTextureLayer;
    std::optional<bool> hasElectricalLayer;
    std::optional<PortableTimepoint> lastWriteTime;

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

                lastWriteTime.emplace(PortableTimepoint(value));

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

    if (!lastWriteTime.has_value())
    {
        lastWriteTime = PortableTimepoint::FromLastWriteTime(shipFilePath);
    }

    return ShipAttributes(
        *fsVersion,
        *shipSize,
        *hasTextureLayer,
        *hasElectricalLayer,
        *lastWriteTime);
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

            case static_cast<uint32_t>(MetadataTagType::Description):
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.Description = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Scale) :
            {
                float inputUnits;
                size_t readOffset = buffer.ReadAt<float>(offset, inputUnits);

                float outputUnits;
                buffer.ReadAt<float>(offset + readOffset, outputUnits);

                metadata.Scale = ShipSpaceToWorldSpaceCoordsRatio(inputUnits, outputUnits);

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

            case static_cast<uint32_t>(PhysicsDataTagType::InternalPressure) :
            {
                buffer.ReadAt<float>(offset, physicsData.InternalPressure);

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

void ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
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
            case static_cast<uint32_t>(StructuralLayerTagType::Buffer) :
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
                        auto const materialIt = materialMap.find(colorKey);
                        if (materialIt == materialMap.cend())
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

            case static_cast<uint32_t>(StructuralLayerTagType::Tail) :
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

        if (sectionHeader.Tag == static_cast<uint32_t>(StructuralLayerTagType::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }
}

void ShipDefinitionFormatDeSerializer::ReadElectricalLayer(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialDatabase::MaterialMap<ElectricalMaterial> const & materialMap,
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
            case static_cast<uint32_t>(ElectricalLayerTagType::Buffer) :
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
                        auto const materialIt = materialMap.find(colorKey);
                        if (materialIt == materialMap.cend())
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

            case static_cast<uint32_t>(ElectricalLayerTagType::Panel) :
            {
                electricalLayer->Panel.clear();

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

                    auto const res = electricalLayer->Panel.try_emplace(
                        static_cast<ElectricalElementInstanceIndex>(instanceIndex),
                        ElectricalPanelElementMetadata(
                            panelCoordinates,
                            label,
                            isHidden));

                    if (!res.second)
                    {
                        LogMessage("WARNING: Duplicate electrical element instance index \"", instanceIndex, "\"");
                    }
                }

                break;
            }

            case static_cast<uint32_t>(ElectricalLayerTagType::Tail) :
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

        if (sectionHeader.Tag == static_cast<uint32_t>(ElectricalLayerTagType::Tail))
        {
            // We're done
            break;
        }

        readOffset += sectionHeader.SectionBodySize;
    }
}

void ShipDefinitionFormatDeSerializer::ReadRopesLayer(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
    std::unique_ptr<RopesLayerData> & ropesLayer)
{
    size_t readOffset = 0;

    // Allocate layer
    ropesLayer.reset(new RopesLayerData());

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
                    auto const materialIt = materialMap.find(colorKey);
                    if (materialIt == materialMap.cend())
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