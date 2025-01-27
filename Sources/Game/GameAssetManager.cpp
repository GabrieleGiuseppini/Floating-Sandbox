/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-01-27
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "GameAssetManager.h"

#include "FileSystem.h"

#include <Core/GameException.h>
#include <Core/Log.h>
#include <Core/PngTools.h>
#include <Core/Utils.h>

GameAssetManager::GameAssetManager(std::string const & argv0)
	: mDataRoot(std::filesystem::canonical(std::filesystem::path(argv0)).parent_path() / "Data")
	, mTextureDatabaseRoot(mDataRoot / "Textures")
{
}

GameAssetManager::GameAssetManager(std::filesystem::path const & textureDatabaseRoot)
	: mDataRoot(textureDatabaseRoot) // Just because (not needed in this scenario)
	, mTextureDatabaseRoot(textureDatabaseRoot)
{
}

picojson::value GameAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName)
{
	return Utils::ParseJSONString(
        FileSystem::LoadTextFile(mTextureDatabaseRoot / databaseName / "database.json"));
}

ImageSize GameAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameFileName)
{
    auto const imageFile = FileSystem::LoadBinaryFile(mTextureDatabaseRoot / databaseName / frameFileName);
    return PngTools::GetImageSize(imageFile);
}

RgbaImageData GameAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameFileName)
{
    auto const imageFile = FileSystem::LoadBinaryFile(mTextureDatabaseRoot / databaseName / frameFileName);
    return PngTools::DecodeImageRgba(imageFile);
}

std::vector<std::string> GameAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName)
{
    std::vector<std::string> frameFilenames;

    for (auto const & entryIt : std::filesystem::recursive_directory_iterator(mTextureDatabaseRoot / databaseName))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() != ".json")
        {
            // We only expect png's
            if (entryIt.path().extension().string() == ".png")
            {
                frameFilenames.push_back(entryIt.path().filename().string());
            }
            else if (entryIt.path().extension().string() != ".txt") // txt allowed
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading a texture database");
            }
        }
    }

    return frameFilenames;
}

picojson::value GameAssetManager::LoadTetureAtlasSpecification(std::string const & textureDatabaseName)
{
    return Utils::ParseJSONString(
        FileSystem::LoadTextFile(mTextureDatabaseRoot / "Atlases" / (textureDatabaseName + ".atlas.json")));
}

RgbaImageData GameAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName)
{
    auto const imageFile = FileSystem::LoadBinaryFile(mTextureDatabaseRoot / "Atlases" / (textureDatabaseName + ".atlas.png"));
    return PngTools::DecodeImageRgba(imageFile);
}

