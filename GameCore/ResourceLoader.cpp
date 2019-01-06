/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ResourceLoader.h"

#include "GameException.h"
#include "Log.h"
#include "Utils.h"

#include <IL/il.h>
#include <IL/ilu.h>

#include <algorithm>
#include <cstring>
#include <regex>

ResourceLoader::ResourceLoader()
{
    // Initialize DevIL
    ilInit();
    iluInit();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Ships
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetDefaultShipDefinitionFilePath() const
{
    std::filesystem::path defaultShipDefinitionFilePath = std::filesystem::path("Ships") / "default_ship.shp";
    if (!std::filesystem::exists(defaultShipDefinitionFilePath))
    {
        defaultShipDefinitionFilePath = std::filesystem::path("Ships") / "default_ship.png";
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
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() == ".flac")
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

////////////////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////////////////

std::filesystem::path ResourceLoader::GetOceanFloorBumpMapFilepath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Misc" / "ocean_floor_bumpmap.png";
    return std::filesystem::absolute(localPath);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Help
////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////
// Images
////////////////////////////////////////////////////////////////////////////////////////////

ImageSize ResourceLoader::GetImageSize(std::filesystem::path const & filepath)
{
    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    //
    // Load image
    //

    std::string filepathStr = filepath.string();
    ILconst_string ilFilename(filepathStr.c_str());
    if (!ilLoadImage(ilFilename))
    {
        ILint devilError = ilGetError();
        std::string devilErrorMessage(iluErrorString(devilError));
        throw GameException("Could not load image \"" + filepathStr + "\": " + devilErrorMessage);
    }

    //
    // Get size
    //

    int const width = ilGetInteger(IL_IMAGE_WIDTH);
    int const height = ilGetInteger(IL_IMAGE_HEIGHT);


    //
    // Delete image
    //

    ilDeleteImage(imghandle);

    return ImageSize(width, height);
}

ImageData ResourceLoader::LoadImageRgbaUpperLeft(
    std::filesystem::path const & filepath,
    int resize)
{
    return LoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_UPPER_LEFT,
        resize);
}

ImageData ResourceLoader::LoadImageRgbaLowerLeft(
    std::filesystem::path const & filepath,
    int resize)
{
    return LoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        resize);
}

ImageData ResourceLoader::LoadImageRgbUpperLeft(
    std::filesystem::path const & filepath,
    int resize)
{
    return LoadImage(
        filepath,
        IL_RGB,
        IL_ORIGIN_UPPER_LEFT,
        resize);
}

ImageData ResourceLoader::LoadImageRgbLowerLeft(
    std::filesystem::path const & filepath,
    int resize)
{
    return LoadImage(
        filepath,
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT,
        resize);
}

ImageData ResourceLoader::LoadImage(
    std::filesystem::path const & filepath,
    int targetFormat,
    int targetOrigin,
    int resize)
{
    //
    // Load image
    //

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    std::string filepathStr = filepath.string();
    ILconst_string ilFilename(filepathStr.c_str());
    if (!ilLoadImage(ilFilename))
    {
        ILint devilError = ilGetError();
        std::string devilErrorMessage(iluErrorString(devilError));
        throw GameException("Could not load image \"" + filepathStr + "\": " + devilErrorMessage);
    }

    //
    // Check if we need to convert it
    //

    int imageFormat = ilGetInteger(IL_IMAGE_FORMAT);
    int imageType = ilGetInteger(IL_IMAGE_TYPE);
    if (targetFormat != imageFormat || IL_UNSIGNED_BYTE != imageType)
    {
        if (!ilConvertImage(targetFormat, IL_UNSIGNED_BYTE))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw GameException("Could not convert image \"" + filepathStr + "\": " + devilErrorMessage);
        }
    }

    int imageOrigin = ilGetInteger(IL_IMAGE_ORIGIN);
    if (targetOrigin != imageOrigin)
    {
        iluFlipImage();
    }


    //
    // Get metadata
    //

    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    int const depth = ilGetInteger(IL_IMAGE_DEPTH);
    int const bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);



    //
    // Resize it
    //

    if (resize != 1)
    {
        //
        // Resize with nearest
        //

        iluImageParameter(ILU_FILTER, ILU_NEAREST);

        if (!iluScale(width * resize, height * resize, depth))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw GameException("Could not resize image: " + devilErrorMessage);
        }

        width *= resize;
        height *= resize;
    }


    //
    // Create data
    //

    ILubyte const * imageData = ilGetData();
    auto data = std::make_unique<unsigned char []>(width * height * bpp);
    std::memcpy(data.get(), imageData, width * height * bpp);


    //
    // Delete image
    //

    ilDeleteImage(imghandle);

    return ImageData(
        width,
        height,
        std::move(data));
}

void ResourceLoader::SaveImage(
    std::filesystem::path filepath,
    ImageData const & image)
{
    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    ilTexImage(
        image.Size.Width,
        image.Size.Height,
        1,
        4,
        IL_RGBA,
        IL_UNSIGNED_BYTE,
        reinterpret_cast<void *>(const_cast<unsigned char *>(image.Data.get())));

    ilEnable(IL_FILE_OVERWRITE);
    ilSave(IL_PNG, filepath.string().c_str());

    ilDeleteImage(imghandle);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////