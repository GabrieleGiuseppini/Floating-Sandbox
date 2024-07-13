/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ResourceLocator.h"

#include <GameCore/Log.h>
#include <GameCore/Utils.h>

#include <regex>

ResourceLocator::ResourceLocator(std::string const & argv0)
    : ResourceLocator(std::filesystem::canonical(std::filesystem::path(argv0)).parent_path())
{
    LogMessage("ResourceLocator: argv0=", argv0, " rootPath=", mRootPath,
        " currentPath=", std::filesystem::current_path());
}

ResourceLocator::ResourceLocator(std::filesystem::path const & rootProgramPath)
    : mRootPath(rootProgramPath)
{
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ships
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetInstalledShipFolderPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Ships"));
}

std::filesystem::path ResourceLocator::GetDefaultShipDefinitionFilePath() const
{
    std::filesystem::path defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.shp2";
    if (!std::filesystem::exists(defaultShipDefinitionFilePath))
    {
        defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.png";
    }

    return defaultShipDefinitionFilePath;
}

std::filesystem::path ResourceLocator::GetFallbackShipDefinitionFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Built-in Ships" / "fallback_ship.png";
}

std::filesystem::path ResourceLocator::GetApril1stShipDefinitionFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Built-in Ships" / "Floating Sandbox Logo.shp";
}

std::filesystem::path ResourceLocator::GetHolidaysShipDefinitionFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Built-in Ships" / "R.M.S. Titanic (on Holidays).shp";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Textures
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetTexturesRootFolderPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Textures";
}

std::filesystem::path ResourceLocator::GetMaterialTextureFilePath(std::string const & materialTextureName) const
{
    return GetTexturesRootFolderPath() / "Material" / (materialTextureName + ".png");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Fonts
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::filesystem::path> ResourceLocator::GetFontPaths() const
{
    std::vector<std::filesystem::path> filepaths;

    for (auto const & entryIt : std::filesystem::directory_iterator(MakeAbsolutePath(std::filesystem::path("Data")) / "Fonts"))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() == ".bff")
        {
            filepaths.push_back(entryIt.path());
        }
    }

    return filepaths;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Materials
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetMaterialDatabaseRootFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data"));
}

////////////////////////////////////////////////////////////////////////////////////////////
// Music
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLocator::GetMusicNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(MakeAbsolutePath(std::filesystem::path("Data")) / "Music"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path ResourceLocator::GetMusicFilePath(std::string const & musicName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Music" / (musicName + ".ogg");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLocator::GetSoundNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(MakeAbsolutePath(std::filesystem::path("Data")) / "Sounds"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path ResourceLocator::GetSoundFilePath(std::string const & soundName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Sounds" / (soundName + ".flac");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Resources
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetCursorFilePath(std::string const & cursorName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Resources" / (cursorName + ".png");
}

std::filesystem::path ResourceLocator::GetIconFilePath(std::string const & iconName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Resources" / (iconName + ".png");
}

std::filesystem::path ResourceLocator::GetArtFilePath(std::string const & artName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Resources" / (artName + ".png");
}

std::filesystem::path ResourceLocator::GetBitmapFilePath(std::string const & bitmapName) const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Resources" / (bitmapName + ".png");
}

std::vector<std::filesystem::path> ResourceLocator::GetBitmapFilePaths(std::string const & bitmapNamePattern) const
{
    std::filesystem::path const directoryPath = MakeAbsolutePath(std::filesystem::path("Data")) / "Resources";

    std::regex const searchRe = Utils::MakeFilenameMatchRegex(bitmapNamePattern);

    std::vector<std::filesystem::path> filepaths;
    for (auto const & entryIt : std::filesystem::directory_iterator(directoryPath))
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

std::filesystem::path ResourceLocator::GetThemeSettingsRootFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Themes" / "Settings";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetDefaultOceanFloorTerrainFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Misc" / "default_ocean_floor_terrain.png";
}

std::filesystem::path ResourceLocator::GetFishSpeciesDatabaseFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Misc" / "fish_species.json";
}

std::filesystem::path ResourceLocator::GetNpcDatabaseFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Misc" / "npcs.json";
}

std::filesystem::path ResourceLocator::GetShipNamePrefixListFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Misc" / "ship_name_prefixes.txt";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Help
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetStartupTipFilePath(
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

std::filesystem::path ResourceLocator::GetHelpFilePath(
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
// Shaders
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetGameShadersRootPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Shaders" / "Game";
}

std::filesystem::path ResourceLocator::GetShipBuilderShadersRootPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Shaders" / "ShipBuilder";
}

std::filesystem::path ResourceLocator::GetGPUCalcShadersRootPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Shaders" / "GPUCalc";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Localization
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetLanguagesRootPath() const
{
    return MakeAbsolutePath(std::filesystem::path("Data")) / "Languages";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Boot settings
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetBootSettingsFilePath() const
{
    return MakeAbsolutePath(std::filesystem::path("boot_settings.json"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::MakeAbsolutePath(std::filesystem::path const & relativePath) const
{
    return mRootPath / relativePath;
}