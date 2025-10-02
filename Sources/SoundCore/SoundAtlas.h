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
    std::string Name;
    SoundAssetProperties Properties;
    SoundAssetBuffer Buffer;

    SoundAtlasAssetMetadata(
        std::string const & name,
        SoundAssetProperties const & properties,
        SoundAssetBuffer const & buffer)
        : Name(name)
        , Properties(properties)
        , Buffer(buffer)
    { }
};

struct SoundAtlas
{
public:

    // Metadata - indexed by asset name
    std::unordered_map<std::string, SoundAtlasAssetMetadata> AssetsMetadata;

    // The buffer itself, owned by the atlas
    Buffer<float> AtlasBuffer;

    SoundAtlas(
        std::unordered_map<std::string, SoundAtlasAssetMetadata> && assetsMetadata,
        Buffer<float> && atlasBuffer)
        : AssetsMetadata(std::move(assetsMetadata))
        , AtlasBuffer(std::move(atlasBuffer))
    {
    }

    //
    // De/Serialization
    //

    static SoundAtlas Deserialize(
        picojson::value const & atlasJson,
        BinaryReadStream & atlasData);
};

class SoundAtlasBuilder
{
public:

    static SoundAtlas BuildAtlas(
        std::vector<std::string> const & assetNames,
        std::unordered_map<std::string, SoundAssetProperties> assetPropertiesProvider,
        std::function<Buffer<float>(std::string const & assetName)> const & assetLoader);
};