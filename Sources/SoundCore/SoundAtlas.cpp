/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundAtlas.h"

#include <Core/Log.h>
#include <Core/SysSpecifics.h>

#include <cassert>

SoundAtlas SoundAtlas::Deserialize(
    picojson::value const & atlasJson,
    BinaryReadStream & atlasDataStream)
{
    //
    // Load metadata
    //

    SoundAtlasAssetsMetadata metadata = SoundAtlasAssetsMetadata::Dererialize(atlasJson);

    //
    // Load entire stream into buffer
    //

    auto const atlasDataSizeBytes = atlasDataStream.GetSize();

    assert((atlasDataSizeBytes % vectorization_byte_count<size_t>) == 0);

    auto const atlastDataSizeFloats = atlasDataSizeBytes / sizeof(float);
    Buffer<float> buf(atlastDataSizeFloats);
    atlasDataStream.Read(reinterpret_cast<std::uint8_t *>(buf.data()), atlasDataSizeBytes);

    return SoundAtlas(
        std::move(metadata),
        std::move(buf));
}

SoundAtlasAssetsMetadata SoundAtlasBuilder::BuildAtlas(
    std::vector<std::string> const & assetNames,
    std::unordered_map<std::string, SoundAssetProperties> const & assetPropertiesProvider,
    std::function<Buffer<float>(std::string const & assetName)> const & assetLoader,
    BinaryWriteStream & outputStream)
{
    std::unordered_map<std::string, SoundAtlasAssetMetadata> atlasEntriesMetadata;

    size_t currentInAtlasOffset = 0; // Floats
    for (auto const & assetName : assetNames)
    {
        LogMessage("Loading sound asset \"", assetName, "\"");

        //
        // Load asset
        //

        Buffer<float> buf = assetLoader(assetName);

        //
        // Create metadata
        //

        auto const & assetPropertiesSearchIt = assetPropertiesProvider.find(assetName);

        assert(atlasEntriesMetadata.find(assetName) == atlasEntriesMetadata.end());

        atlasEntriesMetadata.emplace(
            assetName,
            SoundAtlasAssetMetadata(
                assetName, // TODO
                assetPropertiesSearchIt != assetPropertiesProvider.end()
                    ? assetPropertiesSearchIt->second
                    : SoundAssetProperties(
                        assetName,
                        std::nullopt,
                        1.0f),
                    SoundAssetBuffer(
                        currentInAtlasOffset,
                        buf.GetSize())));

        //
        // Write asset
        //

        outputStream.Write(reinterpret_cast<std::uint8_t const *>(buf.data()), Buffer<float>::CalculateByteSize(buf.GetSize()));

        currentInAtlasOffset += buf.GetSize();
    }

    return SoundAtlasAssetsMetadata(std::move(atlasEntriesMetadata));
}