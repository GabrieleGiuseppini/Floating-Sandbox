/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer2D.h"
#include "Colors.h"
#include "GameTypes.h"

template <typename TColor>
using ImageData = Buffer2D<TColor, struct ImageTag>;

using RgbImageData = ImageData<rgbColor>;
using RgbaImageData = ImageData<rgbaColor>;