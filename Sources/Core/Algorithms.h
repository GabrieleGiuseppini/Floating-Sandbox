/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AABB.h"
#include "GameMath.h"
#include "GameTypes.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <optional>
#include <vector>

namespace Algorithms {

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffuseLight
///////////////////////////////////////////////////////////////////////////////////////////////////////

// Currently unused - just by benchmarks
inline void DiffuseLight_Naive(
    vec2f const * pointPositions,
    PlaneId const * pointPlaneIds,
    ElementIndex const pointCount,
    vec2f const * lampPositions,
    PlaneId const * lampPlaneIds,
    float const * lampDistanceCoeffs,
    float const * lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer) noexcept
{
    for (ElementIndex p = 0; p < pointCount; ++p)
    {
        auto const pointPosition = pointPositions[p];
        auto const pointPlane = pointPlaneIds[p];

        float pointLight = 0.0f;

        // Go through all lamps;
        // can safely visit deleted lamps as their current will always be zero
        for (ElementIndex l = 0; l < lampCount; ++l)
        {
            if (pointPlane <= lampPlaneIds[l])
            {
                float const distance = (pointPosition - lampPositions[l]).length();

                // Light from this lamp = max(0.0, lum*(spread-distance)/spread)
                float const newLight =
                    lampDistanceCoeffs[l]
                    * (lampSpreadMaxDistances[l] - distance); // If negative, max(.) below will clamp down to 0.0

                // Point's light is just max, to avoid having to normalize everything to 1.0
                pointLight = std::max(
                    newLight,
                    pointLight);
            }
        }

        // Cap light to 1.0
        outLightBuffer[p] = std::min(1.0f, pointLight);
    }
}

inline void DiffuseLight_Vectorized(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    vec2f const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    vec2f const * restrict lampPositions,
    PlaneId const * restrict lampPlaneIds,
    float const * restrict lampDistanceCoeffs,
    float const * restrict lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer) noexcept
{
    // This code is vectorized for 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);
    assert(is_aligned_to_float_element_count(pointStart));
    assert(is_aligned_to_float_element_count(pointEnd));
    assert(is_aligned_to_float_element_count(lampCount));
    assert(is_aligned_to_vectorization_word(pointPositions));
    assert(is_aligned_to_vectorization_word(pointPlaneIds));
    assert(is_aligned_to_vectorization_word(lampPositions));
    assert(is_aligned_to_vectorization_word(lampPlaneIds));
    assert(is_aligned_to_vectorization_word(lampDistanceCoeffs));
    assert(is_aligned_to_vectorization_word(lampSpreadMaxDistances));
    assert(is_aligned_to_vectorization_word(outLightBuffer));

    // Caller is assumed to have skipped this when there are no lamps
    assert(lampCount > 0);

    // Clear all output lights
    std::fill(
        outLightBuffer + pointStart,
        outLightBuffer + pointEnd,
        0.0f);

    //
    // Visit all points, in groups of 4
    //

    for (ElementIndex p = pointStart; p < pointEnd; p += 4)
    {
        vec2f const * const restrict batchPointPositions = &(pointPositions[p]);
        PlaneId const * const restrict batchPointPlaneIds = &(pointPlaneIds[p]);
        float * const restrict batchOutLightBuffer = &(outLightBuffer[p]);

        //
        // Go through all lamps;
        // can safely visit deleted lamps as their current will always be zero
        //

        for (ElementIndex l = 0; l < lampCount; ++l)
        {
            // Calculate distances
            std::array<float, 4> tmpPointDistances;
            for (ElementIndex p2 = 0; p2 < 4; ++p2)
                tmpPointDistances[p2] = (batchPointPositions[p2] - lampPositions[l]).length();

            // Light from this lamp = max(0.0, lum*(spread-distance)/spread)
            for (ElementIndex p2 = 0; p2 < 4; ++p2)
            {
                float newLight =
                    lampDistanceCoeffs[l]
                    * (lampSpreadMaxDistances[l] - tmpPointDistances[p2]); // If negative, max(.) below will clamp down to 0.0

                // Obey plane ID constraints
                if (batchPointPlaneIds[p2] > lampPlaneIds[l])
                    newLight = 0.0f;

                batchOutLightBuffer[p2] = std::max(
                    newLight,
                    batchOutLightBuffer[p2]);
            }
        }

        //
        // Cap output lights
        //

        for (ElementIndex p2 = 0; p2 < 4; ++p2)
        {
            batchOutLightBuffer[p2] = std::min(1.0f, batchOutLightBuffer[p2]);
        }
    }
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
inline void DiffuseLight_SSEVectorized(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    vec2f const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    vec2f const * restrict lampPositions,
    PlaneId const * restrict lampPlaneIds,
    float const * restrict lampDistanceCoeffs,
    float const * restrict lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer) noexcept
{
    // This code is vectorized for SSE = 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);
    assert(is_aligned_to_float_element_count(pointStart));
    assert(is_aligned_to_float_element_count(pointEnd));
    assert(is_aligned_to_float_element_count(lampCount));
    assert(is_aligned_to_vectorization_word(pointPositions));
    assert(is_aligned_to_vectorization_word(pointPlaneIds));
    assert(is_aligned_to_vectorization_word(lampPositions));
    assert(is_aligned_to_vectorization_word(lampPlaneIds));
    assert(is_aligned_to_vectorization_word(lampDistanceCoeffs));
    assert(is_aligned_to_vectorization_word(lampSpreadMaxDistances));
    assert(is_aligned_to_vectorization_word(outLightBuffer));

    // Caller is assumed to have skipped this when there are no lamps
    assert(lampCount > 0);

    //
    // Visit all points in groups of 4
    //

    for (ElementIndex p = pointStart; p < pointEnd; p += 4)
    {
        //
        // Prepare point data at slots 0,1,2,3
        //

        // Point positions
        __m128 const pointPos01_4 = _mm_load_ps(reinterpret_cast<float const *>(pointPositions + p)); // x0,y0,x1,y1
        __m128 const pointPos23_4 = _mm_load_ps(reinterpret_cast<float const *>(pointPositions + p + 2)); // x2,y2,x3,y3
        __m128 pointPosX_4 = _mm_shuffle_ps(pointPos01_4, pointPos23_4, _MM_SHUFFLE(2, 0, 2, 0)); // x0,x1,x2,x3
        __m128 pointPosY_4 = _mm_shuffle_ps(pointPos01_4, pointPos23_4, _MM_SHUFFLE(3, 1, 3, 1)); // y0,y1,y2,y3

        // Point planes
        __m128i pointPlaneId_4 = _mm_load_si128(reinterpret_cast<__m128i const *>(pointPlaneIds + p)); // 0,1,2,3

        // Resultant point light
        __m128 pointLight_4 = _mm_setzero_ps();

        //
        // Go through all lamps, 4 by 4;
        // can safely visit deleted lamps as their current will always be zero
        //

        for (ElementIndex l = 0; l < lampCount; l += 4)
        {
            // Lamp positions
            __m128 const lampPos01_4 = _mm_load_ps(reinterpret_cast<float const *>(lampPositions + l)); // x0,y0,x1,y1
            __m128 const lampPos23_4 = _mm_load_ps(reinterpret_cast<float const *>(lampPositions + l + 2)); // x2,y2,x3,y3
            __m128 const lampPosX_4 = _mm_shuffle_ps(lampPos01_4, lampPos23_4, _MM_SHUFFLE(2, 0, 2, 0)); // x0,x1,x2,x3
            __m128 const lampPosY_4 = _mm_shuffle_ps(lampPos01_4, lampPos23_4, _MM_SHUFFLE(3, 1, 3, 1)); // y0,y1,y2,y3

            // Lamp planes
            __m128i lampPlaneId_4 = _mm_load_si128(reinterpret_cast<__m128i const *>(lampPlaneIds + l)); // 0,1,2,3

            // Coeffs
            __m128 const lampDistanceCoeff_4 = _mm_load_ps(lampDistanceCoeffs + l);
            __m128 const lampSpreadMaxDistance_4 = _mm_load_ps(lampSpreadMaxDistances + l);

            //
            // We now perform the following four times, each time rotating the 4 points around the four slots
            // of their registers:
            //  distance = pointPosition - lampPosition
            //  newLight = lampDistanceCoeff * (lampSpreadMaxDistance - distance)
            //  pointLight = max(newLight, pointLight) // Just max, to avoid having to normalize everything to 1.0
            //

            for (int rot = 0; rot < 4; ++rot)
            {
                // Calculate distance
                __m128 displacementX_4 = _mm_sub_ps(pointPosX_4, lampPosX_4);
                __m128 displacementY_4 = _mm_sub_ps(pointPosY_4, lampPosY_4);
                __m128 distanceSquare_4 = _mm_add_ps(
                    _mm_mul_ps(displacementX_4, displacementX_4),
                    _mm_mul_ps(displacementY_4, displacementY_4));
                __m128 distance_4 = _mm_sqrt_ps(distanceSquare_4);

                // Calculate new light
                __m128 newLight_4 = _mm_mul_ps(
                    lampDistanceCoeff_4,
                    _mm_sub_ps(lampSpreadMaxDistance_4, distance_4));

                // Mask with plane ID
                __m128i planeMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
                newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(planeMask), newLight_4);

                // Point light
                pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);

                // Rotate: 0,1,2,3 -> 1,2,3,0
                pointPosX_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosX_4), _MM_SHUFFLE(0, 3, 2, 1)));
                pointPosY_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosY_4), _MM_SHUFFLE(0, 3, 2, 1)));
                pointPlaneId_4 = _mm_shuffle_epi32(pointPlaneId_4, _MM_SHUFFLE(0, 3, 2, 1));
                pointLight_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointLight_4), _MM_SHUFFLE(0, 3, 2, 1)));
            }
        }

        //
        // Store the 4 point lights, capping them to 1.0
        //

        pointLight_4 = _mm_min_ps(pointLight_4, *(__m128*)One4f);
        _mm_store_ps(outLightBuffer + p, pointLight_4);
    }
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
inline void DiffuseLight_NeonVectorized(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    vec2f const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    vec2f const * restrict lampPositions,
    PlaneId const * restrict lampPlaneIds,
    float const * restrict lampDistanceCoeffs,
    float const * restrict lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer) noexcept
{
    // This implementation is for 4-float vectorization
    static_assert(vectorization_float_count<size_t> >= 4);
    assert((pointStart % 4) == 0);
    assert((pointEnd % 4) == 0);
    assert((lampCount % 4) == 0);
    assert(is_aligned_to_vectorization_word(pointPositions));
    assert(is_aligned_to_vectorization_word(pointPlaneIds));
    assert(is_aligned_to_vectorization_word(lampPositions));
    assert(is_aligned_to_vectorization_word(lampPlaneIds));
    assert(is_aligned_to_vectorization_word(lampDistanceCoeffs));
    assert(is_aligned_to_vectorization_word(lampSpreadMaxDistances));
    assert(is_aligned_to_vectorization_word(outLightBuffer));

    // Caller is assumed to have skipped this when there are no lamps
    assert(lampCount > 0);

    float32x4_t const zero_4 = vdupq_n_f32(0.0f);
    float32x4_t const one_4 = vdupq_n_f32(1.0f);

    //
    // Visit all points in groups of 4
    //

    for (ElementIndex p = pointStart; p < pointEnd; p += 4)
    {
        //
        // Prepare point data
        //

        // Load point positions
        float32x4x2_t pointPos01020304_xxxx_yyyy = vld2q_f32(reinterpret_cast<float const *>(pointPositions + p));

        // Load point planes
        uint32x4_t pointPln01020304 = vld1q_u32(reinterpret_cast<std::uint32_t const *>(pointPlaneIds + p));

        // Resultant point light
        float32x4_t pointLgt01020304 = zero_4;

        //
        // Go through all lamps, 4 by 4
        // can safely visit deleted lamps as their current will always be zero
        //

        for (ElementIndex l = 0; l < lampCount; l += 4)
        {
            // Load lamp positions
            float32x4x2_t const lampPos01020304_xxxx_yyyy = vld2q_f32(reinterpret_cast<float const *>(lampPositions + l));

            // Load lamp planes
            uint32x4_t const lampPln01020304 = vld1q_u32(reinterpret_cast<std::uint32_t const *>(lampPlaneIds + l));

            // Load lamp coeffs
            float32x4_t const lampDistanceCoeff01020304 = vld1q_f32(reinterpret_cast<float const *>(lampDistanceCoeffs + l));
            float32x4_t const lampSpreadMaxDistance01020304 = vld1q_f32(reinterpret_cast<float const *>(lampSpreadMaxDistances + l));

            //
            // We now perform the following four times, each time rotating the 4 points around the four slots
            // of their registers:
            //  distance = pointPosition - lampPosition
            //  newLight = lampDistanceCoeff * (lampSpreadMaxDistance - distance)
            //  pointLight = max(newLight, pointLight) // Just max, to avoid having to normalize everything to 1.0
            //

            for (int rot = 0; rot < 4; ++rot)
            {
                // Calculate distance

                float32x4_t displacementX = vsubq_f32(
                    pointPos01020304_xxxx_yyyy.val[0],
                    lampPos01020304_xxxx_yyyy.val[0]);
                float32x4_t displacementY = vsubq_f32(
                    pointPos01020304_xxxx_yyyy.val[1],
                    lampPos01020304_xxxx_yyyy.val[1]);
                float32x4_t distanceSquare_4 = vaddq_f32(
                    vmulq_f32(displacementX, displacementX),
                    vmulq_f32(displacementY, displacementY));

                uint32x4_t const validMask = vcgtq_f32(distanceSquare_4, zero_4); // Valid where > 0 (distance is always >= 0)

                // Zero newtown-rhapson steps, it's for lighting after all
                float32x4_t distance_4_inv = vrsqrteq_f32(distanceSquare_4);

                // Zero newtown-rhapson steps, it's for lighting after all
                float32x4_t distance_4 = vrecpeq_f32(distance_4_inv);

                distance_4 =
                    vandq_u32(
                        distance_4,
                        validMask);

                // Calculate new light
                float32x4_t newLight_4 = vmulq_f32(
                    lampDistanceCoeff01020304,
                    vsubq_f32(lampSpreadMaxDistance01020304, distance_4));

                // Mask with plane ID
                uint32x4_t const planeMask = vcleq_u32(pointPln01020304, lampPln01020304);
                newLight_4 = vandq_u32(newLight_4, planeMask);

                // Point light
                pointLgt01020304 = vmaxq_f32(pointLgt01020304, newLight_4);

                // Rotate: 0,1,2,3 -> 1,2,3,0
                pointPos01020304_xxxx_yyyy.val[0] = vextq_f32(
                    pointPos01020304_xxxx_yyyy.val[0],
                    pointPos01020304_xxxx_yyyy.val[0],
                    1);
                pointPos01020304_xxxx_yyyy.val[1] = vextq_f32(
                    pointPos01020304_xxxx_yyyy.val[1],
                    pointPos01020304_xxxx_yyyy.val[1],
                    1);
                pointPln01020304 = vextq_f32(pointPln01020304, pointPln01020304, 1);
                pointLgt01020304 = vextq_f32(pointLgt01020304, pointLgt01020304, 1);
            }
        }

        //
        // Store the 4 point lights, capping them to 1.0
        //

        vst1q_f32(outLightBuffer + p, vminq_f32(pointLgt01020304, one_4));
    }
}
#endif

/*
 * Diffuse light from each lamp to all points on the same or lower plane ID,
 * inverse-proportionally to the lamp-point distance
 */
inline void DiffuseLight(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    vec2f const * pointPositions,
    PlaneId const * pointPlaneIds,
    vec2f const * lampPositions,
    PlaneId const * lampPlaneIds,
    float const * lampDistanceCoeffs,
    float const * lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer) noexcept
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    DiffuseLight_SSEVectorized(
        pointStart,
        pointEnd,
        pointPositions,
        pointPlaneIds,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        lampCount,
        outLightBuffer);
#elif FS_IS_ARM_NEON()
    DiffuseLight_NeonVectorized(
        pointStart,
        pointEnd,
        pointPositions,
        pointPlaneIds,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        lampCount,
        outLightBuffer);
#else
    DiffuseLight_Vectorized(
        pointStart,
        pointEnd,
        pointPositions,
        pointPlaneIds,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        lampCount,
        outLightBuffer);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferSmoothing
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<size_t BufferSize, size_t SmoothingSize>
inline void SmoothBufferAndAdd_Naive(
    float const * restrict inBuffer,
    float * restrict outBuffer) noexcept
{
    static_assert((SmoothingSize % 2) == 1);

    for (size_t i = 0; i < BufferSize; ++i)
    {
        // Central sample
        float accumulatedHeight = inBuffer[i] * static_cast<float>((SmoothingSize / 2) + 1);

        // Lateral samples; l is offset from central
        for (size_t l = 1; l <= SmoothingSize / 2; ++l)
        {
            float const lateralWeight = static_cast<float>((SmoothingSize / 2) + 1 - l);

            accumulatedHeight +=
                inBuffer[i - l] * lateralWeight
                + inBuffer[i + l] * lateralWeight;
        }

        // Update height field
        outBuffer[i] +=
            (1.0f / static_cast<float>(SmoothingSize))
            * (1.0f / static_cast<float>(SmoothingSize))
            * accumulatedHeight;
    }
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<size_t BufferSize, size_t SmoothingSize>
inline void SmoothBufferAndAdd_SSEVectorized(
    float const * restrict inBuffer,
    float * restrict outBuffer) noexcept
{
    // This code is vectorized for SSE = 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);
    static_assert(is_aligned_to_float_element_count(BufferSize));
    static_assert((SmoothingSize % 2) == 1);
    assert(is_aligned_to_vectorization_word(inBuffer));
    assert(is_aligned_to_vectorization_word(outBuffer));

    __m128 const centralWeight = _mm_set_ps1(static_cast<float>((SmoothingSize / 2) + 1));
    __m128 const scaling = _mm_set_ps1(
        (1.0f / static_cast<float>(SmoothingSize))
        * (1.0f / static_cast<float>(SmoothingSize)));

    for (size_t i = 0; i < BufferSize; i += 4)
    {
        // Central sample
        __m128 accumulatedHeight = _mm_mul_ps(
            _mm_load_ps(inBuffer + i),
            centralWeight);

        // Lateral samples; l is offset from central
        for (size_t l = 1; l <= SmoothingSize / 2; ++l)
        {
            __m128 const lateralWeight = _mm_set_ps1(static_cast<float>((SmoothingSize / 2) + 1 - l));

            accumulatedHeight = _mm_add_ps(
                accumulatedHeight,
                _mm_mul_ps(
                    _mm_add_ps(
                        _mm_loadu_ps(inBuffer + i - l),
                        _mm_loadu_ps(inBuffer + i + l)),
                    lateralWeight));
        }

        // Update output
        _mm_store_ps(
            outBuffer + i,
            _mm_add_ps(
                _mm_load_ps(outBuffer + i),
                _mm_mul_ps(
                    accumulatedHeight,
                    scaling)));
    }
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
template<size_t BufferSize, size_t SmoothingSize>
inline void SmoothBufferAndAdd_NeonVectorized(
    float const * restrict inBuffer,
    float * restrict outBuffer) noexcept
{
    // This code is vectorized for Neon = 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);
    static_assert(is_aligned_to_float_element_count(BufferSize));
    static_assert((SmoothingSize % 2) == 1);
    assert(is_aligned_to_vectorization_word(inBuffer));
    assert(is_aligned_to_vectorization_word(outBuffer));

    float32x4_t const centralWeight = vdupq_n_f32(static_cast<float>((SmoothingSize / 2) + 1));
    float32x4_t const scaling = vdupq_n_f32(
        (1.0f / static_cast<float>(SmoothingSize))
        * (1.0f / static_cast<float>(SmoothingSize)));

    for (size_t i = 0; i < BufferSize; i += 4)
    {
        // Central sample
        float32x4_t accumulatedHeight = vmulq_f32(
            vld1q_f32(inBuffer + i),
            centralWeight);

        // Lateral samples; l is offset from central
        for (size_t l = 1; l <= SmoothingSize / 2; ++l)
        {
            float32x4_t const lateralWeight = vdupq_n_f32(static_cast<float>((SmoothingSize / 2) + 1 - l));

            accumulatedHeight = vmlaq_f32(
                accumulatedHeight,
                vaddq_f32(
                        vld1q_f32(inBuffer + i - l),
                        vld1q_f32(inBuffer + i + l)),
                lateralWeight);
        }

        // Update output
        vst1q_f32(
            outBuffer + i,
            vmlaq_f32(
                vld1q_f32(outBuffer + i),
                accumulatedHeight,
                scaling));
    }
}
#endif

/*
 * Calculates a two-pass average on a window of width SmoothingSize,
 * centered on the sample.
 *
 * The input buffer is assumed to be extended left and right - outside of the BufferSize - with zeroes.
 */
template<size_t BufferSize, size_t SmoothingSize>
inline void SmoothBufferAndAdd(
    float const * restrict inBuffer,
    float * restrict outBuffer) noexcept
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    SmoothBufferAndAdd_SSEVectorized<BufferSize, SmoothingSize>(inBuffer, outBuffer);
#elif FS_IS_ARM_NEON()
    SmoothBufferAndAdd_NeonVectorized<BufferSize, SmoothingSize>(inBuffer, outBuffer);
#else
    SmoothBufferAndAdd_Naive<BufferSize, SmoothingSize>(inBuffer, outBuffer);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateSpringVectors
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TEndpoints>
inline void CalculateSpringVectors_Naive(
    ElementIndex springIndex,
    vec2f const * restrict const positionBuffer,
    TEndpoints const * restrict const endpointsBuffer,
    float * restrict const outCachedLengthBuffer,
    vec2f * restrict const outCachedNormalizedVectorBuffer)
{
    for (size_t s = springIndex; s < springIndex + 4; ++s)
    {
        vec2f const dis = (positionBuffer[endpointsBuffer[s].PointBIndex] - positionBuffer[endpointsBuffer[s].PointAIndex]);
        float const springLength = dis.length();
        outCachedLengthBuffer[s] = springLength;
        outCachedNormalizedVectorBuffer[s] = dis.normalise_approx(springLength);
    }
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename TEndpoints>
inline void CalculateSpringVectors_SSEVectorized(
    ElementIndex springIndex,
    vec2f const * restrict const positionBuffer,
    TEndpoints const * restrict const endpointsBuffer,
    float * restrict const outCachedLengthBuffer,
    vec2f * restrict const outCachedNormalizedVectorBuffer)
{
    // This code is vectorized for at least 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);

    __m128 const Zero = _mm_setzero_ps();

    // Spring 0 displacement (s0_position.x, s0_position.y, *, *)
    __m128 const s0pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 0].PointAIndex)));
    __m128 const s0pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 0].PointBIndex)));
    // s0_displacement.x, s0_displacement.y, *, *
    __m128 const s0_displacement_xy = _mm_sub_ps(s0pb_pos_xy, s0pa_pos_xy);

    // Spring 1 displacement (s1_position.x, s1_position.y, *, *)
    __m128 const s1pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 1].PointAIndex)));
    __m128 const s1pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 1].PointBIndex)));
    // s1_displacement.x, s1_displacement.y
    __m128 const s1_displacement_xy = _mm_sub_ps(s1pb_pos_xy, s1pa_pos_xy);

    // s0_displacement.x, s0_displacement.y, s1_displacement.x, s1_displacement.y
    __m128 const s0s1_displacement_xy = _mm_movelh_ps(s0_displacement_xy, s1_displacement_xy); // First argument goes low

    // Spring 2 displacement (s2_position.x, s2_position.y, *, *)
    __m128 const s2pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 2].PointAIndex)));
    __m128 const s2pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 2].PointBIndex)));
    // s2_displacement.x, s2_displacement.y
    __m128 const s2_displacement_xy = _mm_sub_ps(s2pb_pos_xy, s2pa_pos_xy);

    // Spring 3 displacement (s3_position.x, s3_position.y, *, *)
    __m128 const s3pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 3].PointAIndex)));
    __m128 const s3pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[springIndex + 3].PointBIndex)));
    // s3_displacement.x, s3_displacement.y
    __m128 const s3_displacement_xy = _mm_sub_ps(s3pb_pos_xy, s3pa_pos_xy);

    // s2_displacement.x, s2_displacement.y, s3_displacement.x, s3_displacement.y
    __m128 const s2s3_displacement_xy = _mm_movelh_ps(s2_displacement_xy, s3_displacement_xy); // First argument goes low

    // Shuffle displacements:
    // s0_displacement.x, s1_displacement.x, s2_displacement.x, s3_displacement.x
    __m128 s0s1s2s3_displacement_x = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0x88);
    // s0_displacement.y, s1_displacement.y, s2_displacement.y, s3_displacement.y
    __m128 s0s1s2s3_displacement_y = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0xDD);

    // Calculate spring lengths

    // s0_displacement.x^2, s1_displacement.x^2, s2_displacement.x^2, s3_displacement.x^2
    __m128 const s0s1s2s3_displacement_x2 = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_displacement_x);
    // s0_displacement.y^2, s1_displacement.y^2, s2_displacement.y^2, s3_displacement.y^2
    __m128 const s0s1s2s3_displacement_y2 = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_displacement_y);

    // s0_displacement.x^2 + s0_displacement.y^2, s1_displacement.x^2 + s1_displacement.y^2, s2_displacement..., s3_displacement...
    __m128 const s0s1s2s3_displacement_x2_p_y2 = _mm_add_ps(s0s1s2s3_displacement_x2, s0s1s2s3_displacement_y2);

    __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_displacement_x2_p_y2, Zero);

    __m128 const s0s1s2s3_springLength_inv =
        _mm_and_ps(
            _mm_rsqrt_ps(s0s1s2s3_displacement_x2_p_y2),
            validMask);

    __m128 const s0s1s2s3_springLength =
        _mm_and_ps(
            _mm_rcp_ps(s0s1s2s3_springLength_inv),
            validMask);

    // Store length
    _mm_store_ps(outCachedLengthBuffer + springIndex, s0s1s2s3_springLength);

    // Calculate spring directions
    __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength_inv);
    __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength_inv);

    // Store directions
    __m128 s0s1_sdir_xy = _mm_unpacklo_ps(s0s1s2s3_sdir_x, s0s1s2s3_sdir_y); // a[0], b[0], a[1], b[1]
    __m128 s2s3_sdir_xy = _mm_unpackhi_ps(s0s1s2s3_sdir_x, s0s1s2s3_sdir_y); // a[2], b[2], a[3], b[3]
    _mm_store_ps(reinterpret_cast<float *>(outCachedNormalizedVectorBuffer + springIndex), s0s1_sdir_xy);
    _mm_store_ps(reinterpret_cast<float *>(outCachedNormalizedVectorBuffer + springIndex + 2), s2s3_sdir_xy);
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
template<typename TEndpoints>
inline void CalculateSpringVectors_NeonVectorized(
    ElementIndex springIndex,
    vec2f const * restrict const positionBuffer,
    TEndpoints const * restrict const endpointsBuffer,
    float * restrict const outCachedLengthBuffer,
    vec2f * restrict const outCachedNormalizedVectorBuffer)
{
    // This code is vectorized for at least 4 floats
    static_assert(vectorization_float_count<size_t> >= 4);

    float32x4_t const Zero = vdupq_n_f32(0.0f);

    //
    // Calculate displacements, string lengths, and spring directions
    //

    float32x2_t const s0pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 0].PointAIndex));
    float32x2_t const s0pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 0].PointBIndex));
    float32x2_t const s0_dis_xy = vsub_f32(s0pb_pos_xy, s0pa_pos_xy);

    float32x2_t const s1pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 1].PointAIndex));
    float32x2_t const s1pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 1].PointBIndex));
    float32x2_t const s1_dis_xy = vsub_f32(s1pb_pos_xy, s1pa_pos_xy);

    float32x2_t const s2pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 2].PointAIndex));
    float32x2_t const s2pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 2].PointBIndex));
    float32x2_t const s2_dis_xy = vsub_f32(s2pb_pos_xy, s2pa_pos_xy);

    float32x2_t const s3pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 3].PointAIndex));
    float32x2_t const s3pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[springIndex + 3].PointBIndex));
    float32x2_t const s3_dis_xy = vsub_f32(s3pb_pos_xy, s3pa_pos_xy);

    // Combine all into xxxx,yyyyy

    float32x4_t const s0s2_dis_xyxy = vcombine_f32(s0_dis_xy, s2_dis_xy);
    float32x4_t const s1s3_dis_xyxy = vcombine_f32(s1_dis_xy, s3_dis_xy);
    float32x4x2_t const s0s1s2s3_dis_xxxx_yyyy = vtrnq_f32(s0s2_dis_xyxy, s1s3_dis_xyxy);

    // Calculate spring lengths: sqrt( x*x + y*y )

    float32x4_t const sq_len =
        vaddq_f32(
            vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[0], s0s1s2s3_dis_xxxx_yyyy.val[0]),
            vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[1], s0s1s2s3_dis_xxxx_yyyy.val[1]));

    uint32x4_t const validMask = vcgtq_f32(sq_len, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f

    // One newtown-rhapson step
    float32x4_t s0s1s2s3_springLength_inv = vrsqrteq_f32(sq_len);
    s0s1s2s3_springLength_inv = vmulq_f32(
        s0s1s2s3_springLength_inv,
        vrsqrtsq_f32(
            vmulq_f32(sq_len, s0s1s2s3_springLength_inv),
            s0s1s2s3_springLength_inv));

    s0s1s2s3_springLength_inv =
        vandq_u32(
            s0s1s2s3_springLength_inv,
            validMask);

    // One newtown-rhapson step
    float32x4_t s0s1s2s3_springLength = vrecpeq_f32(s0s1s2s3_springLength_inv);
    s0s1s2s3_springLength = vmulq_f32(
        s0s1s2s3_springLength,
        vrecpsq_f32(
            s0s1s2s3_springLength_inv,
            s0s1s2s3_springLength));

    s0s1s2s3_springLength =
        vandq_u32(
            s0s1s2s3_springLength,
            validMask);

    // Store
    vst1q_f32(outCachedLengthBuffer + springIndex, s0s1s2s3_springLength);

    // Calculate spring directions

    float32x4_t const s0s1s2s3_sdir_x = vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[0], s0s1s2s3_springLength_inv);
    float32x4_t const s0s1s2s3_sdir_y = vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[1], s0s1s2s3_springLength_inv);

    // Store directions
    float32x4x2_t const s0xy_s1xy_s2xy_s3xy = vzipq_f32(s0s1s2s3_sdir_x, s0s1s2s3_sdir_y);
    vst1q_f32(reinterpret_cast<float *>(outCachedNormalizedVectorBuffer + springIndex), s0xy_s1xy_s2xy_s3xy.val[0]);
    vst1q_f32(reinterpret_cast<float *>(outCachedNormalizedVectorBuffer + springIndex + 2), s0xy_s1xy_s2xy_s3xy.val[1]);
}
#endif

template<typename TEndpoints>
inline void CalculateSpringVectors(
    ElementIndex springIndex,
    vec2f const * restrict const positionBuffer,
    TEndpoints const * restrict const endpointsBuffer,
    float * restrict const outCachedLengthBuffer,
    vec2f * restrict const cachedNormalizedVectorBuffer)
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    CalculateSpringVectors_SSEVectorized<TEndpoints>(springIndex, positionBuffer, endpointsBuffer, outCachedLengthBuffer, cachedNormalizedVectorBuffer);
#elif FS_IS_ARM_NEON()
    CalculateSpringVectors_NeonVectorized<TEndpoints>(springIndex, positionBuffer, endpointsBuffer, outCachedLengthBuffer, cachedNormalizedVectorBuffer);
#else
    CalculateSpringVectors_Naive<TEndpoints>(springIndex, positionBuffer, endpointsBuffer, outCachedLengthBuffer, cachedNormalizedVectorBuffer);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// IntegrateAndResetDynamicForces
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TPoints>
inline void IntegrateAndResetDynamicForces_Naive(
    TPoints & points,
    size_t nBuffers,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    float * const restrict * dynamicForceBuffers,
    float dt,
    float velocityFactor) noexcept
{
    //
    // This loop is compiled with packed SSE instructions on MSVC 2022,
    // integrating two points at each iteration
    //
    // We loop by floats
    //

    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible

    float * restrict const positionBuffer = points.GetPositionBufferAsFloat() + startPointIndex * 2;
    float * restrict const velocityBuffer = points.GetVelocityBufferAsFloat() + startPointIndex * 2;
    float const * const restrict staticForceBuffer = points.GetStaticForceBufferAsFloat() + startPointIndex * 2;
    float const * const restrict integrationFactorBuffer = points.GetIntegrationFactorBufferAsFloat() + startPointIndex * 2;

    size_t const count = (endPointIndex - startPointIndex) * 2;
    for (size_t i = 0; i < count; ++i)
    {
        float totalDynamicForce = 0.0f;
        for (size_t b = 0; b < nBuffers; ++b)
        {
            totalDynamicForce += (dynamicForceBuffers[b] + startPointIndex * 2)[i];
        }

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (totalDynamicForce + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        for (size_t b = 0; b < nBuffers; ++b)
        {
            (dynamicForceBuffers[b] + startPointIndex * 2)[i] = 0.0f;
        }
    }
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename TPoints>
inline void IntegrateAndResetDynamicForces_SSEVectorized(
    TPoints & points,
    size_t nBuffers,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    float * const restrict * dynamicForceBuffers,
    float dt,
    float velocityFactor) noexcept
{
    // This implementation is for 4-float SSE
    static_assert(vectorization_float_count<int> >= 4);

    assert(((endPointIndex - startPointIndex) % 2) == 0);

    float * restrict const positionBuffer = points.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = points.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = points.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = points.GetIntegrationFactorBufferAsFloat();

    float * const restrict * restrict const dynamicForceBufferOfBuffers = dynamicForceBuffers;

    __m128 const zero_4 = _mm_setzero_ps();
    __m128 const dt_4 = _mm_load1_ps(&dt);
    __m128 const velocityFactor_4 = _mm_load1_ps(&velocityFactor);

    for (size_t i = startPointIndex * 2; i < endPointIndex * 2; i += 4) // Two components per vector
    {
        __m128 springForce_2 = zero_4;
        for (size_t b = 0; b < nBuffers; ++b)
        {
            springForce_2 =
                _mm_add_ps(
                    springForce_2,
                    _mm_load_ps(dynamicForceBufferOfBuffers[b] + i));
        }

        // vec2f const deltaPos =
        //    velocityBuffer[i] * dt
        //    + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];
        __m128 const deltaPos_2 =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_load_ps(velocityBuffer + i),
                    dt_4),
                _mm_mul_ps(
                    _mm_add_ps(
                        springForce_2,
                        _mm_load_ps(staticForceBuffer + i)),
                    _mm_load_ps(integrationFactorBuffer + i)));

        // positionBuffer[i] += deltaPos;
        __m128 pos_2 = _mm_load_ps(positionBuffer + i);
        pos_2 = _mm_add_ps(pos_2, deltaPos_2);
        _mm_store_ps(positionBuffer + i, pos_2);

        // velocityBuffer[i] = deltaPos * velocityFactor;
        __m128 const vel_2 =
            _mm_mul_ps(
                deltaPos_2,
                velocityFactor_4);
        _mm_store_ps(velocityBuffer + i, vel_2);

        // Zero out spring forces now that we've integrated them
        for (size_t b = 0; b < nBuffers; ++b)
        {
            _mm_store_ps(dynamicForceBufferOfBuffers[b] + i, zero_4);
        }
    }
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
template<typename TPoints>
inline void IntegrateAndResetDynamicForces_NeonVectorized(
    TPoints & points,
    size_t nBuffers,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    float * const restrict * dynamicForceBuffers,
    float dt,
    float velocityFactor) noexcept
{
    // This implementation is for 4-float times 4 elements vectorization
    static_assert(vectorization_float_count<int> >= 4 * 4);
    assert(is_aligned_to_float_element_count(endPointIndex - startPointIndex));

    float * restrict const positionBuffer = points.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = points.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = points.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = points.GetIntegrationFactorBufferAsFloat();

    float * const restrict * restrict const dynamicForceBufferOfBuffers = dynamicForceBuffers;

    float32x4x4_t zero_4_4;
    for (int e = 0; e < 4; ++e)
    {
        zero_4_4.val[e] = vdupq_n_f32(0.0f);
    }
    float32x4_t const dt_4 = vdupq_n_f32(dt);
    float32x4_t const velocityFactor_4 = vdupq_n_f32(velocityFactor);

    for (size_t i = startPointIndex * 2; i < endPointIndex * 2; i += 4 * 4) // Two components per vector, 4 vectors at a time
    {
        // Add spring forces

        float32x4x4_t springForce = zero_4_4;
        for (size_t b = 0; b < nBuffers; ++b)
        {
            float32x4x4_t dynamicForces = vld4q_f32(dynamicForceBufferOfBuffers[b] + i);

            for (int e = 0; e < 4; ++e)
            {
                springForce.val[e] = vaddq_f32(
                    springForce.val[e],
                    dynamicForces.val[e]);
            }
        }

        // Calculate deltaPos =
        //         velocity[i] * dt
        //         + (springForce[i] + staticForce[i]) * integrationFactor[i];

        // Update positions and velocities:
        //      position[i] += deltaPos;
        //      velocity[i] = deltaPos * velocityFactor;

        float32x4x4_t velocity = vld4q_f32(velocityBuffer + i);
        float32x4x4_t staticForce = vld4q_f32(staticForceBuffer + i);
        float32x4x4_t integrationFactor = vld4q_f32(integrationFactorBuffer + i);
        float32x4x4_t position = vld4q_f32(positionBuffer + i);

        for (int e = 0; e < 4; ++e)
        {
            float32x4_t const deltaPos = vaddq_f32(
                vmulq_f32(
                    velocity.val[e],
                    dt_4),
                vmulq_f32(
                    vaddq_f32(
                        springForce.val[e],
                        staticForce.val[e]),
                    integrationFactor.val[e]));

            position.val[e] = vaddq_f32(position.val[e], deltaPos);
            velocity.val[e] = vmulq_f32(deltaPos, velocityFactor_4);
        }

        vst4q_f32(positionBuffer + i, position);
        vst4q_f32(velocityBuffer + i, velocity);

        // Zero out spring forces now that we've integrated them
        for (size_t b = 0; b < nBuffers; ++b)
        {
            vst4q_f32(dynamicForceBufferOfBuffers[b] + i, zero_4_4);
        }
    }
}
#endif

/*
 * Integrates forces and resets dynamic forces.
 */

template<typename TPoints>
inline void IntegrateAndResetDynamicForces(
    TPoints & points,
    size_t nBuffers,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    float * const restrict * dynamicForceBuffers,
    float dt,
    float velocityFactor) noexcept
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    IntegrateAndResetDynamicForces_SSEVectorized<TPoints>(points, nBuffers, startPointIndex, endPointIndex, dynamicForceBuffers, dt, velocityFactor);
#elif FS_IS_ARM_NEON()
    IntegrateAndResetDynamicForces_NeonVectorized<TPoints>(points, nBuffers, startPointIndex, endPointIndex, dynamicForceBuffers, dt, velocityFactor);
#else
    IntegrateAndResetDynamicForces_Naive<TPoints>(points, nBuffers, startPointIndex, endPointIndex, dynamicForceBuffers, dt, velocityFactor);
#endif
}

// Variant with compile-time number of buffers, facilitates loop unrolls
template<typename TPoints, size_t NBuffers>
inline void IntegrateAndResetDynamicForces(
    TPoints & points,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    float * const restrict * dynamicForceBuffers,
    float dt,
    float velocityFactor) noexcept
{
    IntegrateAndResetDynamicForces<TPoints>(
        points,
        NBuffers,
        startPointIndex,
        endPointIndex,
        dynamicForceBuffers,
        dt,
        velocityFactor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplySpringForces
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TPoints, typename TSprings>
inline void ApplySpringsForces_Naive(
    TPoints const & points,
    TSprings const & springs,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
    static_assert(vectorization_float_count<int> >= 4);

    vec2f const * restrict const positionBuffer = points.GetPositionBufferAsVec2();
    vec2f const * restrict const velocityBuffer = points.GetVelocityBufferAsVec2();

    typename TSprings::Endpoints const * restrict const endpointsBuffer = springs.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = springs.GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = springs.GetStiffnessCoefficientBuffer();
    float const * restrict const dampingCoefficientBuffer = springs.GetDampingCoefficientBuffer();

    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, springs.GetPerfectSquareCount() * 4);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
        //
        //    J          M   ---  a
        //    |\        /|
        //    | \s0  s1/ |
        //    |  \    /  |
        //  s2|   \  /   |s3
        //    |    \/    |
        //    |    /\    |
        //    |   /  \   |
        //    |  /    \  |
        //    | /      \ |
        //    |/        \|
        //    K          L  ---  b
        //

        //
        // Calculate displacements, string lengths, and spring directions
        //

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        vec2f const pointJPos = positionBuffer[pointJIndex];
        vec2f const pointKPos = positionBuffer[pointKIndex];
        vec2f const pointLPos = positionBuffer[pointLIndex];
        vec2f const pointMPos = positionBuffer[pointMIndex];

        vec2f const s0_dis = pointLPos - pointJPos;
        vec2f const s1_dis = pointKPos - pointMPos;
        vec2f const s2_dis = pointKPos - pointJPos;
        vec2f const s3_dis = pointLPos - pointMPos;

        float const s0_len = s0_dis.length();
        float const s1_len = s1_dis.length();
        float const s2_len = s2_dis.length();
        float const s3_len = s3_dis.length();

        vec2f const s0_dir = s0_dis.normalise(s0_len);
        vec2f const s1_dir = s1_dis.normalise(s1_len);
        vec2f const s2_dir = s2_dis.normalise(s2_len);
        vec2f const s3_dir = s3_dis.normalise(s3_len);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        float const s0_hookForceMag = (s0_len - restLengthBuffer[s]) * stiffnessCoefficientBuffer[s];
        float const s1_hookForceMag = (s1_len - restLengthBuffer[s + 1]) * stiffnessCoefficientBuffer[s + 1];
        float const s2_hookForceMag = (s2_len - restLengthBuffer[s + 2]) * stiffnessCoefficientBuffer[s + 2];
        float const s3_hookForceMag = (s3_len - restLengthBuffer[s + 3]) * stiffnessCoefficientBuffer[s + 3];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //

        vec2f const pointJVel = velocityBuffer[pointJIndex];
        vec2f const pointKVel = velocityBuffer[pointKIndex];
        vec2f const pointLVel = velocityBuffer[pointLIndex];
        vec2f const pointMVel = velocityBuffer[pointMIndex];

        vec2f const s0_relVel = pointLVel - pointJVel;
        vec2f const s1_relVel = pointKVel - pointMVel;
        vec2f const s2_relVel = pointKVel - pointJVel;
        vec2f const s3_relVel = pointLVel - pointMVel;

        float const s0_dampForceMag = s0_relVel.dot(s0_dir) * dampingCoefficientBuffer[s];
        float const s1_dampForceMag = s1_relVel.dot(s1_dir) * dampingCoefficientBuffer[s + 1];
        float const s2_dampForceMag = s2_relVel.dot(s2_dir) * dampingCoefficientBuffer[s + 2];
        float const s3_dampForceMag = s3_relVel.dot(s3_dir) * dampingCoefficientBuffer[s + 3];

        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //

        vec2f const s0_forceA = s0_dir * (s0_hookForceMag + s0_dampForceMag);
        vec2f const s1_forceA = s1_dir * (s1_hookForceMag + s1_dampForceMag);
        vec2f const s2_forceA = s2_dir * (s2_hookForceMag + s2_dampForceMag);
        vec2f const s3_forceA = s3_dir * (s3_hookForceMag + s3_dampForceMag);

        dynamicForceBuffer[pointJIndex] += (s0_forceA + s2_forceA);
        dynamicForceBuffer[pointLIndex] -= (s0_forceA + s3_forceA);
        dynamicForceBuffer[pointMIndex] += (s1_forceA + s3_forceA);
        dynamicForceBuffer[pointKIndex] -= (s1_forceA + s2_forceA);
    }

    //
    // 2. Remaining one-by-one's
    //

    for (; s < endSpringIndex; ++s)
    {
        auto const pointAIndex = endpointsBuffer[s].PointAIndex;
        auto const pointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = positionBuffer[pointBIndex] - positionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[s])
            * stiffnessCoefficientBuffer[s];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = velocityBuffer[pointBIndex] - velocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * dampingCoefficientBuffer[s];

        //
        // 3. Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        dynamicForceBuffer[pointAIndex] += forceA;
        dynamicForceBuffer[pointBIndex] -= forceA;
    }
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename TPoints, typename TSprings>
inline void ApplySpringsForces_SSEVectorized(
    TPoints const & points,
    TSprings const & springs,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
    // This implementation is for 4-float SSE
    static_assert(vectorization_float_count<int> >= 4);

    vec2f const * restrict const positionBuffer = points.GetPositionBufferAsVec2();
    vec2f const * restrict const velocityBuffer = points.GetVelocityBufferAsVec2();

    typename TSprings::Endpoints const * restrict const endpointsBuffer = springs.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = springs.GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = springs.GetStiffnessCoefficientBuffer();
    float const * restrict const dampingCoefficientBuffer = springs.GetDampingCoefficientBuffer();

    __m128 const Zero = _mm_setzero_ps();
    aligned_to_vword vec2f tmpSpringForces[4];

    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, springs.GetPerfectSquareCount() * 4);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
        // XMM register notation:
        //   low (left, or top) -> height (right, or bottom)

        //
        //    J          M   ---  a
        //    |\        /|
        //    | \s0  s1/ |
        //    |  \    /  |
        //  s2|   \  /   |s3
        //    |    \/    |
        //    |    /\    |
        //    |   /  \   |
        //    |  /    \  |
        //    | /      \ |
        //    |/        \|
        //    K          L  ---  b
        //

        //
        // Calculate displacements, string lengths, and spring directions
        //
        // Steps:
        //
        // l_pos_x   -   j_pos_x   =  s0_dis_x
        // l_pos_y   -   j_pos_y   =  s0_dis_y
        // k_pos_x   -   m_pos_x   =  s1_dis_x
        // k_pos_y   -   m_pos_y   =  s1_dis_y
        //
        // Swap 2H with 2L in first register, then:
        //
        // k_pos_x   -   j_pos_x   =  s2_dis_x
        // k_pos_y   -   j_pos_y   =  s2_dis_y
        // l_pos_x   -   m_pos_x   =  s3_dis_x
        // l_pos_y   -   m_pos_y   =  s3_dis_y
        //

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        // ?_pos_x
        // ?_pos_y
        // *
        // *
        __m128 const j_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointJIndex)));
        __m128 const k_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointKIndex)));
        __m128 const l_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointLIndex)));
        __m128 const m_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointMIndex)));

        __m128 const jm_pos_xy = _mm_movelh_ps(j_pos_xy, m_pos_xy); // First argument goes low
        __m128 lk_pos_xy = _mm_movelh_ps(l_pos_xy, k_pos_xy); // First argument goes low
        __m128 const s0s1_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);
        lk_pos_xy = _mm_shuffle_ps(lk_pos_xy, lk_pos_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);

        // Shuffle:
        //
        // s0_dis_x     s0_dis_y
        // s1_dis_x     s1_dis_y
        // s2_dis_x     s2_dis_y
        // s3_dis_x     s3_dis_y
        __m128 const s0s1s2s3_dis_x = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0x88);
        __m128 const s0s1s2s3_dis_y = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0xDD);

        // Calculate spring lengths: sqrt( x*x + y*y )
        //
        // Note: the kung-fu below (reciprocal square, then reciprocal, etc.) should be faster:
        //
        //  Standard: sqrt 12, (div 11, and 1), (div 11, and 1) = 5instrs/36cycles
        //  This one: rsqrt 4, and 1, (mul 4), (mul 4), rec 4, and 1 = 6instrs/18cycles

        __m128 const sq_len =
            _mm_add_ps(
                _mm_mul_ps(s0s1s2s3_dis_x, s0s1s2s3_dis_x),
                _mm_mul_ps(s0s1s2s3_dis_y, s0s1s2s3_dis_y));

        __m128 const validMask = _mm_cmpneq_ps(sq_len, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f

        __m128 const s0s1s2s3_springLength_inv =
            _mm_and_ps(
                _mm_rsqrt_ps(sq_len),
                validMask);

        __m128 const s0s1s2s3_springLength =
            _mm_and_ps(
                _mm_rcp_ps(s0s1s2s3_springLength_inv),
                validMask);

        // Calculate spring directions
        __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_dis_x, s0s1s2s3_springLength_inv);
        __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_dis_y, s0s1s2s3_springLength_inv);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        __m128 const s0s1s2s3_hooke_forceModuli =
            _mm_mul_ps(
                _mm_sub_ps(
                    s0s1s2s3_springLength,
                    _mm_load_ps(restLengthBuffer + s)),
                _mm_load_ps(stiffnessCoefficientBuffer + s));

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy:
        //
        // (s0_relv_x * s0_sdir_x  +  s0_relv_y * s0_sdir_y) * dampCoeff[s0]
        // (s1_relv_x * s1_sdir_x  +  s1_relv_y * s1_sdir_y) * dampCoeff[s1]
        // (s2_relv_x * s2_sdir_x  +  s2_relv_y * s2_sdir_y) * dampCoeff[s2]
        // (s3_relv_x * s3_sdir_x  +  s3_relv_y * s3_sdir_y) * dampCoeff[s3]
        //

        // ?_vel_x
        // ?_vel_y
        // *
        // *
        __m128 const j_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointJIndex)));
        __m128 const k_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointKIndex)));
        __m128 const l_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointLIndex)));
        __m128 const m_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointMIndex)));

        __m128 const jm_vel_xy = _mm_movelh_ps(j_vel_xy, m_vel_xy); // First argument goes low
        __m128 lk_vel_xy = _mm_movelh_ps(l_vel_xy, k_vel_xy); // First argument goes low
        __m128 const s0s1_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);
        lk_vel_xy = _mm_shuffle_ps(lk_vel_xy, lk_vel_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);

        __m128 s0s1s2s3_rvel_x = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0x88);
        __m128 s0s1s2s3_rvel_y = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0xDD);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_rvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_rvel_y, s0s1s2s3_sdir_y)),
                _mm_load_ps(dampingCoefficientBuffer + s));

        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  s0_tforce_a_x  =   s0_sdir_x  *  (  hookeForce[s0] + dampingForce[s0] )
        //  s1_tforce_a_x  =   s1_sdir_x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_x  =   s2_sdir_x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_x  =   s3_sdir_x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  s0_tforce_a_y  =   s0_sdir_y  *  (  hookeForce[s0] + dampingForce[s0] )
        //  s1_tforce_a_y  =   s1_sdir_y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_y  =   s2_sdir_y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_y  =   s3_sdir_y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        __m128 const tForceModuli = _mm_add_ps(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        __m128 const s0s1s2s3_tforceA_x =
            _mm_mul_ps(
                s0s1s2s3_sdir_x,
                tForceModuli);

        __m128 const s0s1s2s3_tforceA_y =
            _mm_mul_ps(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      dynamicForceBuffer[pointAIndex] += total_forceA;
        //      dynamicForceBuffer[pointBIndex] -= total_forceA;
        //
        // j_sforce += s0_a_tforce + s2_a_tforce
        // m_sforce += s1_a_tforce + s3_a_tforce
        //
        // l_sforce -= s0_a_tforce + s3_a_tforce
        // k_sforce -= s1_a_tforce + s2_a_tforce


        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        __m128 const jm_sforce_xy = _mm_add_ps(s0s1_tforceA_xy, s2s3_tforceA_xy);
        s2s3_tforceA_xy = _mm_shuffle_ps(s2s3_tforceA_xy, s2s3_tforceA_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const lk_sforce_xy = _mm_add_ps(s0s1_tforceA_xy, s2s3_tforceA_xy);

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), jm_sforce_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), lk_sforce_xy);

        dynamicForceBuffer[pointJIndex] += tmpSpringForces[0];
        dynamicForceBuffer[pointMIndex] += tmpSpringForces[1];
        dynamicForceBuffer[pointLIndex] -= tmpSpringForces[2];
        dynamicForceBuffer[pointKIndex] -= tmpSpringForces[3];
    }

    //
    // 2. Remaining four-by-four's
    //

    ElementCount const endSpringIndexVectorized = endSpringIndex - (endSpringIndex % 4);

    for (; s < endSpringIndexVectorized; s += 4)
    {
        // Spring 0 displacement (s0_position.x, s0_position.y, *, *)
        __m128 const s0pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_displacement.x, s0_displacement.y, *, *
        __m128 const s0_displacement_xy = _mm_sub_ps(s0pb_pos_xy, s0pa_pos_xy);

        // Spring 1 displacement (s1_position.x, s1_position.y, *, *)
        __m128 const s1pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_displacement.x, s1_displacement.y
        __m128 const s1_displacement_xy = _mm_sub_ps(s1pb_pos_xy, s1pa_pos_xy);

        // s0_displacement.x, s0_displacement.y, s1_displacement.x, s1_displacement.y
        __m128 const s0s1_displacement_xy = _mm_movelh_ps(s0_displacement_xy, s1_displacement_xy); // First argument goes low

        // Spring 2 displacement (s2_position.x, s2_position.y, *, *)
        __m128 const s2pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_displacement.x, s2_displacement.y
        __m128 const s2_displacement_xy = _mm_sub_ps(s2pb_pos_xy, s2pa_pos_xy);

        // Spring 3 displacement (s3_position.x, s3_position.y, *, *)
        __m128 const s3pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_displacement.x, s3_displacement.y
        __m128 const s3_displacement_xy = _mm_sub_ps(s3pb_pos_xy, s3pa_pos_xy);

        // s2_displacement.x, s2_displacement.y, s3_displacement.x, s3_displacement.y
        __m128 const s2s3_displacement_xy = _mm_movelh_ps(s2_displacement_xy, s3_displacement_xy); // First argument goes low

        // Shuffle displacements:
        // s0_displacement.x, s1_displacement.x, s2_displacement.x, s3_displacement.x
        __m128 s0s1s2s3_displacement_x = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0x88);
        // s0_displacement.y, s1_displacement.y, s2_displacement.y, s3_displacement.y
        __m128 s0s1s2s3_displacement_y = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0xDD);

        // Calculate spring lengths

        // s0_displacement.x^2, s1_displacement.x^2, s2_displacement.x^2, s3_displacement.x^2
        __m128 const s0s1s2s3_displacement_x2 = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_displacement_x);
        // s0_displacement.y^2, s1_displacement.y^2, s2_displacement.y^2, s3_displacement.y^2
        __m128 const s0s1s2s3_displacement_y2 = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_displacement_y);

        // s0_displacement.x^2 + s0_displacement.y^2, s1_displacement.x^2 + s1_displacement.y^2, s2_displacement..., s3_displacement...
        __m128 const s0s1s2s3_displacement_x2_p_y2 = _mm_add_ps(s0s1s2s3_displacement_x2, s0s1s2s3_displacement_y2);

        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_displacement_x2_p_y2, Zero);

        __m128 const s0s1s2s3_springLength_inv =
            _mm_and_ps(
                _mm_rsqrt_ps(s0s1s2s3_displacement_x2_p_y2),
                validMask);

        __m128 const s0s1s2s3_springLength =
            _mm_and_ps(
                _mm_rcp_ps(s0s1s2s3_springLength_inv),
                validMask);

        // Calculate spring directions
        __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength_inv);
        __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength_inv);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        __m128 const s0s1s2s3_restLength = _mm_load_ps(restLengthBuffer + s);
        __m128 const s0s1s2s3_stiffness = _mm_load_ps(stiffnessCoefficientBuffer + s);

        __m128 const s0s1s2s3_hooke_forceModuli = _mm_mul_ps(
            _mm_sub_ps(s0s1s2s3_springLength, s0s1s2s3_restLength),
            s0s1s2s3_stiffness);

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy:
        //
        // ( relV[s0].x * sprDir[s0].x  +  relV[s0].y * sprDir[s0].y )  *  dampCoeff[s0]
        // ( relV[s1].x * sprDir[s1].x  +  relV[s1].y * sprDir[s1].y )  *  dampCoeff[s1]
        // ( relV[s2].x * sprDir[s2].x  +  relV[s2].y * sprDir[s2].y )  *  dampCoeff[s2]
        // ( relV[s3].x * sprDir[s3].x  +  relV[s3].y * sprDir[s3].y )  *  dampCoeff[s3]
        //

        // Spring 0 rel vel (s0_vel.x, s0_vel.y, *, *)
        __m128 const s0pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_relvel_x, s0_relvel_y, *, *
        __m128 const s0_relvel_xy = _mm_sub_ps(s0pb_vel_xy, s0pa_vel_xy);

        // Spring 1 rel vel (s1_vel.x, s1_vel.y, *, *)
        __m128 const s1pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_relvel_x, s1_relvel_y, *, *
        __m128 const s1_relvel_xy = _mm_sub_ps(s1pb_vel_xy, s1pa_vel_xy);

        // s0_relvel.x, s0_relvel.y, s1_relvel.x, s1_relvel.y
        __m128 const s0s1_relvel_xy = _mm_movelh_ps(s0_relvel_xy, s1_relvel_xy); // First argument goes low

        // Spring 2 rel vel (s2_vel.x, s2_vel.y, *, *)
        __m128 const s2pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_relvel_x, s2_relvel_y, *, *
        __m128 const s2_relvel_xy = _mm_sub_ps(s2pb_vel_xy, s2pa_vel_xy);

        // Spring 3 rel vel (s3_vel.x, s3_vel.y, *, *)
        __m128 const s3pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_relvel_x, s3_relvel_y, *, *
        __m128 const s3_relvel_xy = _mm_sub_ps(s3pb_vel_xy, s3pa_vel_xy);

        // s2_relvel.x, s2_relvel.y, s3_relvel.x, s3_relvel.y
        __m128 const s2s3_relvel_xy = _mm_movelh_ps(s2_relvel_xy, s3_relvel_xy); // First argument goes low

        // Shuffle rel vals:
        // s0_relvel.x, s1_relvel.x, s2_relvel.x, s3_relvel.x
        __m128 s0s1s2s3_relvel_x = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0x88);
        // s0_relvel.y, s1_relvel.y, s2_relvel.y, s3_relvel.y
        __m128 s0s1s2s3_relvel_y = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0xDD);

        // Damping coeffs
        __m128 const s0s1s2s3_dampingCoeff = _mm_load_ps(dampingCoefficientBuffer + s);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_relvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_relvel_y, s0s1s2s3_sdir_y)),
                s0s1s2s3_dampingCoeff);

        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  total_forceA[s0].x  =   springDir[s0].x  *  (  hookeForce[s0] + dampingForce[s0] )
        //  total_forceA[s1].x  =   springDir[s1].x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].x  =   springDir[s2].x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].x  =   springDir[s3].x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  total_forceA[s0].y  =   springDir[s0].y  *  (  hookeForce[s0] + dampingForce[s0] )
        //  total_forceA[s1].y  =   springDir[s1].y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].y  =   springDir[s2].y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].y  =   springDir[s3].y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        __m128 const tForceModuli = _mm_add_ps(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        __m128 const s0s1s2s3_tforceA_x =
            _mm_mul_ps(
                s0s1s2s3_sdir_x,
                tForceModuli);

        __m128 const s0s1s2s3_tforceA_y =
            _mm_mul_ps(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      pointSpringForceBuffer[pointAIndex] += total_forceA;
        //      pointSpringForceBuffer[pointBIndex] -= total_forceA;
        //

        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), s0s1_tforceA_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), s2s3_tforceA_xy);

        dynamicForceBuffer[endpointsBuffer[s + 0].PointAIndex] += tmpSpringForces[0];
        dynamicForceBuffer[endpointsBuffer[s + 0].PointBIndex] -= tmpSpringForces[0];
        dynamicForceBuffer[endpointsBuffer[s + 1].PointAIndex] += tmpSpringForces[1];
        dynamicForceBuffer[endpointsBuffer[s + 1].PointBIndex] -= tmpSpringForces[1];
        dynamicForceBuffer[endpointsBuffer[s + 2].PointAIndex] += tmpSpringForces[2];
        dynamicForceBuffer[endpointsBuffer[s + 2].PointBIndex] -= tmpSpringForces[2];
        dynamicForceBuffer[endpointsBuffer[s + 3].PointAIndex] += tmpSpringForces[3];
        dynamicForceBuffer[endpointsBuffer[s + 3].PointBIndex] -= tmpSpringForces[3];
    }

    //
    // 3. Remaining one-by-one's
    //

    for (; s < endSpringIndex; ++s)
    {
        auto const pointAIndex = endpointsBuffer[s].PointAIndex;
        auto const pointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = positionBuffer[pointBIndex] - positionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[s])
            * stiffnessCoefficientBuffer[s];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = velocityBuffer[pointBIndex] - velocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * dampingCoefficientBuffer[s];

        //
        // 3. Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        dynamicForceBuffer[pointAIndex] += forceA;
        dynamicForceBuffer[pointBIndex] -= forceA;
    }
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
template<typename TPoints, typename TSprings>
inline void ApplySpringsForces_NeonVectorized(
    TPoints const & points,
    TSprings const & springs,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
    // This implementation is for 4-float Neon
    static_assert(vectorization_float_count<int> >= 4);

    vec2f const * restrict const positionBuffer = points.GetPositionBufferAsVec2();
    vec2f const * restrict const velocityBuffer = points.GetVelocityBufferAsVec2();

    typename TSprings::Endpoints const * restrict const endpointsBuffer = springs.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = springs.GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = springs.GetStiffnessCoefficientBuffer();
    float const * restrict const dampingCoefficientBuffer = springs.GetDampingCoefficientBuffer();

    float32x4_t const Zero = vdupq_n_f32(0.0f);

    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, springs.GetPerfectSquareCount() * 4);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
        // Q register notation:
        //   low (left, or top) -> height (right, or bottom)

        //
        //    J          M   ---  a
        //    |\        /|
        //    | \s0  s1/ |
        //    |  \    /  |
        //  s2|   \  /   |s3
        //    |    \/    |
        //    |    /\    |
        //    |   /  \   |
        //    |  /    \  |
        //    | /      \ |
        //    |/        \|
        //    K          L  ---  b
        //

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        //
        // Calculate displacements, string lengths, and spring directions
        //

        float32x2_t const j_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + pointJIndex));
        float32x2_t const k_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + pointKIndex));
        float32x2_t const l_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + pointLIndex));
        float32x2_t const m_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + pointMIndex));

        float32x4_t const lk_pos_xyxy = vcombine_f32(l_pos_xy, k_pos_xy);
        float32x4_t const kl_pos_xyxy = vcombine_f32(k_pos_xy, l_pos_xy);
        float32x4_t const jj_pos_xyxy = vcombine_f32(j_pos_xy, j_pos_xy);
        float32x4_t const mm_pos_xyxy = vcombine_f32(m_pos_xy, m_pos_xy);

        float32x4_t const dis_s0x_s0y_s2x_s2y = vsubq_f32(lk_pos_xyxy, jj_pos_xyxy);
        float32x4_t const dis_s1x_s1y_s3x_s3y = vsubq_f32(kl_pos_xyxy, mm_pos_xyxy);

        float32x4x2_t const dis_s0s1s2s3_xxxx_yyyy = vtrnq_f32(dis_s0x_s0y_s2x_s2y, dis_s1x_s1y_s3x_s3y);


        // Calculate spring lengths: sqrt( x*x + y*y )

        float32x4_t const sq_len =
            vaddq_f32(
                vmulq_f32(dis_s0s1s2s3_xxxx_yyyy.val[0], dis_s0s1s2s3_xxxx_yyyy.val[0]),
                vmulq_f32(dis_s0s1s2s3_xxxx_yyyy.val[1], dis_s0s1s2s3_xxxx_yyyy.val[1]));

        uint32x4_t const validMask = vcgtq_f32(sq_len, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f

        // One newtown-rhapson step
        float32x4_t s0s1s2s3_springLength_inv = vrsqrteq_f32(sq_len);
        s0s1s2s3_springLength_inv = vmulq_f32(
            s0s1s2s3_springLength_inv,
            vrsqrtsq_f32(
                vmulq_f32(sq_len, s0s1s2s3_springLength_inv),
                s0s1s2s3_springLength_inv));

        s0s1s2s3_springLength_inv =
            vandq_u32(
                s0s1s2s3_springLength_inv,
                validMask);

        // One newtown-rhapson step
        float32x4_t s0s1s2s3_springLength = vrecpeq_f32(s0s1s2s3_springLength_inv);
        s0s1s2s3_springLength = vmulq_f32(
            s0s1s2s3_springLength,
            vrecpsq_f32(
                s0s1s2s3_springLength_inv,
                s0s1s2s3_springLength));

        s0s1s2s3_springLength =
            vandq_u32(
                s0s1s2s3_springLength,
                validMask);


        // Calculate spring directions

        float32x4_t const s0s1s2s3_sdir_x = vmulq_f32(dis_s0s1s2s3_xxxx_yyyy.val[0], s0s1s2s3_springLength_inv);
        float32x4_t const s0s1s2s3_sdir_y = vmulq_f32(dis_s0s1s2s3_xxxx_yyyy.val[1], s0s1s2s3_springLength_inv);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        float32x4_t const s0s1s2s3_hooke_forceModuli =
            vmulq_f32(
                vsubq_f32(
                    s0s1s2s3_springLength,
                    vld1q_f32(reinterpret_cast<float const *>(restLengthBuffer + s))),
                vld1q_f32(reinterpret_cast<float const *>(stiffnessCoefficientBuffer + s)));


        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy:
        //
        // (s0_relv_x * s0_sdir_x  +  s0_relv_y * s0_sdir_y) * dampCoeff[s0]
        // (s1_relv_x * s1_sdir_x  +  s1_relv_y * s1_sdir_y) * dampCoeff[s1]
        // (s2_relv_x * s2_sdir_x  +  s2_relv_y * s2_sdir_y) * dampCoeff[s2]
        // (s3_relv_x * s3_sdir_x  +  s3_relv_y * s3_sdir_y) * dampCoeff[s3]
        //

        float32x2_t const j_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + pointJIndex));
        float32x2_t const k_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + pointKIndex));
        float32x2_t const l_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + pointLIndex));
        float32x2_t const m_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + pointMIndex));

        float32x4_t const lk_vel_xyxy = vcombine_f32(l_vel_xy, k_vel_xy);
        float32x4_t const kl_vel_xyxy = vcombine_f32(k_vel_xy, l_vel_xy);
        float32x4_t const jj_vel_xyxy = vcombine_f32(j_vel_xy, j_vel_xy);
        float32x4_t const mm_vel_xyxy = vcombine_f32(m_vel_xy, m_vel_xy);

        float32x4_t const rvel_s0x_s0y_s2x_s2y = vsubq_f32(lk_vel_xyxy, jj_vel_xyxy);
        float32x4_t const rvel_s1x_s1y_s3x_s3y = vsubq_f32(kl_vel_xyxy, mm_vel_xyxy);

        float32x4x2_t const rvel_s0s1s2s3_xxxx_yyyy = vtrnq_f32(rvel_s0x_s0y_s2x_s2y, rvel_s1x_s1y_s3x_s3y);

        float32x4_t const s0s1s2s3_damping_forceModuli =
            vmulq_f32(
                vaddq_f32( // Dot product
                    vmulq_f32(rvel_s0s1s2s3_xxxx_yyyy.val[0], s0s1s2s3_sdir_x),
                    vmulq_f32(rvel_s0s1s2s3_xxxx_yyyy.val[1], s0s1s2s3_sdir_y)),
                vld1q_f32(reinterpret_cast<float const *>(dampingCoefficientBuffer + s)));


        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  s0_tforce_a_x  =   s0_sdir_x  *  (  hookeForce[s0] + dampingForce[s0] )
        //  s1_tforce_a_x  =   s1_sdir_x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_x  =   s2_sdir_x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_x  =   s3_sdir_x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  s0_tforce_a_y  =   s0_sdir_y  *  (  hookeForce[s0] + dampingForce[s0] )
        //  s1_tforce_a_y  =   s1_sdir_y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_y  =   s2_sdir_y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_y  =   s3_sdir_y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        float32x4_t const tForceModuli = vaddq_f32(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        float32x4_t const s0s1s2s3_tforceA_x =
            vmulq_f32(
                s0s1s2s3_sdir_x,
                tForceModuli);

        float32x4_t const s0s1s2s3_tforceA_y =
            vmulq_f32(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      dynamicForceBuffer[pointAIndex] += total_forceA;
        //      dynamicForceBuffer[pointBIndex] -= total_forceA;
        //
        // j_dforce += s0_a_tforce + s2_a_tforce
        // m_dforce += s1_a_tforce + s3_a_tforce
        //
        // l_dforce -= s0_a_tforce + s3_a_tforce
        // k_dforce -= s1_a_tforce + s2_a_tforce

        float32x4x2_t const s0xy_s1xy_s2xy_s3xy = vzipq_f32(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y);

        float32x4_t const jfxy_mfxy = vaddq_f32(
            s0xy_s1xy_s2xy_s3xy.val[0],
            s0xy_s1xy_s2xy_s3xy.val[1]);

        float32x4_t const lfxy_kfxy = vaddq_f32(
            s0xy_s1xy_s2xy_s3xy.val[0],
            vextq_f32(s0xy_s1xy_s2xy_s3xy.val[1], s0xy_s1xy_s2xy_s3xy.val[1], 2)); // Flip S2 and S3

        float32x2_t jf = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + pointJIndex));
        jf = vadd_f32(jf, vget_low_f32(jfxy_mfxy));
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + pointJIndex), jf);

        float32x2_t mf = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + pointMIndex));
        mf = vadd_f32(mf, vget_high_f32(jfxy_mfxy));
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + pointMIndex), mf);

        float32x2_t lf = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + pointLIndex));
        lf = vsub_f32(lf, vget_low_f32(lfxy_kfxy));
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + pointLIndex), lf);

        float32x2_t kf = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + pointKIndex));
        kf = vsub_f32(kf, vget_high_f32(lfxy_kfxy));
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + pointKIndex), kf);
    }

    //
    // 2. Remaining four-by-four's
    //

    ElementCount const endSpringIndexVectorized = endSpringIndex - (endSpringIndex % 4);

    for (; s < endSpringIndexVectorized; s += 4)
    {
        //
        // Calculate displacements, string lengths, and spring directions
        //

        float32x2_t const s0pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 0].PointAIndex));
        float32x2_t const s0pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 0].PointBIndex));
        float32x2_t const s0_dis_xy = vsub_f32(s0pb_pos_xy, s0pa_pos_xy);

        float32x2_t const s1pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 1].PointAIndex));
        float32x2_t const s1pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 1].PointBIndex));
        float32x2_t const s1_dis_xy = vsub_f32(s1pb_pos_xy, s1pa_pos_xy);

        float32x2_t const s2pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 2].PointAIndex));
        float32x2_t const s2pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 2].PointBIndex));
        float32x2_t const s2_dis_xy = vsub_f32(s2pb_pos_xy, s2pa_pos_xy);

        float32x2_t const s3pa_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 3].PointAIndex));
        float32x2_t const s3pb_pos_xy = vld1_f32(reinterpret_cast<float const *>(positionBuffer + endpointsBuffer[s + 3].PointBIndex));
        float32x2_t const s3_dis_xy = vsub_f32(s3pb_pos_xy, s3pa_pos_xy);

        // Combine all into xxxx,yyyyy

        float32x4_t const s0s2_dis_xyxy = vcombine_f32(s0_dis_xy, s2_dis_xy);
        float32x4_t const s1s3_dis_xyxy = vcombine_f32(s1_dis_xy, s3_dis_xy);
        float32x4x2_t const s0s1s2s3_dis_xxxx_yyyy = vtrnq_f32(s0s2_dis_xyxy, s1s3_dis_xyxy);



        // Calculate spring lengths: sqrt( x*x + y*y )

        float32x4_t const sq_len =
            vaddq_f32(
                vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[0], s0s1s2s3_dis_xxxx_yyyy.val[0]),
                vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[1], s0s1s2s3_dis_xxxx_yyyy.val[1]));

        uint32x4_t const validMask = vcgtq_f32(sq_len, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f

        // One newtown-rhapson step
        float32x4_t s0s1s2s3_springLength_inv = vrsqrteq_f32(sq_len);
        s0s1s2s3_springLength_inv = vmulq_f32(
            s0s1s2s3_springLength_inv,
            vrsqrtsq_f32(
                vmulq_f32(sq_len, s0s1s2s3_springLength_inv),
                s0s1s2s3_springLength_inv));

        s0s1s2s3_springLength_inv =
            vandq_u32(
                s0s1s2s3_springLength_inv,
                validMask);

        // One newtown-rhapson step
        float32x4_t s0s1s2s3_springLength = vrecpeq_f32(s0s1s2s3_springLength_inv);
        s0s1s2s3_springLength = vmulq_f32(
            s0s1s2s3_springLength,
            vrecpsq_f32(
                s0s1s2s3_springLength_inv,
                s0s1s2s3_springLength));

        s0s1s2s3_springLength =
            vandq_u32(
                s0s1s2s3_springLength,
                validMask);


        // Calculate spring directions

        float32x4_t const s0s1s2s3_sdir_x = vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[0], s0s1s2s3_springLength_inv);
        float32x4_t const s0s1s2s3_sdir_y = vmulq_f32(s0s1s2s3_dis_xxxx_yyyy.val[1], s0s1s2s3_springLength_inv);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        float32x4_t const s0s1s2s3_hooke_forceModuli =
            vmulq_f32(
                vsubq_f32(
                    s0s1s2s3_springLength,
                    vld1q_f32(reinterpret_cast<float const *>(restLengthBuffer + s))),
                vld1q_f32(reinterpret_cast<float const *>(stiffnessCoefficientBuffer + s)));

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy:
        //
        // ( relV[s0].x * sprDir[s0].x  +  relV[s0].y * sprDir[s0].y )  *  dampCoeff[s0]
        // ( relV[s1].x * sprDir[s1].x  +  relV[s1].y * sprDir[s1].y )  *  dampCoeff[s1]
        // ( relV[s2].x * sprDir[s2].x  +  relV[s2].y * sprDir[s2].y )  *  dampCoeff[s2]
        // ( relV[s3].x * sprDir[s3].x  +  relV[s3].y * sprDir[s3].y )  *  dampCoeff[s3]
        //

        float32x2_t const s0pa_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 0].PointAIndex));
        float32x2_t const s0pb_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 0].PointBIndex));
        float32x2_t const s0_rvel_xy = vsub_f32(s0pb_vel_xy, s0pa_vel_xy);

        float32x2_t const s1pa_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 1].PointAIndex));
        float32x2_t const s1pb_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 1].PointBIndex));
        float32x2_t const s1_rvel_xy = vsub_f32(s1pb_vel_xy, s1pa_vel_xy);

        float32x2_t const s2pa_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 2].PointAIndex));
        float32x2_t const s2pb_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 2].PointBIndex));
        float32x2_t const s2_rvel_xy = vsub_f32(s2pb_vel_xy, s2pa_vel_xy);

        float32x2_t const s3pa_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 3].PointAIndex));
        float32x2_t const s3pb_vel_xy = vld1_f32(reinterpret_cast<float const *>(velocityBuffer + endpointsBuffer[s + 3].PointBIndex));
        float32x2_t const s3_rvel_xy = vsub_f32(s3pb_vel_xy, s3pa_vel_xy);

        float32x4_t const rvel_s0x_s0y_s2x_s2y = vcombine_f32(s0_rvel_xy, s2_rvel_xy);
        float32x4_t const rvel_s1x_s1y_s3x_s3y = vcombine_f32(s1_rvel_xy, s3_rvel_xy);

        float32x4x2_t const rvel_s0s1s2s3_xxxx_yyyy = vtrnq_f32(rvel_s0x_s0y_s2x_s2y, rvel_s1x_s1y_s3x_s3y);

        float32x4_t const s0s1s2s3_damping_forceModuli =
            vmulq_f32(
                vaddq_f32( // Dot product
                    vmulq_f32(rvel_s0s1s2s3_xxxx_yyyy.val[0], s0s1s2s3_sdir_x),
                    vmulq_f32(rvel_s0s1s2s3_xxxx_yyyy.val[1], s0s1s2s3_sdir_y)),
                vld1q_f32(reinterpret_cast<float const *>(dampingCoefficientBuffer + s)));

        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  total_forceA[s0].x  =   springDir[s0].x  *  (  hookeForce[s0] + dampingForce[s0] )
        //  total_forceA[s1].x  =   springDir[s1].x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].x  =   springDir[s2].x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].x  =   springDir[s3].x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  total_forceA[s0].y  =   springDir[s0].y  *  (  hookeForce[s0] + dampingForce[s0] )
        //  total_forceA[s1].y  =   springDir[s1].y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].y  =   springDir[s2].y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].y  =   springDir[s3].y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        float32x4_t const tForceModuli = vaddq_f32(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        float32x4_t const s0s1s2s3_tforceA_x =
            vmulq_f32(
                s0s1s2s3_sdir_x,
                tForceModuli);

        float32x4_t const s0s1s2s3_tforceA_y =
            vmulq_f32(
                s0s1s2s3_sdir_y,
                tForceModuli);


        //
        // Unpack and add forces:
        //      pointSpringForceBuffer[pointAIndex] += total_forceA;
        //      pointSpringForceBuffer[pointBIndex] -= total_forceA;
        //

        float32x4x2_t const s0xy_s1xy_s2xy_s3xy = vzipq_f32(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y);

        float32x2_t const s0f = vget_low_f32(s0xy_s1xy_s2xy_s3xy.val[0]);
        float32x2_t s0f_pa = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 0].PointAIndex));
        s0f_pa = vadd_f32(s0f_pa, s0f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 0].PointAIndex), s0f_pa);
        float32x2_t s0f_pb = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 0].PointBIndex));
        s0f_pb = vsub_f32(s0f_pb, s0f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 0].PointBIndex), s0f_pb);

        float32x2_t const s1f = vget_high_f32(s0xy_s1xy_s2xy_s3xy.val[0]);
        float32x2_t s1f_pa = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 1].PointAIndex));
        s1f_pa = vadd_f32(s1f_pa, s1f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 1].PointAIndex), s1f_pa);
        float32x2_t s1f_pb = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 1].PointBIndex));
        s1f_pb = vsub_f32(s1f_pb, s1f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 1].PointBIndex), s1f_pb);

        float32x2_t const s2f = vget_low_f32(s0xy_s1xy_s2xy_s3xy.val[1]);
        float32x2_t s2f_pa = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 2].PointAIndex));
        s2f_pa = vadd_f32(s2f_pa, s2f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 2].PointAIndex), s2f_pa);
        float32x2_t s2f_pb = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 2].PointBIndex));
        s2f_pb = vsub_f32(s2f_pb, s2f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 2].PointBIndex), s2f_pb);

        float32x2_t const s3f = vget_high_f32(s0xy_s1xy_s2xy_s3xy.val[1]);
        float32x2_t s3f_pa = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 3].PointAIndex));
        s3f_pa = vadd_f32(s3f_pa, s3f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 3].PointAIndex), s3f_pa);
        float32x2_t s3f_pb = vld1_f32(reinterpret_cast<float const *>(dynamicForceBuffer + endpointsBuffer[s + 3].PointBIndex));
        s3f_pb = vsub_f32(s3f_pb, s3f);
        vst1_f32(reinterpret_cast<float *>(dynamicForceBuffer + endpointsBuffer[s + 3].PointBIndex), s3f_pb);
    }

    //
    // 3. Remaining one-by-one's
    //

    for (; s < endSpringIndex; ++s)
    {
        auto const pointAIndex = endpointsBuffer[s].PointAIndex;
        auto const pointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = positionBuffer[pointBIndex] - positionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[s])
            * stiffnessCoefficientBuffer[s];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = velocityBuffer[pointBIndex] - velocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * dampingCoefficientBuffer[s];

        //
        // 3. Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        dynamicForceBuffer[pointAIndex] += forceA;
        dynamicForceBuffer[pointBIndex] -= forceA;
    }
}
#endif

/*
 * Applies spring forces to the specified points.
 */

template<typename TPoints, typename TSprings>
inline void ApplySpringsForces(
    TPoints const & points,
    TSprings const & springs,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    ApplySpringsForces_SSEVectorized<TPoints>(points, springs, startSpringIndex, endSpringIndex, dynamicForceBuffer);
#elif FS_IS_ARM_NEON()
    ApplySpringsForces_NeonVectorized<TPoints>(points, springs, startSpringIndex, endSpringIndex, dynamicForceBuffer);
#else
    ApplySpringsForces_Naive<TPoints>(points, springs, startSpringIndex, endSpringIndex, dynamicForceBuffer);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// MakeAABBWeightedUnion
///////////////////////////////////////////////////////////////////////////////////////////////////////

// Currently unused - just by benchmarks
inline std::optional<Geometry::AABB> MakeAABBWeightedUnion_Naive(std::vector<Geometry::ShipAABB> const & aabbs) noexcept
{
    //
    // Centers
    //

    float constexpr FrontierEdgeCountThreshold = 3.0f;

    vec2f centersSum = vec2f::zero();
    float weightsSum = 0.0f;
    float maxWeight = 0.0f;
    for (auto const & aabb : aabbs)
    {
        if (aabb.FrontierEdgeCount > FrontierEdgeCountThreshold)
        {
            float const w = aabb.FrontierEdgeCount - FrontierEdgeCountThreshold;

            centersSum += vec2f(
                aabb.TopRight.x + aabb.BottomLeft.x,
                aabb.TopRight.y + aabb.BottomLeft.y) * w;
            weightsSum += w;
            maxWeight = std::max(maxWeight, w);
        }
    }

    if (weightsSum == 0.0f)
    {
        return std::nullopt;
    }

    vec2f const center = centersSum / 2.0f / weightsSum;

    //
    // Extent
    //

    float leftOffset = 0.0f;
    float rightOffset = 0.0f;
    float topOffset = 0.0f;
    float bottomOffset = 0.0f;

    for (auto const & aabb : aabbs)
    {
        if (aabb.FrontierEdgeCount > FrontierEdgeCountThreshold)
        {
            float const w = (aabb.FrontierEdgeCount - FrontierEdgeCountThreshold) / maxWeight;

            float const lp = (aabb.BottomLeft.x - center.x) * w;
            leftOffset = std::min(leftOffset, lp);
            float const rp = (aabb.TopRight.x - center.x) * w;
            rightOffset = std::max(rightOffset, rp);
            float const tp = (aabb.TopRight.y - center.y) * w;
            topOffset = std::max(topOffset, tp);
            float const bp = (aabb.BottomLeft.y - center.y) * w;
            bottomOffset = std::min(bottomOffset, bp);
        }
    }

    //
    // Produce result
    //

    auto const result = Geometry::AABB(
        center + vec2f(rightOffset, topOffset),
        center + vec2f(leftOffset, bottomOffset));

    return result;
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
inline std::optional<Geometry::AABB> MakeAABBWeightedUnion_SSEVectorized(std::vector<Geometry::ShipAABB> const & aabbs) noexcept
{
    //
    // Centers
    //

    float constexpr FrontierEdgeCountThreshold = 3.0f;

    __m128 centersSum = _mm_setzero_ps(); // CxCyCxCy (yes, repeated - no choice with SSE)
    float weightsSum = 0.0f;
    float maxWeight = 0.0f;
    for (auto const & aabb : aabbs)
    {
        if (aabb.FrontierEdgeCount > FrontierEdgeCountThreshold)
        {
            float const w = aabb.FrontierEdgeCount - FrontierEdgeCountThreshold;

            __m128 const rtlb = _mm_loadu_ps(&(aabb.TopRight.x));
            __m128 const lbrt = _mm_shuffle_ps(rtlb, rtlb, 0x4E);

            // centersSum = centersSum + (rtlb + lbrt) * w
            centersSum = _mm_add_ps(
                _mm_mul_ps(
                    _mm_add_ps(rtlb, lbrt),
                    _mm_load_ps1(&w)),
                centersSum);

            weightsSum += w;
            maxWeight = std::max(maxWeight, w);
        }
    }

    if (weightsSum == 0.0f)
    {
        return std::nullopt;
    }

    // center_4 = center / 2.0 / weightsSum
    __m128 const center_4 = _mm_div_ps(
        _mm_mul_ps(centersSum, *(__m128 *)ZeroPointFive4f),
        _mm_load_ps1(&weightsSum));

    //
    // Extent
    //

    __m128 rtlb_offsets_max = _mm_setzero_ps();
    __m128 rtlb_offsets_min = _mm_setzero_ps();
    float const maxWeightRep = 1.0f / maxWeight;

    for (auto const & aabb : aabbs)
    {
        if (aabb.FrontierEdgeCount > FrontierEdgeCountThreshold)
        {
            float const w = (aabb.FrontierEdgeCount - FrontierEdgeCountThreshold) * maxWeightRep;

            __m128 const rtlb = _mm_loadu_ps(&(aabb.TopRight.x));

            // rtlb_weighted_offsets = (rtlb - cxcycxcy) * w
            __m128 const rtlb_weighted_offsets = _mm_mul_ps(
                _mm_sub_ps(rtlb, center_4),
                _mm_load_ps1(&w));

            rtlb_offsets_max = _mm_max_ps(rtlb_offsets_max, rtlb_weighted_offsets);
            rtlb_offsets_min = _mm_min_ps(rtlb_offsets_min, rtlb_weighted_offsets);
        }
    }

    //
    // Produce result
    //

    __m128 const res1 = _mm_add_ps(center_4, rtlb_offsets_max); // Of this one we want the top two lanes
    __m128 const res2 = _mm_add_ps(center_4, rtlb_offsets_min); // Of this one we want the bottom two lanes
    __m128 const res = _mm_shuffle_ps(res1, res2, 0xE4);

    Geometry::AABB result;

    _mm_storeu_ps(&(result.TopRight.x), res);

    return result;
}
#endif

#if FS_IS_ARM_NEON() // Implies ARM anyways
inline std::optional<Geometry::AABB> MakeAABBWeightedUnion_NeonVectorized(std::vector<Geometry::ShipAABB> const & aabbs) noexcept
{
    // TODOTEST
    return MakeAABBWeightedUnion_Naive(aabbs);
}
#endif

inline std::optional<Geometry::AABB> MakeAABBWeightedUnion(std::vector<Geometry::ShipAABB> const & aabbs) noexcept
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    return MakeAABBWeightedUnion_SSEVectorized(aabbs);
#elif FS_IS_ARM_NEON()
    return MakeAABBWeightedUnion_NeonVectorized(aabbs);
#else
    return MakeAABBWeightedUnion_Naive(aabbs);
#endif
}


}