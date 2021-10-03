/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFormatDeSerializer.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/Version.h>

#include <cassert>
#include <cstddef>

ShipDefinition ShipDefinitionFormatDeSerializer::Load(std::filesystem::path const & shipFilePath)
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


    // TODOTEST
    StructuralLayerBuffer sBuf(ShipSpaceSize(10, 10));
    return ShipDefinition(
        ShipSpaceSize(10, 10), // TODO
        std::move(sBuf), // TODO
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
        // TODOHERE
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
    DeSerializationBuffer<BigEndianess> & buffer)
{
    ReadIntoBuffer(inputFile, buffer, sizeof(FileHeader));

    if (std::memcmp(buffer.GetData(), HeaderTitle, sizeof(FileHeader::Title))
        || buffer.ReadAt<std::uint8_t>(offsetof(FileHeader, FileFormatVersion)) > CurrentFileFormatVersion)
    {
        ThrowInvalidFile();
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
    return SectionHeader{
        buffer.ReadAt<std::uint32_t>(offset),
        buffer.ReadAt<std::uint32_t>(offset + sizeof(std::uint32_t))
    };
}

void ShipDefinitionFormatDeSerializer::ReadMetadata(
    DeSerializationBuffer<BigEndianess> & buffer,
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
                metadata.ArtCredits = buffer.ReadAt<std::string>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Author) :
            {
                metadata.Author = buffer.ReadAt<std::string>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Description) :
            {
                metadata.Description = buffer.ReadAt<std::string>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideElectricalsInPreview) :
            {
                metadata.DoHideElectricalsInPreview = buffer.ReadAt<bool>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::DoHideHDInPreview) :
            {
                metadata.DoHideHDInPreview = buffer.ReadAt<bool>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::ElectricalPanelMetadata) :
            {
                // TODOHERE
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::Password) :
            {
                metadata.Password = static_cast<PasswordHash>(buffer.ReadAt<std::uint64_t>(offset));
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::ShipName) :
            {
                metadata.ShipName = buffer.ReadAt<std::string>(offset);
                break;
            }

            case static_cast<uint32_t>(MetadataTagType::YearBuilt) :
            {
                metadata.YearBuilt = buffer.ReadAt<std::string>(offset);
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

void ShipDefinitionFormatDeSerializer::ThrowInvalidFile()
{
    throw GameException("The file is not a valid ship definition file");
}