/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <Game/TextureAtlas.h>
#include <Game/TextureDatabase.h>
#include <Game/TextureTypes.h>

#include <GameCore/Utils.h>

#include <filesystem>
#include <iostream>
#include <string>

class Baker
{
public:

    struct AtlasBakingOptions
    {
        bool AlphaPremultiply;
        bool BinaryTransparencySmoothing;
        bool MipMappable;
        bool Regular;

        static AtlasBakingOptions Deserialize(std::filesystem::path const & optionsJsonFilePath)
        {
            picojson::object rootJsonObject = Utils::GetJsonValueAsObject(Utils::ParseJSONFile(optionsJsonFilePath), "root");

            bool alphaPreMultiply = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "alphaPreMultiply");
            bool mipMappable = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "mipMappable");
            bool binaryTransparencySmoothing = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "binaryTransparencySmoothing");
            bool regular = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "regular");

            return AtlasBakingOptions({
                alphaPreMultiply,
                binaryTransparencySmoothing,
                mipMappable,
                regular });
        }
    };

public:

    template<typename TextureDatabaseTraits>
    static size_t BakeAtlas(
        std::filesystem::path const & texturesRootDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        AtlasBakingOptions const & options)
    {
        if (!std::filesystem::exists(texturesRootDirectoryPath))
        {
            throw std::runtime_error("Textures root directory '" + texturesRootDirectoryPath.string() + "' does not exist");
        }

        if (!std::filesystem::exists(outputDirectoryPath))
        {
            throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
        }

        // Load database
        auto textureDatabase = Render::TextureDatabase<TextureDatabaseTraits>::Load(
            texturesRootDirectoryPath);

        // Create atlas

        std::cout << "Creating " << (options.Regular ? "regular " : "") << "atlas..";

        Render::AtlasOptions atlasOptions = Render::AtlasOptions::None;
        if (options.AlphaPremultiply)
            atlasOptions = atlasOptions | Render::AtlasOptions::AlphaPremultiply;
        if (options.BinaryTransparencySmoothing)
            atlasOptions = atlasOptions | Render::AtlasOptions::BinaryTransparencySmoothing;
        if (options.MipMappable)
            atlasOptions = atlasOptions | Render::AtlasOptions::MipMappable;

        auto textureAtlas = options.Regular
            ? Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildRegularAtlas(
                textureDatabase,
                atlasOptions,
                [](float, ProgressMessageType)
                {
                    std::cout << ".";
                })
            : Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildAtlas(
                textureDatabase,
                atlasOptions,
                [](float, ProgressMessageType)
                {
                    std::cout << ".";
                });

        std::cout << std::endl;

        // Serialize atlas
        textureAtlas.Serialize(
            TextureDatabaseTraits::DatabaseName,
            outputDirectoryPath);

        return textureAtlas.Metadata.GetFrameCount();
    }
};
