/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <Game/TextureAtlas.h>
#include <Game/TextureDatabase.h>
#include <Game/TextureTypes.h>

#include <filesystem>
#include <iostream>
#include <string>

class Baker
{
public:

    template<typename TextureDatabaseTraits>
    static size_t BakeAtlas(
        std::filesystem::path const & databaseRootDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        bool doAlphaPremultiply,
        bool doMipMapped,
        bool doRegular)
    {
        if (!std::filesystem::exists(databaseRootDirectoryPath))
        {
            throw std::runtime_error("Database root directory '" + databaseRootDirectoryPath.string() + "' does not exist");
        }

        if (!std::filesystem::exists(outputDirectoryPath))
        {
            throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
        }

        // Load database
        auto textureDatabase = Render::TextureDatabase<TextureDatabaseTraits>::Load(
            databaseRootDirectoryPath);

        // Create atlas

        std::cout << "Creating " << (doRegular ? "regular " : "") << "atlas..";

        Render::AtlasOptions options = Render::AtlasOptions::None;
        if (doAlphaPremultiply)
            options = options | Render::AtlasOptions::AlphaPremultiply;
        if (doMipMapped)
            options = options | Render::AtlasOptions::MipMappable;

        auto textureAtlas = doRegular
            ? Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildRegularAtlas(
                textureDatabase,
                options,
                [](float, ProgressMessageType)
                {
                    std::cout << ".";
                })
            : Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildAtlas(
                textureDatabase,
                options,
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
