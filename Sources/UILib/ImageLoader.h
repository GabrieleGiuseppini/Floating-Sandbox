/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-25
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/ImageData.h>

#include <filesystem>

namespace ImageLoader {

RgbaImageData LoadImageRgba(std::filesystem::path const & filepath);

}