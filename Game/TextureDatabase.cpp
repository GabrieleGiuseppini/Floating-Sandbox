/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextureDatabase.h"

namespace Render {

template <typename TextureGroups>
void TextureFrameMetadata<TextureGroups>::Serialize(picojson::object & root) const
{
    picojson::object size;
    size["width"] = picojson::value(static_cast<int64_t>(Size.Width));
    size["height"] = picojson::value(static_cast<int64_t>(Size.Height));
    root["size"] = picojson::value(size);

    picojson::object worldSize;
    worldSize["width"] = picojson::value(static_cast<double>(WorldWidth));
    worldSize["height"] = picojson::value(static_cast<double>(WorldHeight));
    root["world_size"] = picojson::value(worldSize);

    root["has_own_ambient_light"] = picojson::value(HasOwnAmbientLight);

    picojson::object anchorWorld;
    anchorWorld["x"] = picojson::value(static_cast<double>(AnchorWorldX));
    anchorWorld["y"] = picojson::value(static_cast<double>(AnchorWorldY));
    root["anchor_world"] = picojson::value(anchorWorld);

    picojson::object frameId;
    frameId["group"] = picojson::value(static_cast<int64_t>(FrameId.Group));
    frameId["frameIndex"] = picojson::value(static_cast<int64_t>(FrameId.FrameIndex));
    root["id"] = picojson::value(frameId);

    root["name"] = picojson::value(FrameName);
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

    picojson::object const & anchorWorldJson = root.at("anchor_world").get<picojson::object>();
    float anchorWorldX = static_cast<float>(anchorWorldJson.at("x").get<double>());
    float anchorWorldY = static_cast<float>(anchorWorldJson.at("y").get<double>());

    picojson::object const & frameIdJson = root.at("id").get<picojson::object>();
    TextureGroups group = static_cast<TextureGroups>(frameIdJson.at("group").get<std::int64_t>());
    TextureFrameIndex frameIndex = static_cast<TextureFrameIndex>(frameIdJson.at("frameIndex").get<std::int64_t>());

    std::string const & name = root.at("name").get<std::string>();

    return TextureFrameMetadata<TextureGroups>(
        size,
        worldWidth,
        worldHeight,
        hasOwnAmbientLight,
        anchorWorldX,
        anchorWorldY,
        TextureFrameId<TextureGroups>(group, frameIndex),
        name);
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
            std::string const stem = entryIt.path().filename().stem().string();

            allTextureFiles.emplace_back(
                entryIt.path(),
                stem);
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

    float const framesToLoad = static_cast<float>(allTextureFiles.size());

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
            std::optional<int> frameOptionalIndex = Utils::GetOptionalJsonMember<int>(frameJson, "index");
            std::optional<float> frameWorldScaling = Utils::GetOptionalJsonMember<float>(frameJson, "worldScaling");
            std::optional<float> frameWorldWidth = Utils::GetOptionalJsonMember<float>(frameJson, "worldWidth");
            std::optional<float> frameWorldHeight = Utils::GetOptionalJsonMember<float>(frameJson, "worldHeight");
            std::optional<bool> frameHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(frameJson, "hasOwnAmbientLight");
            std::optional<int> frameAnchorOffsetX = Utils::GetOptionalJsonMember<int>(frameJson, "anchorOffsetX");
            std::optional<int> frameAnchorOffsetY = Utils::GetOptionalJsonMember<int>(frameJson, "anchorOffsetY");
            std::optional<std::string> frameName = Utils::GetOptionalJsonMember<std::string>(frameJson, "name");

            // Get filename and make regex out of it
            std::string frameFilename = Utils::GetMandatoryJsonMember<std::string>(frameJson, "filename");
            std::regex const frameFilenameRegex("^" + frameFilename + "$");

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
                    if (!!frameWorldWidth || !!frameWorldHeight)
                    {
                        if (!frameWorldWidth)
                        {
                            throw GameException("Texture database: frame \"" + frameFilename + "\" has worldHeight but no worldWidth");
                        }

                        if (!frameWorldHeight)
                        {
                            throw GameException("Texture database: frame \"" + frameFilename + "\" has worldWidth but no worldHeight");
                        }

                        worldWidth = *frameWorldWidth;
                        worldHeight = *frameWorldHeight;
                    }
                    else if (!!frameWorldScaling)
                    {
                        worldWidth = static_cast<float>(textureSize.Width) * (*frameWorldScaling);
                        worldHeight = static_cast<float>(textureSize.Height) * (*frameWorldScaling);
                    }
                    else if (!!groupWorldWidth || !!groupWorldHeight)
                    {
                        if (!groupWorldWidth)
                        {
                            throw GameException("Texture database: group \"" + groupName + "\" has worldHeight but no worldWidth");
                        }

                        if (!groupWorldHeight)
                        {
                            throw GameException("Texture database: group \"" + groupName + "\" has worldWidth but no worldHeight");
                        }

                        worldWidth = *groupWorldWidth;
                        worldHeight = *groupWorldHeight;
                    }
                    else if (!!groupWorldScaling)
                    {
                        worldWidth = static_cast<float>(textureSize.Width) * (*groupWorldScaling);
                        worldHeight = static_cast<float>(textureSize.Height) * (*groupWorldScaling);
                    }
                    else
                    {
                        throw GameException("Texture database: cannot find world dimensions for frame \"" + frameFilename + "\"");
                    }

                    bool hasOwnAmbientLight = !!frameHasOwnAmbientLight ? *frameHasOwnAmbientLight : groupHasOwnAmbientLight;

                    int anchorX = (textureSize.Width / 2) + (!!frameAnchorOffsetX ? *frameAnchorOffsetX : groupAnchorOffsetX);
                    int anchorY = (textureSize.Height / 2) + (!!frameAnchorOffsetY ? *frameAnchorOffsetY : groupAnchorOffsetY);

                    // Transform to world
                    float anchorWorldX = static_cast<float>(anchorX) * worldWidth / static_cast<float>(textureSize.Width);
                    float anchorWorldY = static_cast<float>(textureSize.Height - anchorY) * worldHeight / static_cast<float>(textureSize.Height);

                    std::string name = !!frameName ? *frameName : std::to_string(frameIndex);


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
                                anchorWorldX,
                                anchorWorldY,
                                TextureFrameId<TextureGroups>(group, frameIndex),
                                name),
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
                throw GameException("Texture database: couldn't match any file to frame file \"" + frameFilename + "\"");
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

//
// Explicit specializations for all texture groups
//

#include "TextureTypes.h"

template struct Render::TextureFrameMetadata<Render::CloudTextureGroups>;
template struct Render::TextureFrameMetadata<Render::WorldTextureGroups>;
template struct Render::TextureFrameMetadata<Render::NoiseTextureGroups>;
template struct Render::TextureFrameMetadata<Render::GenericLinearTextureGroups>;
template struct Render::TextureFrameMetadata<Render::GenericMipMappedTextureGroups>;
template struct Render::TextureFrameMetadata<Render::ExplosionTextureGroups>;

template class Render::TextureDatabase<Render::CloudTextureDatabaseTraits>;
template class Render::TextureDatabase<Render::WorldTextureDatabaseTraits>;
template class Render::TextureDatabase<Render::NoiseTextureDatabaseTraits>;
template class Render::TextureDatabase<Render::GenericLinearTextureTextureDatabaseTraits>;
template class Render::TextureDatabase<Render::GenericMipMappedTextureTextureDatabaseTraits>;
template class Render::TextureDatabase<Render::ExplosionTextureDatabaseTraits>;