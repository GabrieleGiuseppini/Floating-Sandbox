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
    size_t const pixelCount = imageData.Size.GetLinearSize();
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

    for (int baseR = y, overlayR = 0; baseR < baseSize.height && overlayR < overlaySize.width; ++baseR, ++overlayR)
    {
        auto const baseRowStartIndex = baseR * baseSize.width;
        auto const overlayRowStartIndex = overlayR * overlaySize.width;

        for (int baseC = x, overlayC = 0; baseC < baseSize.width && overlayC < overlaySize.width; ++baseC, ++overlayC)
        {
            baseBuffer[baseRowStartIndex + baseC] =
                baseBuffer[baseRowStartIndex + baseC].blend(overlayBuffer[overlayRowStartIndex + overlayC]);
        }
    }
}

void ImageTools::AlphaPreMultiply(RgbaImageData & imageData)
{
    size_t const pixelCount = imageData.Size.GetLinearSize();
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

    std::unique_ptr<rgbaColor[]> newImageData = std::make_unique<rgbaColor[]>(finalImageSize.GetLinearSize());

    for (int r = 0; r < finalImageSize.height; ++r)
    {
        auto const readRowStartIndex = r * imageData.Size.width;
        auto const writeRowStartIndex = r * finalImageSize.width;
        for (int c = 0; c < finalImageSize.width; ++c)
        {
            newImageData[writeRowStartIndex + c] = imageData.Data[readRowStartIndex + c];
        }
    }

    return RgbaImageData(finalImageSize, std::move(newImageData));
}

RgbImageData ImageTools::ToRgb(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.GetLinearSize());

    for (int r = 0; r < imageData.Size.height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.width;
        for (int c = 0; c < imageData.Size.width; ++c)
        {
            newImageData[rowStartIndex + c] = imageData.Data[rowStartIndex + c].toRgbColor();
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}

RgbImageData ImageTools::ToAlpha(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.GetLinearSize());

    for (int r = 0; r < imageData.Size.height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.width;
        for (int c = 0; c < imageData.Size.width; ++c)
        {
            auto const a = imageData.Data[rowStartIndex + c].a;
            rgbColor c2(a, a, a);
            newImageData[rowStartIndex + c] = c2;
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}
