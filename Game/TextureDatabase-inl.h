/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureDatabase.h"

#include <GameCore/Log.h>

namespace Render {

template <typename TextureGroups>
void TextureFrameMetadata<TextureGroups>::Serialize(picojson::object & root) const
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

    picojson::object anchorCenter;
    anchorCenter["x"] = picojson::value(static_cast<int64_t>(AnchorCenter.x));
    anchorCenter["y"] = picojson::value(static_cast<int64_t>(AnchorCenter.y));
    root["anchor_center"] = picojson::value(anchorCenter);

    picojson::object anchorCenterWorld;
    anchorCenterWorld["x"] = picojson::value(static_cast<double>(AnchorCenterWorld.x));
    anchorCenterWorld["y"] = picojson::value(static_cast<double>(AnchorCenterWorld.y));
    root["anchor_center_world"] = picojson::value(anchorCenterWorld);

    picojson::object frameId;
    frameId["group"] = picojson::value(static_cast<int64_t>(FrameId.Group));
    frameId["frameIndex"] = picojson::value(static_cast<int64_t>(FrameId.FrameIndex));
    root["id"] = picojson::value(frameId);

    root["displayName"] = picojson::value(FrameDisplayName);
}

template <typename TextureGroups>
TextureFrameMetadata<TextureGroups> TextureFrameMetadata<TextureGroups>::Deserialize(picojson::object const & root)
{
    picojson::object const & sizeJson = root.at("size").get<picojson::object>();
    ImageSize size(
        static_cast<int>(sizeJson.at("width").get<std::int64_t>()),
        static_cast<int>(sizeJson.at("height").get<std::int64_t>()));

    picojson::object const & worldSizeJson = root.at("world_size").get<picojson::object>();
    float worldWidth = static_cast<float>(worldSizeJson.at("width").get<double>());
    float worldHeight = static_cast<float>(worldSizeJson.at("height").get<double>());

    bool hasOwnAmbientLight = root.at("has_own_ambient_light").get<bool>();

    picojson::object const & anchorCenterJson = root.at("anchor_center").get<picojson::object>();
    ImageCoordinates anchorCenter(
        static_cast<int>(anchorCenterJson.at("x").get<int64_t>()),
        static_cast<int>(anchorCenterJson.at("y").get<int64_t>()));

    picojson::object const & anchorCenterWorldJson = root.at("anchor_center_world").get<picojson::object>();
    vec2f anchorCenterWorld(
        static_cast<float>(anchorCenterWorldJson.at("x").get<double>()),
        static_cast<float>(anchorCenterWorldJson.at("y").get<double>()));

    picojson::object const & frameIdJson = root.at("id").get<picojson::object>();
    TextureGroups group = static_cast<TextureGroups>(frameIdJson.at("group").get<std::int64_t>());
    TextureFrameIndex frameIndex = static_cast<TextureFrameIndex>(frameIdJson.at("frameIndex").get<std::int64_t>());

    std::string const & displayName = root.at("displayName").get<std::string>();

    return TextureFrameMetadata<TextureGroups>(
        size,
        worldWidth,
        worldHeight,
        hasOwnAmbientLight,
        anchorCenter,
        anchorCenterWorld,
        TextureFrameId<TextureGroups>(group, frameIndex),
        displayName);
}

template <typename TextureDatabaseTraits>
TextureDatabase<TextureDatabaseTraits> TextureDatabase<TextureDatabaseTraits>::Load(std::filesystem::path const & texturesRootFolderPath)
{
    std::filesystem::path const databaseFolderPath = texturesRootFolderPath / TextureDatabaseTraits::DatabaseName;

    //
    // Visit directory and build set of all files
    //

    struct FileData
    {
        std::filesystem::path Path;
        std::string Stem;

        FileData(
            std::filesystem::path path,
            std::string stem)
            : Path(path)
            , Stem(stem)
        {}
    };

    std::vector<FileData> allTextureFiles;

    for (auto const & entryIt : std::filesystem::directory_iterator(databaseFolderPath))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() != ".json")
        {
            // We only expect png's
            if (entryIt.path().extension().string() == ".png")
            {
                std::string const stem = entryIt.path().filename().stem().string();

                allTextureFiles.emplace_back(
                    entryIt.path(),
                    stem);
            }
            else
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading a texture database");
            }
        }
    }


    //
    // Load JSON file
    //

    std::filesystem::path const jsonFilePath = databaseFolderPath / "database.json";
    picojson::value root = Utils::ParseJSONFile(jsonFilePath.string());
    if (!root.is<picojson::array>())
    {
        throw GameException("Texture database \"" + TextureDatabaseTraits::DatabaseName + "\": file \"" + jsonFilePath.string() + "\" does not contain a JSON array");
    }


    //
    // Process JSON groups and build texture groups
    //

    std::vector<TextureGroup<TextureGroups>> textureGroups;

    std::set<std::filesystem::path> matchedTextureFiles;

    for (auto const & groupValue : root.get<picojson::array>())
    {
        if (!groupValue.is<picojson::object>())
        {
            throw GameException("Texture database: found a non-object group in database");
        }

        auto groupJson = groupValue.get<picojson::object>();

        std::string groupName = Utils::GetMandatoryJsonMember<std::string>(groupJson, "groupName");
        TextureGroups group = TextureDatabaseTraits::StrToTextureGroup(groupName);

        // Load group-wide defaults
        std::optional<float> groupWorldScaling = Utils::GetOptionalJsonMember<float>(groupJson, "worldScaling");
        std::optional<float> groupWorldWidth = Utils::GetOptionalJsonMember<float>(groupJson, "worldWidth");
        std::optional<float> groupWorldHeight = Utils::GetOptionalJsonMember<float>(groupJson, "worldHeight");
        bool groupHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(groupJson, "hasOwnAmbientLight", false);
        int groupAnchorOffsetX = Utils::GetOptionalJsonMember<int>(groupJson, "anchorOffsetX", 0);
        int groupAnchorOffsetY = Utils::GetOptionalJsonMember<int>(groupJson, "anchorOffsetY", 0);

        //
        // Process frames from JSON and build texture frames
        //

        std::vector<TextureFrameSpecification<TextureGroups>> textureFrames;

        for (auto const & frameValue : Utils::GetMandatoryJsonArray(groupJson, "frames"))
        {
            if (!frameValue.is<picojson::object>())
            {
                throw GameException("Texture database: found a non-object frame in database");
            }

            auto frameJson = frameValue.get<picojson::object>();

            // Get frame properties
            std::optional<int> const frameOptionalIndex = Utils::GetOptionalJsonMember<int>(frameJson, "index");
            std::optional<float> const frameWorldScaling = Utils::GetOptionalJsonMember<float>(frameJson, "worldScaling");
            std::optional<float> frameWorldWidth = Utils::GetOptionalJsonMember<float>(frameJson, "worldWidth");
            std::optional<float> frameWorldHeight = Utils::GetOptionalJsonMember<float>(frameJson, "worldHeight");
            std::optional<bool> const frameHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(frameJson, "hasOwnAmbientLight");
            std::optional<int> const frameAnchorOffsetX = Utils::GetOptionalJsonMember<int>(frameJson, "anchorOffsetX");
            std::optional<int> const frameAnchorOffsetY = Utils::GetOptionalJsonMember<int>(frameJson, "anchorOffsetY");
            std::optional<std::string> const frameDisplayName = Utils::GetOptionalJsonMember<std::string>(frameJson, "displayName");

            // Get filename and make regex out of it
            std::string const frameFilenamePattern = Utils::GetMandatoryJsonMember<std::string>(frameJson, "filenamePattern");
            std::regex const frameFilenameRegex("^" + frameFilenamePattern + "$");

            // Find all files matching the regex
            int filesFoundFromFrameCount = 0;
            for (auto fileIt = allTextureFiles.cbegin(); fileIt != allTextureFiles.cend(); ++fileIt)
            {
                FileData const & fileData = *fileIt;

                if (std::regex_match(fileData.Stem, frameFilenameRegex))
                {
                    // This file belongs to this group

                    //
                    // Get frame size
                    //

                    ImageSize textureSize = ImageFileTools::GetImageSize(fileData.Path);


                    //
                    // Extract frame index
                    //

                    TextureFrameIndex frameIndex;

                    if (!!frameOptionalIndex)
                    {
                        // Take provided index
                        frameIndex = static_cast<TextureFrameIndex>(*frameOptionalIndex);
                    }
                    else
                    {
                        // Extract index from filename
                        static std::regex const TextureFilenameFrameIndexRegex("^.+?_(\\d+)$");
                        std::smatch frameIndexMatch;
                        if (!std::regex_match(fileData.Stem, frameIndexMatch, TextureFilenameFrameIndexRegex))
                        {
                            throw GameException("Texture database: cannot find frame index in texture filename \"" + fileData.Stem + "\"");
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
                    else if (groupWorldScaling)
                    {
                        worldWidth = static_cast<float>(textureSize.width) * (*groupWorldScaling);
                        worldHeight = static_cast<float>(textureSize.height) * (*groupWorldScaling);
                    }
                    else
                    {
                        throw GameException("Texture database: cannot find world dimensions for frame \"" + frameFilenamePattern + "\"");
                    }

                    bool hasOwnAmbientLight = frameHasOwnAmbientLight.has_value() ? *frameHasOwnAmbientLight : groupHasOwnAmbientLight;

                    int anchorX = (textureSize.width / 2) + (frameAnchorOffsetX ? *frameAnchorOffsetX : groupAnchorOffsetX);
                    int anchorY = (textureSize.height / 2) + (frameAnchorOffsetY ? *frameAnchorOffsetY : groupAnchorOffsetY);

                    // Transform to world
                    float anchorWorldX = static_cast<float>(anchorX) * worldWidth / static_cast<float>(textureSize.width);
                    float anchorWorldY = static_cast<float>(textureSize.height - anchorY) * worldHeight / static_cast<float>(textureSize.height);

                    //
                    // Store frame specification
                    //

                    textureFrames.emplace_back(
                        TextureFrameSpecification<TextureGroups>(
                            TextureFrameMetadata<TextureGroups>(
                                textureSize,
                                worldWidth,
                                worldHeight,
                                hasOwnAmbientLight,
                                ImageCoordinates(
                                    anchorX,
                                    anchorY),
                                vec2f(
                                    anchorWorldX,
                                    anchorWorldY),
                                TextureFrameId<TextureGroups>(group, frameIndex),
                                frameDisplayName.has_value() ? *frameDisplayName : fileData.Stem),
                            fileData.Path));


                    //
                    // Remember this file was matched
                    //

                    matchedTextureFiles.insert(fileData.Path);

                    ++filesFoundFromFrameCount;
                }
            }

            // Make sure at least one matching file was found for this frame specification
            if (0 == filesFoundFromFrameCount)
            {
                throw GameException("Texture database: couldn't match any file to frame filename pattern \"" + frameFilenamePattern + "\"");
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

    // Make sure all group indices are found
    for (uint16_t expectedIndex = 0; expectedIndex <= static_cast<uint16_t>(TextureGroups::_Last); ++expectedIndex)
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
    if (matchedTextureFiles.size() != allTextureFiles.size())
    {
        throw GameException(
            "Texture database: couldn't match "
            + std::to_string(allTextureFiles.size() - matchedTextureFiles.size())
            + " texture files to texture specifications");
    }

    return TextureDatabase(std::move(textureGroups));
}

}
