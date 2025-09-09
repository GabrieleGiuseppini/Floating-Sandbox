/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/ImageData.h>

#include <string>

class ImageFileTools
{
public:

    static void SaveImage(
        RgbaImageData const & imageData,
        std::string const & path);
};
