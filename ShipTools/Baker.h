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

    template <typename TextureDatabaseTraits>
    static void BakeRegularAtlas(
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

        auto textureAtlas = Render::TextureAtlasBuilder<typename TextureDatabaseTraits::TextureGroups>::BuildRegularAtlas(
            textureDatabase,
            doAlphaPremultiply ? Render::AtlasOptions::AlphaPremultiply : Render::AtlasOptions::None,
            [](float, std::string const &)
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
