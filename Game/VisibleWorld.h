/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-10-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Vectors.h>

struct VisibleWorld
{
    vec2f Center;

    float Width;
    float Height;

    vec2f TopLeft;
    vec2f BottomRight;
};