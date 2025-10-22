/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundAtlas.h"

#include <Core/Log.h>
#include <Core/SysSpecifics.h>

#include <cassert>
#include <optional>
#include <regex>

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
    assert((atlasDataSizeBytes % sizeof(float)) == 0);

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
    //
    // Bake regexes for searching asset names
    //

    struct SearchEntry
    {
        std::string AssetName;
        std::regex AssetNamePattern;
        SoundAssetProperties AssetProperties;
        bool HasBeenVisited;
    };

    std::vector<SearchEntry> assetPropertiesSearchEntries;
    for (auto const & it : assetPropertiesProvider)
    {
        assetPropertiesSearchEntries.emplace_back(SearchEntry({
            it.first,
            std::regex(std::string("^") + it.first + "$"),
            it.second,
            false }));
    }

    //
    // Visit all assets
    //

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
        // Lookup properties
        //

        std::optional<std::size_t> assetPropropertiesIndex;
        for (size_t p = 0; p < assetPropertiesSearchEntries.size(); ++p)
        {
            if (std::regex_match(assetName, assetPropertiesSearchEntries[p].AssetNamePattern))
            {
                LogMessage("    Property match: \"", assetPropertiesSearchEntries[p].AssetProperties.Name, "\"");
                assetPropropertiesIndex = p;
                assetPropertiesSearchEntries[p].HasBeenVisited = true;
                break;
            }

        }

        //
        // Create metadata
        //

        assert(atlasEntriesMetadata.find(assetName) == atlasEntriesMetadata.end());

        atlasEntriesMetadata.emplace(
            assetName,
            SoundAtlasAssetMetadata(
                assetPropropertiesIndex.has_value()
                    ? SoundAssetProperties(
                        assetName,
                        assetPropertiesSearchEntries[*assetPropropertiesIndex].AssetProperties.LoopPoints,
                        assetPropertiesSearchEntries[*assetPropropertiesIndex].AssetProperties.Volume)
                    : SoundAssetProperties(
                        assetName,
                        std::nullopt,
                        1.0f),
                SoundAssetBuffer(
                    static_cast<std::int32_t>(currentInAtlasOffset),
                    static_cast<std::int32_t>(buf.GetSize()))));

        //
        // Write asset
        //

        outputStream.Write(reinterpret_cast<std::uint8_t const *>(buf.data()), Buffer<float>::CalculateByteSize(buf.GetSize()));

        currentInAtlasOffset += buf.GetSize();
    }

    //
    // Ensure we have used all provided asset properties
    //

    for (auto const & ase : assetPropertiesSearchEntries)
    {
        if (!ase.HasBeenVisited)
        {
            LogMessage("WARNING: Properties of asset \"", ase.AssetName, "\" have not been consumed!");
        }
    }

    return SoundAtlasAssetsMetadata(std::move(atlasEntriesMetadata));
}