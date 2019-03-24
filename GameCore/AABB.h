/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Vectors.h"

namespace Geometry {

// Axis-Aligned Bounding Box
class AABB
{
public:
    vec2f TopRight;
    vec2f BottomLeft;

    AABB(
        float left,
        float right,
        float top,
        float bottom)
        : TopRight(right, top)
        , BottomLeft(left, bottom)
    {}

    AABB(
        vec2f const topRight,
        vec2f const bottomLeft)
        : TopRight(topRight)
        , BottomLeft(bottomLeft)
    {}

    inline float GetWidth() const
    {
        return TopRight.x - BottomLeft.x;
    }

    inline float GetHeight() const
    {
        return TopRight.y - BottomLeft.y;
    }

    void ExtendTo(AABB const & other)
    {
        if (other.TopRight.x > TopRight.x)
            TopRight.x = other.TopRight.x;
        if (other.TopRight.y > TopRight.y)
            TopRight.y = other.TopRight.y;
        if (other.BottomLeft.x < BottomLeft.x)
            BottomLeft.x = other.BottomLeft.x;
        if (other.BottomLeft.y < BottomLeft.y)
            BottomLeft.y = other.BottomLeft.y;
    }

    inline bool Contains(vec2f const & point) const
    {
        return point.x >= BottomLeft.x
            && point.x <= TopRight.x
            && point.y >= BottomLeft.y
            && point.y <= TopRight.y;
    }
};

}