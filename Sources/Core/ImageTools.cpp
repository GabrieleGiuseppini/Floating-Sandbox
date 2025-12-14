/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageTools.h"

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

    // Conversion factors
    float const tgtToSrcX = static_cast<float>(image.Size.width) / static_cast<float>(newSize.width);
    float const tgtToSrcY = static_cast<float>(image.Size.height) / static_cast<float>(newSize.height);

    // Strategy: for each target pixel, find source pixel)
    for (int tgtY = 0; tgtY < newSize.height; ++tgtY)
    {
        // Calculate source y
        float const srcY = static_cast<float>(tgtY) * tgtToSrcY;
        assert(srcY >= 0.0f);
        int const srcY0 = std::min(static_cast<int>(FastTruncateToArchInt(srcY)), image.Size.height - 1);
        int const srcY1 = std::min(srcY0 + 1, image.Size.height - 1);

        float const dy = srcY - static_cast<float>(srcY0);
        assert(dy >= 0.0f && dy < 1.0f);

        for (int tgtX = 0; tgtX < newSize.width; ++tgtX)
        {
            // Calculate source x
            float const srcX = static_cast<float>(tgtX) * tgtToSrcX;
            assert(srcX >= 0.0f);
            int const srcX0 = std::min(static_cast<int>(FastTruncateToArchInt(srcX)), image.Size.width - 1);
            int const srcX1 = std::min(srcX0 + 1, image.Size.width - 1);

            float const dx = srcX - static_cast<float>(srcX0);
            assert(dx >= 0.0f && dx < 1.0f);

            // Interpolate this-y X
            f_vec_type const thisY_X = Mix(imageF[{srcX0, srcY0}], imageF[{srcX1, srcY0}], dx);
            // Interpolate other-t Y
            f_vec_type const otherY_X = Mix(imageF[{srcX0, srcY1}], imageF[{srcX1, srcY1}], dx);
            // Interpolate Y's
            result[{tgtX, tgtY}] = color_type(Mix(thisY_X, otherY_X, dy));
        }
    }


    // TODOOLD
    //// 0-1 space
    //float const tgtToSrcW = static_cast<float>(image.Size.width);
    //float const tgtToSrcH = static_cast<float>(image.Size.height);

    //// We sample target pixels at their center
    //float const tgtPixelDW = 1.0f / static_cast<float>(newSize.width);
    //float const tgtPixelDH = 1.0f / static_cast<float>(newSize.height);
    //float yf = tgtPixelDH / 2.0f;
    //for (int y = 0; y < newSize.height; ++y, yf += tgtPixelDH)
    //{
    //    float const srcYF = yf * tgtToSrcH;
    //    int const srcY = static_cast<int>(FastTruncateToArchInt(srcYF));
    //    float const srcYDF = srcYF - srcY;

    //    int otherSrcY;
    //    float thisDy;
    //    if (srcYDF >= 0.5f)
    //    {
    //        // Next
    //        otherSrcY = (srcY + 1 < image.Size.height)
    //            ? srcY + 1
    //            : srcY; // Reuse same
    //        thisDy = srcYDF - 0.5f;
    //    }
    //    else
    //    {
    //        // Prev
    //        otherSrcY = (srcY > 0)
    //            ? srcY - 1
    //            : srcY; // Reuse same
    //        thisDy = 0.5f - srcYDF;
    //    }

    //    assert(thisDy >= 0.0f && thisDy < 1.0f);

    //    float xf = tgtPixelDW / 2.0f;
    //    for (int x = 0; x < newSize.width; ++x, xf += tgtPixelDW)
    //    {
    //        float const srcXF = xf * tgtToSrcW;
    //        int const srcX = static_cast<int>(FastTruncateToArchInt(srcXF));
    //        float const srcXDF = srcXF - srcX;

    //        int otherSrcX;
    //        float thisDx;
    //        if (srcXDF >= 0.5f)
    //        {
    //            // Next
    //            otherSrcX = (srcX + 1 < image.Size.width)
    //                ? srcX + 1
    //                : srcX; // Reuse same
    //            thisDx = srcXDF - 0.5f;
    //        }
    //        else
    //        {
    //            // Prev
    //            otherSrcX = (srcX > 0)
    //                ? srcX - 1
    //                : srcX; // Reuse same
    //            thisDx = 0.5f - srcXDF;
    //        }

    //        assert(thisDx >= 0.0f && thisDx < 1.0f);

    //        // Interpolate this-y X
    //        f_vec_type const thisY_X = Mix(imageF[{srcX, srcY}], imageF[{otherSrcX, srcY}], thisDx);
    //        // Interpolate other-t Y
    //        f_vec_type const otherY_X = Mix(imageF[{srcX, otherSrcY}], imageF[{otherSrcX, otherSrcY}], thisDx);
    //        // Interpolate Y's
    //        result[{x, y}] = color_type(Mix(thisY_X, otherY_X, thisDy));
    //    }
    //}

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
