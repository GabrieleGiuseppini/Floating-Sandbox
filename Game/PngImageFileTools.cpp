/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PngImageFileTools.h"

#include <GameCore/GameException.h>
#include <GameCore/PngTools.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <regex>

ImageSize PngImageFileTools::GetImageSize(std::filesystem::path const & filepath)
{
    auto const buffer = InternalLoadImageFile(filepath);
    return PngTools::GetImageSize(buffer);
}

template<>
ImageData<rgbaColor> PngImageFileTools::LoadImage<rgbaColor>(std::filesystem::path const & filepath)
{
    return LoadImageRgba(filepath);
}

template<>
ImageData<rgbColor> PngImageFileTools::LoadImage<rgbColor>(std::filesystem::path const & filepath)
{
    return LoadImageRgb(filepath);
}

RgbaImageData PngImageFileTools::LoadImageRgba(std::filesystem::path const & filepath)
{
    auto const buffer = InternalLoadImageFile(filepath);
    return PngTools::DecodeImageRgba(buffer);
}

RgbImageData PngImageFileTools::LoadImageRgb(std::filesystem::path const & filepath)
{
    auto const buffer = InternalLoadImageFile(filepath);
    return PngTools::DecodeImageRgb(buffer);
}

void PngImageFileTools::SavePngImage(
    RgbaImageData const & image,
    std::filesystem::path filepath)
{
    auto const buffer = PngTools::EncodeImage(image);
    InternalSaveImageFile(buffer, filepath);
}

void PngImageFileTools::SavePngImage(
    RgbImageData const & image,
    std::filesystem::path filepath)
{
    auto const buffer = PngTools::EncodeImage(image);
    InternalSaveImageFile(buffer, filepath);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

Buffer<std::uint8_t> PngImageFileTools::InternalLoadImageFile(std::filesystem::path const & filepath)
{
    auto fStream = std::ifstream(
        filepath,
        std::ios_base::in | std::ios_base::binary);
    if (!fStream.is_open())
    {
        throw GameException("Error opening file \"" + filepath.string() + "\"");
    }

    fStream.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = fStream.gcount();
    fStream.clear();
    fStream.seekg(0, std::ios_base::beg);

    Buffer<std::uint8_t> buffer = Buffer<std::uint8_t>(static_cast<size_t>(length));
    fStream.read(
        reinterpret_cast<char *>(buffer.data()),
        length);
    fStream.close();

    return buffer;
}

void PngImageFileTools::InternalSaveImageFile(
    Buffer<std::uint8_t> const & buffer,
    std::filesystem::path const & filepath)
{
    auto fStream = std::ofstream(
        filepath,
        std::ios_base::out | std::ios_base::binary);
    if (!fStream.is_open())
    {
        throw GameException("Error opening file \"" + filepath.string() + "\"");
    }

    fStream.write(
        reinterpret_cast<char const *>(buffer.data()),
        buffer.GetSize());
    fStream.close();
}
