/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageFileTools.h"

#include <GameCore/GameException.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <algorithm>
#include <cstring>
#include <regex>

bool ImageFileTools::mIsInitialized = false;

ImageSize ImageFileTools::GetImageSize(std::filesystem::path const & filepath)
{
    CheckInitialized();

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

ImageData ImageFileTools::LoadImageRgbaLowerLeft(std::filesystem::path const & filepath)
{
    return InternalLoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        std::nullopt);
}

ImageData ImageFileTools::LoadImageRgbaLowerLeftAndMagnify(
    std::filesystem::path const & filepath,
    int magnificationFactor)
{
    auto imageSize = GetImageSize(filepath);

    return InternalLoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        ResizeInfo(
            imageSize.Width * magnificationFactor,
            ILU_NEAREST));
}

ImageData ImageFileTools::LoadImageRgbaLowerLeftAndResize(
    std::filesystem::path const & filepath,
    int resizedWidth)
{
    return InternalLoadImage(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        ResizeInfo(
            resizedWidth,
            ILU_BILINEAR));
}

ImageData ImageFileTools::LoadImageRgbUpperLeft(std::filesystem::path const & filepath)
{
    return InternalLoadImage(
        filepath,
        IL_RGB,
        IL_ORIGIN_UPPER_LEFT,
        std::nullopt);
}

void ImageFileTools::SaveImageRgb(
    std::filesystem::path filepath,
    ImageData const & image)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    ilTexImage(
        image.Size.Width,
        image.Size.Height,
        1,
        3,
        IL_RGB,
        IL_UNSIGNED_BYTE,
        reinterpret_cast<void *>(const_cast<unsigned char *>(image.Data.get())));

    ilEnable(IL_FILE_OVERWRITE);
    ilSave(IL_PNG, filepath.string().c_str());

    ilDeleteImage(imghandle);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void ImageFileTools::CheckInitialized()
{
    if (!mIsInitialized)
    {
        // Initialize DevIL
        ilInit();
        iluInit();

        mIsInitialized = true;
    }
}

ImageData ImageFileTools::InternalLoadImage(
    std::filesystem::path const & filepath,
    int targetFormat,
    int targetOrigin,
    std::optional<ResizeInfo> resizeInfo)
{
    CheckInitialized();

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

    if (!!resizeInfo)
    {
        iluImageParameter(ILU_FILTER, resizeInfo->FilterType);

        int resizedHeight = static_cast<int>(
            round(
                static_cast<float>(height)
                / static_cast<float>(width)
                * static_cast<float>(resizeInfo->ResizedWidth)));

        if (!iluScale(resizeInfo->ResizedWidth, resizedHeight, depth))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw GameException("Could not resize image: " + devilErrorMessage);
        }

        width = resizeInfo->ResizedWidth;
        height = resizedHeight;
    }


    //
    // Create data
    //

    ILubyte const * imageData = ilGetData();
    auto data = std::make_unique<unsigned char[]>(width * height * bpp);
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