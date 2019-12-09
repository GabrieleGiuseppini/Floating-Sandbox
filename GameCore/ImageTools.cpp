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
    size_t const Size = imageData.Size.Width * imageData.Size.Height;
    for (size_t i = 0; i < Size; ++i)
    {
        imageData.Data[i] = imageData.Data[i].mix(color, alpha);
    }
}

void ImageTools::AlphaPreMultiply(RgbaImageData & imageData)
{
    size_t const Size = imageData.Size.Width * imageData.Size.Height;
    for (size_t i = 0; i < Size; ++i)
    {
        imageData.Data[i].alpha_multiply();
    }
}

RgbImageData ImageTools::ToRgb(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.Height * imageData.Size.Width);

    for (int r = 0; r < imageData.Size.Height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.Height;
        for (int c = 0; c < imageData.Size.Width; ++c)
        {
            newImageData[rowStartIndex + c] = imageData.Data[rowStartIndex + c].toRgbColor();
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}

RgbImageData ImageTools::ToAlpha(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbColor[]> newImageData = std::make_unique<rgbColor[]>(imageData.Size.Height * imageData.Size.Width);

    for (int r = 0; r < imageData.Size.Height; ++r)
    {
        auto const rowStartIndex = r * imageData.Size.Height;
        for (int c = 0; c < imageData.Size.Width; ++c)
        {
            auto const a = imageData.Data[rowStartIndex + c].a;
            rgbColor c2(a, a, a);
            newImageData[rowStartIndex + c] = c2;
        }
    }

    return RgbImageData(imageData.Size, std::move(newImageData));
}