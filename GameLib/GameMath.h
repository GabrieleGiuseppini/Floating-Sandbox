/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <xmmintrin.h>

template<typename T>
constexpr T Pi = T(3.1415926535897932385);

template<typename T>
inline T CeilPowerOfTwo(T value)
{
    if (value <= 0)
        return 1;

    // Check if immediately a power of 2
    if (!(value & (value - 1)))
        return value;

    T result = 2;
    while (value >>= 1) result <<= 1;
    return result;
}

/*
 * Converts the floating-point value to a 32-bit integer.
 *
 * Assumes the result fits a 32-bit value. The behavior is undefined if it doesn't.
 */
inline std::int32_t FastFloorInt32(float value) noexcept
{
    return _mm_cvtt_ss2si(_mm_load_ss(&value));
}

/*
 * Converts the floating-point value to a 64-bit integer.
 *
 * Assumes the result fits a 64-bit value. The behavior is undefined if it doesn't.
 */
inline std::int64_t FastFloorInt64(float value) noexcept
{
    return _mm_cvttss_si64(_mm_load_ss(&value));
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
