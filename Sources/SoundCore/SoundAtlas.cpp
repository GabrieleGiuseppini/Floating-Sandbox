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
    std::function<std::unique_ptr<BinaryReadStream>(size_t atlasFileIndex)> && atlasDataInputStreamFactory)
{
    //
    // Load metadata
    //

    SoundAtlasAssetsMetadata metadata = SoundAtlasAssetsMetadata::Dererialize(atlasJson);

    //
    // Load entire stream into buffer
    //

    size_t totalAtlasDataSizeFloats = 0;
    size_t atlasDataFileCount = 0;
    for (size_t atlasFileIndex = 1; ; ++atlasFileIndex)
    {
        auto atlasFileInputStream = atlasDataInputStreamFactory(atlasFileIndex);
        if (!atlasFileInputStream)
            break;

        auto const atlasFileSizeBytes = atlasFileInputStream->GetSize();
        assert((atlasFileSizeBytes % sizeof(float)) == 0);
        totalAtlasDataSizeFloats += atlasFileSizeBytes / sizeof(float);
        ++atlasDataFileCount;
    }

    Buffer<float> buf(totalAtlasDataSizeFloats);

    size_t currentBufferIndexFloats = 0;
    for (size_t atlasFileIndex = 1; atlasFileIndex < atlasDataFileCount; ++atlasFileIndex)
    {
        LogMessage("Loading sound atlas data file ", atlasFileIndex);

        auto atlasFileInputStream = atlasDataInputStreamFactory(atlasFileIndex);
        assert(atlasFileInputStream);

        auto const atlasFileSizeBytes = atlasFileInputStream->GetSize();
        assert((atlasFileSizeBytes % sizeof(float)) == 0);
        atlasFileInputStream->Read(reinterpret_cast<std::uint8_t *>(&buf[currentBufferIndexFloats]), atlasFileSizeBytes);

        currentBufferIndexFloats += atlasFileSizeBytes / sizeof(float);
    }

    return SoundAtlas(
        std::move(metadata),
        std::move(buf));
}

SoundAtlasAssetsMetadata SoundAtlasBuilder::BuildAtlas(
    std::vector<std::string> const & assetNames,
    std::unordered_map<std::string, SoundAssetProperties> const & assetPropertiesProvider,
    std::function<Buffer<float>(std::string const & assetName)> const & assetLoader,
    size_t maxAtlasFileSizeBytes,
    std::function<std::unique_ptr<BinaryWriteStream>(size_t atlasFileIndex)> && outputStreamFactory)
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

    size_t currentAtlasFileSizeBytes = 0;
    size_t currentAtlasFileIndex = 1; // We start at 1
    std::unique_ptr<BinaryWriteStream> currentAtlasFileOutputStream;
    std::unordered_map<std::string, SoundAtlasAssetMetadata> atlasEntriesMetadata;

    size_t currentInAtlasOffsetFloats = 0; // Floats

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
                    static_cast<std::int32_t>(currentInAtlasOffsetFloats),
                    static_cast<std::int32_t>(buf.GetSize()))));

        //
        // Write asset
        //

        if (!currentAtlasFileOutputStream)
        {
            // Create new stream
            currentAtlasFileOutputStream = outputStreamFactory(currentAtlasFileIndex);
            currentAtlasFileSizeBytes = 0;
            ++currentAtlasFileIndex;
        }

        // Write
        size_t const assetByteSize = Buffer<float>::CalculateByteSize(buf.GetSize());
        currentAtlasFileOutputStream->Write(reinterpret_cast<std::uint8_t const *>(buf.data()), assetByteSize);
        currentAtlasFileSizeBytes += assetByteSize;
        currentInAtlasOffsetFloats += buf.GetSize();

        // Check if exceeded size
        if (currentAtlasFileSizeBytes >= maxAtlasFileSizeBytes)
        {
            // Close current file
            assert(currentAtlasFileOutputStream);
            currentAtlasFileOutputStream.reset();
        }
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