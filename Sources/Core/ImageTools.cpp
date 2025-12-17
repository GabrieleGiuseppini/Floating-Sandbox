/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageTools.h"

// TODOTEST
#include "Log.h"

#include <type_traits>

template<typename TImageData>
TImageData ImageTools::Resize(
    TImageData const & image,
    ImageSize const & newSize,
    FilterKind filter)
{
    switch (filter)
    {
        case FilterKind::Bilinear:
        {
            return InternalResizeBilinear(image, newSize);
        }

        case FilterKind::Nearest:
        {
            return InternalResizeNearest(image, newSize);
        }
    }

    assert(false);
    return TImageData(ImageSize(0, 0));
}

template RgbaImageData ImageTools::Resize(RgbaImageData const & image, ImageSize const & newSize, FilterKind filter);
template RgbImageData ImageTools::Resize(RgbImageData const & image, ImageSize const & newSize, FilterKind filter);

template<typename TImageData>
TImageData ImageTools::ResizeNicer(
    TImageData const & image,
    ImageSize const & newSize)
{
    using color_type = typename TImageData::element_type;
    using f_color_type = typename TImageData::element_type::f_vector_type;

    assert(image.Size.width > 0 && image.Size.height > 0);
    assert(newSize.width > 0 && newSize.height > 0);

    //
    // Width
    //

    float const srcWidth = static_cast<float>(image.Size.width);
    float const tgtWidth = static_cast<float>(newSize.width);
    float const widthScaleFactor = tgtWidth / srcWidth;

    LogMessage("TODO: widthScaleFactor=", widthScaleFactor);

    auto const srcImageF = InternalToFloat(image);
    Buffer2D<f_color_type, struct ImageTag> widthImageF(newSize.width, image.Size.height);

    // TODOTEST
    //for (int srcY = 0; srcY < image.Size.height; ++srcY)
    //{
    //    InternalResizeDimension_TriangleFilter<TImageData>(
    //        image.Size.width,
    //        widthScaleFactor,
    //        [&](int srcX) -> f_color_type
    //        {
    //            assert(srcX >= 0 && srcX < image.Size.width);
    //            return srcImageF[{srcX, srcY}];
    //        },
    //        [&](int tgtX, f_color_type const & c)
    //        {
    //            assert(tgtX >= 0 && tgtX < newSize.width);
    //            widthImageF[{tgtX, srcY}] = c;
    //        });
    //}

    // TODOTEST
    //for (int srcY = 0; srcY < image.Size.height; ++srcY)
    //{
    //    InternalResizeDimension_BoxFilter<TImageData>(
    //        newSize.width,
    //        srcWidth,
    //        1.0f / widthScaleFactor,
    //        [&](int srcX) -> f_color_type
    //        {
    //            assert(srcX >= 0 && srcX < image.Size.width);
    //            return srcImageF[{srcX, srcY}];
    //        },
    //        [&](int tgtX, f_color_type const & c)
    //        {
    //            assert(tgtX >= 0 && tgtX < newSize.width);
    //            widthImageF[{tgtX, srcY}] = c;
    //        });
    //}

    for (int srcY = 0; srcY < image.Size.height; ++srcY)
    {
        InternalResizeDimension_BoxFilter<TImageData>(
            image.Size.width,
            widthScaleFactor,
            [&](int srcX) -> f_color_type
            {
                assert(srcX >= 0 && srcX < image.Size.width);
                return srcImageF[{srcX, srcY}];
            },
            [&](int tgtX, f_color_type const & c)
            {
                assert(tgtX >= 0 && tgtX < newSize.width);
                widthImageF[{tgtX, srcY}] = c;
            });
    }

    //
    // Height
    //

    float const srcHeight = static_cast<float>(image.Size.height);
    float const tgtHeight = static_cast<float>(newSize.height);
    float const heightScaleFactor = tgtHeight / srcHeight;

    LogMessage("TODO: heightScaleFactor=", heightScaleFactor);

    Buffer2D<f_color_type, struct ImageTag> heightImageF(newSize.width, newSize.height);

    // TODOTEST
    //for (int srcX = 0; srcX < newSize.width; ++srcX)
    //{
    //    InternalResizeDimension_TriangleFilter<TImageData>(
    //        image.Size.height,
    //        heightScaleFactor,
    //        [&](int srcY) -> f_color_type
    //        {
    //            assert(srcY >= 0 && srcY < image.Size.height);
    //            return widthImageF[{srcX, srcY}];
    //        },
    //        [&](int tgtY, f_color_type const & c)
    //        {
    //            assert(tgtY >= 0 && tgtY < newSize.height);
    //            heightImageF[{srcX, tgtY}] = c;
    //        });
    //}

    // TODOTEST
    //for (int srcX = 0; srcX < newSize.width; ++srcX)
    //{
    //    InternalResizeDimension_BoxFilter<TImageData>(
    //        newSize.height,
    //        srcHeight,
    //        1.0f / heightScaleFactor,
    //        [&](int srcY) -> f_color_type
    //        {
    //            assert(srcY >= 0 && srcY < image.Size.height);
    //            return widthImageF[{srcX, srcY}];
    //        },
    //        [&](int tgtY, f_color_type const & c)
    //        {
    //            assert(tgtY >= 0 && tgtY < newSize.height);
    //            heightImageF[{srcX, tgtY}] = c;
    //        });
    //}

    for (int srcX = 0; srcX < newSize.width; ++srcX)
    {
        InternalResizeDimension_BoxFilter<TImageData>(
            image.Size.height,
            heightScaleFactor,
            [&](int srcY) -> f_color_type
            {
                assert(srcY >= 0 && srcY < image.Size.height);
                return widthImageF[{srcX, srcY}];
            },
            [&](int tgtY, f_color_type const & c)
            {
                assert(tgtY >= 0 && tgtY < newSize.height);
                heightImageF[{srcX, tgtY}] = c;
            });
    }

    return InternalFromFloat<TImageData>(heightImageF);
}

template RgbaImageData ImageTools::ResizeNicer(RgbaImageData const & image, ImageSize const & newSize);
template RgbImageData ImageTools::ResizeNicer(RgbImageData const & image, ImageSize const & newSize);

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

RgbaImageData ImageTools::MakeGreyscale(RgbaImageData const & imageData)
{
    std::unique_ptr<rgbaColor[]> newImageData = std::make_unique<rgbaColor[]>(imageData.Size.GetLinearSize());

    int const h = imageData.Size.height;
    int const w = imageData.Size.width;
    for (int r = 0; r < h; ++r)
    {
        auto const rowStartIndex = r * w;
        for (int c = 0; c < w; ++c)
        {
            auto const & src = imageData.Data[rowStartIndex + c];
            rgbaColor::data_type const grey = static_cast<rgbaColor::data_type>((static_cast<int>(src.r) + static_cast<int>(src.g) + static_cast<int>(src.b)) / 3);
            newImageData[rowStartIndex + c] = rgbaColor(grey, grey, grey, src.a);
        }
    }

    return RgbaImageData(imageData.Size, std::move(newImageData));
}

RgbaImageData ImageTools::MakeVerticalGradient(
        rgbColor const & startColor,
        rgbColor const & endColor,
        ImageSize imageSize)
{
    std::unique_ptr<rgbaColor[]> newImageData = std::make_unique<rgbaColor[]>(imageSize.GetLinearSize());

    if (imageSize.height > 0)
    {
        int const h = imageSize.height;
        int const w = imageSize.width;
        vec3f startColorF = startColor.toVec3f();
        vec3f endColorF = endColor.toVec3f();
        for (int r = 0; r < h; ++r)
        {
            rgbaColor const color = rgbaColor(
                Mix(endColorF, startColorF, static_cast<float>(r) / static_cast<float>(h - 1)),
                rgbaColor::data_type_max);

            auto const rowStartIndex = r * w;
            for (int c = 0; c < w; ++c)
            {
                newImageData[rowStartIndex + c] = color;
            }
        }
    }

    return RgbaImageData(imageSize, std::move(newImageData));
}

void ImageTools::AlphaPreMultiply(RgbaImageData & imageData)
{
    size_t const pixelCount = imageData.Size.GetLinearSize();
    for (size_t i = 0; i < pixelCount; ++i)
    {
        imageData.Data[i].alpha_multiply();
    }
}

void ImageTools::ApplyBinaryTransparencySmoothing(RgbaImageData & imageData)
{
    rgbaColor * imageDataPtr = imageData.Data.get();

    for (int y = 0; y < imageData.Size.height; ++y)
    {
        size_t const rowIndex = y * imageData.Size.width;

        for (int x = 0; x < imageData.Size.width; ++x)
        {
            if (imageDataPtr[rowIndex + x].a == 0)
            {
                // Pixel is fully transparent...
                // ...calculate avg of opaque neighbors, if any exist

                vec4f srcColorF = vec4f::zero();
                float cnt = 0.0f;

                for (int y2 = std::max(y - 1, 0); y2 <= std::min(y + 1, imageData.Size.height - 1); ++y2)
                {
                    for (int x2 = std::max(x - 1, 0); x2 <= std::min(x + 1, imageData.Size.width - 1); ++x2)
                    {
                        auto const & neighborColor = imageDataPtr[y2 * imageData.Size.width + x2];
                        if (neighborColor.a != 0)
                        {
                            srcColorF += neighborColor.toVec4f();
                            cnt += 1.0f;
                        }
                    }
                }

                if (cnt != 0.0f)
                {
                    srcColorF /= cnt;
                    srcColorF.w = 0.0f;
                    imageDataPtr[rowIndex + x] = rgbaColor(srcColorF);
                }
            }
        }
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

//////////////////////////////////////////////////////////////////////////

template<typename TImageData, typename TSourceGetter, typename TTargetSetter>
static void ImageTools::InternalResizeDimension_TriangleFilter(
    int srcSize,
    float srcToTgt,
    TSourceGetter const & srcGetter,
    TTargetSetter const & tgtSetter)
{
    assert(srcToTgt < 1.0f); // This filter is for cases where each target pixel gets at least one source pixel

    using f_color_type = typename TImageData::element_type::f_vector_type;

    //
    // Strategy: visit each source pixel, decide where it goes in target, and average all the ones
    // going into the same target pixels, using a triangle filter for the weights
    //

    // Currently-accumulated target pixel
    f_color_type currentTgtPixelSum = f_color_type();
    float currentTgtPixelWeightSum = 0.0f;
    int currentTgtI = 0;

    for (int srcI = 0; srcI < srcSize; ++srcI)
    {
        // The coord of pixel 0 is half of a (target) pixel width;
        // this makes most sense when thinking of weights - for example, pixel 0
        // should not have a weight of 0
        float const srcICoords = 0.5f + static_cast<float>(srcI);

        // Calculate the target coord
        float const tgtIf = srcICoords * srcToTgt;
        float const tgtIf_floor = std::floorf(tgtIf);
        int const tgtI = static_cast<int>(tgtIf_floor);

        if (tgtI != currentTgtI)
        {
            // Close current target pixel
            assert(currentTgtPixelWeightSum > 0.0f);
            tgtSetter(currentTgtI, currentTgtPixelSum / currentTgtPixelWeightSum);

            // Open new one
            currentTgtPixelSum = f_color_type();
            currentTgtPixelWeightSum = 0.0f;
            currentTgtI = tgtI;
        }

        // Calculate weight
        float const tgtI_fract = tgtIf - tgtIf_floor;
        assert(tgtI_fract >= 0.0f && tgtI_fract < 1.0f);
        float const w = 1.0f - std::abs(0.5f - tgtI_fract) / 0.5f; // 1.0 at center, 0.0 at edges

        // Update target pixel
        currentTgtPixelSum += srcGetter(srcI) * w;
        currentTgtPixelWeightSum += w;
    }

    // Store last one
    assert(currentTgtPixelWeightSum > 0.0f);
    tgtSetter(currentTgtI, currentTgtPixelSum / currentTgtPixelWeightSum);
}

template<typename TImageData, typename TSourceGetter, typename TTargetSetter>
static void ImageTools::InternalResizeDimension_BoxFilter(
    int srcSize,
    float srcToTgt,
    TSourceGetter const & srcGetter,
    TTargetSetter const & tgtSetter)
{
    assert(srcToTgt < 1.0f); // This filter is for cases where each target pixel gets at least one source pixel

    using f_color_type = typename TImageData::element_type::f_vector_type;

    //
    // Strategy: visit each source pixel, decide where it goes in target, and average all the ones
    // going into the same target pixels, using a box filter for the weights, and the fraction
    // of the source pixel that falls in the target pixel
    //

    float const tgtToSrc = 1.0f / srcToTgt;

    // Currently-accumulated target pixel
    f_color_type currentTgtPixelSum = f_color_type();
    float currentTgtPixelWeightSum = 0.0f;

    // The end of the current target pixel, in target coordinates;
    // this value corresponds to the beginning of the next target pixel
    float currentTgtEnd = 1.0f;

    for (int srcI = 0; srcI < srcSize; ++srcI)
    {
        // Calculate the target coord for (the beginning of) this source pixel
        float const tgtIStart = static_cast<float>(srcI) * srcToTgt;

        // Calculate the target coord of the end of this source pixel
        float const tgtIEnd = tgtIStart + srcToTgt;

        if (tgtIEnd >= currentTgtEnd)
        {
            // This source pixel crosses the target pixel boundary

            assert(tgtIStart <= currentTgtEnd);

            //
            // Incorporate the part of the pixel that falls in this target pixel
            //

            f_color_type const c = srcGetter(srcI);

            // Calculate the fraction of the source pixel within the target pixel
            float const pixelFraction = (currentTgtEnd - tgtIStart) * tgtToSrc;

            // Add fraction to current sum
            currentTgtPixelSum += c * pixelFraction;
            currentTgtPixelWeightSum += pixelFraction;

            //
            // Publish current
            //

            assert(currentTgtPixelWeightSum > 0.0f);
            tgtSetter(static_cast<int>(std::floorf(tgtIStart)), currentTgtPixelSum / currentTgtPixelWeightSum);

            //
            // Move on to next target pixel
            //

            currentTgtPixelSum = c * (1.0f - pixelFraction);
            currentTgtPixelWeightSum = (1.0f - pixelFraction);
            currentTgtEnd += 1.0f;
        }
        else
        {
            // This source pixel falls fully in the target pixel boundary

            // Update target pixel
            currentTgtPixelSum += srcGetter(srcI);
            currentTgtPixelWeightSum += 1.0f;
        }
    }
}

template<typename TImageData, typename TSourceGetter, typename TTargetSetter>
static void ImageTools::InternalResizeDimension_BoxFilter(
    int tgtSize,
    float srcSize,
    float tgtToSrc,
    TSourceGetter const & srcGetter,
    TTargetSetter const & tgtSetter)
{
    assert(tgtToSrc > 1.0f); // This filter is for cases where each target pixel gets at least one source pixel

    using f_color_type = typename TImageData::element_type::f_vector_type;

    //
    // Strategy: visit each target pixel, decide which source pixels falls in it, and average all of them,
    // weighting by how much of the pixel fits in it
    //

    for (int tgtI = 0; tgtI < tgtSize; ++tgtI)
    {
        // Calculate the edges of this pixel in source coordinates
        float const tgtIf = static_cast<float>(tgtI);
        float const srcIStart = std::floorf(tgtIf * tgtToSrc);
        float const srcILast = std::floorf((tgtIf + 1.0f) * tgtToSrc);

        // Visit all source pixels falling into this target pixel
        f_color_type currentTgtPixelSum = f_color_type();
        float currentTgtPixelWeightSum = 0.0f;
        for (float srcI = srcIStart; srcI <= std::min(srcILast, srcSize - 1.0f); srcI += 1.0f)
        {
            // Calculate fraction of this source pixel that falls into the target pixel
            float pixelFraction;
            if (srcI < tgtIf * tgtToSrc)
            {
                assert(srcI + 1.0f >= tgtIf * tgtToSrc); // Not sure

                pixelFraction = 1.0f - (tgtIf * tgtToSrc - srcI);
                assert(pixelFraction <= 1.0f);
            }
            else if (srcI + 1.0f >= (tgtIf + 1.0f) * tgtToSrc)
            {
                assert(srcI <= (tgtIf + 1.0f) * tgtToSrc); // Not sure

                pixelFraction = (tgtIf + 1.0f) * tgtToSrc - srcI;
                assert(pixelFraction <= 1.0f);
            }
            else
            {
                pixelFraction = 1.0f;
            }

            //
            // Accumulate this target pixel, using its fraction as weight
            //

            currentTgtPixelSum += srcGetter(static_cast<int>(srcI)) * pixelFraction;
            currentTgtPixelWeightSum += pixelFraction;
        }

        // Store
        assert(currentTgtPixelWeightSum > 0.0f);
        tgtSetter(tgtI, currentTgtPixelSum / currentTgtPixelWeightSum);
    }
}

template<typename TImageData>
TImageData ImageTools::InternalResizeNearest(
    TImageData const & image,
    ImageSize const & newSize)
{
    TImageData result(newSize);

    // Strategy: for each target pixel, find source pixel

    // 0-1 space
    float const tgtToSrcW = static_cast<float>(image.Size.width);
    float const tgtToSrcH = static_cast<float>(image.Size.height);

    // We sample target pixels at their center
    float const tgtPixelDW = 1.0f / static_cast<float>(newSize.width);
    float const tgtPixelDH = 1.0f / static_cast<float>(newSize.height);
    float yf = tgtPixelDH / 2.0f;
    for (int y = 0; y < newSize.height; ++y, yf += tgtPixelDH)
    {
        int const srcY = static_cast<int>(yf * tgtToSrcH);

        float xf = tgtPixelDW / 2.0f;
        for (int x = 0; x < newSize.width; ++x, xf += tgtPixelDW)
        {
            int const srcX = static_cast<int>(xf * tgtToSrcW);

            assert(srcX >= 0 && srcX < image.Size.width);
            assert(srcY >= 0 && srcY < image.Size.height);

            result[{x, y}] = image[{srcX, srcY}];
        }
    }

    return result;
}

template RgbaImageData ImageTools::InternalResizeNearest(RgbaImageData const & image, ImageSize const & newSize);
template RgbImageData ImageTools::InternalResizeNearest(RgbImageData const & image, ImageSize const & newSize);

template<typename TImageData>
TImageData ImageTools::InternalResizeBilinear(
    TImageData const & image,
    ImageSize const & newSize)
{
    using color_type = typename TImageData::element_type;
    TImageData result(newSize);

    // Convert input to floats
    using f_vec_type = typename TImageData::element_type::f_vector_type;
    auto const imageF = InternalToFloat(image);

    // Strategy: for each target pixel, find source pixel

    // 0-1 space
    float const tgtToSrcW = static_cast<float>(image.Size.width);
    float const tgtToSrcH = static_cast<float>(image.Size.height);

    // We sample target pixels at their center
    float const tgtPixelDW = 1.0f / static_cast<float>(newSize.width);
    float const tgtPixelDH = 1.0f / static_cast<float>(newSize.height);
    float yf = tgtPixelDH / 2.0f;
    for (int y = 0; y < newSize.height; ++y, yf += tgtPixelDH)
    {
        float const srcYF = yf * tgtToSrcH;
        int const srcY = static_cast<int>(FastTruncateToArchInt(srcYF));
        float const srcYDF = srcYF - srcY;

        int otherSrcY;
        float thisDy;
        if (srcYDF >= 0.5f)
        {
            // Next
            otherSrcY = (srcY + 1 < image.Size.height)
                ? srcY + 1
                : srcY; // Reuse same
            thisDy = srcYDF - 0.5f;
        }
        else
        {
            // Prev
            otherSrcY = (srcY > 0)
                ? srcY - 1
                : srcY; // Reuse same
            thisDy = 0.5f - srcYDF;
        }

        assert(thisDy >= 0.0f && thisDy < 1.0f);

        float xf = tgtPixelDW / 2.0f;
        for (int x = 0; x < newSize.width; ++x, xf += tgtPixelDW)
        {
            float const srcXF = xf * tgtToSrcW;
            int const srcX = static_cast<int>(FastTruncateToArchInt(srcXF));
            float const srcXDF = srcXF - srcX;

            int otherSrcX;
            float thisDx;
            if (srcXDF >= 0.5f)
            {
                // Next
                otherSrcX = (srcX + 1 < image.Size.width)
                    ? srcX + 1
                    : srcX; // Reuse same
                thisDx = srcXDF - 0.5f;
            }
            else
            {
                // Prev
                otherSrcX = (srcX > 0)
                    ? srcX - 1
                    : srcX; // Reuse same
                thisDx = 0.5f - srcXDF;
            }

            assert(thisDx >= 0.0f && thisDx < 1.0f);

            // Interpolate this-y X
            f_vec_type const thisY_X = Mix(imageF[{srcX, srcY}], imageF[{otherSrcX, srcY}], thisDx);
            // Interpolate other-t Y
            f_vec_type const otherY_X = Mix(imageF[{srcX, otherSrcY}], imageF[{otherSrcX, otherSrcY}], thisDx);
            // Interpolate Y's
            result[{x, y}] = color_type(Mix(thisY_X, otherY_X, thisDy));
        }
    }

    return result;
}

template RgbaImageData ImageTools::InternalResizeBilinear(RgbaImageData const & image, ImageSize const & newSize);
template RgbImageData ImageTools::InternalResizeBilinear(RgbImageData const & image, ImageSize const & newSize);

template<typename TImageData>
Buffer2D<typename TImageData::element_type::f_vector_type, struct ImageTag> ImageTools::InternalToFloat(TImageData const & imageData)
{
    Buffer2D<typename TImageData::element_type::f_vector_type, struct ImageTag> result(imageData.Size);

    typename TImageData::element_type const * const restrict src = imageData.Data.get();
    typename TImageData::element_type::f_vector_type * const restrict trg = result.Data.get();
    size_t const sz = imageData.GetLinearSize();
    for (size_t i = 0; i < sz; ++i)
    {
        *(trg + i) = (src + i)->toVec();
    }

    return result;
}

template<typename TImageData>
TImageData ImageTools::InternalFromFloat(Buffer2D<typename TImageData::element_type::f_vector_type, struct ImageTag> const & imageData)
{
    TImageData result(imageData.Size);

    typename TImageData::element_type::f_vector_type  const * const restrict src = imageData.Data.get();
    typename TImageData::element_type * const restrict trg = result.Data.get();
    size_t const sz = imageData.GetLinearSize();
    for (size_t i = 0; i < sz; ++i)
    {
        *(trg + i) = TImageData::element_type(*(src + i));
    }

    return result;
}
