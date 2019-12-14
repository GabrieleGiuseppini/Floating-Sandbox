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
};

/*
 * Metadata about a whole texture atlas.
 */
template <typename TextureGroups>
class TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(
        ImageSize size,
        AtlasOptions options,
        std::vector<TextureAtlasFrameMetadata<TextureGroups>> frames)
        : mSize(size)
        , mOptions(options)
        , mFrameMetadata(frames)
        , mFrameMetadataIndices()
    {
        //
        // Store frames indices in vector of vectors, indexed by group and frame index
        //

        std::sort(
            mFrameMetadata.begin(),
            mFrameMetadata.end(),
            [](TextureAtlasFrameMetadata<TextureGroups> const & f1, TextureAtlasFrameMetadata<TextureGroups> const & f2)
            {
                return f1.FrameMetadata.FrameId.Group < f2.FrameMetadata.FrameId.Group
                    || (f1.FrameMetadata.FrameId.Group == f2.FrameMetadata.FrameId.Group
                        && f1.FrameMetadata.FrameId.FrameIndex < f2.FrameMetadata.FrameId.FrameIndex);
            });

        for (size_t frameIndex = 0; frameIndex < mFrameMetadata.size(); ++frameIndex)
        {
            size_t groupIndex = static_cast<size_t>(mFrameMetadata[frameIndex].FrameMetadata.FrameId.Group);
            if (groupIndex >= mFrameMetadataIndices.size())
            {
                mFrameMetadataIndices.resize(groupIndex + 1);
            }

            assert(static_cast<size_t>(mFrameMetadata[frameIndex].FrameMetadata.FrameId.FrameIndex) == mFrameMetadataIndices.back().size());
            mFrameMetadataIndices.back().emplace_back(frameIndex);
        }
    }

    ImageSize const & GetSize() const
    {
        return mSize;
    }

    bool IsAlphaPremultiplied() const
    {
        return !!(mOptions & AtlasOptions::AlphaPremultiply);
    }

    TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(TextureFrameId<TextureGroups> const & textureFrameId) const
    {
        return GetFrameMetadata(textureFrameId.Group, textureFrameId.FrameIndex);
    }

    TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        assert(frameIndex < mFrameMetadataIndices[static_cast<size_t>(group)].size());
        return mFrameMetadata[mFrameMetadataIndices[static_cast<size_t>(group)][frameIndex]];
    }

    std::vector<TextureAtlasFrameMetadata<TextureGroups>> const & GetFrameMetadata() const
    {
        return mFrameMetadata;
    }

    int GetMaxDimension() const
    {
        int maxDimension = std::accumulate(
            mFrameMetadata.cbegin(),
            mFrameMetadata.cend(),
            std::numeric_limits<int>::lowest(),
            [](int minDimension, TextureAtlasFrameMetadata<TextureGroups> const & frameMd)
            {
                return std::max(
                    minDimension,
                    std::max(frameMd.FrameMetadata.Size.Width, frameMd.FrameMetadata.Size.Height));
            });

        return maxDimension;
    }

    void Serialize(picojson::object & root) const;

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
        TextureAtlasMetadata<TextureGroups> const & metadata,
        RgbaImageData atlasData)
        : Metadata(metadata)
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
        std::filesystem::path const & databaseDirectoryPath);

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
     * Adds a group to the set of groups that this instance can be used to build and atlas for.
     */
    void Add(TextureGroup<TextureGroups> const & group)
    {
        for (auto const & frameSpecification : group.GetFrameSpecifications())
        {
            mTextureFrameSpecifications.insert(
                std::make_pair(
                    frameSpecification.Metadata.FrameId,
                    frameSpecification));
        }
    }

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

    static void AddTextureInfos(
        TextureGroup<TextureGroups> const & group,
        std::vector<TextureInfo> & textureInfos)
    {
        for (auto const & frame : group.GetFrameSpecifications())
        {
            textureInfos.emplace_back(
                frame.Metadata.FrameId,
                frame.Metadata.Size);
        }
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

#include "TextureAtlas-inl.h"