/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>
#include <string>
#include <vector>

class ResourceLoader
{
public:

    ResourceLoader();

public:

    //
    // Ships
    //

    static std::filesystem::path GetInstalledShipFolderPath();

    std::filesystem::path GetDefaultShipDefinitionFilePath() const;


    //
    // Textures
    //

    std::filesystem::path GetTexturesRootFolderPath() const;


    //
    // Fonts
    //

    std::vector<std::filesystem::path> GetFontPaths() const;


    //
    // Materials
    //

    std::filesystem::path GetMaterialDatabaseRootFilepath() const;


    //
    // Music
    //

    std::vector<std::string> GetMusicNames() const;

    std::filesystem::path GetMusicFilepath(std::string const & musicName) const;


    //
    // Sounds
    //

    std::vector<std::string> GetSoundNames() const;

    std::filesystem::path GetSoundFilepath(std::string const & soundName) const;


    //
    // Resources
    //

    std::filesystem::path GetCursorFilepath(std::string const & cursorName) const;

    std::filesystem::path GetIconFilepath(std::string const & iconName) const;

    std::filesystem::path GetArtFilepath(std::string const & artName) const;

    std::filesystem::path GetBitmapFilepath(std::string const & iconName) const;


    //
    // Theme Settings
    //

    std::filesystem::path GetThemeSettingsRootFilepath() const;


    //
    // Misc
    //

    std::filesystem::path GetDefaultOceanFloorTerrainFilepath() const;


    //
    // Help
    //

    std::filesystem::path GetStartupTipFilepath() const;

    std::filesystem::path GetHelpFilepath() const;


    //
    // Shaders
    //

    std::filesystem::path GetRenderShadersRootPath() const;

    static std::filesystem::path GetGPUCalcShadersRootPath();
};
