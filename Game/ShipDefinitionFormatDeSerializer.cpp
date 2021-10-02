/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFormatDeSerializer.h"

#include <GameCore/GameException.h>
#include <GameCore/Version.h>

#include <cassert>
#include <fstream>

ShipDefinition ShipDefinitionFormatDeSerializer::Load(std::filesystem::path const & shipFilePath)
{
    // TODOTEST
    StructuralLayerBuffer sBuf(ShipSpaceSize(10, 10));
    return ShipDefinition(
        ShipSpaceSize(10, 10),
        std::move(sBuf),
        nullptr,
        nullptr,
        nullptr,
        ShipMetadata("foo"),
        ShipPhysicsData(),
        std::nullopt);
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

    buffer.Reset();

    AppendHeader(buffer);
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());

    //
    // Write metadata
    //

    buffer.Reset();

    AppendMetadata(shipDefinition.Metadata, buffer);
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());

    // TODOHERE: other sections

    //
    // Write tail
    //

    buffer.Reset();
    buffer.Append(static_cast<uint32_t>(SectionTagType::Tail));
    buffer.Append(static_cast<uint32_t>(SectionTagType::Tail));
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());

    //
    // Close file
    //

    outputFile.flush();
    outputFile.close();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned char const HeaderTitle[] = "FLOATING SANDBOX SHIP\x1a\x00\x00";

uint8_t constexpr CurrentFileFormatVersion = 1;

struct Header
{
    char Title[24];
    std::uint16_t FloatingSandboxVersionMin;
    std::uint16_t FloatingSandboxVersionMaj;
    std::uint8_t FileFormatVersion;
    char Pad[3];
};

struct SectionHeader
{
    std::uint32_t SectionTagType;
    std::uint32_t SectionBodySize; // Excluding header
};

void ShipDefinitionFormatDeSerializer::AppendHeader(DeSerializationBuffer<BigEndianess> & buffer)
{
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

    assert(buffer.GetSize() == 32);
}

void ShipDefinitionFormatDeSerializer::AppendMetadata(
    ShipMetadata const & metadata,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    buffer.Append(static_cast<std::uint32_t>(SectionTagType::Metadata));

    size_t const headerSizeIndex = buffer.ReserveAndAdvance<std::uint32_t>();

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

    // Populate hader
    buffer.WriteAt(static_cast<std::uint32_t>(sectionBodySize), headerSizeIndex);
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