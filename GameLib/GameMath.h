/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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
inline int32_t FastFloorInt32(float value) noexcept
{
    return _mm_cvtt_ss2si(_mm_load_ss(&value));
}