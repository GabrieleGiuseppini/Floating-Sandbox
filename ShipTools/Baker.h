/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <Game/RenderCore.h>
#include <Game/TextureAtlas.h>

#include <filesystem>
#include <string>

class Baker
{
public:

    template <typename TextureDatabaseTraits>
    static void BakeRegularAtlas(
        std::filesystem::path const & databaseDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        bool doAlphaPremultiply)
    {
        if (!std::filesystem::exists(databaseDirectoryPath))
        {
            throw std::runtime_error("Database directory '" + databaseDirectoryPath.string() + "' does not exist");
        }

        if (!std::filesystem::exists(outputDirectoryPath))
        {
            throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
        }

        // Load database
        auto textureDatabase = TextureDatabase<TextureDatabaseTraits>::Load(
            databaseDirectoryPath);

        // Create atlas
        auto textureAtlas = TextureAtlasBuilder<TextureDatabaseTraits::TextureGroups>::BuildAtlas(
            textureDatabase,
            doAlphaPremultiply ? Render::AtlasOptions::AlphaPremultiply : Render::AtlasOptions::None,
            [&progressCallback](float, std::string const &)
            {
            });

        // TODOHERE
    }
};
