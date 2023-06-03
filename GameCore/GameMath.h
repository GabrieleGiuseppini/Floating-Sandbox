/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

template<typename T>
constexpr T Pi = T(3.1415926535897932385);

/*
 * Converts the floating-point value to a 32-bit integer, truncating it towards zero.
 *
 * Assumes the result fits a 32-bit value. The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateToInt32(-7.6) == -7.
 */
inline register_int_32 FastTruncateToInt32(float value) noexcept
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    return _mm_cvtt_ss2si(_mm_load_ss(&value));
#else
    return static_cast<register_int_32>(value);
#endif
}

/*
 * Converts the floating-point value to a 32-bit integer, truncating it towards negative infinity.
 *
 * Assumes the result fits a 32-bit value. The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateToInt32TowardsNInfinity(-7.6) == -8.
 */
inline register_int_32 FastTruncateToInt32TowardsNInfinity(float value) noexcept
{
    register_int_32 const v =
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
        _mm_cvtt_ss2si(_mm_load_ss(&value));
#else
        static_cast<register_int_32>(value);
#endif

    return (v > value) ? v - 1 : v;
}

/*
 * Converts the floating-point value to a 64-bit integer, truncating it towards zero.
 *
 * Assumes the result fits a 64-bit value. The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateInt64(-7.6) == -7.
 */
inline register_int_64 FastTruncateToInt64(float value) noexcept
{
#if FS_IS_ARCHITECTURE_X86_64()
    return _mm_cvttss_si64(_mm_load_ss(&value));
#else
    return static_cast<register_int_64>(value);
#endif
}

/*
 *
 * Converts the floating - point value to a 64 - bit integer, truncating it towards negative infinity.
 *
 * Assumes the result fits a 64 - bit value.The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateToInt64TowardsNInfinity(-7.6) == -8.
 */
inline register_int_64 FastTruncateToInt64TowardsNInfinity(float value) noexcept
{
    register_int_64 const v =
#if FS_IS_ARCHITECTURE_X86_64()
        _mm_cvttss_si64(_mm_load_ss(&value));
#else
        static_cast<register_int_64>(value);
#endif

    return (v > value) ? v - 1 : v;
}

/*
 * Converts the floating-point value to an integer of the same width as the
 * architecture's registers, truncating it towards zero.
 * Used when the implementation doesn't really care about the returned
 * type - for example because it needs to be used as an index.
 *
 * Assumes the result fits the integer. The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateToArchInt(-7.6) == -7.
 */

inline register_int FastTruncateToArchInt(float value) noexcept
{
#if FS_IS_REGISTER_WIDTH_32()
    return FastTruncateToInt32(value);
#elif FS_IS_REGISTER_WIDTH_64()
    return FastTruncateToInt64(value);
#endif
}

/*
 * Converts the floating-point value to an integer of the same width as the
 * architecture's registers, truncating it towards negative infinity.
 * Used when the implementation doesn't really care about the returned 
 * type - for example because it needs to be used as an index.
 *
 * Assumes the result fits the integer. The behavior is undefined if it doesn't.
 *
 * As one would expect, FastTruncateToArchIntTowardsNInfinity(-7.6) == -8.
 */

inline register_int FastTruncateToArchIntTowardsNInfinity(float value) noexcept
{
#if FS_IS_REGISTER_WIDTH_32()
    return FastTruncateToInt32TowardsNInfinity(value);
#elif FS_IS_REGISTER_WIDTH_64()
    return FastTruncateToInt64TowardsNInfinity(value);
#endif
}

/*
 * Adapted from Paul Mineiro - Copyright (C) 2011 Paul Mineiro.
 */
inline float FastLog2(float x)
{
    union { float f; std::uint32_t i; } vx = { x };
    union { std::uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
    float y = static_cast<float>(vx.i);
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f
        - 1.498030302f * mx.f
        - 1.72587999f / (0.3520887068f + mx.f);
}

/*
 * Adapted from Paul Mineiro - Copyright (C) 2011 Paul Mineiro.
 */
inline float FastLog(float x)
{
    return 0.69314718f * FastLog2(x);
}

inline float DiscreteLog2(float x)
{
    typedef union {
        float f;
        struct {
            unsigned int mantissa : 23;
            unsigned int exponent : 8;
            unsigned int sign : 1;
        } parts;
    } float_cast;

    float_cast d1 = { x };

    return static_cast<float>(static_cast<int>(d1.parts.exponent) - 127);
}

/*
 * Adapted from Paul Mineiro - Copyright (C) 2011 Paul Mineiro.
 */
inline float FastPow2(float p)
{
    float offset = (p < 0) ? 1.0f : 0.0f;
    float clipp = (p < -126) ? -126.0f : p;
    int w = static_cast<int>(clipp);
    float z = clipp - w + offset;
    union { std::uint32_t i; float f; } v = { static_cast<std::uint32_t>((1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)) };

    return v.f;
}

/*
 * Adapted from Paul Mineiro - Copyright (C) 2011 Paul Mineiro.
 */
inline float FastExp(float p)
{
    return FastPow2(1.442695040f * p);
}

/*
 * Adapted from Paul Mineiro - Copyright (C) 2011 Paul Mineiro.
 */
inline float FastPow(
    float x,
    float p)
{
    return FastPow2(p * FastLog2(x));
}

namespace detail {
    float constexpr SqrtNewtonRaphson(float x, float curr, float prev)
    {
        return curr == prev
            ? curr
            : SqrtNewtonRaphson(x, 0.5f * (curr + x / curr), curr);
    }
}

float constexpr CompileTimeSqrt(float x)
{
    return detail::SqrtNewtonRaphson(x, x, 0.0f);
}

template<typename T>
inline T Clamp(
    T x,
    T lLimit,
    T rLimit) noexcept
{
    assert(lLimit <= rLimit);

    T const mx = x < lLimit ? lLimit : x;
    return mx > rLimit ? rLimit : mx;
}

template<typename T>
inline T Mix(
    T val1,
    T val2,
    float x) noexcept
{
    //return val1 * (1.0f - x) + val2 * x; // Original form; our form saves one subtraction
    return val1 + (val2 - val1) * x;
}

inline float Step(
    float lEdge,
    float x) noexcept
{
    return x < lEdge ? 0.0f : 1.0f;
}

template <typename T> 
inline int Sign(T val)  // 0.0 returns +1.0
{
    return (T(0) <= val) - (val < T(0));
}

inline float SignStep(
    float lEdge,
    float x) noexcept
{
    return x < lEdge ? -1.0f : 1.0f;
}

inline float LinearStep(
    float lEdge,
    float rEdge,
    float x) noexcept
{
    assert(lEdge <= rEdge);

    return Clamp((x - lEdge) / (rEdge - lEdge), 0.0f, 1.0f);
}

inline float SmoothStep(
    float lEdge,
    float rEdge,
    float x) noexcept
{
    assert(lEdge <= rEdge);

    x = Clamp((x - lEdge) / (rEdge - lEdge), 0.0f, 1.0f);

    return x * x * (3.0f - 2.0f * x); // 3x^2 -2x^3, Cubic Hermite
}

inline float SmootherStep(
    float lEdge,
    float rEdge,
    float x) noexcept
{
    assert(lEdge <= rEdge);

    x = Clamp((x - lEdge) / (rEdge - lEdge), 0.0f, 1.0f);

    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f); // 6x^5 - 15x^4 + 10x^3, fifth-order
}

inline float InverseSmoothStep(float x) noexcept
{
    return 0.5f - std::sin(std::asin(1.0f - 2.0f * x) / 3.0f);
}

/*
 * Maps a x value, belonging to [minX, maxX], to [minOutput, maxOutput],
 * such that when x is 1.0, output is oneOutput.
 */
inline float MixPiecewiseLinear(
	float minOutput,
	float oneOutput,
	float maxOutput,
	float minX,
	float maxX,
	float x) noexcept
{
	assert((minOutput <= oneOutput) && (oneOutput <= maxOutput));
	assert(minX <= x && x <= maxX);
	assert(minX < 1.0 && 1.0 < maxX);

	return x <= 1.0f
		? minOutput + (oneOutput - minOutput) * (x - minX) / (1.0f - minX)
		: oneOutput + (maxOutput - oneOutput) * (x - 1.0f) / (maxX - 1.0f);
}