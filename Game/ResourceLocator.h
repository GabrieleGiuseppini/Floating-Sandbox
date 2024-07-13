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

    explicit ResourceLocator(std::string const & argv0);

    explicit ResourceLocator(std::filesystem::path const & rootProgramPath);

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

    std::filesystem::path GetMaterialTextureFilePath(std::string const & materialTextureName) const;


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

    std::filesystem::path GetNpcDatabaseFilePath() const;

    std::filesystem::path GetShipNamePrefixListFilePath() const;


    //
    // Help
    //

    std::filesystem::path GetStartupTipFilePath(
        std::string const & desiredLanguageIdentifier,
        std::string const & defaultLanguageIdentifier) const;

    std::filesystem::path GetHelpFilePath(
        std::string const & desiredLanguageIdentifier,
        std::string const & defaultLanguageIdentifier) const;


    //
    // Shaders
    //

    std::filesystem::path GetGameShadersRootPath() const;

    std::filesystem::path GetShipBuilderShadersRootPath() const;

    std::filesystem::path GetGPUCalcShadersRootPath() const;


    //
    // Localization
    //

    std::filesystem::path GetLanguagesRootPath() const;


    //
    // Boot settings
    //

    std::filesystem::path GetBootSettingsFilePath() const;

private:

    std::filesystem::path MakeAbsolutePath(std::filesystem::path const & relativePath) const;

    std::filesystem::path const mRootPath;
};
