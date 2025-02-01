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
	, mTextureRoot(mDataRoot / "Textures")
    , mShaderRoot(mDataRoot / "Shaders")
{
}

GameAssetManager::GameAssetManager(std::filesystem::path const & textureRoot)
	: mDataRoot(textureRoot) // Just because (not needed in this scenario)
	, mTextureRoot(textureRoot)
{
}

picojson::value GameAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName)
{
    return LoadJson(mTextureRoot / databaseName / "database.json");
}

ImageSize GameAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath)
{
    return GetImageSize(mTextureRoot / databaseName / frameRelativePath);
}

RgbaImageData GameAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath)
{
    return LoadPngImageRgba(mTextureRoot / databaseName / frameRelativePath);
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName)
{
    std::vector<AssetDescriptor> frameDescriptors;

    std::filesystem::path const databaseRootPath = mTextureRoot / databaseName;
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
    return LoadJson(mTextureRoot / "Atlases" / MakeAtlasSpecificationFilename(textureDatabaseName));
}

RgbaImageData GameAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName)
{
    return LoadPngImageRgba(mTextureRoot / "Atlases" / MakeAtlasImageFilename(textureDatabaseName));
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateShaders(std::string const & shaderSetName)
{
    std::vector<AssetDescriptor> shaderDescriptors;

    std::filesystem::path const shaderSetRootPath = mShaderRoot / shaderSetName;
    for (auto const & entryIt : std::filesystem::directory_iterator(shaderSetRootPath))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            if (entryIt.path().extension() == ".glsl" || entryIt.path().extension() == ".glslinc")
            {
                std::string shaderFilename = entryIt.path().filename().string();
                shaderDescriptors.push_back(
                    AssetDescriptor{
                        entryIt.path().filename().string(),
                        std::filesystem::relative(entryIt.path(), shaderSetRootPath).string()
                    });
            }
            else
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading shaders");
            }
        }
    }

    return shaderDescriptors;
}

std::string GameAssetManager::LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath)
{
    return FileSystem::LoadTextFile(mShaderRoot / shaderSetName / shaderRelativePath);
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateFonts(std::string const & fontSetName)
{
    std::vector<AssetDescriptor> fontDescriptors;

    std::filesystem::path const fontSetRootPath = mDataRoot / "Fonts" / fontSetName;
    for (auto const & entryIt : std::filesystem::recursive_directory_iterator(fontSetRootPath))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            auto const fontPath = entryIt.path();
            fontDescriptors.push_back(
                AssetDescriptor{
                    fontPath.filename().stem().string(),
                    std::filesystem::relative(fontPath, fontSetRootPath).string()
                });
        }
    }

    return fontDescriptors;
}

Buffer<std::uint8_t> GameAssetManager::LoadFont(std::string const & fontSetName, std::string const & fontRelativePath)
{
    return FileSystem::LoadBinaryFile(mDataRoot / "Fonts" / fontSetName / fontRelativePath);
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
