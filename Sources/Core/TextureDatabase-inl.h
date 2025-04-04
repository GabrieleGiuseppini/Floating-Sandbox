/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureDatabase.h"

#include "Utils.h"

template <typename TTextureDatabase>
void TextureFrameMetadata<TTextureDatabase>::Serialize(picojson::object & root) const
{
    picojson::object size;
    size["width"] = picojson::value(static_cast<int64_t>(Size.width));
    size["height"] = picojson::value(static_cast<int64_t>(Size.height));
    root["size"] = picojson::value(size);

    picojson::object worldSize;
    worldSize["width"] = picojson::value(static_cast<double>(WorldWidth));
    worldSize["height"] = picojson::value(static_cast<double>(WorldHeight));
    root["world_size"] = picojson::value(worldSize);

    root["has_own_ambient_light"] = picojson::value(HasOwnAmbientLight);

    picojson::object anchorCenterPixel;
    anchorCenterPixel["x"] = picojson::value(static_cast<int64_t>(AnchorCenterPixel.x));
    anchorCenterPixel["y"] = picojson::value(static_cast<int64_t>(AnchorCenterPixel.y));
    root["anchor_center_pixel"] = picojson::value(anchorCenterPixel);

    picojson::object anchorCenterWorld;
    anchorCenterWorld["x"] = picojson::value(static_cast<double>(AnchorCenterWorld.x));
    anchorCenterWorld["y"] = picojson::value(static_cast<double>(AnchorCenterWorld.y));
    root["anchor_center_world"] = picojson::value(anchorCenterWorld);

    picojson::object anchorOffsetFrame;
    anchorOffsetFrame["x"] = picojson::value(static_cast<double>(AnchorOffsetFrame.x));
    anchorOffsetFrame["y"] = picojson::value(static_cast<double>(AnchorOffsetFrame.y));
    root["anchor_center_frame"] = picojson::value(anchorOffsetFrame);

    picojson::object frameId;
    frameId["group"] = picojson::value(static_cast<int64_t>(FrameId.Group));
    frameId["frame_index"] = picojson::value(static_cast<int64_t>(FrameId.FrameIndex));
    root["id"] = picojson::value(frameId);
    root["frame_name"] = picojson::value(FrameName);
    root["display_name"] = picojson::value(DisplayName);
}

template <typename TTextureDatabase>
TextureFrameMetadata<TTextureDatabase> TextureFrameMetadata<TTextureDatabase>::Deserialize(picojson::object const & root)
{
    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    picojson::object const & sizeJson = root.at("size").get<picojson::object>();
    ImageSize size(
        static_cast<int>(sizeJson.at("width").get<std::int64_t>()),
        static_cast<int>(sizeJson.at("height").get<std::int64_t>()));

    picojson::object const & worldSizeJson = root.at("world_size").get<picojson::object>();
    float worldWidth = static_cast<float>(worldSizeJson.at("width").get<double>());
    float worldHeight = static_cast<float>(worldSizeJson.at("height").get<double>());

    bool hasOwnAmbientLight = root.at("has_own_ambient_light").get<bool>();

    picojson::object const & anchorCenterPixelJson = root.at("anchor_center_pixel").get<picojson::object>();
    ImageCoordinates anchorCenterPixel(
        static_cast<int>(anchorCenterPixelJson.at("x").get<int64_t>()),
        static_cast<int>(anchorCenterPixelJson.at("y").get<int64_t>()));

    picojson::object const & anchorCenterWorldJson = root.at("anchor_center_world").get<picojson::object>();
    vec2f anchorCenterWorld(
        static_cast<float>(anchorCenterWorldJson.at("x").get<double>()),
        static_cast<float>(anchorCenterWorldJson.at("y").get<double>()));

    picojson::object const & anchorOffsetFrameJson = root.at("anchor_offset_frame").get<picojson::object>();
    vec2f anchorOffsetFrame(
        static_cast<float>(anchorOffsetFrameJson.at("x").get<double>()),
        static_cast<float>(anchorOffsetFrameJson.at("y").get<double>()));

    picojson::object const & frameIdJson = root.at("id").get<picojson::object>();
    TTextureGroups group = static_cast<TTextureGroups>(frameIdJson.at("group").get<std::int64_t>());
    TextureFrameIndex frameIndex = static_cast<TextureFrameIndex>(frameIdJson.at("frame_index").get<std::int64_t>());
    std::string const & frameName = root.at("frame_name").get<std::string>();
    std::string const & displayName = root.at("display_name").get<std::string>();

    return TextureFrameMetadata<TTextureDatabase>(
        size,
        worldWidth,
        worldHeight,
        hasOwnAmbientLight,
        anchorCenterPixel,
        anchorCenterWorld,
        anchorOffsetFrame,
        TextureFrameId<TTextureGroups>(group, frameIndex),
        frameName,
        displayName);
}

template <typename TTextureDatabase>
TextureDatabase<TTextureDatabase> TextureDatabase<TTextureDatabase>::Load(IAssetManager const & assetManager)
{
    using TTextureGroups = typename TTextureDatabase::TextureGroupsType;

    //
    // Load JSON file
    //

    picojson::value root = assetManager.LoadTetureDatabaseSpecification(TTextureDatabase::DatabaseName);
    if (!root.is<picojson::array>())
    {
        throw GameException("Texture database \"" + TTextureDatabase::DatabaseName + "\" specification file is not contain a JSON array");
    }

    //
    // Get list of frame descriptors
    //

    auto const allTextureFrameDescriptors = assetManager.EnumerateTextureDatabaseFrames(TTextureDatabase::DatabaseName);

    //
    // Process JSON groups and build texture groups
    //

    std::vector<TextureGroup<TTextureDatabase>> textureGroups;

    std::set<std::string> matchedTextureFrameNames;

    for (auto const & groupValue : root.get<picojson::array>())
    {
        if (!groupValue.is<picojson::object>())
        {
            throw GameException("Texture database: found a non-object group in database");
        }

        auto groupJson = groupValue.get<picojson::object>();

        std::string groupName = Utils::GetMandatoryJsonMember<std::string>(groupJson, "group_name");
        TTextureGroups group = TTextureDatabase::StrToTextureGroup(groupName);

        // Load group-wide defaults
        float groupWorldScaling = Utils::GetOptionalJsonMember<float>(groupJson, "world_scaling", 1.0f); // We default to 1.0
        std::optional<float> groupWorldWidth = Utils::GetOptionalJsonMember<float>(groupJson, "world_width");
        std::optional<float> groupWorldHeight = Utils::GetOptionalJsonMember<float>(groupJson, "world_height");
        bool groupHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(groupJson, "has_own_ambient_light", false);
        int groupAnchorOffsetX = Utils::GetOptionalJsonMember<int>(groupJson, "anchor_offset_x", 0);
        int groupAnchorOffsetY = Utils::GetOptionalJsonMember<int>(groupJson, "anchor_offset_y", 0);
        bool const doAutoAssignFrameIndices = Utils::GetOptionalJsonMember<bool>(groupJson, "auto_assign_frame_indices", false);

        //
        // Process frames from JSON and build texture frames
        //

        std::vector<TextureFrameSpecification<TTextureDatabase>> textureFrames;

        for (auto const & frameValue : Utils::GetMandatoryJsonArray(groupJson, "frames"))
        {
            if (!frameValue.is<picojson::object>())
            {
                throw GameException("Texture database: found a non-object frame in database");
            }

            auto frameJson = frameValue.get<picojson::object>();

            // Get frame properties
            std::optional<float> const frameWorldScaling = Utils::GetOptionalJsonMember<float>(frameJson, "world_scaling");
            std::optional<float> frameWorldWidth = Utils::GetOptionalJsonMember<float>(frameJson, "world_width");
            std::optional<float> frameWorldHeight = Utils::GetOptionalJsonMember<float>(frameJson, "world_height");
            std::optional<bool> const frameHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(frameJson, "has_own_ambient_light");
            std::optional<int> const frameAnchorOffsetX = Utils::GetOptionalJsonMember<int>(frameJson, "anchor_offset_x");
            std::optional<int> const frameAnchorOffsetY = Utils::GetOptionalJsonMember<int>(frameJson, "anchor_offset_y");
            std::optional<std::string> const frameDisplayName = Utils::GetOptionalJsonMember<std::string>(frameJson, "display_name");

            // Get filename and make regex out of it
            std::string const frameNamePattern = Utils::GetMandatoryJsonMember<std::string>(frameJson, "frame_name_pattern");
            std::regex const frameNameRegex("^" + frameNamePattern + "$");

            // Find all files matching the regex
            int filesFoundFromFrameCount = 0;
            for (auto const & frameDescriptor : allTextureFrameDescriptors)
            {
                if (std::regex_match(frameDescriptor.Name, frameNameRegex))
                {
                    // This file belongs to this group

                    //
                    // Get frame size
                    //

                    ImageSize textureSize = assetManager.GetTextureDatabaseFrameSize(TTextureDatabase::DatabaseName, frameDescriptor.RelativePath);


                    //
                    // Extract frame index
                    //

                    TextureFrameIndex frameIndex;
                    if (doAutoAssignFrameIndices)
                    {
                        // Assign frame ID
                        frameIndex = static_cast<TextureFrameIndex>(textureFrames.size());
                    }
                    else
                    {
                        // Extract index from name
                        static std::regex const TextureNameFrameIndexRegex("^.+?_(\\d+)$");
                        std::smatch frameIndexMatch;
                        if (!std::regex_match(frameDescriptor.Name, frameIndexMatch, TextureNameFrameIndexRegex))
                        {
                            throw GameException("Texture database: cannot extract frame index from texture name \"" + frameDescriptor.Name + "\", and auto-assigning indices is disabled");
                        }

                        assert(frameIndexMatch.size() == 2);
                        frameIndex = static_cast<TextureFrameIndex>(std::stoi(frameIndexMatch[1].str()));
                    }

                    //
                    // Resolve properties
                    //

                    float worldWidth;
                    float worldHeight;
                    if (frameWorldWidth || frameWorldHeight)
                    {
                        if (!frameWorldWidth)
                        {
                            frameWorldWidth = *frameWorldHeight / static_cast<float>(textureSize.height) * static_cast<float>(textureSize.width);
                        }

                        if (!frameWorldHeight)
                        {
                            frameWorldHeight = *frameWorldWidth / static_cast<float>(textureSize.width) * static_cast<float>(textureSize.height);
                        }

                        worldWidth = *frameWorldWidth;
                        worldHeight = *frameWorldHeight;
                    }
                    else if (frameWorldScaling)
                    {
                        worldWidth = static_cast<float>(textureSize.width) * (*frameWorldScaling);
                        worldHeight = static_cast<float>(textureSize.height) * (*frameWorldScaling);
                    }
                    else if (groupWorldWidth || groupWorldHeight)
                    {
                        if (!groupWorldWidth)
                        {
                            groupWorldWidth = *groupWorldHeight / static_cast<float>(textureSize.height) * static_cast<float>(textureSize.width);
                        }

                        if (!groupWorldHeight)
                        {
                            groupWorldHeight = *groupWorldWidth / static_cast<float>(textureSize.width) * static_cast<float>(textureSize.height);
                        }

                        worldWidth = *groupWorldWidth;
                        worldHeight = *groupWorldHeight;
                    }
                    else
                    {
                        worldWidth = static_cast<float>(textureSize.width) * groupWorldScaling;
                        worldHeight = static_cast<float>(textureSize.height) * groupWorldScaling;
                    }

                    bool hasOwnAmbientLight = frameHasOwnAmbientLight.has_value() ? *frameHasOwnAmbientLight : groupHasOwnAmbientLight;

                    // Transform to center-based, pixels
                    int anchorCenterPixelX = (textureSize.width / 2) + (frameAnchorOffsetX ? *frameAnchorOffsetX : groupAnchorOffsetX);
                    int anchorCenterPixelY = (textureSize.height / 2) + (frameAnchorOffsetY ? *frameAnchorOffsetY : groupAnchorOffsetY);

                    // Transform to center-based, world
                    float anchorCenterWorldX = static_cast<float>(anchorCenterPixelX) * worldWidth / static_cast<float>(textureSize.width);
                    float anchorCenterWorldY = static_cast<float>(textureSize.height - anchorCenterPixelY) * worldHeight / static_cast<float>(textureSize.height);

                    // Transform to frame
                    float anchorOffsetFrameX = static_cast<float>(frameAnchorOffsetX ? *frameAnchorOffsetX : groupAnchorOffsetX) / static_cast<float>(textureSize.width);
                    float anchorOffsetFrameY = static_cast<float>(frameAnchorOffsetY ? *frameAnchorOffsetY : groupAnchorOffsetY) / static_cast<float>(textureSize.height);

                    //
                    // Store frame specification
                    //

                    textureFrames.emplace_back(
                        TextureFrameSpecification<TTextureDatabase>(
                            TextureFrameMetadata<TTextureDatabase>(
                                textureSize,
                                worldWidth,
                                worldHeight,
                                hasOwnAmbientLight,
                                ImageCoordinates(
                                    anchorCenterPixelX,
                                    anchorCenterPixelY),
                                vec2f(
                                    anchorCenterWorldX,
                                    anchorCenterWorldY),
                                vec2f(
                                    anchorOffsetFrameX,
                                    anchorOffsetFrameY),
                                TextureFrameId<TTextureGroups>(group, frameIndex),
                                frameDescriptor.Name,
                                frameDisplayName.has_value() ? *frameDisplayName : frameDescriptor.Name),
                            frameDescriptor.RelativePath));

                    //
                    // Remember this frame file was matched
                    //

                    matchedTextureFrameNames.insert(frameDescriptor.Name);

                    ++filesFoundFromFrameCount;
                }
            }

            // Make sure at least one matching file was found for this frame specification
            if (0 == filesFoundFromFrameCount)
            {
                throw GameException("Texture database: couldn't match any file to frame filename pattern \"" + frameNamePattern + "\"");
            }
        }

        // Sort frames by frame index
        std::sort(
            textureFrames.begin(),
            textureFrames.end(),
            [](auto const & a, auto const & b)
            {
                return a.Metadata.FrameId.FrameIndex < b.Metadata.FrameId.FrameIndex;
            });

        // Make sure all indices are found
        for (size_t expectedIndex = 0; expectedIndex < textureFrames.size(); ++expectedIndex)
        {
            if (textureFrames[expectedIndex].Metadata.FrameId.FrameIndex < expectedIndex)
            {
                throw GameException("Texture database: duplicate frame \"" + std::to_string(textureFrames[expectedIndex].Metadata.FrameId.FrameIndex) + "\" in group \"" + groupName + "\"");
            }
            else if (textureFrames[expectedIndex].Metadata.FrameId.FrameIndex > expectedIndex)
            {
                throw GameException("Texture database: missing frame \"" + std::to_string(expectedIndex) + "\" in group \"" + groupName + "\"");
            }
        }

        // Store texture frames
        textureGroups.emplace_back(
            group,
            textureFrames);
    }

    // Sort groups by group index
    std::sort(
        textureGroups.begin(),
        textureGroups.end(),
        [](auto const & a, auto const & b)
        {
            return a.Group < b.Group;
        });

    // Make sure all groups are found
    if (textureGroups.size() != static_cast<size_t>(TTextureGroups::_Last) + 1)
    {
        throw GameException("Texture database: not all groups have been found");
    }

    // Make sure all group indices are found
    for (uint16_t expectedIndex = 0; expectedIndex <= static_cast<uint16_t>(TTextureGroups::_Last); ++expectedIndex)
    {
        if (static_cast<uint16_t>(textureGroups[expectedIndex].Group) < expectedIndex)
        {
            throw GameException("Texture database: duplicate group \"" + std::to_string(static_cast<uint16_t>(textureGroups[expectedIndex].Group)) + "\"");
        }
        else if (static_cast<uint16_t>(textureGroups[expectedIndex].Group) > expectedIndex)
        {
            throw GameException("Texture database: missing group \"" + std::to_string(expectedIndex) + "\"");
        }
    }

    // Make sure all textures found in file system have been exhausted
    if (matchedTextureFrameNames.size() != allTextureFrameDescriptors.size())
    {
        throw GameException(
            "Texture database: couldn't match "
            + std::to_string(allTextureFrameDescriptors.size() - matchedTextureFrameNames.size())
            + " texture frame files to texture specifications");
    }

    return TextureDatabase(std::move(textureGroups));
}
