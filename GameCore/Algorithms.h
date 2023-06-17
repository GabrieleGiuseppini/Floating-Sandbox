/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

namespace Algorithms {

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector normalization
///////////////////////////////////////////////////////////////////////////////////////////////////////

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename TVector>
inline TVector NormalizeVector2_SSE(TVector const & v) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 x = _mm_load_ss(&(v.x));
    __m128 y = _mm_load_ss(&(v.y));

    __m128 len = _mm_sqrt_ss(
        _mm_add_ss(
            _mm_mul_ss(x, x),
            _mm_mul_ss(y, y)));

    __m128 invLen = _mm_div_ss(One, len);
    __m128 validMask = _mm_cmpneq_ss(invLen, Zero);
    invLen = _mm_and_ps(invLen, validMask);

    x = _mm_mul_ss(x, invLen);
    y = _mm_mul_ss(y, invLen);

    return TVector(_mm_cvtss_f32(x), _mm_cvtss_f32(y));
}
#endif

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename TVector>
inline TVector NormalizeVector2_SSE(TVector const & v, float length) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 _l = _mm_set_ss(length);
    __m128 _revl = _mm_div_ss(One, _l);
    __m128 validMask = _mm_cmpneq_ss(_l, Zero);
    _revl = _mm_and_ps(_revl, validMask);

    __m128 _x = _mm_mul_ss(_mm_load_ss(&(v.x)), _revl);
    __m128 _y = _mm_mul_ss(_mm_load_ss(&(v.y)), _revl);

    return TVector(_mm_cvtss_f32(_x), _mm_cvtss_f32(_y));
}
#endif

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
template<typename EndpointStruct, typename TVector>
inline void CalculateVectorDirsAndReciprocalLengths_SSE(
    TVector const * pointPositions,
    EndpointStruct const * endpoints,
    TVector * restrict outDirs,
    float * restrict outReciprocalLengths,
    size_t const elementCount)
{
    assert((elementCount % 4) == 0); // Element counts are aligned

    __m128 const Zero = _mm_setzero_ps();

    for (size_t s = 0; s < elementCount; s += 4)
    {
        __m128 const vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 0].PointAIndex)));
        __m128 const vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 0].PointBIndex)));
        __m128 const vecD0 = _mm_sub_ps(vecB0, vecA0);

        __m128 const vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 1].PointAIndex)));
        __m128 const vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 1].PointBIndex)));
        __m128 const vecD1 = _mm_sub_ps(vecB1, vecA1);

        __m128 const vecD01 = _mm_movelh_ps(vecD0, vecD1); //x0,y0,x1,y1

        __m128 const vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 2].PointAIndex)));
        __m128 const vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 2].PointBIndex)));
        __m128 const vecD2 = _mm_sub_ps(vecB2, vecA2);

        __m128 const vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 3].PointAIndex)));
        __m128 const vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositions + endpoints[s + 3].PointBIndex)));
        __m128 const vecD3 = _mm_sub_ps(vecB3, vecA3);

        __m128 const vecD23 = _mm_movelh_ps(vecD2, vecD3); //x2,y2,x3,y3

        __m128 displacementX = _mm_shuffle_ps(vecD01, vecD23, 0x88); // x0,x1,x2,x3
        __m128 displacementY = _mm_shuffle_ps(vecD01, vecD23, 0xDD); // y0,y1,y2,y3

        __m128 const displacementX2 = _mm_mul_ps(displacementX, displacementX);
        __m128 const displacementY2 = _mm_mul_ps(displacementY, displacementY);

        __m128 const displacementXY = _mm_add_ps(displacementX2, displacementY2); // x^2 + y^2

        __m128 const validMask = _mm_cmpneq_ps(displacementXY, Zero);
        __m128 rspringLength = _mm_rsqrt_ps(displacementXY);

        // L==0 => 1/L == 0, to maintain normal == (0, 0) from vec2f
        rspringLength = _mm_and_ps(rspringLength, validMask);

        displacementX = _mm_mul_ps(displacementX, rspringLength);
        displacementY = _mm_mul_ps(displacementY, rspringLength);

        _mm_store_ps(outReciprocalLengths + s, rspringLength);

        __m128 s01 = _mm_unpacklo_ps(displacementX, displacementY);
        __m128 s23 = _mm_unpackhi_ps(displacementX, displacementY);

        _mm_store_ps(reinterpret_cast<float * restrict>(outDirs + s), s01);
        _mm_store_ps(reinterpret_cast<float * restrict>(outDirs + s + 2), s23);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffuseLight
///////////////////////////////////////////////////////////////////////////////////////////////////////

// Currently unused - just by benchmarks
template<typename TVector>
inline void DiffuseLight_Naive(
    TVector const * pointPositions,
    PlaneId const * pointPlaneIds,
    ElementIndex const pointCount,
    TVector const * lampPositions,
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

template<typename TVector>
inline void DiffuseLight_Vectorized(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    TVector const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    TVector const * restrict lampPositions,
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
        TVector const * const restrict batchPointPositions = &(pointPositions[p]);
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
template<typename TVector>
inline void DiffuseLight_SSEVectorized(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    TVector const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    TVector const * restrict lampPositions,
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
            // 1 - 0,1,2,3
            //

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
            __m128i invalidMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
            newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(invalidMask), newLight_4);

            // Point light
            pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);

            // Rotate -> 1,2,3,0
            pointPosX_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosX_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPosY_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosY_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPlaneId_4 = _mm_shuffle_epi32(pointPlaneId_4, _MM_SHUFFLE(0, 3, 2, 1));
            pointLight_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointLight_4), _MM_SHUFFLE(0, 3, 2, 1)));

            //
            // 2 - 1,2,3,0
            //

            // Calculate distance
            displacementX_4 = _mm_sub_ps(pointPosX_4, lampPosX_4);
            displacementY_4 = _mm_sub_ps(pointPosY_4, lampPosY_4);
            distanceSquare_4 = _mm_add_ps(
                _mm_mul_ps(displacementX_4, displacementX_4),
                _mm_mul_ps(displacementY_4, displacementY_4));
            distance_4 = _mm_sqrt_ps(distanceSquare_4);

            // Calculate new light
            newLight_4 = _mm_mul_ps(
                lampDistanceCoeff_4,
                _mm_sub_ps(lampSpreadMaxDistance_4, distance_4));

            // Mask with plane ID
            invalidMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
            newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(invalidMask), newLight_4);

            // Point light
            pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);

            // Rotate -> 2,3,0,1
            pointPosX_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosX_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPosY_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosY_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPlaneId_4 = _mm_shuffle_epi32(pointPlaneId_4, _MM_SHUFFLE(0, 3, 2, 1));
            pointLight_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointLight_4), _MM_SHUFFLE(0, 3, 2, 1)));

            //
            // 3 - 2,3,0,1
            //

            // Calculate distance
            displacementX_4 = _mm_sub_ps(pointPosX_4, lampPosX_4);
            displacementY_4 = _mm_sub_ps(pointPosY_4, lampPosY_4);
            distanceSquare_4 = _mm_add_ps(
                _mm_mul_ps(displacementX_4, displacementX_4),
                _mm_mul_ps(displacementY_4, displacementY_4));
            distance_4 = _mm_sqrt_ps(distanceSquare_4);

            // Calculate new light
            newLight_4 = _mm_mul_ps(
                lampDistanceCoeff_4,
                _mm_sub_ps(lampSpreadMaxDistance_4, distance_4));

            // Mask with plane ID
            invalidMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
            newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(invalidMask), newLight_4);

            // Point light
            pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);

            // Rotate -> 3,0,1,2
            pointPosX_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosX_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPosY_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosY_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPlaneId_4 = _mm_shuffle_epi32(pointPlaneId_4, _MM_SHUFFLE(0, 3, 2, 1));
            pointLight_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointLight_4), _MM_SHUFFLE(0, 3, 2, 1)));

            //
            // 4 - 3,0,1,2
            //

            // Calculate distance
            displacementX_4 = _mm_sub_ps(pointPosX_4, lampPosX_4);
            displacementY_4 = _mm_sub_ps(pointPosY_4, lampPosY_4);
            distanceSquare_4 = _mm_add_ps(
                _mm_mul_ps(displacementX_4, displacementX_4),
                _mm_mul_ps(displacementY_4, displacementY_4));
            distance_4 = _mm_sqrt_ps(distanceSquare_4);

            // Calculate new light
            newLight_4 = _mm_mul_ps(
                lampDistanceCoeff_4,
                _mm_sub_ps(lampSpreadMaxDistance_4, distance_4));

            // Mask with plane ID
            invalidMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
            newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(invalidMask), newLight_4);

            // Point light
            pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);

            // Rotate -> 0,1,2,3
            pointPosX_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosX_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPosY_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointPosY_4), _MM_SHUFFLE(0, 3, 2, 1)));
            pointPlaneId_4 = _mm_shuffle_epi32(pointPlaneId_4, _MM_SHUFFLE(0, 3, 2, 1));
            pointLight_4 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(pointLight_4), _MM_SHUFFLE(0, 3, 2, 1)));
        }

        //
        // Store the 4 point lights, capping them to 1.0
        //

        pointLight_4 = _mm_min_ps(pointLight_4, _mm_set1_ps(1.0f));
        _mm_store_ps(outLightBuffer + p, pointLight_4);
    }
}
#endif

/*
 * Diffuse light from each lamp to all points on the same or lower plane ID,
 * inverse-proportionally to the lamp-point distance
 */
template<typename TVector>
inline void DiffuseLight(
    ElementIndex const pointStart,
    ElementIndex const pointEnd,
    TVector const * pointPositions,
    PlaneId const * pointPlaneIds,
    TVector const * lampPositions,
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
#else
    SmoothBufferAndAdd_Naive<BufferSize, SmoothingSize>(inBuffer, outBuffer);
#endif
}

}