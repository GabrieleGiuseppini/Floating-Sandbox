/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PngImageFileTools.h"

#include "FileSystem.h"

#include <Core/GameException.h>
#include <Core/PngTools.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <regex>

ImageSize PngImageFileTools::GetImageSize(std::filesystem::path const & filepath)
{
    auto const buffer = FileSystem::LoadBinaryFile(filepath);
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
    auto const buffer = FileSystem::LoadBinaryFile(filepath);
    return PngTools::DecodeImageRgba(buffer);
}

RgbImageData PngImageFileTools::LoadImageRgb(std::filesystem::path const & filepath)
{
    auto const buffer = FileSystem::LoadBinaryFile(filepath);
    return PngTools::DecodeImageRgb(buffer);
}

void PngImageFileTools::SavePngImage(
    RgbaImageData const & image,
    std::filesystem::path filepath)
{
    auto const buffer = PngTools::EncodeImage(image);
    FileSystem::SaveBinaryFile(buffer, filepath);
}

void PngImageFileTools::SavePngImage(
    RgbImageData const & image,
    std::filesystem::path filepath)
{
    auto const buffer = PngTools::EncodeImage(image);
    FileSystem::SaveBinaryFile(buffer, filepath);
}
