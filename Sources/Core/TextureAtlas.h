/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EnumFlags.h"
#include "GameException.h"
#include "ImageData.h"
#include "ProgressCallback.h"
#include "TextureDatabase.h"
#include "Vectors.h""

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <numeric>
#include <type_traits>
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
template <typename TextureGroups>
struct TextureAtlasFrameMetadata
{
public:

    float TextureSpaceWidth; // Width in [0.0, 1.0] space (where 1.0 is the atlas' width), exclusive of dead-center dx's
    float TextureSpaceHeight; // Height in [0.0, 1.0] space (where 1.0 is the atlas' height), exclusive of dead-center dx's

    vec2f TextureCoordinatesBottomLeft; // In [0.0, 1.0] space, inclusive dead-center dx
    vec2f TextureCoordinatesAnchorCenter; // In [0.0, 1.0] space, inclusive dead-center dx
    vec2f TextureCoordinatesTopRight; // In [0.0, 1.0] space, inclusive dead-center dx

    int FrameLeftX; // In pixel-coordinate space
    int FrameBottomY; // In pixel-coordinate space

    TextureFrameMetadata<TextureGroups> FrameMetadata;

    TextureAtlasFrameMetadata(
        float textureSpaceWidth,
        float textureSpaceHeight,
        vec2f textureCoordinatesBottomLeft,
        vec2f textureCoordinatesAnchorCenter,
        vec2f textureCoordinatesTopRight,
        int frameLeftX,
        int frameBottomY,
        TextureFrameMetadata<TextureGroups> frameMetadata)
        : TextureSpaceWidth(textureSpaceWidth)
        , TextureSpaceHeight(textureSpaceHeight)
        , TextureCoordinatesBottomLeft(textureCoordinatesBottomLeft)
        , TextureCoordinatesAnchorCenter(textureCoordinatesAnchorCenter)
        , TextureCoordinatesTopRight(textureCoordinatesTopRight)
        , FrameLeftX(frameLeftX)
        , FrameBottomY(frameBottomY)
        , FrameMetadata(frameMetadata)
    {}

    /*
     * Creates a copy of self with same in-atlas properties but for a new frame.
     */
    TextureAtlasFrameMetadata CloneForNewTextureFrame(TextureFrameMetadata<TextureGroups> const & newFrameMetadata) const
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
template <typename TextureGroups>
struct TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(
        ImageSize size,
        TextureAtlasOptions options,
        std::vector<TextureAtlasFrameMetadata<TextureGroups>> && frames);

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

    inline std::vector<TextureAtlasFrameMetadata<TextureGroups>> const & GetAllFramesMetadata() const
    {
        return mFrameMetadata;
    }

    inline TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(TextureFrameId<TextureGroups> const & frameId) const
    {
        return GetFrameMetadata(frameId.Group, frameId.FrameIndex);
    }

    inline TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        assert(frameIndex < mFrameMetadataIndices[static_cast<size_t>(group)].size());
        return mFrameMetadata[mFrameMetadataIndices[static_cast<size_t>(group)][frameIndex]];
    }

    inline TextureAtlasFrameMetadata<TextureGroups> const & GetFrameMetadata(std::string const & filenameStem) const
    {
        if (mFrameMetadataByFilenameStem.count(filenameStem) != 1)
        {
            throw GameException("The requested frame filename stem \"" + filenameStem + "\" could not be found in texture atlas");
        }

        return mFrameMetadata[mFrameMetadataByFilenameStem.at(filenameStem)];
    }

    inline size_t GetFrameCount(TextureGroups group) const
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

    std::vector<TextureAtlasFrameMetadata<TextureGroups>> mFrameMetadata;

    // Indexed by group first and frame index then
    std::vector<std::vector<size_t>> mFrameMetadataIndices;

    // Indexed by filename stem, value is index in FrameMetadata array
    std::map<std::string, size_t> mFrameMetadataByFilenameStem;
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

    // TODOHERE
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
     * Builds an atlas with the entire content of the specified database.
     */
    template<typename TextureDatabaseTraits>
    static TextureAtlas<TextureGroups> BuildAtlas(
        TextureDatabase<TextureDatabaseTraits> const & database,
        TextureAtlasOptions options,
        ProgressCallback const & progressCallback)
    {
        static_assert(std::is_same<TextureGroups, typename TextureDatabaseTraits::TextureGroups>::value);

        auto frameLoader = [&database](TextureFrameId<TextureGroups> const & frameId) -> TextureFrame<TextureGroups>
            {
                return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex);
            };

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            AddTextureInfos(group, options, textureInfos);
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
    static TextureAtlas<TextureGroups> BuildAtlas(
        std::vector<TextureFrame<TextureGroups>> && textureFrames,
        TextureAtlasOptions options)
    {
        auto frameLoader = [&textureFrames](TextureFrameId<TextureGroups> const & frameId) -> TextureFrame<TextureGroups>
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
            [](float, ProgressMessageType) {});
    }

    /*
     * Builds an atlas with the specified database, composed of a power-of-two number of
     * frames with identical sizes, each having power-of-two dimensions.
     * Allows for algorithmic generation of texture coordinates (e.g. from within a shader),
     * without having to rely on a specification.
     * The atlas produced is suitable for mipmapping.
     */
    template<typename TextureDatabaseTraits>
    static TextureAtlas<TextureGroups> BuildRegularAtlas(
        TextureDatabase<TextureDatabaseTraits> const & database,
        TextureAtlasOptions options,
        ProgressCallback const & progressCallback)
    {
        static_assert(std::is_same<TextureGroups, typename TextureDatabaseTraits::TextureGroups>::value);

        if (!!(options & TextureAtlasOptions::SuppressDuplicates))
        {
            throw GameException("Duplicate suppression is not implemented with regular atlases");
        }

        // Build TextureInfo's
        std::vector<TextureInfo> textureInfos;
        for (auto const & group : database.GetGroups())
        {
            // Note: we'll verify later whether dimensions are suitable for a regular atlas
            AddTextureInfos(group, options, textureInfos);
        }

        // Build specification - verifies whether dimensions are suitable for a regular atlas
        auto const specification = BuildRegularAtlasSpecification(textureInfos);

        // Build atlas
        return InternalBuildAtlas(
            specification,
            options | TextureAtlasOptions::MipMappable,
            [&database](TextureFrameId<TextureGroups> const & frameId)
            {
                return database.GetGroup(frameId.Group).LoadFrame(frameId.FrameIndex);
            },
            progressCallback);
    }

private:

    struct TextureInfo
    {
        TextureFrameId<TextureGroups> FrameId;
        ImageSize InAtlasSize;

        TextureInfo(
            TextureFrameId<TextureGroups> frameId,
            ImageSize inAtlasSize)
            : FrameId(frameId)
            , InAtlasSize(inAtlasSize)
        {}
    };

    struct AtlasSpecification
    {
        struct TextureLocationInfo
        {
            TextureFrameId<TextureGroups> FrameId;
            vec2i InAtlasBottomLeft;
            ImageSize InAtlasSize;

            TextureLocationInfo(
                TextureFrameId<TextureGroups> frameId,
                vec2i inAtlasBottomLeft,
                ImageSize inAtlasSize)
                : FrameId(frameId)
                , InAtlasBottomLeft(inAtlasBottomLeft)
                , InAtlasSize(inAtlasSize)
            {}
        };

        struct DuplicateTextureInfo
        {
            TextureFrameMetadata<TextureGroups> DuplicateFrameMetadata;
            TextureFrameId<TextureGroups> OriginalFrameId;

            DuplicateTextureInfo(
                TextureFrameMetadata<TextureGroups> duplicateFrameMetadata,
                TextureFrameId<TextureGroups> originalFrameId)
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
        AtlasOptions options,
        std::function<TextureFrame<TextureGroups>(TextureFrameId<TextureGroups> const &)> frameLoader);

    // Unit-tested
    static AtlasSpecification BuildRegularAtlasSpecification(
        std::vector<TextureInfo> const & inputTextureInfos);

    // Unit-tested
    static TextureAtlas<TextureGroups> InternalBuildAtlas(
        AtlasSpecification const & specification,
        AtlasOptions options,
        std::function<TextureFrame<TextureGroups>(TextureFrameId<TextureGroups> const &)> frameLoader,
        ProgressCallback const & progressCallback);

    static void CopyImage(
        ImageData<rgbaColor> && sourceImage,
        rgbaColor * destImage,
        ImageSize destImageSize,
        vec2i const & destinationBottomLeftPosition);

    static inline void AddTextureInfos(
        TextureGroup<TextureGroups> const & group,
        AtlasOptions options,
        std::vector<TextureInfo> /* out */ & textureInfos)
    {
        std::transform(
            group.GetFrameSpecifications().cbegin(),
            group.GetFrameSpecifications().cend(),
            std::back_inserter(textureInfos),
            [&options](auto const & frame)
            {
                return TextureInfo(
                    frame.Metadata.FrameId,
                    MakeInAtlasSize(frame.Metadata.Size, options));
            });
    }

    static ImageSize MakeInAtlasSize(
        ImageSize originalSize,
        AtlasOptions options)
    {
        // If we need a mip-mappable atlas, enforce dimensions to be power-of-two
        return ((options & AtlasOptions::MipMappable) != AtlasOptions::None)
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