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

    template <typename TextureDatabaseTraits, bool IsRegular>
    static void BakeAtlas(
        std::filesystem::path const & databaseRootDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        bool doAlphaPremultiply)
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

        std::cout << "Creating atlas..";

        auto textureAtlas = IsRegular
            ? Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildRegularAtlas(
                textureDatabase,
                doAlphaPremultiply ? Render::AtlasOptions::AlphaPremultiply : Render::AtlasOptions::None,
                [](float, ProgressMessageType)
                {
                    std::cout << ".";
                })
            : Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildAtlas(
                textureDatabase,
                doAlphaPremultiply ? Render::AtlasOptions::AlphaPremultiply : Render::AtlasOptions::None,
                [](float, ProgressMessageType)
                {
                    std::cout << ".";
                });

        std::cout << std::endl;

        // Serialize atlas
        textureAtlas.Serialize(
            TextureDatabaseTraits::DatabaseName,
            outputDirectoryPath);
    }
};
