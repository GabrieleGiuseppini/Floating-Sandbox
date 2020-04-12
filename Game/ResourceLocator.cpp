/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ResourceLocator.h"

#include <GameCore/Utils.h>

#include <regex>

ResourceLocator::ResourceLocator()
{
    // Nothing special, for now.
    // We'll be busy though when Resource Packs are implemented.
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ships
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetInstalledShipFolderPath() const
{
    return std::filesystem::canonical(std::filesystem::path("Ships"));
}

std::filesystem::path ResourceLocator::GetDefaultShipDefinitionFilePath() const
{
    std::filesystem::path defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.shp";
    if (!std::filesystem::exists(defaultShipDefinitionFilePath))
    {
        defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.png";
    }

    return defaultShipDefinitionFilePath;
}

std::filesystem::path ResourceLocator::GetFallbackShipDefinitionFilePath() const
{
    return std::filesystem::path("Data") / "Misc" / "fallback_ship.png";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Textures
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetTexturesRootFolderPath() const
{
    return std::filesystem::path("Data") / "Textures";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Fonts
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::filesystem::path> ResourceLocator::GetFontPaths() const
{
    std::vector<std::filesystem::path> filepaths;

    for (auto const & entryIt : std::filesystem::directory_iterator(std::filesystem::path("Data") / "Fonts"))
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

std::filesystem::path ResourceLocator::GetMaterialDatabaseRootFilepath() const
{
    return std::filesystem::path("Data");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Music
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLocator::GetMusicNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(std::filesystem::path("Data") / "Music"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path ResourceLocator::GetMusicFilepath(std::string const & musicName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Music" / (musicName + ".ogg");
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLocator::GetSoundNames() const
{
    std::vector<std::string> filenames;
    for (auto const & entryIt : std::filesystem::directory_iterator(std::filesystem::path("Data") / "Sounds"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            filenames.push_back(entryIt.path().stem().string());
        }
    }

    return filenames;
}

std::filesystem::path ResourceLocator::GetSoundFilepath(std::string const & soundName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Sounds" / (soundName + ".flac");
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Resources
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetCursorFilepath(std::string const & cursorName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (cursorName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLocator::GetIconFilepath(std::string const & iconName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (iconName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLocator::GetArtFilepath(std::string const & artName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (artName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLocator::GetBitmapFilepath(std::string const & bitmapName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (bitmapName + ".png");
    return std::filesystem::absolute(localPath);
}

std::vector<std::filesystem::path> ResourceLocator::GetBitmapFilepaths(std::string const & bitmapNamePattern) const
{
    std::filesystem::path const directoryPath = std::filesystem::path("Data") / "Resources";

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

std::filesystem::path ResourceLocator::GetThemeSettingsRootFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Themes" / "Settings";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetDefaultOceanFloorTerrainFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Misc" / "default_ocean_floor_terrain.png";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Help
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetStartupTipFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Help" / "startup_tip.html";
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLocator::GetHelpFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Help" / "index.html";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLocator::GetRenderShadersRootPath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Shaders" / "Render";
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLocator::GetGPUCalcShadersRootPath()
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Shaders" / "GPUCalc";
    return std::filesystem::absolute(localPath);
}