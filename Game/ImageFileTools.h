/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageData.h>

#include <filesystem>
#include <optional>

class ImageFileTools
{
public:

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    static ImageData LoadImageRgbaLowerLeft(std::filesystem::path const & filepath);
    static ImageData LoadImageRgbaLowerLeftAndMagnify(std::filesystem::path const & filepath, int magnificationFactor);
    static ImageData LoadImageRgbaLowerLeftAndResize(std::filesystem::path const & filepath, int resizedWidth);

    static ImageData LoadImageRgbUpperLeft(std::filesystem::path const & filepath);

    static void SaveImageRgb(
        std::filesystem::path filepath,
        ImageData const & image);

private:

    static void CheckInitialized();

    struct ResizeInfo
    {
        int ResizedWidth;
        int FilterType;

        ResizeInfo(
            int resizedWidth,
            int filterType)
            : ResizedWidth(resizedWidth)
            , FilterType(filterType)
        {}
    };

    static ImageData InternalLoadImage(
        std::filesystem::path const & filepath,
        int targetFormat,
        int targetOrigin,
        std::optional<ResizeInfo> resizeInfo);

private:

    static bool mIsInitialized;
};
