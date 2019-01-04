/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"
#include "ProgressCallback.h"
#include "SysSpecifics.h"

#include <cstdint>
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

    std::filesystem::path GetDefaultShipDefinitionFilePath() const;


    //
    // Textures
    //

    std::filesystem::path GetTexturesFilePath() const;


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


    //
    // Misc
    //

    std::filesystem::path GetOceanFloorBumpMapFilepath() const;


    //
    // Help
    //

    std::filesystem::path GetHelpFilepath() const;


    //
    // Shaders
    //

    std::filesystem::path GetShadersRootPath() const;


    //
    // Images
    //

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    static ImageData LoadImageRgbaUpperLeft(std::filesystem::path const & filepath, int resize = 1);
    static ImageData LoadImageRgbaLowerLeft(std::filesystem::path const & filepath, int resize = 1);
    static ImageData LoadImageRgbUpperLeft(std::filesystem::path const & filepath, int resize = 1);
    static ImageData LoadImageRgbLowerLeft(std::filesystem::path const & filepath, int resize = 1);

    static void SaveImage(
        std::filesystem::path filepath,
        ImageData const & image);

private:

    static ImageData LoadImage(
        std::filesystem::path const & filepath,
        int targetFormat,
        int targetOrigin,
        int resize);
};
