/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundTypes.h"

#include <Core/Buffer.h>
#include <Core/Streams.h>

#include <functional>
#include <picojson.h>
#include <string>
#include <unordered_map>
#include <vector>

struct SoundAtlasAssetMetadata
{
    SoundAssetProperties Properties;
    SoundAssetBuffer Buffer;

    SoundAtlasAssetMetadata(
        SoundAssetProperties const & properties,
        SoundAssetBuffer const & buffer)
        : Properties(properties)
        , Buffer(buffer)
    { }

    picojson::value Serialize() const
    {
        picojson::object root;

        root.emplace("properties", Properties.Serialize());
        root.emplace("buffer", Buffer.Serialize());

        return picojson::value(root);
    }

    static SoundAtlasAssetMetadata Deserialize(picojson::value const & value)
    {
        auto const & rootObject = Utils::GetJsonValueAsObject(value, "SoundAtlasAssetMetadata");

        auto const assetName = Utils::GetMandatoryJsonMember<std::string>(rootObject, "name");

        return SoundAtlasAssetMetadata(
            SoundAssetProperties::Deserialize(assetName, Utils::GetMandatoryJsonValue(rootObject, "properties")),
            SoundAssetBuffer::Deserialize(Utils::GetMandatoryJsonValue(rootObject, "buffer")));
    }
};

struct SoundAtlasAssetsMetadata
{
    // Indexed by asset name
    std::unordered_map<std::string, SoundAtlasAssetMetadata> Entries;

    explicit SoundAtlasAssetsMetadata(std::unordered_map<std::string, SoundAtlasAssetMetadata> && entries)
        : Entries(std::move(entries))
    {
    }

    picojson::value Serialize() const
    {
        picojson::object root;

        for (auto const & entry : Entries)
        {
            root.emplace(entry.first, entry.second.Serialize());
        }

        return picojson::value(root);
    }

    static SoundAtlasAssetsMetadata Dererialize(picojson::value const & value)
    {
        auto const & rootObject = Utils::GetJsonValueAsObject(value, "SoundAtlasAssetsMetadata");

        std::unordered_map<std::string, SoundAtlasAssetMetadata> entries;
        for (auto const & entry : rootObject)
        {
            entries.emplace(entry.first, SoundAtlasAssetMetadata::Deserialize(entry.second));
        }

        return SoundAtlasAssetsMetadata(std::move(entries));
    }
};

struct SoundAtlas
{
public:

    // Metadata - indexed by asset name
    SoundAtlasAssetsMetadata AssetsMetadata;

    // The buffer itself, owned by the atlas
    Buffer<float> AtlasData;

    SoundAtlas(
        SoundAtlasAssetsMetadata && assetsMetadata,
        Buffer<float> && atlasData)
        : AssetsMetadata(std::move(assetsMetadata))
        , AtlasData(std::move(atlasData))
    {
    }

    //
    // De/Serialization
    //

    static SoundAtlas Deserialize(
        picojson::value const & atlasJson,
        BinaryReadStream & atlasDataStream);
};

class SoundAtlasBuilder
{
public:

    static SoundAtlasAssetsMetadata BuildAtlas(
        std::vector<std::string> const & assetNames,
        std::unordered_map<std::string, SoundAssetProperties> const & assetPropertiesProvider,
        std::function<Buffer<float>(std::string const & assetName)> const & assetLoader,
        BinaryWriteStream & outputStream);
};