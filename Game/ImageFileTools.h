/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageData.h>

#include <filesystem>
#include <functional>
#include <optional>

class ImageFileTools
{
public:

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    static RgbaImageData LoadImageRgbaLowerLeft(std::filesystem::path const & filepath);
    static RgbImageData LoadImageRgbLowerLeft(std::filesystem::path const & filepath);
    static RgbaImageData LoadImageRgbaLowerLeftAndMagnify(std::filesystem::path const & filepath, int magnificationFactor);
    static RgbaImageData LoadImageRgbaLowerLeftAndResize(std::filesystem::path const & filepath, int resizedWidth);
    static RgbaImageData LoadImageRgbaLowerLeftAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);
    static RgbImageData LoadImageRgbLowerLeftAndResize(std::filesystem::path const & filepath, ImageSize const & maxSize);
    static RgbImageData LoadImageRgbUpperLeft(std::filesystem::path const & filepath);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbaImageData const & image);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbImageData const & image);

private:

    static void CheckInitialized();

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
    static ImageData<TColor> InternalLoadImage(
        std::filesystem::path const & filepath,
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
