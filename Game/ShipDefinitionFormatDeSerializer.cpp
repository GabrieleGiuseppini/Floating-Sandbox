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
    std::unique_ptr<StructuralLayerBuffer> structuralLayer;
    std::unique_ptr<TextureLayerBuffer> textureLayer;

    Parse(
        shipFilePath,
        [&](SectionHeader const & sectionHeader, std::ifstream & inputFile) -> bool
        {
            switch (sectionHeader.Tag)
            {
                case static_cast<uint32_t>(MainSectionTagType::ShipAttributes) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipAttributes = ReadShipAttributes(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Metadata) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    shipMetadata = ReadMetadata(buffer);

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::StructuralLayer) :
                {
                    // Make sure we've already gotten the ship size
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

                case static_cast<uint32_t>(MainSectionTagType::TextureLayer_PNG) :
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    RgbaImageData image = ReadPngImage(buffer);

                    // Make texture out of this image
                    textureLayer = std::make_unique<RgbaImageData>(std::move(image));

                    break;
                }

                // TODOHERE: other sections

                case static_cast<uint32_t>(MainSectionTagType::Tail) :
                {
                    // We're done
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

    if (!shipAttributes.has_value() || !shipMetadata.has_value() || !structuralLayer)
    {
        throw UserGameException(UserGameException::MessageIdType::InvalidShipFile);
    }


    return ShipDefinition(
        shipAttributes->ShipSize,
        std::move(*structuralLayer),
        nullptr, // TODO
        nullptr, // TODO
        std::move(textureLayer),
        *shipMetadata,
        ShipPhysicsData(), // TODO
        std::nullopt); // TODO
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
                    shipAttributes = ReadShipAttributes(buffer);

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
        hasElectricals);
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

                    LogMessage("Gotten preview from texture layer");

                    break;
                }

                case static_cast<uint32_t>(MainSectionTagType::Preview_PNG):
                {
                    ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                    previewImage.emplace(ReadPngImageAndResize(buffer, maxSize));

                    LogMessage("Gotten preview from preview");

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
        APPLICATION_VERSION_MAJOR,
        APPLICATION_VERSION_MINOR,
        shipDefinition.StructuralLayer.Size,
        shipDefinition.TextureLayer != nullptr,
        shipDefinition.ElectricalLayer != nullptr);

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

    if (shipDefinition.TextureLayer)
    {
        //
        // Write texture
        //

        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::TextureLayer_PNG),
            [&]() { return AppendPngImage(*shipDefinition.TextureLayer, buffer); },
            buffer);
    }
    else
    {
        //
        // Make and write a preview image
        //

        AppendSection(
            outputFile,
            static_cast<std::uint32_t>(MainSectionTagType::Preview_PNG),
            [&]() { return AppendPngPreview(shipDefinition.StructuralLayer, buffer); },
            buffer);
    }

    //
    // Write structural layer
    //

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::StructuralLayer),
        [&]() { return AppendStructuralLayer(shipDefinition.StructuralLayer, buffer); },
        buffer);

    // TODOHERE: other sections

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Write

template<typename TSectionBodyAppender>
static void ShipDefinitionFormatDeSerializer::AppendSection(
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
        buffer.Append(static_cast<std::uint32_t>(ShipAttributesTagType::FSVersion));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        size_t valueSize = 0;

        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersionMaj));
        valueSize += buffer.Append(static_cast<uint16_t>(shipAttributes.FileFSVersionMin));

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

    if (!metadata.ElectricalPanelMetadata.empty())
    {
        // Tag and size
        buffer.Append(static_cast<std::uint32_t>(MetadataTagType::ElectricalPanelMetadata_V1));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

        size_t valueSize = 0;

        // Number of entries
        std::uint16_t count = static_cast<std::uint16_t>(metadata.ElectricalPanelMetadata.size());
        valueSize += buffer.Append(count);

        // Entries
        for (auto const & entry : metadata.ElectricalPanelMetadata)
        {
            valueSize += buffer.Append(static_cast<std::uint32_t>(entry.first));

            valueSize += buffer.Append(entry.second.PanelCoordinates.has_value());
            if (entry.second.PanelCoordinates.has_value())
            {
                valueSize += buffer.Append(static_cast<std::int32_t>(entry.second.PanelCoordinates->x));
                valueSize += buffer.Append(static_cast<std::int32_t>(entry.second.PanelCoordinates->y));
            }

            valueSize += buffer.Append(entry.second.Label.has_value());
            if (entry.second.Label.has_value())
            {
                valueSize += buffer.Append(*entry.second.Label);
            }

            valueSize += buffer.Append(entry.second.IsHidden);
        }

        // Store size
        buffer.WriteAt(static_cast<std::uint32_t>(valueSize), valueSizeIndex);

        sectionBodySize += sizeof(std::uint32_t) + sizeof(std::uint32_t) + valueSize;
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

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayer(
    StructuralLayerBuffer const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    //
    // Encode layer with RLE of RGB color key buffer
    //

    size_t const layerLinearSize = structuralLayer.Size.GetLinearSize();
    DeSerializationBuffer<BigEndianess> rleBuffer(layerLinearSize * sizeof(MaterialColorKey)); // Upper bound

    StructuralElement const * structuralElementBuffer = structuralLayer.Data.get();
    for (size_t i = 0; i < layerLinearSize; /*incremented in loop*/)
    {
        // Get the material at this positioj
        auto const * material = structuralElementBuffer[i].Material;
        ++i;

        // Count consecutive identical values
        std::uint16_t materialCount = 1;
        for (;
            i < layerLinearSize
            && structuralElementBuffer[i].Material == material
            && materialCount < std::numeric_limits<var_uint16_t>::max().value();
            ++i, ++materialCount);

        // Serialize
        rleBuffer.Append<var_uint16_t>(var_uint16_t(materialCount));
        MaterialColorKey const colorKey = material == nullptr ? EmptyMaterialColorKey : material->ColorKey;
        rleBuffer.Append(reinterpret_cast<unsigned char const *>(&colorKey), sizeof(MaterialColorKey));
    }

    //
    // Serialize RLE buffer
    //

    sectionBodySize += buffer.Append(
        rleBuffer.GetData(),
        rleBuffer.GetSize());

    return sectionBodySize;
}

size_t ShipDefinitionFormatDeSerializer::AppendPngPreview(
    StructuralLayerBuffer const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    //
    // Make preview
    //

    RgbaImageData previewRawData = RgbaImageData(ImageSize(structuralLayer.Size.width, structuralLayer.Size.height));

    std::transform(
        structuralLayer.Data.get(),
        structuralLayer.Data.get() + structuralLayer.Size.GetLinearSize(),
        previewRawData.Data.get(),
        [](StructuralElement const & element) -> rgbaColor
        {
            if (element.Material != nullptr)
                return rgbaColor(element.Material->RenderColor, 255);
            else
                return rgbaColor(EmptyMaterialColorKey, 255);
        });

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
    if (std::tuple(currentVersion.GetMajor(), currentVersion.GetMinor())
        < std::tuple(shipAttributes.FileFSVersionMaj, shipAttributes.FileFSVersionMin))
    {
        throw UserGameException(
            UserGameException::MessageIdType::LoadShipMaterialNotFoundLaterVersion,
            { std::to_string(shipAttributes.FileFSVersionMaj) + "." + std::to_string(shipAttributes.FileFSVersionMin) });
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

ShipDefinitionFormatDeSerializer::ShipAttributes ShipDefinitionFormatDeSerializer::ReadShipAttributes(DeSerializationBuffer<BigEndianess> const & buffer)
{
    std::optional<std::tuple<std::uint16_t, std::uint16_t>> fsVersion;
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
            case static_cast<uint32_t>(ShipAttributesTagType::FSVersion):
            {
                std::uint16_t versionMaj;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMaj);

                std::uint16_t versionMin;
                offset += buffer.ReadAt<std::uint16_t>(offset, versionMin);

                fsVersion.emplace(std::make_tuple(versionMaj, versionMin));

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
        std::get<0>(*fsVersion),
        std::get<1>(*fsVersion),
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

            case static_cast<uint32_t>(MetadataTagType::ElectricalPanelMetadata_V1):
            {
                metadata.ElectricalPanelMetadata.clear();

                size_t elecPanelOffset = offset;

                // Number of entries
                std::uint16_t entryCount;
                elecPanelOffset += buffer.ReadAt<std::uint16_t>(elecPanelOffset, entryCount);

                // Entries
                for (int i = 0; i < entryCount; ++i)
                {
                    std::uint32_t instanceIndex;
                    elecPanelOffset += buffer.ReadAt<std::uint32_t>(elecPanelOffset, instanceIndex);

                    std::optional<IntegralCoordinates> panelCoordinates;
                    bool panelCoordsHasValue;
                    elecPanelOffset += buffer.ReadAt<bool>(elecPanelOffset, panelCoordsHasValue);
                    if (panelCoordsHasValue)
                    {
                        int32_t x;
                        elecPanelOffset += buffer.ReadAt<std::int32_t>(elecPanelOffset, x);
                        int32_t y;
                        elecPanelOffset += buffer.ReadAt<std::int32_t>(elecPanelOffset, y);

                        panelCoordinates = IntegralCoordinates(static_cast<int>(x), static_cast<int>(y));
                    }

                    std::optional<std::string> label;
                    bool labelHasValue;
                    elecPanelOffset += buffer.ReadAt<bool>(elecPanelOffset, labelHasValue);
                    if (labelHasValue)
                    {
                        std::string labelStr;
                        elecPanelOffset += buffer.ReadAt<std::string>(elecPanelOffset, labelStr);
                        label = labelStr;
                    }

                    bool isHidden;
                    elecPanelOffset += buffer.ReadAt<bool>(elecPanelOffset, isHidden);

                    auto const res = metadata.ElectricalPanelMetadata.try_emplace(
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

void ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipAttributes const & shipAttributes,
    MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
    std::unique_ptr<StructuralLayerBuffer> & structuralLayerBuffer)
{
    size_t readOffset = 0;

    // Allocate buffer
    structuralLayerBuffer.reset(new StructuralLayerBuffer(shipAttributes.ShipSize));

    // Decode RLE buffer
    size_t writeOffset = 0;
    StructuralElement * structuralLayerWrite = structuralLayerBuffer->Data.get();
    for (; readOffset < buffer.GetSize(); /*incremented in loop*/)
    {
        // Count
        var_uint16_t count;
        readOffset += buffer.ReadAt(readOffset, count);

        // ColorKey value
        MaterialColorKey colorKey;
        readOffset += buffer.ReadAt(readOffset, reinterpret_cast<unsigned char *>(&colorKey), sizeof(colorKey));

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
}