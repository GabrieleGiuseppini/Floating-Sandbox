/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ResourceLoader.h"

ResourceLoader::ResourceLoader()
{
    // Nothing special, for now.
    // We'll be busy though when Resource Packs are implemented.
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ships
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetInstalledShipFolderPath()
{
    return std::filesystem::canonical(std::filesystem::path("Ships"));
}

std::filesystem::path ResourceLoader::GetDefaultShipDefinitionFilePath() const
{
    std::filesystem::path defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.shp";
    if (!std::filesystem::exists(defaultShipDefinitionFilePath))
    {
        defaultShipDefinitionFilePath = GetInstalledShipFolderPath() / "default_ship.png";
    }

    return defaultShipDefinitionFilePath;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Textures
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetTexturesFilePath() const
{
    return std::filesystem::path("Data") / "Textures";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Fonts
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::filesystem::path> ResourceLoader::GetFontPaths() const
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

std::filesystem::path ResourceLoader::GetMaterialDatabaseRootFilepath() const
{
    return std::filesystem::path("Data");
}

////////////////////////////////////////////////////////////////////////////////////////////
// Music
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLoader::GetMusicNames() const
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

std::filesystem::path ResourceLoader::GetMusicFilepath(std::string const & musicName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Music" / (musicName + ".flac");
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ResourceLoader::GetSoundNames() const
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

std::filesystem::path ResourceLoader::GetSoundFilepath(std::string const & soundName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Sounds" / (soundName + ".flac");
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Resources
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetCursorFilepath(std::string const & cursorName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (cursorName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLoader::GetIconFilepath(std::string const & iconName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (iconName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLoader::GetArtFilepath(std::string const & artName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (artName + ".png");
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLoader::GetBitmapFilepath(std::string const & bitmapName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (bitmapName + ".png");
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Theme Settings
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetThemeSettingsRootFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Themes" / "Settings";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetDefaultOceanFloorTerrainFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Misc" / "default_ocean_floor_terrain.png";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Help
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetStartupTipFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Help" / "startup_tip.html";
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLoader::GetHelpFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Help" / "index.html";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetRenderShadersRootPath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Shaders" / "Render";
    return std::filesystem::absolute(localPath);
}

std::filesystem::path ResourceLoader::GetGPUCalcShadersRootPath()
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Shaders" / "GPUCalc";
    return std::filesystem::absolute(localPath);
}