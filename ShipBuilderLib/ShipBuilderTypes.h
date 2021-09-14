/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Vectors.h>

#include <cmath>
#include <cstdint>
#include <sstream>

namespace ShipBuilder {

enum class ToolType : std::uint32_t
{
    StructuralPencil,
    StructuralEraser,
    ElectricalPencil,
    ElectricalEraser,
    RopesHarness,
    RopesEraser,

    _Last = RopesEraser
};

enum class LayerType : std::uint32_t
{
    Structural = 0,
    Electrical,
    Texture,
    Ropes
};

size_t constexpr LayerCount = static_cast<size_t>(LayerType::Ropes) + 1;

enum class MaterialPlaneType
{
    Foreground,
    Background
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Geometry
////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TTag>
struct _IntegralSize
{
    int width;
    int height;

    _IntegralSize(
        int _width,
        int _height)
        : width(_width)
        , height(_height)
    {}

    static _IntegralSize<TTag> FromFloat(vec2f const & vec)
    {
        return _IntegralSize<TTag>(
            static_cast<int>(std::round(vec.x)),
            static_cast<int>(std::round(vec.y)));
    }

    inline bool operator==(_IntegralSize<TTag> const & other) const
    {
        return this->width == other.width
            && this->height == other.height;
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << width << ", " << height << ")";
        return ss.str();
    }
};

using WorkSpaceSize = _IntegralSize<struct WorkSpaceTag>;
using DisplayLogicalSize = _IntegralSize<struct DisplayLogicalTag>;
using DisplayPhysicalSize = _IntegralSize<struct DisplayPhysicalTag>;

template<typename TTag>
struct _IntegralCoordinates
{
    int x;
    int y;

    _IntegralCoordinates(
        int _x,
        int _y)
        : x(_x)
        , y(_y)
    {}

    static _IntegralCoordinates<TTag> FromFloat(vec2f const & vec)
    {
        return _IntegralCoordinates<TTag>(
            static_cast<int>(std::round(vec.x)),
            static_cast<int>(std::round(vec.y)));
    }

    inline bool operator==(_IntegralCoordinates<TTag> const & other) const
    {
        return this->x == other.x
            && this->y == other.y;
    }

    inline _IntegralCoordinates<TTag> operator+(_IntegralSize<TTag> const & sz) const
    {
        return _IntegralCoordinates<TTag>(
            this->x + sz.width,
            this->y + sz.height);
    }

    inline _IntegralSize<TTag> operator-(_IntegralCoordinates<TTag> const & other) const
    {
        return _IntegralSize<TTag>(
            this->x - other.x,
            this->y - other.y);
    }

    template<typename TRect>
    bool IsInRect(TRect const & rect) const
    {
        return x >= 0 && x < rect.width && y >= 0 && y < rect.height;
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(x),
            static_cast<float>(y));
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }
};

using WorkSpaceCoordinates = _IntegralCoordinates<struct WorkSpaceTag>;
using DisplayLogicalCoordinates = _IntegralCoordinates<struct DisplayLogicalTag>;
using DisplayPhysicalCoordinates = _IntegralCoordinates<struct DisplayPhysicalTag>;

}