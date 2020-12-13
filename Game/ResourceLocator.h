/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>
#include <string>
#include <vector>

class ResourceLocator
{
public:

    ResourceLocator();

public:

    //
    // Ships
    //

    std::filesystem::path GetInstalledShipFolderPath() const;

    std::filesystem::path GetDefaultShipDefinitionFilePath() const;

    std::filesystem::path GetFallbackShipDefinitionFilePath() const;

    std::filesystem::path GetApril1stShipDefinitionFilePath() const;

    std::filesystem::path GetHolidaysShipDefinitionFilePath() const;


    //
    // Textures
    //

    std::filesystem::path GetTexturesRootFolderPath() const;

    std::filesystem::path GetMaterialTexturesFolderPath() const;


    //
    // Fonts
    //

    std::vector<std::filesystem::path> GetFontPaths() const;


    //
    // Materials
    //

    std::filesystem::path GetMaterialDatabaseRootFilePath() const;


    //
    // Music
    //

    std::vector<std::string> GetMusicNames() const;

    std::filesystem::path GetMusicFilePath(std::string const & musicName) const;


    //
    // Sounds
    //

    std::vector<std::string> GetSoundNames() const;

    std::filesystem::path GetSoundFilePath(std::string const & soundName) const;


    //
    // Resources
    //

    std::filesystem::path GetCursorFilePath(std::string const & cursorName) const;

    std::filesystem::path GetIconFilePath(std::string const & iconName) const;

    std::filesystem::path GetArtFilePath(std::string const & artName) const;

    std::filesystem::path GetBitmapFilePath(std::string const & bitmapName) const;

    std::vector<std::filesystem::path> GetBitmapFilePaths(std::string const & bitmapNamePattern) const;


    //
    // Theme Settings
    //

    std::filesystem::path GetThemeSettingsRootFilePath() const;


    //
    // Misc
    //

    std::filesystem::path GetDefaultOceanFloorTerrainFilePath() const;

    std::filesystem::path GetFishSpeciesDatabaseFilePath() const;


    //
    // Help
    //

    static std::filesystem::path GetStartupTipFilePath(
        std::string const & desiredLanguageIdentifier,
        std::string const & defaultLanguageIdentifier);

    static std::filesystem::path GetHelpFilePath(
        std::string const & desiredLanguageIdentifier,
        std::string const & defaultLanguageIdentifier);


    //
    // Shaders
    //

    std::filesystem::path GetRenderShadersRootPath() const;

    static std::filesystem::path GetGPUCalcShadersRootPath();


    //
    // Localization
    //

    static std::filesystem::path GetLanguagesRootPath();
};
