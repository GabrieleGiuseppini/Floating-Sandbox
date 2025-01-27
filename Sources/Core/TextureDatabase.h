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
 *
 * A database is defined by a "type traits" structure, which here is called "Texture Database".
 */

#include "GameException.h"
#include "GameTypes.h"
#include "IAssetManager.h"
#include "ImageData.h"

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <vector>

template <typename TTextureDatabase>
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
    // world coordinate, that is the coordinate of this point, in frame (pixel) coordinates
    ImageCoordinates AnchorCenter;

    // Anchor point in texture frame coordinates scaled to world coordinates
    // (i.e. [0.0, 1.0] * WorldWidth/Height)
    vec2f AnchorCenterWorld;

    // The ID of this frame
    TextureFrameId<typename TTextureDatabase::TextureGroupsType> FrameId;

    // The filename stem of this frame's texture
    std::string FilenameStem;

    // The optional name of the frame
    std::string DisplayName;

    TextureFrameMetadata(
        ImageSize size,
        float worldWidth,
        float worldHeight,
        bool hasOwnAmbientLight,
        ImageCoordinates const & anchorCenter,
        vec2f const & anchorCenterWorld,
        TextureFrameId<typename TTextureDatabase::TextureGroupsType> frameId,
        std::string const & filenameStem,
        std::string const & displayName)
        : Size(size)
        , WorldWidth(worldWidth)
        , WorldHeight(worldHeight)
        , HasOwnAmbientLight(hasOwnAmbientLight)
        , AnchorCenter(anchorCenter)
        , AnchorCenterWorld(anchorCenterWorld)
        , FrameId(frameId)
        , FilenameStem(filenameStem)
        , DisplayName(displayName)
    {}

    void Serialize(picojson::object & root) const;

    static TextureFrameMetadata Deserialize(picojson::object const & root);
};

template <typename TTextureDatabase>
struct TextureFrame
{
    // Metadata
    TextureFrameMetadata<TTextureDatabase> Metadata;

    // The image itself
    RgbaImageData TextureData;

    TextureFrame(
        TextureFrameMetadata<TTextureDatabase> const & metadata,
        RgbaImageData textureData)
        : Metadata(metadata)
        , TextureData(std::move(textureData))
    {}

    TextureFrame Clone() const
    {
        return TextureFrame(
            Metadata,
            TextureData.Clone());
    }
};

template <typename TTextureDatabase>
struct TextureFrameSpecification
{
    // Metadata
    TextureFrameMetadata<TTextureDatabase> Metadata;

    // The relative path of this frame's texture
    std::string RelativePath;

    TextureFrameSpecification(
        TextureFrameMetadata<TTextureDatabase> const & metadata,
        std::string const & relativePath)
        : Metadata(metadata)
        , RelativePath(relativePath)
    {}

    TextureFrame<TTextureDatabase> LoadFrame(IAssetManager & assetManager) const
    {
        RgbaImageData imageData = assetManager.LoadTextureDatabaseFrameRGBA(
            TTextureDatabase::DatabaseName,
            RelativePath);

        return TextureFrame<TTextureDatabase>(
            Metadata,
            std::move(imageData));
    }
};

/*
 * This class models a group of textures, and it has all the necessary information
 * to load individual frames at runtime.
 */
template <typename TTextureDatabase>
class TextureGroup
{
public:

    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    // The group
    TTextureGroups Group;

    TextureGroup(
        TTextureGroups group,
        std::vector<TextureFrameSpecification<TTextureDatabase>> frameSpecifications)
        : Group(group)
        , mFrameSpecifications(std::move(frameSpecifications))
    {}

    inline auto const & GetFrameSpecification(TextureFrameIndex frameIndex) const
    {
        return mFrameSpecifications[frameIndex];
    }

    inline auto const & GetFrameSpecifications() const
    {
        return mFrameSpecifications;
    }

    // Gets the number of frames in this group
    inline TextureFrameIndex GetFrameCount() const
    {
        return static_cast<TextureFrameIndex>(mFrameSpecifications.size());
    }

    inline TextureFrame<TTextureDatabase> LoadFrame(TextureFrameIndex frameIndex, IAssetManager & assetManager) const
    {
        return mFrameSpecifications[frameIndex].LoadFrame(assetManager);
    }

private:

    std::vector<TextureFrameSpecification<TTextureDatabase>> mFrameSpecifications;
};


/*
 * A whole set of textures.
 */
template <typename TTextureDatabase>
class TextureDatabase
{
public:

    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    static TextureDatabase Load(IAssetManager & assetManager);

    inline auto const & GetGroups() const
    {
        return mGroups;
    }

    inline TextureGroup<TTextureDatabase> const & GetGroup(TTextureGroups group) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        return mGroups[static_cast<size_t>(group)];
    }

    inline TextureFrameMetadata<TTextureDatabase> const & GetFrameMetadata(
        TTextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mGroups.size());
        assert(frameIndex < mGroups[static_cast<size_t>(group)].GetFrameCount());
        return mGroups[static_cast<size_t>(group)].GetFrameSpecifications()[frameIndex].Metadata;
    }

private:

    explicit TextureDatabase(std::vector<TextureGroup<TTextureDatabase>> groups)
        : mGroups(std::move(groups))
    {}

    std::vector<TextureGroup<TTextureDatabase>> mGroups;
};

#include "TextureDatabase-inl.h"