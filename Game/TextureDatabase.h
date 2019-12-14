/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

/*
 * Object model for management of textures.
 *
 * A frame is a single texture.
 * A group is a collection of related frames; for example, a group is an animation.
 * A database is a collection of groups.
 */

#include "ImageFileTools.h"

#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/Utils.h>

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace Render {

template <typename TextureGroups>
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
    TextureFrameId<TextureGroups> FrameId;

    // The optional name of the frame
    std::string FrameName;

    TextureFrameMetadata(
        ImageSize size,
        float worldWidth,
        float worldHeight,
        bool hasOwnAmbientLight,
        float anchorWorldX,
        float anchorWorldY,
        TextureFrameId<TextureGroups> frameId,
        std::string const & frameName)
        : Size(size)
        , WorldWidth(worldWidth)
        , WorldHeight(worldHeight)
        , HasOwnAmbientLight(hasOwnAmbientLight)
        , AnchorWorldX(anchorWorldX)
        , AnchorWorldY(anchorWorldY)
        , FrameId(frameId)
        , FrameName(frameName)
    {}

    void Serialize(picojson::object & root) const;
};

template <typename TextureGroups>
struct TextureFrame
{
    // Metadata
    TextureFrameMetadata<TextureGroups> Metadata;

    // The image itself
    RgbaImageData TextureData;

    TextureFrame(
        TextureFrameMetadata<TextureGroups> const & metadata,
        RgbaImageData textureData)
        : Metadata(metadata)
        , TextureData(std::move(textureData))
    {}
};

template <typename TextureGroups>
struct TextureFrameSpecification
{
    // Metadata
    TextureFrameMetadata<TextureGroups> Metadata;

    // The path to the image
    std::filesystem::path FilePath;

    TextureFrameSpecification(
        TextureFrameMetadata<TextureGroups> const & metadata,
        std::filesystem::path filePath)
        : Metadata(metadata)
        , FilePath(filePath)
    {}

    TextureFrame<TextureGroups> LoadFrame() const
    {
        RgbaImageData imageData = ImageFileTools::LoadImageRgbaLowerLeft(FilePath);

        return TextureFrame<TextureGroups>(
            Metadata,
            std::move(imageData));
    }
};

/*
 * This class models a group of textures, and it has all the necessary information
 * to load individual frames at runtime.
 */
template <typename TextureGroups>
class TextureGroup
{
public:

    // The group
    TextureGroups Group;

    TextureGroup(
        TextureGroups group,
        std::vector<TextureFrameSpecification<TextureGroups>> frameSpecifications)
        : Group(group)
        , mFrameSpecifications(std::move(frameSpecifications))
    {}

    auto const & GetFrameSpecification(TextureFrameIndex frameIndex) const
    {
        return mFrameSpecifications[frameIndex];
    }

    auto const & GetFrameSpecifications() const
    {
        return mFrameSpecifications;
    }

    // Gets the number of frames in this group
    TextureFrameIndex GetFrameCount() const
    {
        return static_cast<TextureFrameIndex>(mFrameSpecifications.size());
    }

    TextureFrame<TextureGroups> LoadFrame(TextureFrameIndex frameIndex) const
    {
        return mFrameSpecifications[frameIndex].LoadFrame();
    }

private:

    std::vector<TextureFrameSpecification<TextureGroups>> mFrameSpecifications;
};


/*
 * A whole set of textures.
 */
template <typename TextureDatabaseTraits>
class TextureDatabase
{
public:

    using TextureGroups = typename TextureDatabaseTraits::TextureGroups;

    static TextureDatabase Load(std::filesystem::path const & texturesRootFolderPath);

    auto const & GetGroups() const
    {
        return mGroups;
    }

    TextureGroup<TextureGroups> const & GetGroup(TextureGroups group) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        return mGroups[static_cast<size_t>(group)];
    }

    TextureFrameMetadata<TextureGroups> const & GetFrameMetadata(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        assert(frameIndex < mGroups[static_cast<size_t>(group)].GetFrameCount());
        return mGroups[static_cast<size_t>(group)].GetFrameSpecifications()[frameIndex].Metadata;
    }

private:

    explicit TextureDatabase(std::vector<TextureGroup<TextureGroups>> groups)
        : mGroups(std::move(groups))
    {}

    std::vector<TextureGroup<TextureGroups>> mGroups;
};

}

#include "TextureDatabase-inl.h"