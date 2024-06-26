/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

//
// Contains portions of works from other authors, acknowledged as it happens.
//

#include "SysSpecifics.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

template<typename T>
constexpr T Pi = T(3.1415926535897932385);

template<typename T>
constexpr T Epsilon = T(1e-7);

// Vectorial global constants

#define DECLARE_4F_CONST(Name, Val) \
  static FS_ALIGN16_BEG float constexpr Name ## 4f[4] FS_ALIGN16_END = { Val, Val, Val, Val }
#define DECLARE_4I_CONST(Name, Val) \
  static FS_ALIGN16_BEG int constexpr Name ## 4i[4] FS_ALIGN16_END = { Val, Val, Val, Val }

DECLARE_4F_CONST(One, 1.0f);
DECLARE_4F_CONST(ZeroPointFive, 0.5f);
DECLARE_4F_CONST(FourOverPi, 4.0f / Pi<float>);

DECLARE_4I_CONST(One, 1);
DECLARE_4I_CONST(OneInv, ~1);
DECLARE_4I_CONST(Two, 2);
DECLARE_4I_CONST(Four, 4);
DECLARE_4I_CONST(SignMask, (int)0x80000000);
DECLARE_4I_CONST(SignMaskInv, (int)~0x80000000);


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

inline float FastMod(float value, float divisor)
{
    return value - ((int)(value / divisor)) * divisor;
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

inline bool IsAlmostZero(float value, float epsilon)
{
    return (value > -epsilon) && (value < epsilon);
}

inline bool IsAlmostZero(float value)
{
    return IsAlmostZero(value, Epsilon<float>);
}

inline bool AreAlmostEqual(float value1, float value2, float epsilon)
{
    float const d = value1 - value2;
    return (d > -epsilon) && (d < epsilon);
}

////////////////////////////////////////////////
// Trigonometric
////////////////////////////////////////////////

/*
 * Adapted from Julien Pommier - Copyright (C) 2007 Julien Pommier.
 */

DECLARE_4F_CONST(MinusCephesDP1, -0.78515625f);
DECLARE_4F_CONST(MinusCephesDP2, -2.4187564849853515625e-4f);
DECLARE_4F_CONST(MinusCephesDP3, -3.77489497744594108e-8f);
DECLARE_4F_CONST(SinCofP0, -1.9515295891E-4f);
DECLARE_4F_CONST(SinCofP1, 8.3321608736E-3f);
DECLARE_4F_CONST(SinCofP2, -1.6666654611E-1f);
DECLARE_4F_CONST(CosCofP0, 2.443315711809948E-005f);
DECLARE_4F_CONST(CosCofP1, -1.388731625493765E-003f);
DECLARE_4F_CONST(CosCofP2, 4.166664568298827E-002f);

inline void SinCos4(float const * const xPtr, float * const sPtr, float * const cPtr)
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    __m128 x = _mm_loadu_ps(xPtr);

    __m128 sign_bit_sin = x;
    /* take the absolute value */
    x = _mm_and_ps(x, *(__m128 *)SignMaskInv4i);
    /* extract the sign bit (upper one) */
    sign_bit_sin = _mm_and_ps(sign_bit_sin, *(__m128 *)SignMask4i);

    /* scale by 4/Pi */
    __m128 y = _mm_mul_ps(x, *(__m128 *)FourOverPi4f);

    /* store the integer part of y in emm2 */
    __m128i emm2 = _mm_cvttps_epi32(y);

    /* j=(j+1) & (~1) (see the cephes sources) */
    emm2 = _mm_add_epi32(emm2, *(__m128i *)One4i);
    emm2 = _mm_and_si128(emm2, *(__m128i *)OneInv4i);
    y = _mm_cvtepi32_ps(emm2);

    __m128i emm4 = emm2;

    /* get the swap sign flag for the sine */
    __m128i emm0 = _mm_and_si128(emm2, *(__m128i *)Four4i);
    emm0 = _mm_slli_epi32(emm0, 29);
    __m128 swap_sign_bit_sin = _mm_castsi128_ps(emm0);

    /* get the polynom selection mask for the sine*/
    emm2 = _mm_and_si128(emm2, *(__m128i *)Two4i);
    emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());
    __m128 poly_mask = _mm_castsi128_ps(emm2);

    /* The magic pass: "Extended precision modular arithmetic"
       x = ((x - y * DP1) - y * DP2) - y * DP3; */
    __m128 xmm1 = *(__m128 *)MinusCephesDP14f;
    __m128 xmm2 = *(__m128 *)MinusCephesDP24f;
    __m128 xmm3 = *(__m128 *)MinusCephesDP34f;
    xmm1 = _mm_mul_ps(y, xmm1);
    xmm2 = _mm_mul_ps(y, xmm2);
    xmm3 = _mm_mul_ps(y, xmm3);
    x = _mm_add_ps(x, xmm1);
    x = _mm_add_ps(x, xmm2);
    x = _mm_add_ps(x, xmm3);

    emm4 = _mm_sub_epi32(emm4, *(__m128i *)Two4i);
    emm4 = _mm_andnot_si128(emm4, *(__m128i *)Four4i);
    emm4 = _mm_slli_epi32(emm4, 29);
    __m128 sign_bit_cos = _mm_castsi128_ps(emm4);

    sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);

    /* Evaluate the first polynom  (0 <= x <= Pi/4) */
    __m128 z = _mm_mul_ps(x, x);
    y = *(__m128 *)CosCofP04f;

    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128 *)CosCofP14f);
    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128 *)CosCofP24f);
    y = _mm_mul_ps(y, z);
    y = _mm_mul_ps(y, z);
    __m128 tmp = _mm_mul_ps(z, *(__m128 *)ZeroPointFive4f);
    y = _mm_sub_ps(y, tmp);
    y = _mm_add_ps(y, *(__m128 *)One4f);

    /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

    __m128 y2 = *(__m128 *)SinCofP04f;
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128 *)SinCofP14f);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128 *)SinCofP24f);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_mul_ps(y2, x);
    y2 = _mm_add_ps(y2, x);

    /* select the correct result from the two polynoms */
    xmm3 = poly_mask;
    __m128 ysin2 = _mm_and_ps(xmm3, y2);
    __m128 ysin1 = _mm_andnot_ps(xmm3, y);
    y2 = _mm_sub_ps(y2, ysin2);
    y = _mm_sub_ps(y, ysin1);

    xmm1 = _mm_add_ps(ysin1, ysin2);
    xmm2 = _mm_add_ps(y, y2);

    /* update the sign */
    _mm_storeu_ps(sPtr, _mm_xor_ps(xmm1, sign_bit_sin));
    _mm_storeu_ps(cPtr, _mm_xor_ps(xmm2, sign_bit_cos));
#else
    sPtr[0] = std::sinf(xPtr[0]);
    sPtr[1] = std::sinf(xPtr[1]);
    sPtr[2] = std::sinf(xPtr[2]);
    sPtr[3] = std::sinf(xPtr[3]);

    cPtr[0] = std::cosf(xPtr[0]);
    cPtr[1] = std::cosf(xPtr[1]);
    cPtr[2] = std::cosf(xPtr[2]);
    cPtr[3] = std::cosf(xPtr[3]);
#endif
}
