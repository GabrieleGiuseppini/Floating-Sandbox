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
#include <cassert>
#include <cstring>
#include <regex>

bool ImageFileTools::mIsInitialized = false;

ImageSize ImageFileTools::GetImageSize(std::filesystem::path const & filepath)
{
    //
    // Load image
    //

    ILuint imageHandle = InternalOpenImage(filepath);

    //
    // Get size
    //

    int const width = ilGetInteger(IL_IMAGE_WIDTH);
    int const height = ilGetInteger(IL_IMAGE_HEIGHT);


    //
    // Delete image
    //

    ilDeleteImage(imageHandle);


    //
    // Check
    //

    if (width == 0 || height == 0)
    {
        throw GameException("Could not load image \"" + filepath.string() + "\": image is empty");
    }

    return ImageSize(width, height);
}

template<>
ImageData<rgbaColor> ImageFileTools::LoadImageFile<rgbaColor>(std::filesystem::path const & filepath)
{
    return LoadImageRgba(filepath);
}

template<>
ImageData<rgbColor> ImageFileTools::LoadImageFile<rgbColor>(std::filesystem::path const & filepath)
{
    return LoadImageRgb(filepath);
}

RgbaImageData ImageFileTools::LoadImageRgba(std::filesystem::path const & filepath)
{
    return InternalLoadImage<rgbaColor>(
        InternalOpenImage(filepath),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        std::nullopt);
}

RgbImageData ImageFileTools::LoadImageRgb(std::filesystem::path const & filepath)
{
    return InternalLoadImage<rgbColor>(
        InternalOpenImage(filepath),
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT,
        std::nullopt);
}

RgbaImageData ImageFileTools::LoadImageRgbaAndMagnify(
    std::filesystem::path const & filepath,
    int magnificationFactor)
{
    return InternalLoadImage<rgbaColor>(
        InternalOpenImage(filepath),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        ResizeInfo(
            [magnificationFactor](ImageSize const & originalImageSize)
            {
                return ImageSize(
                    originalImageSize.width * magnificationFactor,
                    originalImageSize.height * magnificationFactor);
            },
            ILU_NEAREST));
}

RgbaImageData ImageFileTools::LoadImageRgbaAndResize(
    std::filesystem::path const & filepath,
    int resizedWidth)
{
    return InternalLoadImage<rgbaColor>(
        InternalOpenImage(filepath),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        ResizeInfo(
            [resizedWidth](ImageSize const & originalImageSize)
            {
                return ImageSize(
                    resizedWidth,
                    static_cast<int>(
                        round(
                            static_cast<float>(originalImageSize.height)
                            / static_cast<float>(originalImageSize.width)
                            * static_cast<float>(resizedWidth))));
            },
            ILU_BILINEAR));
}

RgbaImageData ImageFileTools::LoadImageRgbaAndResize(
    std::filesystem::path const & filepath,
    ImageSize const & maxSize)
{
    return InternalLoadImageAndResize<rgbaColor>(
        InternalOpenImage(filepath),
        IL_RGBA,
        maxSize);
}

RgbImageData ImageFileTools::LoadImageRgbAndResize(
    std::filesystem::path const & filepath,
    ImageSize const & maxSize)
{
    return InternalLoadImageAndResize<rgbColor>(
        InternalOpenImage(filepath),
        IL_RGB,
        maxSize);
}

void ImageFileTools::SavePngImage(
    RgbaImageData const & image,
    std::filesystem::path filepath)
{
    InternalSavePngImage(
        image.Size,
        image.Data.get(),
        4,
        IL_RGBA,
        filepath);
}

void ImageFileTools::SavePngImage(
    RgbImageData const & image,
    std::filesystem::path filepath)
{
    InternalSavePngImage(
        image.Size,
        image.Data.get(),
        3,
        IL_RGB,
        filepath);
}

RgbaImageData ImageFileTools::DecodePngImage(DeSerializationBuffer<BigEndianess> const & buffer)
{
    return InternalLoadImage<rgbaColor>(
        InternalOpenImage(buffer, IL_PNG),
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT,
        std::nullopt);
}

RgbaImageData ImageFileTools::DecodePngImageAndResize(
    DeSerializationBuffer<BigEndianess> const & buffer,
    ImageSize const & maxSize)
{
    return InternalLoadImageAndResize<rgbaColor>(
        InternalOpenImage(buffer, IL_PNG),
        IL_RGBA,
        maxSize);
}

size_t ImageFileTools::EncodePngImage(
    RgbaImageData const & image,
    DeSerializationBuffer<BigEndianess> & buffer)
{
    CheckInitialized();

    ILuint imageHandle;
    ilGenImages(1, &imageHandle);
    ilBindImage(imageHandle);

    ilTexImage(
        image.Size.width,
        image.Size.height,
        1,
        static_cast<ILubyte>(4), // bpp
        IL_RGBA,
        IL_UNSIGNED_BYTE,
        const_cast<void *>(reinterpret_cast<void const *>(image.Data.get())));

    // Get required size
    auto const requiredSize = ilSaveL(IL_PNG, nullptr, 0);

    // Reserve room and advance
    void * buf = reinterpret_cast<void *>(buffer.Receive(static_cast<size_t>(requiredSize)));

    // Encode to buffer
    ilSaveL(IL_PNG, buf, requiredSize);

    ilDeleteImage(imageHandle);

    return static_cast<size_t>(requiredSize);
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

unsigned int ImageFileTools::InternalOpenImage(std::filesystem::path const & filepath)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    //
    // Load image file
    //

    std::string const filepathStr = filepath.string();
    ILconst_string ilFilename(filepathStr.c_str());
    if (!ilLoadImage(ilFilename))
    {
        ILint const devilError = ilGetError();

        // First check if the file is missing altogether
        if (!std::filesystem::exists(filepath))
        {
            throw GameException("Could not load image \"" + filepathStr + "\": the file does not exist");
        }

        // Provide DevIL's error message now
        std::string const devilErrorMessage(iluErrorString(devilError));
        throw GameException("Could not load image \"" + filepathStr + "\": " + devilErrorMessage);
    }

    return static_cast<unsigned int>(imghandle);
}

unsigned int ImageFileTools::InternalOpenImage(
    DeSerializationBuffer<BigEndianess> const & buffer,
    unsigned int imageType)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    //
    // Decode image
    //

    if (!ilLoadL(
        static_cast<ILenum>(imageType),
        reinterpret_cast<void const *>(buffer.GetData()),
        static_cast<ILuint>(buffer.GetSize())))
    {
        ILint const devilError = ilGetError();

        // Provide DevIL's error message now
        std::string const devilErrorMessage(iluErrorString(devilError));
        throw GameException("Could not load image: " + devilErrorMessage);
    }

    return static_cast<unsigned int>(imghandle);
}

template <typename TColor>
ImageData<TColor> ImageFileTools::InternalLoadImageAndResize(
    unsigned int imageHandle,
    int targetFormat,
    ImageSize const & maxSize)
{
    return InternalLoadImage<TColor>(
        imageHandle,
        targetFormat,
        IL_ORIGIN_LOWER_LEFT,
        ResizeInfo(
            [maxSize](ImageSize const & originalImageSize)
            {
                float wShrinkFactor = static_cast<float>(maxSize.width) / static_cast<float>(originalImageSize.width);
                float hShrinkFactor = static_cast<float>(maxSize.height) / static_cast<float>(originalImageSize.height);
                float shrinkFactor = std::min(
                    std::min(wShrinkFactor, hShrinkFactor),
                    1.0f);

                return ImageSize(
                    static_cast<int>(round(static_cast<float>(originalImageSize.width) * shrinkFactor)),
                    static_cast<int>(round(static_cast<float>(originalImageSize.height) * shrinkFactor)));
            },
            ILU_BILINEAR));
}

template <typename TColor>
ImageData<TColor> ImageFileTools::InternalLoadImage(
    unsigned int imageHandle,
    int targetFormat,
    int targetOrigin,
    std::optional<ResizeInfo> resizeInfo)
{
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
            throw GameException("Could not convert image: " + devilErrorMessage);
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

    ImageSize imageSize(
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT));
    int const depth = ilGetInteger(IL_IMAGE_DEPTH);
    int const bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

    assert(bpp == sizeof(TColor));

    //
    // Resize it
    //

    if (!!resizeInfo)
    {
        iluImageParameter(ILU_FILTER, resizeInfo->FilterType);

        auto newImageSize = resizeInfo->ResizeHandler(imageSize);

        if (!iluScale(newImageSize.width, newImageSize.height, depth))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw GameException("Could not resize image: " + devilErrorMessage);
        }

        imageSize = newImageSize;
    }

    //
    // Create data
    //

    ILubyte const * imageData = ilGetData();
    auto data = std::make_unique<TColor[]>(imageSize.width * imageSize.height);
    std::memcpy(static_cast<void *>(data.get()), imageData, imageSize.width * imageSize.height * bpp);

    //
    // Delete image
    //

    ilDeleteImage(imageHandle);

    return ImageData<TColor>(
        imageSize,
        std::move(data));
}

void ImageFileTools::InternalSavePngImage(
    ImageSize imageSize,
    void const * imageData,
    int bpp,
    int format,
    std::filesystem::path filepath)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    ilTexImage(
        imageSize.width,
        imageSize.height,
        1,
        static_cast<ILubyte>(bpp),
        format,
        IL_UNSIGNED_BYTE,
        const_cast<void *>(imageData));

    ilEnable(IL_FILE_OVERWRITE);
    ilSave(IL_PNG, filepath.string().c_str());

    ilDeleteImage(imghandle);
}