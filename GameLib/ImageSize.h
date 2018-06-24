/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct ImageSize
{
public:

    int Width;
    int Height;

    ImageSize(
        int width,
        int height)
        : Width(width)
        , Height(height)
    {
    }
};