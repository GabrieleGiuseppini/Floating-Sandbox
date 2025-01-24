/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Buffer.h>
#include <GameCore/ImageData.h>

#include <filesystem>

/*
 * Image standards:
 *  - Coordinates have origin at lower-left
 */
class PngImageFileTools
{
public:

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    template<typename TColor>
    static ImageData<TColor> LoadImage(std::filesystem::path const & filepath);
    static RgbaImageData LoadImageRgba(std::filesystem::path const & filepath);
    static RgbImageData LoadImageRgb(std::filesystem::path const & filepath);

    static void SavePngImage(
        RgbaImageData const & image,
        std::filesystem::path filepath);

    static void SavePngImage(
        RgbImageData const & image,
        std::filesystem::path filepath);

private:

    static Buffer<std::uint8_t> InternalLoadImageFile(std::filesystem::path const & filepath);

    static void InternalSaveImageFile(
        Buffer<std::uint8_t> const & buffer,
        std::filesystem::path const & filepath);
};
