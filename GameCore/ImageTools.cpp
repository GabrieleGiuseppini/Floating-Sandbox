/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageTools.h"

void ImageTools::BlendWithColor(
    RgbaImageData & imageData,
    rgbColor const & color,
    float alpha)
{
    size_t const pixelCount = imageData.Size.GetPixelCount();
    for (size_t i = 0; i < pixelCount; ++i)
    {
        imageData.Data[i] = imageData.Data[i].mix(color, alpha);
    }
}

void ImageTools::Overlay(
    RgbaImageData & baseImageData,
    RgbaImageData const & overlayImageData,
    int x,
    int y)
{
    auto const baseSize = baseImageData.Size;
    auto const overlaySize = overlayImageData.Size;

    rgbaColor * const restrict baseBuffer = baseImageData.Data.get();
    rgbaColor const * const restrict overlayBuffer = overlayImageData.Data.get();

    for (int baseR = y, overlayR = 0; baseR < baseSize.Height && overlayR < overlaySize.Width; ++baseR, ++overlayR)
    {
        auto const baseRowStartIndex = baseR * baseSize.Width;
        auto const overlayRowStartIndex = overlayR * overlaySize.Width;

        for (int baseC = x, overlayC = 0; baseC < baseSize.Width && overlayC < overlaySize.Width; ++baseC, ++overlayC)
        {
            baseBuffer[baseRowStartIndex + baseC] =
                baseBuffer[baseRowStartIndex + baseC].blend(overlayBuffer[overlayRowStartIndex + overlayC]);
        }
    }
}

void ImageTools::AlphaPreMultiply(RgbaImageData & imageData)
{
    size_t const pixelCount = imageData.Size.GetPixelCount();
    for (size_t i = 0; i < pixelCount; ++i)
    {
        imageData.Data[i].alpha_multiply();
    }
}

RgbaImageData ImageTools::Truncate(
    RgbaImageData imageData,
    ImageSize imageSize)
{
    ImageSize const finalImageSize = imageSize.Intersection(imageData.Size);

    std::unique_ptr<rgbaColor[]> newImageData = std::make_unique<rgbaColor[]>(finalImageSize.GetPixelCount());

    for (int r = 0; r < finalImageSize.Height; ++r)
    {
        auto const readRowStartIndex = r * imageData.Size.Width;
        auto const writeRowStartIndex = r * finalImageSize.Width;
        for (int c = 0; c < finalImageSize.Width; ++c)
        {
            newImageData[writeRowStartIndex + c] = imageData.Data[readRowStartIndex + c];
        }
    }

    return RgbaImageData(finalImageSize, std::move(newImageData));
}

RgbImageData ImageTools::ToRgb(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.GetPixelCount());

    for (int r = 0; r < imageData.Size.Height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.Width;
        for (int c = 0; c < imageData.Size.Width; ++c)
        {
            newImageData[rowStartIndex + c] = imageData.Data[rowStartIndex + c].toRgbColor();
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}

RgbImageData ImageTools::ToAlpha(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.GetPixelCount());

    for (int r = 0; r < imageData.Size.Height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.Width;
        for (int c = 0; c < imageData.Size.Width; ++c)
        {
            auto const a = imageData.Data[rowStartIndex + c].a;
            rgbColor c2(a, a, a);
            newImageData[rowStartIndex + c] = c2;
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}

Vec3fImageData ImageTools::ToVec3f(RgbImageData const & imageData)
{
    auto const pixelCount = imageData.Size.GetPixelCount();

    std::unique_ptr<vec3f[]> convertedData = std::make_unique<vec3f[]>(pixelCount);

    for (int p = 0; p < pixelCount; ++p)
    {
        convertedData[p] = imageData.Data[p].toVec3f();
    }

    return Vec3fImageData(imageData.Size, std::move(convertedData));
}