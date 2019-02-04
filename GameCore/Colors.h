/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-02-04
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Vectors.h"

#include <cstdint>
#include <ostream>
#include <string>

struct rgbColor
{
public:

    uint8_t r;
    uint8_t g;
    uint8_t b;

    static constexpr const rgbColor zero()
    {
        return rgbColor();
    }

    inline constexpr rgbColor()
        : r(0)
        , g(0)
        , b(0)
    {
    }

    inline constexpr rgbColor(
        uint8_t _r,
        uint8_t _g,
        uint8_t _b)
        : r(_r)
        , g(_g)
        , b(_b)
    {
    }

	inline bool operator==(rgbColor const & other) const
	{
        return r == other.r
            && g == other.g
            && b == other.b;
	}

    inline bool operator!=(rgbColor const & other) const
    {
        return !(*this == other);
    }

	// (lexicographic comparison only)
	inline bool operator<(rgbColor const & other) const
	{
        return r < other.r
            || (r == other.r && g < other.g)
            || (r == other.r && g == other.g && b < other.b);
	}

    inline vec3f toVec3f() const
    {
        return vec3f(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f);
    }

    inline vec4f toVec4f(float a) const
    {
        return vec4f(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            a);
    }

    std::string toString() const;
};

static_assert(offsetof(rgbColor, r) == 0 * sizeof(uint8_t));
static_assert(offsetof(rgbColor, g) == 1 * sizeof(uint8_t));
static_assert(offsetof(rgbColor, b) == 2 * sizeof(uint8_t));
static_assert(sizeof(rgbColor) == 3 * sizeof(uint8_t));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, rgbColor const & c)
{
    os << c.toString();
    return os;
}

struct rgbaColor
{
public:

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    static constexpr const rgbaColor zero()
    {
        return rgbaColor();
    }

    inline constexpr rgbaColor()
        : r(0)
        , g(0)
        , b(0)
        , a(0)
    {
    }

    inline constexpr rgbaColor(
        uint8_t _r,
        uint8_t _g,
        uint8_t _b,
        uint8_t _a)
        : r(_r)
        , g(_g)
        , b(_b)
        , a(_a)
    {
    }

    inline bool operator==(rgbaColor const & other) const
    {
        return r == other.r
            && g == other.g
            && b == other.b
            && a == other.a;
    }

    inline bool operator!=(rgbaColor const & other) const
    {
        return !(*this == other);
    }

    // (lexicographic comparison only)
    inline bool operator<(rgbaColor const & other) const
    {
        return r < other.r
            || (r == other.r && g < other.g)
            || (r == other.r && g == other.g && b < other.b)
            || (r == other.r && g == other.g && b == other.b && a < other.a);
    }

    inline rgbColor toRgbColor() const
    {
        return rgbColor(r, g, b);
    }

    inline vec4f toVec4f() const
    {
        return vec4f(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f);
    }

    std::string toString() const;
};

static_assert(offsetof(rgbaColor, r) == 0 * sizeof(uint8_t));
static_assert(offsetof(rgbaColor, g) == 1 * sizeof(uint8_t));
static_assert(offsetof(rgbaColor, b) == 2 * sizeof(uint8_t));
static_assert(offsetof(rgbaColor, a) == 3 * sizeof(uint8_t));
static_assert(sizeof(rgbaColor) == 4 * sizeof(uint8_t));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, rgbaColor const & c)
{
    os << c.toString();
    return os;
}

struct rgbaColorAccumulation
{
public:

    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
    uint32_t count;

    static constexpr const rgbaColorAccumulation zero()
    {
        return rgbaColorAccumulation();
    }

    inline constexpr rgbaColorAccumulation()
        : r(0)
        , g(0)
        , b(0)
        , a(0)
        , count(0)
    {
    }

    inline constexpr rgbaColorAccumulation(rgbaColor const & c)
        : r(c.r)
        , g(c.g)
        , b(c.b)
        , a(c.a)
        , count(1)
    {
    }

    inline rgbaColorAccumulation & operator+=(rgbaColor const & c)
    {
        r += static_cast<uint32_t>(c.r);
        g += static_cast<uint32_t>(c.g);
        b += static_cast<uint32_t>(c.b);
        a += static_cast<uint32_t>(c.a);
        ++count;

        return *this;
    }

    inline rgbaColor toRgbaColor() const
    {
        if (count > 0)
            return rgbaColor(
                static_cast<uint8_t>(r / count),
                static_cast<uint8_t>(g / count),
                static_cast<uint8_t>(b / count),
                static_cast<uint8_t>(a / count));
        else
            return rgbaColor::zero();
    }
};
