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
#include <vector>

namespace Render {

struct TextureAtlasFrameMetadata
{
    vec2f TextureCoordinatesBottomLeft;
    vec2f TextureCoordinatesTopRight;

    TextureFrameMetadata FrameMetadata;

    TextureAtlasFrameMetadata(
        vec2f textureCoordinatesBottomLeft,
        vec2f textureCoordinatesTopRight,
        TextureFrameMetadata frameMetadata)
        : TextureCoordinatesBottomLeft(textureCoordinatesBottomLeft)
        , TextureCoordinatesTopRight(textureCoordinatesTopRight)
        , FrameMetadata(frameMetadata)
    {}
};

class TextureAtlasMetadata
{
public:

    TextureAtlasMetadata(std::vector<TextureAtlasFrameMetadata> frames)
        : mFrameMetadata()
    {
        //
        // Store frames in vector of vectors, indexed by group and frame index
        //

        std::sort(
            frames.begin(),
            frames.end(),
            [](TextureAtlasFrameMetadata const & f1, TextureAtlasFrameMetadata const & f2)
            {
                return f1.FrameMetadata.FrameId.Group < f2.FrameMetadata.FrameId.Group
                    || (f1.FrameMetadata.FrameId.Group == f2.FrameMetadata.FrameId.Group
                        && f1.FrameMetadata.FrameId.FrameIndex < f2.FrameMetadata.FrameId.FrameIndex);
            });

        for (auto const & frame : frames)
        { 
            size_t groupIndex = static_cast<size_t>(frame.FrameMetadata.FrameId.Group);
            if (groupIndex >= mFrameMetadata.size())
            {
                assert(groupIndex == mFrameMetadata.size());
                mFrameMetadata.emplace_back();
            }

            assert(static_cast<size_t>(frame.FrameMetadata.FrameId.FrameIndex) == mFrameMetadata.back()->size());
            mFrameMetadata.back().emplace_back(frame);
        }
    }

    TextureAtlasFrameMetadata const & GetFrameMetadata(
        TextureGroupType group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameMetadata.size());
        assert(frameIndex < mFrameMetadata[static_cast<size_t>(group)].size());
        return mFrameMetadata[static_cast<size_t>(group)][frameIndex];
    }

private:

    // Indexed by group first and frame index then
    std::vector<std::vector<TextureAtlasFrameMetadata>> mFrameMetadata;
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

    TextureAtlasBuilder()
    {
    }

    /*
     * Builds an atlas with the specified group.
     */
    TextureAtlas BuildAtlas(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    /*
     * Builds an atlas with the entire content of the specified database.
     */
    TextureAtlas BuildAtlas(
        TextureDatabase const & database,
        ProgressCallback const & progressCallback);

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
            int FrameTopY;

            TexturePosition(
                TextureFrameId frameId,
                int frameLeftX,
                int frameTopY)
                : FrameId(frameId)
                , FrameLeftX(frameLeftX)
                , FrameTopY(frameTopY)
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
        int destinationTopY);

private:

    friend class TextureAtlasTests_OneTexture_Test;
    friend class TextureAtlasTests_OptimalPlacement_Test;
};

}