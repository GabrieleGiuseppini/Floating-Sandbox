/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-02-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "ImageData.h"
#include "Vectors.h"

#include <algorithm>
#include <cassert>
#include <functional>

class ImageTools
{
public:

    static void BlendWithColor(
        RgbaImageData & imageData,
        rgbColor const & color,
        float alpha);

    static inline vec4f SamplePixel(
        RgbaImageData const & imageData,
        float x,
        float y)
    {
        assert(x >= 0.0f && x <= static_cast<float>(imageData.Size.Width));
        assert(y >= 0.0f && y <= static_cast<float>(imageData.Size.Height));

        int32_t ix = FastTruncateInt32(x);
        float fx = x - static_cast<float>(ix);
        int32_t iy = FastTruncateInt32(y);
        float fy = y - static_cast<float>(iy);

        vec4f resulty1 = imageData.Data[iy * imageData.Size.Width + ix].toVec4f() * (1.0f - fx);
        if (ix < imageData.Size.Width - 1)
            resulty1 += imageData.Data[iy * imageData.Size.Width + ix + 1].toVec4f() * fx;

        vec4f resulty2 = vec4f::zero();
        if (iy < imageData.Size.Height - 1)
        {
            resulty2 = imageData.Data[(iy + 1) * imageData.Size.Width + ix].toVec4f() * (1.0f - fx);
            if (ix < imageData.Size.Width - 1)
                resulty2 += imageData.Data[(iy + 1) * imageData.Size.Width + ix + 1].toVec4f() * fx;
        }

        return resulty1 * (1.0f - fy) + resulty2 * fy;
    }

    static inline RgbImageData Trim(RgbImageData imageData)
    {
        return InternalTrim<rgbColor>(
            std::move(imageData),
            [](rgbColor const & c) -> bool
            {
                // Trim if white
                return c.r == rgbColor::data_type_max
                    && c.g == rgbColor::data_type_max
                    && c.b == rgbColor::data_type_max;
            });
    }

    static inline RgbaImageData Trim(RgbaImageData imageData)
    {
        return InternalTrim<rgbaColor>(
            std::move(imageData),
            [](rgbaColor const & c) -> bool
            {
                // Trim if white or fully transparent
                return (c.r == rgbColor::data_type_max
                    && c.g == rgbColor::data_type_max
                    && c.b == rgbColor::data_type_max)
                    || (c.a == 0);
            });
    }

private:

    template<typename TColor>
    static inline ImageData<TColor> InternalTrim(
        ImageData<TColor> imageData,
        std::function<bool(TColor const &)> isSpace)
    {
        // We do not handle empty images (for now...)
        assert(imageData.Size.Width > 0 && imageData.Size.Height > 0);

        //
        // Calculate bounding box
        //

        auto readBuffer = imageData.Data.get();

        int minY;
        for (minY = 0; minY < imageData.Size.Height; ++minY)
        {
            int lineOffset = minY * imageData.Size.Width;

            bool hasData = false;
            for (int x = 0; !hasData && x < imageData.Size.Width; ++x)
            {
                hasData = !isSpace(readBuffer[lineOffset + x]);
            }

            if (hasData)
                break;
        }

        // If empty, return empty image
        if (minY == imageData.Size.Height)
        {
            return ImageData<TColor>(
                ImageSize(0, 0),
                std::unique_ptr<TColor[]>(new TColor[0]));
        }

        int maxY;
        for (maxY = imageData.Size.Height - 1; maxY >= 0; --maxY)
        {
            int lineOffset = maxY * imageData.Size.Width;

            bool hasData = false;
            for (int x = 0; !hasData && x < imageData.Size.Width; ++x)
            {
                hasData = !isSpace(readBuffer[lineOffset + x]);
            }

            if (hasData)
                break;
        }

        int minX;
        for (minX = 0; minX < imageData.Size.Width; ++minX)
        {
            bool hasData = false;
            for (int y = 0, lineOffset = 0; !hasData && y < imageData.Size.Height; ++y, lineOffset += imageData.Size.Width)
            {
                hasData = !isSpace(readBuffer[lineOffset + minX]);
            }

            if (hasData)
                break;
        }

        int maxX;
        for (maxX = imageData.Size.Width - 1; maxX >= 0; --maxX)
        {
            bool hasData = false;
            for (int y = 0, lineOffset = 0; !hasData && y < imageData.Size.Height; ++y, lineOffset += imageData.Size.Width)
            {
                hasData = !isSpace(readBuffer[lineOffset + maxX]);
            }

            if (hasData)
                break;
        }

        assert(minY < imageData.Size.Height && maxY >= 0 && minX < imageData.Size.Width && maxX >= 0
                && minY <= maxY && minX <= maxX);

        // Check if we really need to trim
        if (minX > 0 || maxX < imageData.Size.Width - 1
            || minY > 0 || maxY < imageData.Size.Height - 1)
        {
            //
            // Create trimmed version, in-place
            //

            int newWidth = maxX - minX + 1;
            int newHeight = maxY - minY + 1;

            TColor * buffer = imageData.Data.get();

            for (int srcY = minY, dstY = 0, srcLineOffset = minY * imageData.Size.Width, dstLineOffset = 0;
                dstY < newHeight;
                ++srcY, ++dstY, srcLineOffset += imageData.Size.Width, dstLineOffset += newWidth)
            {
                for (int srcX = minX, dstX = 0;
                    dstX < newWidth;
                    ++srcX, ++dstX)
                {
                    assert(dstLineOffset + dstX <= srcLineOffset + srcX);

                    buffer[dstLineOffset + dstX] = buffer[srcLineOffset + srcX];
                }
            }

            return ImageData<TColor>(
                ImageSize(newWidth, newHeight),
                std::move(imageData.Data));
        }
        else
        {
            // Nothing to do
            return imageData;
        }
    }
};
