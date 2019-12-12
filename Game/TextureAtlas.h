/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextureDatabase.h"

#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace Render {

/*
 * Metadata about one single frame in an atlas.
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
};

template <typename TextureGroups>
class TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(
        ImageSize size,
        std::vector<TextureAtlasFrameMetadata<TextureGroups>> frames)
        : mSize(size)
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

private:

    const ImageSize mSize;

    std::vector<TextureAtlasFrameMetadata<TextureGroups>> mFrameMetadata;

    // Indexed by group first and frame index then
    std::vector<std::vector<size_t>> mFrameMetadataIndices;
};

template <typename TextureGroups>
struct TextureAtlas
{
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
        ProgressCallback const & progressCallback);

    /*
     * Builds an atlas with the specified group, composed of a power of two number of
     * frames with identical sizes.
     */
    static TextureAtlas<TextureGroups> BuildRegularAtlas(
        TextureGroup<TextureGroups> const & group,
        ProgressCallback const & progressCallback);

    /*
     * Builds an atlas with the entire content of the specified database.
     */
    template<typename TextureDatabaseTraits>
    static TextureAtlas<TextureGroups> BuildAtlas(
        TextureDatabase<TextureDatabaseTraits> const & database,
        ProgressCallback const & progressCallback);

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
    TextureAtlas<TextureGroups> BuildAtlas(ProgressCallback const & progressCallback);

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