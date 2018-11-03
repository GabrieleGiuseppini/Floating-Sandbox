/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"
#include "ProgressCallback.h"
#include "TextureDatabase.h"
#include "Vectors.h"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Render {

struct TextureAtlasFrameMetadata
{
    vec2f TextureCoordinatesBottomLeft;
    vec2f TextureCoordinatesTopRight;

    int FrameLeftX;
    int FrameBottomY;

    TextureFrameMetadata FrameMetadata;

    TextureAtlasFrameMetadata(
        vec2f textureCoordinatesBottomLeft,
        vec2f textureCoordinatesTopRight,
        int frameLeftX,
        int frameBottomY,
        TextureFrameMetadata frameMetadata)
        : TextureCoordinatesBottomLeft(textureCoordinatesBottomLeft)
        , TextureCoordinatesTopRight(textureCoordinatesTopRight)
        , FrameLeftX(frameLeftX)
        , FrameBottomY(frameBottomY)
        , FrameMetadata(frameMetadata)
    {}
};

class TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(std::vector<TextureAtlasFrameMetadata> frames)
        : mFrameMetadata(frames)
        , mFrameMetadataIndices()
    {
        //
        // Store frames indices in vector of vectors, indexed by group and frame index
        //

        std::sort(
            mFrameMetadata.begin(),
            mFrameMetadata.end(),
            [](TextureAtlasFrameMetadata const & f1, TextureAtlasFrameMetadata const & f2)
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

    TextureAtlasFrameMetadata const & GetFrameMetadata(TextureFrameId const & textureFrameId) const
    {
        return GetFrameMetadata(textureFrameId.Group, textureFrameId.FrameIndex);
    }

    TextureAtlasFrameMetadata const & GetFrameMetadata(
        TextureGroupType group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadataIndices.size());
        assert(frameIndex < mFrameMetadataIndices[static_cast<size_t>(group)].size());
        return mFrameMetadata[mFrameMetadataIndices[static_cast<size_t>(group)][frameIndex]];
    }

    std::vector<TextureAtlasFrameMetadata> const & GetFrameMetadata() const
    {
        return mFrameMetadata;
    }

private:

    std::vector<TextureAtlasFrameMetadata> mFrameMetadata;

    // Indexed by group first and frame index then
    std::vector<std::vector<size_t>> mFrameMetadataIndices;
};

struct TextureAtlas
{
    // Metadata
    TextureAtlasMetadata Metadata;

    // The image itself
    ImageData AtlasData;

    TextureAtlas(
        TextureAtlasMetadata const & metadata,
        ImageData atlasData)
        : Metadata(metadata)
        , AtlasData(std::move(atlasData))
    {}
};

class TextureAtlasBuilder
{
public:

    /*
     * Builds an atlas with the specified group.
     */
    static TextureAtlas BuildAtlas(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    /*
     * Builds an atlas with the entire content of the specified database.
     */
    static TextureAtlas BuildAtlas(
        TextureDatabase const & database,
        ProgressCallback const & progressCallback);

public:

    TextureAtlasBuilder()
        : mTextureFrameSpecifications()
    {}

    /*
     * Adds a group to the set of groups that this instance can be used to build and atlas for.
     */
    void Add(TextureGroup const & group)
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
    TextureAtlas BuildAtlas(ProgressCallback const & progressCallback);

private:

    struct TextureInfo
    {
        TextureFrameId FrameId;
        ImageSize Size;

        TextureInfo(
            TextureFrameId frameId,
            ImageSize size)
            : FrameId(frameId)
            , Size(size)
        {}
    };

    struct AtlasSpecification
    {
        struct TexturePosition
        {
            TextureFrameId FrameId;
            int FrameLeftX;
            int FrameBottomY;

            TexturePosition(
                TextureFrameId frameId,
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

    static TextureAtlas BuildAtlas(
        AtlasSpecification const & specification,
        std::function<TextureFrame(TextureFrameId const &)> frameLoader,
        ProgressCallback const & progressCallback);

    static void CopyImage(
        std::unique_ptr<unsigned char const []> sourceImage,
        ImageSize sourceImageSize,
        unsigned char * destImage,
        ImageSize destImageSize,
        int destinationLeftX,
        int destinationBottomY);

    static void AddTextureInfos(
        TextureGroup const & group,
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

private:

    std::unordered_map<TextureFrameId, TextureFrameSpecification> mTextureFrameSpecifications;
};

}