/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/DeSerializationBuffer.h>
#include <GameCore/ImageData.h>

#include <filesystem>
#include <functional>
#include <optional>

/*
 * Image standards:
 *  - Coordinates have origin at lower-left
 */
class ImageFileTools
{
public:

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    static RgbaImageData LoadImageRgba(std::filesystem::path const & filepath);
    static RgbImageData LoadImageRgb(std::filesystem::path const & filepath);
    static RgbaImageData LoadImageRgbaAndMagnify(std::filesystem::path const & filepath, int magnificationFactor);
    static RgbaImageData LoadImageRgbaAndResize(std::filesystem::path const & filepath, int resizedWidth);
    static RgbaImageData LoadImageRgbaAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);
    static RgbImageData LoadImageRgbAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbaImageData const & image);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbImageData const & image);

    static RgbaImageData DecodePngImage(DeSerializationBuffer<BigEndianess> const & buffer);

    static size_t EncodePngImage(
        RgbaImageData const & image,
        DeSerializationBuffer<BigEndianess> & buffer);

private:

    static void CheckInitialized();

    static unsigned int InternalOpenImage(std::filesystem::path const & filepath);

    static unsigned int InternalOpenImage(
        DeSerializationBuffer<BigEndianess> const & buffer,
        unsigned int imageType);

    struct ResizeInfo
    {
        std::function<ImageSize(ImageSize const &)> ResizeHandler;
        int FilterType;

        ResizeInfo(
            std::function<ImageSize(ImageSize const &)> resizeHandler,
            int filterType)
            : ResizeHandler(std::move(resizeHandler))
            , FilterType(filterType)
        {}
    };

    template <typename TColor>
    static ImageData<TColor> InternalLoadImageAndResize(
        std::filesystem::path const & filepath,
        int targetFormat,
        ImageSize const & maxSize);

    template <typename TColor>
    static ImageData<TColor> InternalLoadImage(
        std::filesystem::path const & filepath,
        int targetFormat,
        int targetOrigin,
        std::optional<ResizeInfo> resizeInfo);

    template <typename TColor>
    static ImageData<TColor> InternalLoadImage(
        unsigned int imageHandle,
        int targetFormat,
        int targetOrigin,
        std::optional<ResizeInfo> resizeInfo);

    static void InternalSaveImage(
        ImageSize imageSize,
        void const * imageData,
        int bpp,
        int format,
        std::filesystem::path filepath);

private:

    static bool mIsInitialized;
};
