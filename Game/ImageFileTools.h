/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Colors.h>
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

    template<typename TColor>
    static ImageData<TColor> LoadImageFile(std::filesystem::path const & filepath);

    template<>
    static ImageData<rgbaColor> LoadImageFile<rgbaColor>(std::filesystem::path const & filepath)
    {
        return LoadImageRgba(filepath);
    }

    template<>
    static ImageData<rgbColor> LoadImageFile<rgbColor>(std::filesystem::path const & filepath)
    {
        return LoadImageRgb(filepath);
    }

    static RgbaImageData LoadImageRgba(std::filesystem::path const & filepath);
    static RgbImageData LoadImageRgb(std::filesystem::path const & filepath);

    static RgbaImageData LoadImageRgbaAndMagnify(std::filesystem::path const & filepath, int magnificationFactor);
    static RgbaImageData LoadImageRgbaAndResize(std::filesystem::path const & filepath, int resizedWidth);
    static RgbaImageData LoadImageRgbaAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);
    static RgbImageData LoadImageRgbAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);

    static void SavePngImage(
        RgbaImageData const & image,
        std::filesystem::path filepath);

    static void SavePngImage(
        RgbImageData const & image,
        std::filesystem::path filepath);

    static RgbaImageData DecodePngImage(DeSerializationBuffer<BigEndianess> const & buffer);

    static RgbaImageData DecodePngImageAndResize(
        DeSerializationBuffer<BigEndianess> const & buffer,
        ImageSize const & maxSize);

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
        unsigned int imageHandle,
        int targetFormat,
        ImageSize const & maxSize);

    template <typename TColor>
    static ImageData<TColor> InternalLoadImage(
        unsigned int imageHandle,
        int targetFormat,
        int targetOrigin,
        std::optional<ResizeInfo> resizeInfo);

    static void InternalSavePngImage(
        ImageSize imageSize,
        void const * imageData,
        int bpp,
        int format,
        std::filesystem::path filepath);

private:

    static bool mIsInitialized;
};
