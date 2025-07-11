/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EnumFlags.h"
#include "GameException.h"
#include "IAssetManager.h"
#include "ImageData.h"
#include "ProgressCallback.h"
#include "TextureDatabase.h"
#include "Vectors.h"

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

/*
 * Atlas creation options.
 */
enum class TextureAtlasOptions
{
    None = 0,
    AlphaPremultiply = 1,
    MipMappable = 2,
    BinaryTransparencySmoothing = 4,
    SuppressDuplicates = 8
};

template <> struct is_flag<TextureAtlasOptions> : std::true_type {};

/*
 * Metadata about one single frame in a texture atlas.
 */
template <typename TTextureDatabase>
struct TextureAtlasFrameMetadata
{
public:

    float TextureSpaceWidth; // Width in [0.0, 1.0] space (where 1.0 is the atlas' width), exclusive of dead-center dx's
    float TextureSpaceHeight; // Height in [0.0, 1.0] space (where 1.0 is the atlas' height), exclusive of dead-center dx's

    vec2f TextureCoordinatesBottomLeft; // In [0.0, 1.0] space, inclusive dead-center dx
    vec2f TextureCoordinatesTopRight; // In [0.0, 1.0] space, inclusive dead-center dx

    int FrameLeftX; // In pixel-coordinate space
    int FrameBottomY; // In pixel-coordinate space

    TextureFrameMetadata<TTextureDatabase> FrameMetadata;

    TextureAtlasFrameMetadata(
        float textureSpaceWidth,
        float textureSpaceHeight,
        vec2f textureCoordinatesBottomLeft,
        vec2f textureCoordinatesTopRight,
        int frameLeftX,
        int frameBottomY,
        TextureFrameMetadata<TTextureDatabase> frameMetadata)
        : TextureSpaceWidth(textureSpaceWidth)
        , TextureSpaceHeight(textureSpaceHeight)
        , TextureCoordinatesBottomLeft(textureCoordinatesBottomLeft)
        , TextureCoordinatesTopRight(textureCoordinatesTopRight)
        , FrameLeftX(frameLeftX)
        , FrameBottomY(frameBottomY)
        , FrameMetadata(frameMetadata)
    {}

    /*
     * Creates a copy of self with same in-atlas properties but for a new frame.
     */
    TextureAtlasFrameMetadata CloneForNewTextureFrame(TextureFrameMetadata<TTextureDatabase> const & newFrameMetadata) const
    {
        TextureAtlasFrameMetadata clone = *this;
        clone.FrameMetadata = newFrameMetadata;
        return clone;
    }

    void Serialize(picojson::object & root) const;

    static TextureAtlasFrameMetadata Deserialize(picojson::object const & root);
};

/*
 * Metadata about a whole texture atlas.
 */
template <typename TTextureDatabase>
struct TextureAtlasMetadata
{
public:

    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    TextureAtlasMetadata(
        ImageSize size,
        TextureAtlasOptions options,
        std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> && frames);

    inline ImageSize const & GetSize() const
    {
        return mSize;
    }

    inline size_t GetFrameCount() const
    {
        return mFrameMetadata.size();
    }

    inline bool IsAlphaPremultiplied() const
    {
        return (mOptions & TextureAtlasOptions::AlphaPremultiply) != TextureAtlasOptions::None;
    }

    inline bool IsSuitableForMipMapping() const
    {
        return (mOptions & TextureAtlasOptions::MipMappable) != TextureAtlasOptions::None;
    }

    inline std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> const & GetAllFramesMetadata() const
    {
        return mFrameMetadata;
    }

    inline TextureAtlasFrameMetadata<TTextureDatabase> const & GetFrameMetadata(TextureFrameId<TTextureGroups> const & frameId) const
    {
        return GetFrameMetadata(frameId.Group, frameId.FrameIndex);
    }

    inline TextureAtlasFrameMetadata<TTextureDatabase> const & GetFrameMetadata(
        TTextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        assert(frameIndex < mFrameMetadataIndices[static_cast<size_t>(group)].size());
        return mFrameMetadata[mFrameMetadataIndices[static_cast<size_t>(group)][frameIndex]];
    }

    inline TextureAtlasFrameMetadata<TTextureDatabase> const & GetFrameMetadata(std::string const & frameName) const
    {
        if (mFrameMetadataByName.count(frameName) != 1)
        {
            throw GameException("The requested frame name \"" + frameName + "\" could not be found in texture atlas");
        }

        return mFrameMetadata[mFrameMetadataByName.at(frameName)];
    }

    inline size_t GetFrameCount(TTextureGroups group) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        return mFrameMetadataIndices[static_cast<size_t>(group)].size();
    }

    int GetMaxDimension() const;

    void Serialize(picojson::object & root) const;

    static TextureAtlasMetadata Deserialize(picojson::object const & root);

private:

    ImageSize const mSize;

    TextureAtlasOptions const mOptions;

    std::vector<TextureAtlasFrameMetadata<TTextureDatabase>> mFrameMetadata;

    // Indexed by group first and frame index then
    std::vector<std::vector<size_t>> mFrameMetadataIndices;

    // Indexed by frame name, value is index in FrameMetadata array
    std::map<std::string, size_t> mFrameMetadataByName;
};

/*
 * A texture atlas.
 */
template <typename TTextureDatabase>
struct TextureAtlas
{
public:

    // Metadata
    TextureAtlasMetadata<TTextureDatabase> Metadata;

    // The image itself
    RgbaImageData Image;

    TextureAtlas(
        TextureAtlasMetadata<TTextureDatabase> && metadata,
        RgbaImageData && image)
        : Metadata(std::move(metadata))
        , Image(std::move(image))
    {}

    //
    // De/Serialization
    //

    std::tuple<picojson::value, RgbaImageData const &> Serialize() const;

    static TextureAtlas Deserialize(IAssetManager const & assetManager);
};

template <typename TTextureDatabase>
class TextureAtlasBuilder
{
public:

    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    /*
     * Builds an atlas with the entire content of the database.
     */
    static TextureAtlas<TTextureDatabase> BuildAtlas(
        TextureDatabase<TTextureDatabase> const & database,
        TextureAtlasOptions options,
        float resizeFactor,
        IAssetManager const & assetManager,
        SimpleProgressCallback const & progressCallback)
    {
        auto frameLoader = [&](TextureFrameId<TTextureGroups> const & frameId) -> TextureFrame<TTextureDatabase>
            {
                if (resizeFactor != 1.0f)
                    return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex, assetManager).Resize(resizeFactor);
                else
                    return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex, assetManager);
            };

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            AddTextureInfos(group, options, resizeFactor, textureInfos);
        }

        // Build specification
        auto const specification = BuildAtlasSpecification(
            textureInfos,
            options,
            frameLoader);

        // Build atlas
        return InternalBuildAtlas(
            specification,
            options,
            frameLoader,
            progressCallback);
    }

    /*
     * Builds an atlas with the specified textures.
     */
    static TextureAtlas<TTextureDatabase> BuildAtlas(
        std::vector<TextureFrame<TTextureDatabase>> const & textureFrames,
        TextureAtlasOptions options)
    {
        auto frameLoader = [&textureFrames](TextureFrameId<TTextureGroups> const & frameId) -> TextureFrame<TTextureDatabase>
            {
                for (size_t t = 0; t < textureFrames.size(); t++)
                {
                    if (frameId == textureFrames[t].Metadata.FrameId)
                        return textureFrames[t].Clone();
                }

                assert(false);
                throw GameException("Cannot find texture frame");
            };

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (size_t t = 0; t < textureFrames.size(); ++t)
        {
            textureInfos.emplace_back(
                textureFrames[t].Metadata.FrameId,
                MakeInAtlasSize(textureFrames[t].Metadata.Size, options));
        }

        // Build specification
        auto const specification = BuildAtlasSpecification(
            textureInfos,
            options,
            frameLoader);

        // Build atlas
        return InternalBuildAtlas(
            specification,
            options,
            frameLoader,
            SimpleProgressCallback::Dummy());
    }

    /*
     * Builds an atlas with the specified database, composed of a power-of-two number of
     * frames with identical sizes, each having power-of-two dimensions.
     * Allows for algorithmic generation of texture coordinates (e.g. from within a shader),
     * without having to rely on a specification.
     * The atlas produced is suitable for mipmapping.
     */
    static TextureAtlas<TTextureDatabase> BuildRegularAtlas(
        TextureDatabase<TTextureDatabase> const & database,
        TextureAtlasOptions options,
        float resizeFactor,
        IAssetManager const & assetManager,
        SimpleProgressCallback const & progressCallback)
    {
        if (!!(options & TextureAtlasOptions::SuppressDuplicates))
        {
            throw GameException("Duplicate suppression is not implemented with regular atlases");
        }

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            // Note: we'll verify later whether dimensions are suitable for a regular atlas
            AddTextureInfos(group, options, resizeFactor, textureInfos);
        }

        // Build specification - verifies whether dimensions are suitable for a regular atlas
        auto const specification = BuildRegularAtlasSpecification(textureInfos);

        // Build atlas
        return InternalBuildAtlas(
            specification,
            options | TextureAtlasOptions::MipMappable,
            [&](TextureFrameId<TTextureGroups> const & frameId) -> TextureFrame<TTextureDatabase>
            {
                if (resizeFactor != 1.0f)
                    return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex, assetManager).Resize(resizeFactor);
                else
                    return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex, assetManager);
            },
            progressCallback);
    }

private:

    struct TextureInfo
    {
        TextureFrameId<TTextureGroups> FrameId;
        ImageSize InAtlasSize;

        TextureInfo(
            TextureFrameId<TTextureGroups> frameId,
            ImageSize inAtlasSize)
            : FrameId(frameId)
            , InAtlasSize(inAtlasSize)
        {}
    };

    struct AtlasSpecification
    {
        struct TextureLocationInfo
        {
            TextureFrameId<TTextureGroups> FrameId;
            vec2i InAtlasBottomLeft;
            ImageSize InAtlasSize;

            TextureLocationInfo(
                TextureFrameId<TTextureGroups> frameId,
                vec2i inAtlasBottomLeft,
                ImageSize inAtlasSize)
                : FrameId(frameId)
                , InAtlasBottomLeft(inAtlasBottomLeft)
                , InAtlasSize(inAtlasSize)
            {}
        };

        struct DuplicateTextureInfo
        {
            TextureFrameMetadata<TTextureDatabase> DuplicateFrameMetadata;
            TextureFrameId<TTextureGroups> OriginalFrameId;

            DuplicateTextureInfo(
                TextureFrameMetadata<TTextureDatabase> duplicateFrameMetadata,
                TextureFrameId<TTextureGroups> originalFrameId)
                : DuplicateFrameMetadata(duplicateFrameMetadata)
                , OriginalFrameId(originalFrameId)
            {}
        };

        // The locations of the textures - contains all database frames except for duplicates
        std::vector<TextureLocationInfo> TextureLocationInfos;

        // The database frames that are duplicate - which do not appear in TextureLocationInfos
        std::vector<DuplicateTextureInfo> DuplicateTextureInfos;

        // The size of the atlas
        ImageSize AtlasSize;

        AtlasSpecification(
            std::vector<TextureLocationInfo> && textureLocationInfos,
            std::vector<DuplicateTextureInfo> && duplicateTextureInfos,
            ImageSize atlasSize)
            : TextureLocationInfos(std::move(textureLocationInfos))
            , DuplicateTextureInfos(std::move(duplicateTextureInfos))
            , AtlasSize(atlasSize)
        {}
    };

    // Unit-tested
    static AtlasSpecification BuildAtlasSpecification(
        std::vector<TextureInfo> const & inputTextureInfos,
        TextureAtlasOptions options,
        std::function<TextureFrame<TTextureDatabase>(TextureFrameId<TTextureGroups> const &)> frameLoader);

    // Unit-tested
    static AtlasSpecification BuildRegularAtlasSpecification(
        std::vector<TextureInfo> const & inputTextureInfos);

    // Unit-tested
    static TextureAtlas<TTextureDatabase> InternalBuildAtlas(
        AtlasSpecification const & specification,
        TextureAtlasOptions options,
        std::function<TextureFrame<TTextureDatabase>(TextureFrameId<TTextureGroups> const &)> frameLoader,
        SimpleProgressCallback const & progressCallback);

    static void CopyImage(
        ImageData<rgbaColor> && sourceImage,
        rgbaColor * destImage,
        ImageSize destImageSize,
        vec2i const & destinationBottomLeftPosition);

    static inline void AddTextureInfos(
        TextureGroup<TTextureDatabase> const & group,
        TextureAtlasOptions options,
        float resizeFactor,
        std::vector<TextureInfo> /* out */ & textureInfos)
    {
        std::transform(
            group.GetFrameSpecifications().cbegin(),
            group.GetFrameSpecifications().cend(),
            std::back_inserter(textureInfos),
            [&](auto const & frame)
            {
                auto const frameSize = frame.Metadata.Size * resizeFactor;

                return TextureInfo(
                    frame.Metadata.FrameId,
                    MakeInAtlasSize(frameSize, options));
            });
    }

    static ImageSize MakeInAtlasSize(
        ImageSize originalSize,
        TextureAtlasOptions options)
    {
        // If we need a mip-mappable atlas, enforce dimensions to be power-of-two
        return ((options & TextureAtlasOptions::MipMappable) != TextureAtlasOptions::None)
            ? ImageSize(
                ceil_power_of_two(originalSize.width),
                ceil_power_of_two(originalSize.height))
            : originalSize;
    }

private:

    friend class TextureAtlasTests_Specification_OneTexture_Test;
    friend class TextureAtlasTests_Specification_MultipleTextures_Test;
    friend class TextureAtlasTests_Specification_RegularAtlas_Test;
    friend class TextureAtlasTests_Specification_RoundsAtlasSize_Test;
    friend class TextureAtlasTests_Specification_DuplicateSuppression_Test;
    friend class TextureAtlasTests_Placement_InAtlasSizeMatchingFrameSize_Test;
    friend class TextureAtlasTests_Placement_InAtlasSizeLargerThanFrameSize_Test;
    friend class TextureAtlasTests_Placement_Duplicates_Test;
};

#include "TextureAtlas-inl.h"