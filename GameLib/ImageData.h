/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageSize.h"

#include <memory>

struct ImageData
{
public:

    ImageSize const Size;
    std::unique_ptr<unsigned char const[]> Data;

    ImageData(
        int width,
        int height,
        std::unique_ptr<unsigned char const[]> data)
        : Size(width, height)
        , Data(std::move(data))
    {
    }
};