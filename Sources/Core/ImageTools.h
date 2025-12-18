/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-02-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer2D.h"
#include "GameMath.h"
#include "GameTypes.h"
#include "ImageData.h"
#include "Vectors.h"

#include <algorithm>
#include <cassert>
#include <functional>

class ImageTools
{
public:

    enum class FilterKind
    {
        Nearest,
        Bilinear
    };

    template<typename TImageData>
    static TImageData Resize(
        TImageData const & image,
        ImageSize const & newSize,
        FilterKind filter);

    template<typename TImageData>
    static TImageData ResizeNicer(
        TImageData const & image,
        ImageSize const & newSize);

    static void BlendWithColor(
        RgbaImageData & imageData,
        rgbColor const & color,
        float alpha);

    static void Overlay(
        RgbaImageData & baseImageData,
        RgbaImageData const & overlayImageData,
        int x,
        int y);

    static RgbaImageData MakeGreyscale(RgbaImageData const & imageData);

    static RgbaImageData MakeVerticalGradient(
        rgbColor const & startColor,
        rgbColor const & endColor,
        ImageSize imageSize);

    /*
     * Multiplies the r, g, and b channels by the alpha channel.
     */
    static void AlphaPreMultiply(RgbaImageData & imageData);

    /*
     * Sets the rgb components of fully-transparent pixels to the average of neighboring
     * fully-opaque pixels, to improve bleeding around transparency.
     */
    static void ApplyBinaryTransparencySmoothing(RgbaImageData & imageData);

    static inline vec4f SamplePixel(
        RgbaImageData const & imageData,
        float x,
        float y)
    {
        assert(x >= 0.0f && x <= static_cast<float>(imageData.Size.width));
        assert(y >= 0.0f && y <= static_cast<float>(imageData.Size.height));

        auto ix = FastTruncateToArchInt(x);
        float fx = x - static_cast<float>(ix);
        auto iy = FastTruncateToArchInt(y);
        float fy = y - static_cast<float>(iy);

        vec4f resulty1 = imageData.Data[iy * imageData.Size.width + ix].toVec4f() * (1.0f - fx);
        if (ix < imageData.Size.width - 1)
            resulty1 += imageData.Data[iy * imageData.Size.width + ix + 1].toVec4f() * fx;

        vec4f resulty2 = vec4f::zero();
        if (iy < imageData.Size.height - 1)
        {
            resulty2 = imageData.Data[(iy + 1) * imageData.Size.width + ix].toVec4f() * (1.0f - fx);
            if (ix < imageData.Size.width - 1)
                resulty2 += imageData.Data[(iy + 1) * imageData.Size.width + ix + 1].toVec4f() * fx;
        }

        return resulty1 * (1.0f - fy) + resulty2 * fy;
    }

    static inline RgbaImageData TrimTransparent(RgbaImageData imageData)
    {
        return InternalTrim<rgbaColor>(
            std::move(imageData),
            [](rgbaColor const & c) -> bool
            {
                // Trim if fully transparent
                return c.a == 0;
            });
    }

    static inline RgbImageData TrimWhite(RgbImageData imageData)
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

    static inline RgbaImageData TrimWhiteOrTransparent(RgbaImageData imageData)
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

    static RgbaImageData Truncate(
        RgbaImageData imageData,
        ImageSize imageSize);

    static RgbImageData ToRgb(RgbaImageData const & imageData);

private:

    template<typename TImageData, typename TSourceGetter, typename TTargetSetter>
    static inline void InternalResizeDimension_Bilinear(
        int srcSize,
        int tgtSize,
        float tgtToSrc,
        TSourceGetter const & srcGetter,
        TTargetSetter const & tgtSetter);

    template<typename TImageData, typename TSourceGetter, typename TTargetSetter>
    static inline void InternalResizeDimension_BoxFilter(
        int srcSize,
        float srcToTgt,
        float tgtToSrc,
        TSourceGetter const & srcGetter,
        TTargetSetter const & tgtSetter);

    template<typename TImageData>
    static TImageData InternalResizeNearest(
        TImageData const & image,
        ImageSize const & newSize);

    template<typename TImageData>
    static TImageData InternalResizeBilinear(
        TImageData const & image,
        ImageSize const & newSize);

    template<typename TColor>
    static inline ImageData<TColor> InternalTrim(
        ImageData<TColor> imageData,
        std::function<bool(TColor const &)> isSpace)
    {
        // We do not handle empty images (for now...)
        assert(imageData.Size.width > 0 && imageData.Size.height > 0);

        //
        // Calculate bounding box
        //

        auto readBuffer = imageData.Data.get();

        int minY;
        for (minY = 0; minY < imageData.Size.height; ++minY)
        {
            int lineOffset = minY * imageData.Size.width;

            bool hasData = false;
            for (int x = 0; !hasData && x < imageData.Size.width; ++x)
            {
                hasData = !isSpace(readBuffer[lineOffset + x]);
            }

            if (hasData)
                break;
        }

        // If empty, return empty image
        if (minY == imageData.Size.height)
        {
            return ImageData<TColor>(
                ImageSize(0, 0),
                std::unique_ptr<TColor[]>(new TColor[0]));
        }

        int maxY;
        for (maxY = imageData.Size.height - 1; maxY >= 0; --maxY)
        {
            int lineOffset = maxY * imageData.Size.width;

            bool hasData = false;
            for (int x = 0; !hasData && x < imageData.Size.width; ++x)
            {
                hasData = !isSpace(readBuffer[lineOffset + x]);
            }

            if (hasData)
                break;
        }

        int minX;
        for (minX = 0; minX < imageData.Size.width; ++minX)
        {
            bool hasData = false;
            for (int y = 0, lineOffset = 0; !hasData && y < imageData.Size.height; ++y, lineOffset += imageData.Size.width)
            {
                hasData = !isSpace(readBuffer[lineOffset + minX]);
            }

            if (hasData)
                break;
        }

        int maxX;
        for (maxX = imageData.Size.width - 1; maxX >= 0; --maxX)
        {
            bool hasData = false;
            for (int y = 0, lineOffset = 0; !hasData && y < imageData.Size.height; ++y, lineOffset += imageData.Size.width)
            {
                hasData = !isSpace(readBuffer[lineOffset + maxX]);
            }

            if (hasData)
                break;
        }

        assert(minY < imageData.Size.height && maxY >= 0 && minX < imageData.Size.width && maxX >= 0
                && minY <= maxY && minX <= maxX);

        // Check if we really need to trim
        if (minX > 0 || maxX < imageData.Size.width - 1
            || minY > 0 || maxY < imageData.Size.height - 1)
        {
            //
            // Create trimmed version, in-place
            //

            int newWidth = maxX - minX + 1;
            int newHeight = maxY - minY + 1;

            TColor * buffer = imageData.Data.get();

            for (int srcY = minY, dstY = 0, srcLineOffset = minY * imageData.Size.width, dstLineOffset = 0;
                dstY < newHeight;
                ++srcY, ++dstY, srcLineOffset += imageData.Size.width, dstLineOffset += newWidth)
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

    template<typename TImageData>
    static Buffer2D<typename TImageData::element_type::f_vector_type, struct ImageTag> InternalToFloat(TImageData const & imageData);

    template<typename TImageData>
    static TImageData InternalFromFloat(Buffer2D<typename TImageData::element_type::f_vector_type, struct ImageTag> const & imageData);
};
