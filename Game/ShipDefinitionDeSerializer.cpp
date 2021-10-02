/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionDeSerializer.h"

#include <GameCore/GameException.h>
#include <GameCore/Version.h>

#include <cassert>
#include <fstream>

ShipDefinition ShipDefinitionDeSerializer::Load(std::filesystem::path const & shipFilePath)
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

ShipPreview ShipDefinitionDeSerializer::LoadPreview(std::filesystem::path const & shipFilePath)
{
    // TODOTEST
    return ShipPreview(
        shipFilePath,
        ShipSpaceSize(10, 10),
        ShipMetadata("foo"),
        false,
        false);
}

void ShipDefinitionDeSerializer::Save(
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

    WriteHeader(buffer);
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());

    //
    // Write metadata
    //

    buffer.Reset();

    WriteMetadata(shipDefinition.Metadata, buffer);
    outputFile.write(reinterpret_cast<char const *>(buffer.GetData()), buffer.GetSize());

    // TODOHERE: other sections

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

void ShipDefinitionDeSerializer::WriteHeader(DeSerializationBuffer<BigEndianess> & buffer)
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

void ShipDefinitionDeSerializer::WriteMetadata(
    ShipMetadata const & metadata,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    // TODOHERE
}