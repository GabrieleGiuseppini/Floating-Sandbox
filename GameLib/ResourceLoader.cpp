/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ResourceLoader.h"

#include "GameException.h"
#include "Log.h"
#include "ShipDefinitionFile.h"
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

ShipDefinition ResourceLoader::LoadShipDefinition(std::filesystem::path const & filepath)
{    
    if (ShipDefinitionFile::IsShipDefinitionFile(filepath))
    {
        // 
        // Load full definition
        //

        picojson::value root = Utils::ParseJSONFile(filepath.string());
        if (!root.is<picojson::object>())
        {
            throw GameException("File \"" + filepath.string() + "\" does not contain a JSON object");
        }

        ShipDefinitionFile sdf = ShipDefinitionFile::Create(root.get<picojson::object>());

        //
        // Make paths absolute
        //

        std::filesystem::path basePath = filepath.parent_path();

        std::filesystem::path absoluteStructuralImageFilePath =
            basePath / std::filesystem::path(sdf.StructuralImageFilePath);

        std::optional<ImageData> textureImage;
        if (!!sdf.TextureImageFilePath)
        {
            std::filesystem::path absoluteTextureImageFilePath = 
                basePath / std::filesystem::path(*sdf.TextureImageFilePath);

            textureImage.emplace(std::move(
                LoadImage(
                    absoluteTextureImageFilePath.string(),
                    IL_RGBA,
                    IL_ORIGIN_LOWER_LEFT)));
        }


        //
        // Load
        //

        return ShipDefinition(
            LoadImage(absoluteStructuralImageFilePath.string(), IL_RGB, IL_ORIGIN_UPPER_LEFT),
            std::move(textureImage),
            sdf.ShipName.empty() 
                ? std ::filesystem::path(filepath).stem().string() 
                : sdf.ShipName,
            sdf.Offset);
    }
    else
    {
        //
        // Assume it's just a structural image
        //
        
        auto imageData = LoadImage(filepath, IL_RGB, IL_ORIGIN_UPPER_LEFT);

        return ShipDefinition(
            std::move(imageData),
            std::nullopt,
            std::filesystem::path(filepath).stem().string(),
            vec2f(0.0f, 0.0f));
    }
}

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

Render::TextureDatabase ResourceLoader::LoadTextureDatabase(ProgressCallback const & progressCallback) const
{
    return Render::TextureDatabase::Load(
        std::filesystem::path("Data") / "Textures",
        progressCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////
// Fonts
////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Render::Font> ResourceLoader::LoadFonts(ProgressCallback const & progressCallback) const
{
    //
    // Get all font file paths
    //

    std::vector<std::filesystem::path> filepaths;

    for (auto const & entryIt : std::filesystem::directory_iterator(std::filesystem::path("Data") / "Fonts"))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() == ".bff")
        {
            filepaths.push_back(entryIt.path());
        }
    }


    //
    // Sort paths
    //

    std::sort(
        filepaths.begin(),
        filepaths.end(),
        [](auto const & fp1, auto const & fp2)
        {
            return fp1 < fp2;
        });


    //
    // Load fonts
    //

    std::vector<Render::Font> fonts;

    for (auto const & filepath : filepaths)
    {
        fonts.emplace_back(
            std::move(
                Render::Font::Load(filepath)));

        progressCallback(
            static_cast<float>(fonts.size()) / static_cast<float>(filepaths.size()),
            "Loading fonts...");
    }

    return fonts;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Materials
////////////////////////////////////////////////////////////////////////////////////////////

MaterialDatabase ResourceLoader::LoadMaterials()
{
    return LoadMaterials(std::filesystem::path("Data") / "materials.json");
}

MaterialDatabase ResourceLoader::LoadMaterials(std::filesystem::path const & filePath)
{
    picojson::value root = Utils::ParseJSONFile(filePath.string());
    return MaterialDatabase::Create(root);
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

std::filesystem::path ResourceLoader::GetArtFilepath(std::string const & artName) const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Resources" / (artName + ".png");
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

std::filesystem::path ResourceLoader::GetShadersRootPath() const
{
    std::filesystem::path localPath = std::filesystem::path("Data") / "Shaders";
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

ImageData ResourceLoader::LoadImageRgbaUpperLeft(std::filesystem::path const & filepath)
{
    return LoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_UPPER_LEFT);
}

ImageData ResourceLoader::LoadImageRgbaLowerLeft(std::filesystem::path const & filepath)
{
    return LoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT);
}

ImageData ResourceLoader::LoadImageRgbUpperLeft(std::filesystem::path const & filepath)
{
    return LoadImage(
        filepath,
        IL_RGB,
        IL_ORIGIN_UPPER_LEFT);
}

ImageData ResourceLoader::LoadImageRgbLowerLeft(std::filesystem::path const & filepath)
{
    return LoadImage(
        filepath,
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT);
}

ImageData ResourceLoader::LoadImage(
    std::filesystem::path const & filepath,
    int targetFormat,
    int targetOrigin)
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
    // Create data
    //

    ILubyte const * imageData = ilGetData();
    int const width = ilGetInteger(IL_IMAGE_WIDTH);
    int const height = ilGetInteger(IL_IMAGE_HEIGHT);
    int const bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

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
