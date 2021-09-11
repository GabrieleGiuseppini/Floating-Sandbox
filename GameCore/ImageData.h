/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer2D.h"
#include "Colors.h"
#include "ImageSize.h"
#include "Vectors.h"

#include <cstring>
#include <memory>

template <typename TColor>
using ImageData = Buffer2D<TColor, ImageSize>;

using RgbImageData = ImageData<rgbColor>;
using RgbaImageData = ImageData<rgbaColor>;
using Vec3fImageData = ImageData<vec3f>;