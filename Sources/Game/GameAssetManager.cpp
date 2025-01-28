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
    return LoadJson(mTextureDatabaseRoot / databaseName / "database.json");
}

ImageSize GameAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath)
{
    return GetImageSize(mTextureDatabaseRoot / databaseName / frameRelativePath);
}

RgbaImageData GameAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath)
{
    return LoadPngImageRgba(mTextureDatabaseRoot / databaseName / frameRelativePath);
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName)
{
    std::vector<AssetDescriptor> frameDescriptors;

    std::filesystem::path const databaseRootPath = mTextureDatabaseRoot / databaseName;
    for (auto const & entryIt : std::filesystem::recursive_directory_iterator(databaseRootPath))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() != ".json")
        {
            // We only expect png's
            if (entryIt.path().extension().string() == ".png")
            {
                auto const framePath = entryIt.path();
                frameDescriptors.push_back(
                    AssetDescriptor{
                        framePath.filename().stem().string(),
                        std::filesystem::relative(framePath, databaseRootPath).string()
                    });
            }
            else if (entryIt.path().extension().string() != ".txt") // txt allowed
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading a texture database");
            }
        }
    }

    return frameDescriptors;
}

picojson::value GameAssetManager::LoadTetureAtlasSpecification(std::string const & textureDatabaseName)
{
    return LoadJson(mTextureDatabaseRoot / "Atlases" / MakeAtlasSpecificationFilename(textureDatabaseName));
}

RgbaImageData GameAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName)
{
    return LoadPngImageRgba(mTextureDatabaseRoot / "Atlases" / MakeAtlasImageFilename(textureDatabaseName));
}

// Helpers

ImageSize GameAssetManager::GetImageSize(std::filesystem::path const & filePath)
{
    auto const buffer = FileSystem::LoadBinaryFile(filePath);
    return PngTools::GetImageSize(buffer);
}

template<>
ImageData<rgbaColor> GameAssetManager::LoadPngImage<rgbaColor>(std::filesystem::path const & filePath)
{
    return LoadPngImageRgba(filePath);
}

template<>
ImageData<rgbColor> GameAssetManager::LoadPngImage<rgbColor>(std::filesystem::path const & filePath)
{
    return LoadPngImageRgb(filePath);
}

RgbaImageData GameAssetManager::LoadPngImageRgba(std::filesystem::path const & filePath)
{
    auto const buffer = FileSystem::LoadBinaryFile(filePath);
    return PngTools::DecodeImageRgba(buffer);
}

RgbImageData GameAssetManager::LoadPngImageRgb(std::filesystem::path const & filePath)
{
    auto const buffer = FileSystem::LoadBinaryFile(filePath);
    return PngTools::DecodeImageRgb(buffer);
}

void GameAssetManager::SavePngImage(
    RgbaImageData const & image,
    std::filesystem::path filePath)
{
    auto const buffer = PngTools::EncodeImage(image);
    FileSystem::SaveBinaryFile(buffer, filePath);
}

void GameAssetManager::SavePngImage(
    RgbImageData const & image,
    std::filesystem::path filePath)
{
    auto const buffer = PngTools::EncodeImage(image);
    FileSystem::SaveBinaryFile(buffer, filePath);
}

picojson::value GameAssetManager::LoadJson(std::filesystem::path const & filePath)
{
    return Utils::ParseJSONString(FileSystem::LoadTextFile(filePath));
}

void GameAssetManager::SaveJson(
    picojson::value const & json,
    std::filesystem::path const & filePath)
{
    std::string serializedJson = json.serialize(true);
    FileSystem::SaveTextFile(serializedJson, filePath);
}
