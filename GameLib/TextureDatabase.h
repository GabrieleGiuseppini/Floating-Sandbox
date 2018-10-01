/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

/*
 * Object model for management of textures.
 */

#include "ImageSize.h"
#include "ProgressCallback.h"
#include "TextureTypes.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

struct TextureFrameMetadata
{
    // Size of the image
    ImageSize Size;

    // World dimensions of this texture
    float WorldWidth;
    float WorldHeight;

    // When true, the texture does not need to be blended with ambient light
    // (i.e. it shines at night)
    bool HasOwnAmbientLight;

    // Anchor point: when this texture is requested to be drawn at a specific
    // world coordinate, that is the coordinate of this point
    float AnchorWorldX;
    float AnchorWorldY;

    // The ID of this frame
    TextureFrameId FrameId;

    TextureFrameMetadata(
        ImageSize size,
        float worldWidth,
        float worldHeight,
        bool hasOwnAmbientLight,
        float anchorWorldX,
        float anchorWorldY,
        TextureFrameId frameId)
        : Size(size)
        , WorldWidth(worldWidth)
        , WorldHeight(worldHeight)
        , HasOwnAmbientLight(hasOwnAmbientLight)
        , AnchorWorldX(anchorWorldX)
        , AnchorWorldY(anchorWorldY)
        , FrameId(frameId)
    {}
};

struct TextureFrame
{
    // Metadata
    TextureFrameMetadata Metadata;

    // The image itself
    std::unique_ptr<unsigned char const[]> Data;

    TextureFrame(
        TextureFrameMetadata const & metadata,
        std::unique_ptr<unsigned char const[]> data)
        : Metadata(metadata)
        , Data(std::move(data))
    {}
};

struct TextureFrameSpecification
{
    // Metadata
    TextureFrameMetadata Metadata;

    // The path to the image
    std::filesystem::path FilePath;

    TextureFrameSpecification(
        TextureFrameMetadata const & metadata,
        std::filesystem::path filePath)
        : Metadata(metadata)
        , FilePath(filePath)
    {}

    TextureFrame LoadFrame() const;
};

/*
 * This class models a group of textures, and it has all the necessary information
 * to load individual frames at runtime.
 */
class TextureGroup
{
public:

    // The group
    TextureGroupType Group;

    TextureGroup(
        TextureGroupType group,
        std::vector<TextureFrameSpecification> frameSpecifications)
        : Group(group)
        , mFrameSpecifications(std::move(frameSpecifications))
    {}

    auto const & GetFrameSpecifications() const
    {
        return mFrameSpecifications;
    }

    // Gets the number of frames in this group
    TextureFrameIndex GetFrameCount() const
    {
        return static_cast<TextureFrameIndex>(mFrameSpecifications.size());
    }

    TextureFrame LoadFrame(TextureFrameIndex frameIndex) const
    {
        return mFrameSpecifications[frameIndex].LoadFrame();
    }

private:

    std::vector<TextureFrameSpecification> mFrameSpecifications;
};


/*
 * The whole set of textures.
 */
class TextureDatabase
{
public:

    static TextureDatabase Load(
        std::filesystem::path const & texturesRoot,
        ProgressCallback const & progressCallback);

    TextureGroup const & GetGroup(TextureGroupType group) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        return mGroups[static_cast<size_t>(group)];
    }

    TextureFrameMetadata const & GetFrameMetadata(
        TextureGroupType group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        assert(frameIndex < mGroups[static_cast<size_t>(group)].GetFrameCount());
        return mGroups[static_cast<size_t>(group)].GetFrameSpecifications()[frameIndex].Metadata;
    }

private:

    explicit TextureDatabase(std::vector<TextureGroup> groups)
        : mGroups(std::move(groups))
    {}

    std::vector<TextureGroup> mGroups;
};
