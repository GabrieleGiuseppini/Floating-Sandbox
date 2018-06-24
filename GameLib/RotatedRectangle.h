/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

/*
 * Describes a rotated rectangle.
 */
struct RotatedRectangle
{
    vec2f TopLeft;
    vec2f TopRight;
    vec2f BottomLeft;
    vec2f BottomRight;

    RotatedRectangle(
        vec2f const & topLeft,
        vec2f const & topRight,
        vec2f const & bottomLeft,
        vec2f const & bottomRight)
        : TopLeft(topLeft)
        , TopRight(topRight)
        , BottomLeft(bottomLeft)
        , BottomRight(bottomRight)
    {}
};
