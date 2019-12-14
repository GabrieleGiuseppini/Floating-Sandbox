/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextureDatabase.h"

#include <GameCore/EnumFlags.h>
#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <memory>
#include <numeric>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Render {

/*
 * Atlas creation options.
 */
enum class AtlasOptions
{
    None = 0,
    AlphaPremultiply = 1
};

/*
 * Metadata about one single frame in a texture atlas.
 */
template <typename TextureGroups>
struct TextureAtlasFrameMetadata
{
public:

    vec2f TextureCoordinatesBottomLeft;
    vec2f TextureCoordinatesTopRight;

    int FrameLeftX;
    int FrameBottomY;

    TextureFrameMetadata<TextureGroups> FrameMetadata;

    TextureAtlasFrameMetadata(
        vec2f textureCoordinatesBottomLeft,
        vec2f textureCoordinatesTopRight,
        int frameLeftX,
        int frameBottomY,
        TextureFrameMetadata<TextureGroups> frameMetadata)
        : TextureCoordinatesBottomLeft(textureCoordinatesBottomLeft)
        , TextureCoordinatesTopRight(textureCoordinatesTopRight)
        , FrameLeftX(frameLeftX)
        , FrameBottomY(frameBottomY)
        , FrameMetadata(frameMetadata)
    {}

    void Serialize(picojson::object & root) const;

    static TextureAtlasFrameMetadata Deserialize(picojson::object const & root);
};

/*
 * Metadata about a whole texture atlas.
 */
template <typename TextureGroups>
struct TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(
        ImageSize size,
        AtlasOptions options,
        std::vector<TextureAtlasFrameMetadata<TextureGroups>> && frames);

    inline ImageSize const & GetSize() const
    {
        return mSize;
    }

    inline bool IsAlphaPremultiplied() const
    {
        return !!(mOptions & AtlasOptions::AlphaPremultiply);
    }

    inline TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(TextureFrameId<TextureGroups> const & textureFrameId) const
    {
        return GetFrameMetadata(textureFrameId.Group, textureFrameId.FrameIndex);
    }

    inline TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        assert(frameIndex < mFrameMetadataIndices[static_cast<size_t>(group)].size());
        return mFrameMetadata[mFrameMetadataIndices[static_cast<size_t>(group)][frameIndex]];
    }

    inline std::vector<TextureAtlasFrameMetadata<TextureGroups>> const & GetFrameMetadata() const
    {
        return mFrameMetadata;
    }

    int GetMaxDimension() const;

    void Serialize(picojson::object & root) const;

    static TextureAtlasMetadata Deserialize(picojson::object const & root);

private:

    ImageSize const mSize;

    AtlasOptions const mOptions;

    std::vector<TextureAtlasFrameMetadata<TextureGroups>> mFrameMetadata;

    // Indexed by group first and frame index then
    std::vector<std::vector<size_t>> mFrameMetadataIndices;
};

/*
 * A texture atlas.
 */
template <typename TextureGroups>
struct TextureAtlas
{
public:

    // Metadata
    TextureAtlasMetadata<TextureGroups> Metadata;

    // The image itself
    RgbaImageData AtlasData;

    TextureAtlas(
        TextureAtlasMetadata<TextureGroups> && metadata,
        RgbaImageData && atlasData)
        : Metadata(std::move(metadata))
        , AtlasData(std::move(atlasData))
    {}

    //
    // De/Serialization
    //

    void Serialize(
        std::string const & databaseName,
        std::filesystem::path const & outputDirectoryPath) const;

    static TextureAtlas Deserialize(
        std::string const & databaseName,
        std::filesystem::path const & databaseRootDirectoryPath);

private:

    static std::filesystem::path MakeMetadataFilename(std::string const & databaseName)
    {
        return databaseName + ".atlas.json";
    }

    static std::filesystem::path MakeImageFilename(std::string const & databaseName)
    {
        return databaseName + ".atlas.png";
    }
};

template <typename TextureGroups>
class TextureAtlasBuilder
{
public:

    /*
     * Builds an atlas with the specified group.
     */
    static TextureAtlas<TextureGroups> BuildAtlas(
        TextureGroup<TextureGroups> const & group,
        AtlasOptions options,
        ProgressCallback const & progressCallback);

    /*
     * Builds an atlas with the specified database, composed of a power of two number of
     * frames with identical sizes.
     */
    template<typename TextureDatabaseTraits>
    static TextureAtlas<TextureGroups> BuildRegularAtlas(
        TextureDatabase<TextureDatabaseTraits> const & database,
        AtlasOptions options,
        ProgressCallback const & progressCallback)
    {
        static_assert(std::is_same<TextureGroups, TextureDatabaseTraits::TextureGroups>::value);

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            AddTextureInfos(group, textureInfos);
        }

        // Build specification
        auto const specification = BuildRegularAtlasSpecification(textureInfos);

        // Build atlas
        return BuildAtlas(
            specification,
            options,
            [&database](TextureFrameId<TextureGroups> const & frameId)
            {
                return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex);
            },
            progressCallback);
    }

    /*
     * Builds an atlas with the entire content of the specified database.
     */
    template<typename TextureDatabaseTraits>
    static TextureAtlas<TextureGroups> BuildAtlas(
        TextureDatabase<TextureDatabaseTraits> const & database,
        AtlasOptions options,
        ProgressCallback const & progressCallback)
    {
        static_assert(std::is_same<TextureGroups, TextureDatabaseTraits::TextureGroups>::value);

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            AddTextureInfos(group, textureInfos);
        }

        // Build specification
        auto const specification = BuildAtlasSpecification(textureInfos);

        // Build atlas
        return BuildAtlas(
            specification,
            options,
            [&database](TextureFrameId<TextureGroups> const & frameId)
            {
                return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex);
            },
            progressCallback);
    }

public:

    TextureAtlasBuilder()
        : mTextureFrameSpecifications()
    {}

    /*
     * Builds an atlas for the groups added so far.
     */
    TextureAtlas<TextureGroups> BuildAtlas(
        AtlasOptions options,
        ProgressCallback const & progressCallback);

private:

    struct TextureInfo
    {
        TextureFrameId<TextureGroups> FrameId;
        ImageSize Size;

        TextureInfo(
            TextureFrameId<TextureGroups> frameId,
            ImageSize size)
            : FrameId(frameId)
            , Size(size)
        {}
    };

    struct AtlasSpecification
    {
        struct TexturePosition
        {
            TextureFrameId<TextureGroups> FrameId;
            int FrameLeftX;
            int FrameBottomY;

            TexturePosition(
                TextureFrameId<TextureGroups> frameId,
                int frameLeftX,
                int frameBottomY)
                : FrameId(frameId)
                , FrameLeftX(frameLeftX)
                , FrameBottomY(frameBottomY)
            {}
        };

        // The positions of the textures
        std::vector<TexturePosition> TexturePositions;

        // The size of the atlas
        ImageSize AtlasSize;

        AtlasSpecification(
            std::vector<TexturePosition> && texturePositions,
            ImageSize atlasSize)
            : TexturePositions(std::move(texturePositions))
            , AtlasSize(atlasSize)
        {}
    };

    // Unit-tested
    static AtlasSpecification BuildAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos);

    // Unit-tested
    static AtlasSpecification BuildRegularAtlasSpecification(std::vector<TextureInfo> const & inputTextureInfos);

    static TextureAtlas<TextureGroups> BuildAtlas(
        AtlasSpecification const & specification,
        AtlasOptions options,
        std::function<TextureFrame<TextureGroups>(TextureFrameId<TextureGroups> const &)> frameLoader,
        ProgressCallback const & progressCallback);

    static void CopyImage(
        std::unique_ptr<rgbaColor const []> sourceImage,
        ImageSize sourceImageSize,
        rgbaColor * destImage,
        ImageSize destImageSize,
        int destinationLeftX,
        int destinationBottomY);

    static inline void AddTextureInfos(
        TextureGroup<TextureGroups> const & group,
        std::vector<TextureInfo> & textureInfos)
    {
        std::transform(
            group.GetFrameSpecifications().cbegin(),
            group.GetFrameSpecifications().cend(),
            std::back_inserter(textureInfos),
            [](auto const & frame)
            {
                return TextureInfo(
                    frame.Metadata.FrameId,
                    frame.Metadata.Size);
            });
    }

private:

    friend class TextureAtlasTests_OneTexture_Test;
    friend class TextureAtlasTests_Placement1_Test;
    friend class TextureAtlasTests_RoundsAtlasSize_Test;
    friend class TextureAtlasTests_RegularAtlas_Test;

private:

    std::unordered_map<TextureFrameId<TextureGroups>, TextureFrameSpecification<TextureGroups>> mTextureFrameSpecifications;
};

}

template <> struct is_flag<Render::AtlasOptions> : std::true_type {};