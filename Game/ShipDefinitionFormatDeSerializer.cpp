/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFormatDeSerializer.h"

#include <GameCore/Colors.h>
#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <cstddef>

ShipDefinition ShipDefinitionFormatDeSerializer::Load(
    std::filesystem::path const & shipFilePath,
    MaterialDatabase const & materialDatabase)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    //
    // Open file
    //

    std::ifstream inputFile = OpenFileForRead(shipFilePath);

    //
    // Read header
    //

    MajorMinorVersion fileFloatingSandboxVersion;
    ReadFileHeader(inputFile, fileFloatingSandboxVersion, buffer);

    //
    // Read and process sections
    //

    std::unique_ptr<StructuralLayerBuffer> structuralLayer;
    ShipMetadata shipMetadata = ShipMetadata(shipFilePath.filename().string());

    while (true)
    {
        SectionHeader const sectionHeader = ReadSectionHeader(inputFile, buffer);
        switch (sectionHeader.Tag)
        {
            case static_cast<uint32_t>(MainSectionTagType::Metadata):
            {
                ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                ReadMetadata(buffer, shipMetadata);

                break;
            }

            case static_cast<uint32_t>(MainSectionTagType::StructuralLayer):
            {
                ReadIntoBuffer(inputFile, buffer, sectionHeader.SectionBodySize);
                ReadStructuralLayer(buffer, materialDatabase.GetStructuralMaterialMap(), structuralLayer);

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

        if (sectionHeader.Tag == static_cast<uint32_t>(MainSectionTagType::Tail))
        {
            // We're done
            break;
        }
    }

    // Close file
    inputFile.close();

    //
    // Ensure all the required sections have been seen
    //

    if (!structuralLayer)
    {
        ThrowInvalidFile();
    }


    return ShipDefinition(
        structuralLayer->Size,
        std::move(*structuralLayer),
        nullptr, // TODO
        nullptr, // TODO
        nullptr, // TODO
        shipMetadata,
        ShipPhysicsData(), // TODO
        std::nullopt); // TODO
}

ShipPreview ShipDefinitionFormatDeSerializer::LoadPreview(std::filesystem::path const & shipFilePath)
{
    // TODOTEST
    return ShipPreview(
        shipFilePath,
        ShipSpaceSize(10, 10),
        ShipMetadata("foo"),
        false,
        false);
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
    // Write metadata
    //

    AppendSection(
        outputFile,
        static_cast<std::uint32_t>(MainSectionTagType::Metadata),
        [&]() { return AppendMetadata(shipDefinition.Metadata, buffer); },
        buffer);

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

unsigned char const HeaderTitle[] = "FLOATING SANDBOX SHIP\x1a\x00\x00";

uint8_t constexpr CurrentFileFormatVersion = 1;

#pragma pack(push, 1)

struct FileHeader
{
    char Title[24];
    std::uint16_t FloatingSandboxVersionMin;
    std::uint16_t FloatingSandboxVersionMaj;
    std::uint8_t FileFormatVersion;
    char Pad[3];
};

static_assert(sizeof(FileHeader) == 32);

#pragma pack(pop)

// Write

void ShipDefinitionFormatDeSerializer::AppendFileHeader(
    std::ofstream & outputFile,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Reset();

    static_assert(sizeof(HeaderTitle) == 24 + 1);

    std::memcpy(
        buffer.Receive(24),
        HeaderTitle,
        24);

    buffer.Append<std::uint16_t>(APPLICATION_VERSION_MAJOR);
    buffer.Append<std::uint16_t>(APPLICATION_VERSION_MINOR);
    buffer.Append<std::uint8_t>(CurrentFileFormatVersion);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);
    buffer.Append<std::uint8_t>(0);

    assert(buffer.GetSize() == sizeof(FileHeader));

    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());
}

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
        size_t valueSize = 0;

        // Tag and size
        buffer.Append(static_cast<std::uint32_t>(MetadataTagType::ElectricalPanelMetadataV1));
        size_t const valueSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

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
        buffer.Append(static_cast<std::uint32_t>(MetadataTagType::Tail));
        buffer.Append(static_cast<std::uint32_t>(0));
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

    return sizeof(std::uint32_t) + sizeof(std::uint32_t) + valueSize;
}

size_t ShipDefinitionFormatDeSerializer::AppendStructuralLayer(
    StructuralLayerBuffer const & structuralLayer,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    size_t sectionBodySize = 0;

    // Append 2D size
    sectionBodySize += buffer.Append(static_cast<std::uint32_t>(structuralLayer.Size.width));
    sectionBodySize += buffer.Append(static_cast<std::uint32_t>(structuralLayer.Size.height));

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

// Read

std::ifstream ShipDefinitionFormatDeSerializer::OpenFileForRead(std::filesystem::path const & shipFilePath)
{
    return std::ifstream(
        shipFilePath,
        std::ios_base::in | std::ios_base::binary);
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
        ThrowInvalidFile();
    }
}

void ShipDefinitionFormatDeSerializer::ReadFileHeader(
    std::ifstream & inputFile,
    MajorMinorVersion & fileFloatingSandboxVersion,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    ReadIntoBuffer(inputFile, buffer, sizeof(FileHeader));

    std::uint8_t fileFormatVersion;
    buffer.ReadAt<std::uint8_t>(offsetof(FileHeader, FileFormatVersion), fileFormatVersion);

    if (std::memcmp(buffer.GetData(), HeaderTitle, sizeof(FileHeader::Title))
        || fileFormatVersion > CurrentFileFormatVersion)
    {
        ThrowInvalidFile();
    }

    // Read FS version
    std::uint16_t majorVersion;
    buffer.ReadAt<std::uint16_t>(offsetof(FileHeader, FloatingSandboxVersionMaj), majorVersion);
    std::uint16_t minorVersion;
    buffer.ReadAt<std::uint16_t>(offsetof(FileHeader, FloatingSandboxVersionMin), minorVersion);

    fileFloatingSandboxVersion = MajorMinorVersion(static_cast<int>(majorVersion), static_cast<int>(minorVersion));
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

void ShipDefinitionFormatDeSerializer::ReadMetadata(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ShipMetadata & metadata)
{
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

            case static_cast<uint32_t>(MetadataTagType::Author) :
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.Author = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Description) :
            {
                std::string tmpStr;
                buffer.ReadAt<std::string>(offset, tmpStr);
                metadata.Description = tmpStr;

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideElectricalsInPreview) :
            {
                buffer.ReadAt<bool>(offset, metadata.DoHideElectricalsInPreview);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideHDInPreview) :
            {
                buffer.ReadAt<bool>(offset, metadata.DoHideHDInPreview);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::ElectricalPanelMetadataV1) :
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

            case static_cast<uint32_t>(MetadataTagType::Password) :
            {
                std::uint64_t password;
                buffer.ReadAt<std::uint64_t>(offset, password);
                metadata.Password = static_cast<PasswordHash>(password);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::ShipName) :
            {
                buffer.ReadAt<std::string>(offset, metadata.ShipName);

                break;
            }

            case static_cast<uint32_t>(MetadataTagType::YearBuilt) :
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
}

void ShipDefinitionFormatDeSerializer::ReadStructuralLayer(
    DeSerializationBuffer<BigEndianess> const & buffer,
    MaterialDatabase::MaterialMap<StructuralMaterial> const & materialMap,
    std::unique_ptr<StructuralLayerBuffer> & structuralLayerBuffer)
{
    size_t readOffset = 0;

    // Read 2D size
    std::uint32_t width;
    readOffset += buffer.ReadAt(readOffset, width);
    std::uint32_t height;
    readOffset += buffer.ReadAt(readOffset, height);
    ShipSpaceSize const shipSize(width, height);

    // Allocate buffer
    structuralLayerBuffer.reset(new StructuralLayerBuffer(shipSize));

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
                // TODOHERE: if not found, throw ad-hoc exception (declared @ this class' public), containing FS version from header
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

    assert(writeOffset == shipSize.GetLinearSize());
}

void ShipDefinitionFormatDeSerializer::ThrowInvalidFile()
{
    throw GameException("The file is not a valid ship definition file");
}