/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Vectors.h"

#include <limits>

namespace Geometry {

// Axis-Aligned Bounding Box
struct AABB
{
public:

    vec2f TopRight;
    vec2f BottomLeft;

    AABB()
        : TopRight(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest())
        , BottomLeft(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    {}

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

    inline vec2f GetSize() const
    {
        return vec2f(GetWidth(), GetHeight());
    }

    inline vec2f CalculateCenter() const
    {
        return vec2f(
            (TopRight.x + BottomLeft.x) / 2.0f,
            (TopRight.y + BottomLeft.y) / 2.0f);
    }

    inline float CalculateArea() const
    {
        return GetWidth() * GetHeight();
    }

    inline void ExtendTo(vec2f const & point)
    {
        if (point.x > TopRight.x)
            TopRight.x = point.x;
        if (point.y > TopRight.y)
            TopRight.y = point.y;
        if (point.x < BottomLeft.x)
            BottomLeft.x = point.x;
        if (point.y < BottomLeft.y)
            BottomLeft.y = point.y;
    }

    inline void ExtendTo(AABB const & other)
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

    inline AABB AdjustSize(
        float widthMultiplier,
        float heightMultiplier) const
    {
        float const newWidth = GetWidth() * widthMultiplier;
        float const newHeight = GetHeight() * heightMultiplier;
        auto const center = CalculateCenter();
        return AABB(
            center.x - newWidth / 2.0f,
            center.x + newWidth / 2.0f,
            center.y + newHeight / 2.0f,
            center.y - newHeight / 2.0f);
    }

    inline bool Contains(vec2f const & point) const noexcept
    {
        return point.x >= BottomLeft.x
            && point.x <= TopRight.x
            && point.y >= BottomLeft.y
            && point.y <= TopRight.y;
    }

    inline bool Contains(
        vec2f const & point,
        float margin) const noexcept
    {
        return point.x >= BottomLeft.x - margin
            && point.x <= TopRight.x + margin
            && point.y >= BottomLeft.y - margin
            && point.y <= TopRight.y + margin;
    }
};

struct ShipAABB : public AABB
{
public:

    float FrontierEdgeCount;

    ShipAABB()
        : AABB()
        , FrontierEdgeCount(0)
    {
    }

    ShipAABB(
        float left,
        float right,
        float top,
        float bottom,
        ElementCount frontierEdgeCount)
        : AABB(
            left,
            right,
            top,
            bottom)
        , FrontierEdgeCount(static_cast<float>(frontierEdgeCount))
    {
    }

    ShipAABB(
        vec2f const topRight,
        vec2f const bottomLeft,
        ElementCount frontierEdgeCount)
        : AABB(
            topRight,
            bottomLeft)
        , FrontierEdgeCount(static_cast<float>(frontierEdgeCount))
    {
    }

    inline void ExtendTo(ShipAABB const & other)
    {
        if (other.TopRight.x > TopRight.x)
            TopRight.x = other.TopRight.x;
        if (other.TopRight.y > TopRight.y)
            TopRight.y = other.TopRight.y;
        if (other.BottomLeft.x < BottomLeft.x)
            BottomLeft.x = other.BottomLeft.x;
        if (other.BottomLeft.y < BottomLeft.y)
            BottomLeft.y = other.BottomLeft.y;

        FrontierEdgeCount += other.FrontierEdgeCount;
    }

    using AABB::ExtendTo;
};

}