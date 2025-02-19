/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-01-27
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "GameAssetManager.h"

#include "FileStreams.h"
#include "FileSystem.h"

#include <Core/GameException.h>
#include <Core/Log.h>
#include <Core/PngTools.h>
#include <Core/Streams.h>
#include <Core/Utils.h>

#include <regex>

 ////////////////////////////////////////////////////////////////////////////////////////////
 // IAssetManager
 ////////////////////////////////////////////////////////////////////////////////////////////

GameAssetManager::GameAssetManager(std::string const && argv0)
	: mGameRoot(std::filesystem::canonical(std::filesystem::path(argv0)).parent_path())
    , mDataRoot(mGameRoot / "Data")
    , mResourcesRoot(mDataRoot / "Resources")
	, mTextureRoot(mDataRoot / "Textures")
    , mShaderRoot(mDataRoot / "Shaders")
{
}

GameAssetManager::GameAssetManager(std::filesystem::path const & textureRoot)
	: mDataRoot(textureRoot) // Just because (not needed in this scenario)
	, mTextureRoot(textureRoot)
{
}

picojson::value GameAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName) const
{
    return LoadJson(mTextureRoot / databaseName / "database.json");
}

ImageSize GameAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const
{
    return GetImageSize(mTextureRoot / databaseName / frameRelativePath);
}

RgbaImageData GameAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const
{
    return LoadPngImageRgba(mTextureRoot / databaseName / frameRelativePath);
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName) const
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
                        framePath.filename().string(),
                        std::filesystem::relative(framePath, databaseRootPath).string()
                    });
            }
            else if (entryIt.path().extension().string() != ".txt") // txt allowed
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading texture database \"" + databaseName + "\"");
            }
        }
    }

    return frameDescriptors;
}

std::string GameAssetManager::GetMaterialTextureRelativePath(std::string const & materialTextureName) const
{
    std::filesystem::path const materialTexturesRootPath = MakeMaterialTexturesRootPath();
    std::filesystem::path const fullPath = materialTexturesRootPath / (materialTextureName + ".png");

    // Make sure file exists
    if (!std::filesystem::exists(fullPath)
        || !std::filesystem::is_regular_file(fullPath))
    {
        throw GameException(
            "Cannot find material texture file for texture name \"" + materialTextureName + "\"");
    }

    return std::filesystem::relative(fullPath, materialTexturesRootPath).string();
}

RgbImageData GameAssetManager::LoadMaterialTexture(std::string const & frameRelativePath) const
{
    return LoadPngImageRgb(MakeMaterialTexturesRootPath() / frameRelativePath);
}

picojson::value GameAssetManager::LoadTetureAtlasSpecification(std::string const & textureDatabaseName) const
{
    return LoadJson(mTextureRoot / "Atlases" / MakeAtlasSpecificationFilename(textureDatabaseName));
}

RgbaImageData GameAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) const
{
    return LoadPngImageRgba(mTextureRoot / "Atlases" / MakeAtlasImageFilename(textureDatabaseName));
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateShaders(std::string const & shaderSetName) const
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
                        entryIt.path().filename().stem().string(),
                        entryIt.path().filename().string(),
                        std::filesystem::relative(entryIt.path(), shaderSetRootPath).string()
                    });
            }
            else
            {
                LogMessage("WARNING: found file \"" + entryIt.path().string() + "\" with unexpected extension while loading shader set \"" + shaderSetName + "\"");
            }
        }
    }

    return shaderDescriptors;
}

std::string GameAssetManager::LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath) const
{
    return FileTextReadStream(mShaderRoot / shaderSetName / shaderRelativePath).ReadAll();
}

std::vector<IAssetManager::AssetDescriptor> GameAssetManager::EnumerateFonts(std::string const & fontSetName) const
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
                    fontPath.filename().string(),
                    std::filesystem::relative(fontPath, fontSetRootPath).string()
                });
        }
    }

    return fontDescriptors;
}

std::unique_ptr<BinaryReadStream> GameAssetManager::LoadFont(std::string const & fontSetName, std::string const & fontRelativePath) const
{
    return std::make_unique<FileBinaryReadStream>(mDataRoot / "Fonts" / fontSetName / fontRelativePath);
}

picojson::value GameAssetManager::LoadStructuralMaterialDatabase() const
{
    return LoadJson(mDataRoot / "Misc" / "materials_structural.json");
}

picojson::value GameAssetManager::LoadElectricalMaterialDatabase() const
{
    return LoadJson(mDataRoot / "Misc" / "materials_electrical.json");
}

picojson::value GameAssetManager::LoadFishSpeciesDatabase() const
{
    return LoadJson(mDataRoot / "Misc" / "fish_species.json");
}

picojson::value GameAssetManager::LoadNpcDatabase() const
{
    return LoadJson(mDataRoot / "Misc" / "npcs.json");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ships
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetInstalledShipFolderPath() const
{
    return mGameRoot / "Ships";
}

std::filesystem::path GameAssetManager::GetDefaultShipDefinitionFilePath() const
{
    std::filesystem::path defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.shp2";
    if (!std::filesystem::exists(defaultShipDefinitionFilePath))
    {
        defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.png";
    }

    return defaultShipDefinitionFilePath;
}

std::filesystem::path GameAssetManager::GetFallbackShipDefinitionFilePath() const
{
    return mDataRoot / "Built-in Ships" / "fallback_ship.png";
}

std::filesystem::path GameAssetManager::GetApril1stShipDefinitionFilePath() const
{
    return mDataRoot / "Built-in Ships" / "Floating Sandbox Logo.shp";
}

std::filesystem::path GameAssetManager::GetHolidaysShipDefinitionFilePath() const
{
    return mDataRoot / "Built-in Ships" / "R.M.S. Titanic (on Holidays).shp";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Music
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> GameAssetManager::GetMusicNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(mDataRoot / "Music"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path GameAssetManager::GetMusicFilePath(std::string const & musicName) const
{
    return mDataRoot / "Music" / (musicName + ".ogg");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> GameAssetManager::GetSoundNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(mDataRoot / "Sounds"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path GameAssetManager::GetSoundFilePath(std::string const & soundName) const
{
    return mDataRoot / "Sounds" / (soundName + ".flac");
}

////////////////////////////////////////////////////////////////////////////////////////////
// UI Resources
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetCursorFilePath(std::string const & cursorName) const
{
    return mResourcesRoot / (cursorName + ".png");
}

std::filesystem::path GameAssetManager::GetIconFilePath(std::string const & iconName) const
{
    return mResourcesRoot / (iconName + ".png");
}

std::filesystem::path GameAssetManager::GetArtFilePath(std::string const & artName) const
{
    return mResourcesRoot / (artName + ".png");
}

std::filesystem::path GameAssetManager::GetPngImageFilePath(std::string const & pngImageName) const
{
    return mResourcesRoot / (pngImageName + ".png");
}

std::vector<std::filesystem::path> GameAssetManager::GetPngImageFilePaths(std::string const & ngImageNamePattern) const
{
    std::regex const searchRe = FileSystem::MakeFilenameMatchRegex(ngImageNamePattern);

    std::vector<std::filesystem::path> filepaths;
    for (auto const & entryIt : std::filesystem::directory_iterator(mResourcesRoot))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() == ".png"
            && std::regex_match(entryIt.path().stem().string(), searchRe))
        {
            filepaths.push_back(entryIt.path());
        }
    }

    return filepaths;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Theme Settings
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetThemeSettingsRootFilePath() const
{
    return mDataRoot / "Themes" / "Settings";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ship
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetDefaultOceanFloorHeightMapFilePath() const
{
    return mDataRoot / "Misc" / "default_ocean_floor_height_map.png";
}

std::filesystem::path GameAssetManager::GetShipNamePrefixListFilePath() const
{
    return mDataRoot / "Misc" / "ship_name_prefixes.txt";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Localization
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetLanguagesRootPath() const
{
    return mDataRoot / "Languages";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Boot settings
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetBootSettingsFilePath() const
{
    return mGameRoot / "boot_settings.json";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Help
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path GameAssetManager::GetStartupTipFilePath(
    std::string const & desiredLanguageIdentifier,
    std::string const & defaultLanguageIdentifier) const
{
    static std::filesystem::path const Filename = std::filesystem::path("startup_tip.html");

    std::filesystem::path localPath = GetLanguagesRootPath() / desiredLanguageIdentifier / Filename;

    if (!std::filesystem::exists(localPath))
    {
        LogMessage("WARNING: cannot find startup tip file for language \"", desiredLanguageIdentifier, "\"");

        localPath = GetLanguagesRootPath() / defaultLanguageIdentifier / Filename;
    }

    return localPath;
}

std::filesystem::path GameAssetManager::GetHelpFilePath(
    std::string const & desiredLanguageIdentifier,
    std::string const & defaultLanguageIdentifier) const
{
    static std::filesystem::path const Filename = std::filesystem::path("help.html");

    std::filesystem::path localPath = GetLanguagesRootPath() / desiredLanguageIdentifier / Filename;

    if (!std::filesystem::exists(localPath))
    {
        LogMessage("WARNING: cannot find help file for language \"", desiredLanguageIdentifier, "\"");

        localPath = GetLanguagesRootPath() / defaultLanguageIdentifier / Filename;
    }

    return localPath;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////////////////

bool GameAssetManager::Exists(std::filesystem::path const & filePath)
{
    return std::filesystem::exists(filePath);
}

ImageSize GameAssetManager::GetImageSize(std::filesystem::path const & filePath)
{
    auto readStream = FileBinaryReadStream(filePath);
    return PngTools::GetImageSize(readStream);
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
    auto readStream = FileBinaryReadStream(filePath);
    return PngTools::DecodeImageRgba(readStream);
}

RgbImageData GameAssetManager::LoadPngImageRgb(std::filesystem::path const & filePath)
{
    auto readStream = FileBinaryReadStream(filePath);
    return PngTools::DecodeImageRgb(readStream);
}

void GameAssetManager::SavePngImage(
    RgbaImageData const & image,
    std::filesystem::path filePath)
{
    auto writeStream = FileBinaryWriteStream(filePath);
    PngTools::EncodeImage(image, writeStream);
}

void GameAssetManager::SavePngImage(
    RgbImageData const & image,
    std::filesystem::path filePath)
{
    auto writeStream = FileBinaryWriteStream(filePath);
    PngTools::EncodeImage(image, writeStream);
}

picojson::value GameAssetManager::LoadJson(std::filesystem::path const & filePath)
{
    try
    {
        return Utils::ParseJSONString(FileTextReadStream(filePath).ReadAll());
    }
    catch (GameException const & ex)
    {
        throw GameException("Error loading \"" + filePath.filename().string() + "\": " + ex.what());
    }
}

void GameAssetManager::SaveJson(
    picojson::value const & json,
    std::filesystem::path const & filePath)
{
    SaveTextFile(json.serialize(true), filePath);
}

void GameAssetManager::SaveTextFile(
    std::string const & content,
    std::filesystem::path const & filePath)
{
    FileTextWriteStream(filePath).Write(content);
}