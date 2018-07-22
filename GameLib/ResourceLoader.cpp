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

        std::filesystem::path absoluteStructuralImageFilePath = std::filesystem::absolute(
            sdf.StructuralImageFilePath,
            basePath);

        std::optional<ImageData> textureImage;
        if (!!sdf.TextureImageFilePath)
        {
            std::filesystem::path absoluteTextureImageFilePath = std::filesystem::absolute(
                *sdf.TextureImageFilePath,
                basePath);

            textureImage.emplace(std::move(LoadTextureRgba(absoluteTextureImageFilePath)));
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

ImageData ResourceLoader::LoadTextureRgb(std::string const & name)
{
    return LoadImage(
        (std::filesystem::path("Data") / "Textures" / name).string(),
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT);
}

ImageData ResourceLoader::LoadTextureRgb(std::filesystem::path const & filePath)
{
    return LoadImage(
        filePath.string(),
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT);
}

ImageData ResourceLoader::LoadTextureRgba(std::string const & name)
{
    return LoadImage(
        (std::filesystem::path("Data") / "Textures" / name).string(),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT);
}

ImageData ResourceLoader::LoadTextureRgba(std::filesystem::path const & filePath)
{
    return LoadImage(
        filePath.string(),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT);
}

std::vector<ImageData> ResourceLoader::LoadTexturesRgba(
    std::string const & prefix,
    ProgressCallback progressCallback)
{
    std::regex textureFilenameRegex(R"((.+)_(\d+)\.png)");
    std::smatch match;

    std::vector<std::pair<int, std::filesystem::path>> matchingEntries;
    for (auto const & entryIt : std::filesystem::directory_iterator(std::filesystem::path("Data") / "Textures"))
    {
        std::string filename = entryIt.path().filename().string();

        if (std::filesystem::is_regular_file(entryIt.path())
            && std::regex_match(filename, match, textureFilenameRegex))
        {
            assert(match.size() == 1 + 2 && match[1].matched && match[2].matched);

            if (match[1].str() == prefix)
            {
                int textureIndex = std::stoi(match[2].str());

                matchingEntries.emplace_back(
                    textureIndex,
                    entryIt.path());
            }
        }
    }

    std::sort(
        matchingEntries.begin(),
        matchingEntries.end(),
        [](auto const & entry1, auto const & entry2)
        {
            return entry1.first < entry2.first;
        });

    std::vector<ImageData> textures;
    for (size_t i = 0; i < matchingEntries.size(); ++i)
    {
        if (progressCallback)
            progressCallback(static_cast<float>(i + 1) / static_cast<float>(matchingEntries.size()), "Loading texture...");

        auto imageData = LoadImage(
            matchingEntries[i].second,
            IL_RGBA,
            IL_ORIGIN_LOWER_LEFT);

        textures.emplace_back(std::move(imageData));
    }

    return textures;
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
// Images
////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
