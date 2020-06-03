/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"
#include "ImageSize.h"
#include "Vectors.h"

#include <cstring>
#include <memory>

template <typename TColor>
struct ImageData
{
public:

    using color_type = TColor;

public:

    ImageSize const Size;
    std::unique_ptr<color_type[]> Data;

    ImageData(
        int width,
        int height,
        std::unique_ptr<color_type[]> data)
        : Size(width, height)
        , Data(std::move(data))
    {
    }

    ImageData(
        ImageSize size,
        std::unique_ptr<color_type[]> data)
        : Size(size)
        , Data(std::move(data))
    {
    }

    ImageData(ImageData && other) noexcept
        : Size(other.Size)
        , Data(std::move(other.Data))
    {
    }

    size_t GetByteSize() const
    {
        return static_cast<size_t>(Size.GetPixelCount()) * sizeof(color_type);
    }

    std::unique_ptr<ImageData> MakeCopy() const
    {
        auto newData = std::make_unique<color_type[]>(Size.GetPixelCount());
        std::memcpy(newData.get(), Data.get(), static_cast<size_t>(Size.GetPixelCount()) * sizeof(color_type));

        return std::make_unique<ImageData>(
            Size,
            std::move(newData));
    }
};

using RgbImageData = ImageData<rgbColor>;
using RgbaImageData = ImageData<rgbaColor>;
using Vec3fImageData = ImageData<vec3f>;