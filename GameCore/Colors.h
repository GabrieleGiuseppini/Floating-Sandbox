/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-02-04
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "Vectors.h"

#include <cstdint>
#include <limits>
#include <ostream>
#include <string>

#pragma pack(push, 1)

struct rgbColor
{
public:

    typedef uint8_t data_type;
    static constexpr uint8_t data_type_max = std::numeric_limits<uint8_t>::max();
    static constexpr size_t channel_count = 3;

public:

    uint8_t r;
    uint8_t g;
    uint8_t b;

    static constexpr rgbColor zero()
    {
        return rgbColor(0, 0, 0);
    }

    rgbColor()
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

    inline constexpr explicit rgbColor(vec3f const & c)
        : r(static_cast<uint8_t>(c.x * 255.0f + 0.5f))
        , g(static_cast<uint8_t>(c.y * 255.0f + 0.5f))
        , b(static_cast<uint8_t>(c.z * 255.0f + 0.5f))
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

    inline constexpr vec3f toVec3f() const
    {
        return vec3f(
            _toFloat(r),
            _toFloat(g),
            _toFloat(b));
    }

    inline constexpr vec4f toVec4f(float a) const
    {
        return vec4f(
            _toFloat(r),
            _toFloat(g),
            _toFloat(b),
            a);
    }

    static rgbColor fromString(std::string const & str);

    std::string toString() const;

private:

    static inline constexpr float _toFloat(uint8_t val) noexcept
    {
        return static_cast<float>(val) / 255.0f;
    }
};

#pragma pack(pop)

static_assert(offsetof(rgbColor, r) == 0 * sizeof(uint8_t));
static_assert(offsetof(rgbColor, g) == 1 * sizeof(uint8_t));
static_assert(offsetof(rgbColor, b) == 2 * sizeof(uint8_t));
static_assert(sizeof(rgbColor) == 3 * sizeof(uint8_t));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, rgbColor const & c)
{
    os << c.toString();
    return os;
}

#pragma pack(push, 1)

struct rgbaColor
{
public:

    typedef uint8_t data_type;
    static constexpr uint8_t data_type_max = std::numeric_limits<uint8_t>::max();
    static constexpr size_t channel_count = 4;

public:

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    static constexpr rgbaColor zero()
    {
        return rgbaColor(0, 0, 0, 0);
    }

    rgbaColor()
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

    inline constexpr rgbaColor(
        rgbColor const & c,
        uint8_t _a)
        : r(c.r)
        , g(c.g)
        , b(c.b)
        , a(_a)
    {
    }

    inline constexpr explicit rgbaColor(vec4f const & c)
        : r(static_cast<uint8_t>(c.x * 255.0f + 0.5f))
        , g(static_cast<uint8_t>(c.y * 255.0f + 0.5f))
        , b(static_cast<uint8_t>(c.z * 255.0f + 0.5f))
        , a(static_cast<uint8_t>(c.w * 255.0f + 0.5f))
    {
    }

    inline constexpr rgbaColor(
        vec3f const & c,
        uint8_t _a)
        : r(static_cast<uint8_t>(c.x * 255.0f + 0.5f))
        , g(static_cast<uint8_t>(c.y * 255.0f + 0.5f))
        , b(static_cast<uint8_t>(c.z * 255.0f + 0.5f))
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

    inline void alpha_multiply()
    {
        float const alpha = static_cast<float>(a) / 255.0f;
        r = static_cast<uint8_t>(r * alpha + 0.5f);
        g = static_cast<uint8_t>(g * alpha + 0.5f);
        b = static_cast<uint8_t>(b * alpha + 0.5f);
    }

    inline rgbaColor blend(rgbaColor const & otherColor) const noexcept
    {
        float const thisAlpha = static_cast<float>(this->a) / 255.0f;
        float const otherAlpha = static_cast<float>(otherColor.a) / 255.0f;

        vec3f const result = Mix(
            this->toVec3f(),
            otherColor.toVec3f(),
            otherAlpha);

        float const finalAlpha = thisAlpha + otherAlpha * (1.0f - thisAlpha);

        return rgbaColor(result, static_cast<uint8_t>(finalAlpha * 255.0f + 0.5f));
    }

    inline rgbaColor mix(
        rgbColor const & otherColor,
        float alpha) const noexcept
    {
        vec3f const result = Mix(this->toVec3f(), otherColor.toVec3f(), alpha);

        return rgbaColor(result, this->a);
    }

    inline constexpr rgbColor toRgbColor() const
    {
        return rgbColor(r, g, b);
    }

    inline constexpr vec3f toVec3f() const noexcept
    {
        return vec3f(
            _toFloat(r),
            _toFloat(g),
            _toFloat(b));
    }

    inline constexpr vec4f toVec4f() const noexcept
    {
        return vec4f(
            _toFloat(r),
            _toFloat(g),
            _toFloat(b),
            _toFloat(a));
    }

    inline constexpr float alphaAsFloat() const noexcept
    {
        return _toFloat(a);
    }

    static rgbaColor fromString(std::string const & str);

    std::string toString() const;

private:

    static inline constexpr float _toFloat(uint8_t val) noexcept
    {
        return static_cast<float>(val) / 255.0f;
    }
};

#pragma pack(pop)

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

    static constexpr rgbaColorAccumulation zero()
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
