/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "SysSpecifics.h"
#include "Vectors.h"

namespace Algorithms {

template<typename EndpointStruct>
inline void CalculateVectorDirsAndReciprocalLengths(
    vec2f const * pointPositions,
    EndpointStruct const * endpoints,
    vec2f * restrict outDirs,
    float * restrict outReciprocalLengths,
    size_t const elementCount)
{
    assert(elementCount % 4 == 0); // Element counts are aligned

    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
    
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

inline void DiffuseLight_Naive(
    vec2f const * pointPositions,
    PlaneId const * pointPlaneIds,
    ElementIndex const pointCount,
    vec2f const * lampPositions,
    PlaneId const * lampPlaneIds,
    float const * lampDistanceCoeffs,
    float const * lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer)
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

                float const newLight =
                    lampDistanceCoeffs[l]
                    * (lampSpreadMaxDistances[l] - distance); // If negative, max(.) below will clamp down to 0.0

                // Point's light is just max, to avoid having to normalize everything to 1.0
                pointLight = std::max(
                    newLight,
                    pointLight);
            }
        }

        outLightBuffer[p] = std::min(1.0f, pointLight);
    }
}

inline void DiffuseLight_Vectorized(
    vec2f const * restrict pointPositions,
    PlaneId const * restrict pointPlaneIds,
    ElementIndex const pointCount,
    vec2f const * restrict lampPositions,
    PlaneId const * restrict lampPlaneIds,
    float const * restrict lampDistanceCoeffs,
    float const * restrict lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer)
{
    // This code is vectorized for SSE = 4 floats
    assert(is_aligned_to_vectorization_word(pointPositions));
    assert(is_aligned_to_vectorization_word(pointPlaneIds));
    assert(is_aligned_to_vectorization_word(lampPositions));
    assert(is_aligned_to_vectorization_word(lampPlaneIds));
    assert(is_aligned_to_vectorization_word(lampDistanceCoeffs));
    assert(is_aligned_to_vectorization_word(lampSpreadMaxDistances));
    assert(is_aligned_to_vectorization_word(outLightBuffer));

    // Caller is assumed to have skipped this when there are no lamps
    assert(lampCount > 0);

    if (lampCount < 4)
    {
        // Shortcut: in this case there's no point in vectorizing over lamps
        DiffuseLight_Naive(
            pointPositions,
            pointPlaneIds,
            pointCount,
            lampPositions,
            lampPlaneIds,
            lampDistanceCoeffs,
            lampSpreadMaxDistances,
            lampCount,
            outLightBuffer);

        return;
    }

    //
    // Visit all points
    //

    for (ElementIndex p = 0; p < pointCount; ++p)
    {   
        // Point position, repeated 4 times
        auto const pointPosition = pointPositions[p];
        __m128 const pointPosX_4 = _mm_set1_ps(pointPosition.x); // x0,x0,x0,x0
        __m128 const pointPosY_4 = _mm_set1_ps(pointPosition.y); // y0,y0,y0,y0

        // Point plane, repeated 4 times
        auto const pointPlaneId = pointPlaneIds[p];
        __m128i const pointPlaneId_4 = _mm_castps_si128(_mm_load_ps1(reinterpret_cast<float const *>(&pointPlaneId)));

        // Resultant point light
        __m128 pointLight_4 = _mm_setzero_ps();

        // Go through all lamps, 4 by 4;
        // can safely visit deleted lamps as their current will always be zero
        ElementIndex l;
        for (l = 0; l + 4 <= lampCount; l += 4)
        {
            // Lamp positions
            __m128 const lampPos12_4 = _mm_load_ps(reinterpret_cast<float const *>(lampPositions + l)); // x1, y1, x2, y2
            __m128 const lampPos34_4 = _mm_load_ps(reinterpret_cast<float const *>(lampPositions + l + 2)); // x3, y3, x4, y4        
            __m128 const lampPosX_4 = _mm_shuffle_ps(lampPos12_4, lampPos34_4, 0x88); // x0,x1,x2,x3
            __m128 const lampPosY_4 = _mm_shuffle_ps(lampPos12_4, lampPos34_4, 0xDD); // y0,y1,y2,y3

            // Lamp planes
            __m128i lampPlaneId_4 = _mm_load_si128(reinterpret_cast<__m128i const *>(lampPlaneIds + l));

            // Calculate distance
            __m128 const displacementX_4 = _mm_sub_ps(pointPosX_4, lampPosX_4);
            __m128 const displacementY_4 = _mm_sub_ps(pointPosY_4, lampPosY_4);
            __m128 const distanceSquare_4 = _mm_add_ps(
                _mm_mul_ps(displacementX_4, displacementX_4),
                _mm_mul_ps(displacementY_4, displacementY_4));
            __m128 const distance_4 = _mm_sqrt_ps(distanceSquare_4);

            // Calculate new light
            __m128 newLight_4 = _mm_mul_ps(
                _mm_load_ps(lampDistanceCoeffs + l),
                _mm_sub_ps(
                    _mm_load_ps(lampSpreadMaxDistances + l),
                    distance_4));

            // Mask with plane ID
            __m128i const invalidMask = _mm_cmpgt_epi32(pointPlaneId_4, lampPlaneId_4);
            newLight_4 = _mm_andnot_ps(_mm_castsi128_ps(invalidMask), newLight_4);

            // Point's light is just max, to avoid having to normalize everything to 1.0
            pointLight_4 = _mm_max_ps(pointLight_4, newLight_4);
        }

        __m128 pointLightTmp = _mm_max_ps(
            pointLight_4, 
            _mm_movehl_ps(pointLight_4, pointLight_4)); // max(abcd, cdcd) -> (max(a,c), max(b,d), ...)
        pointLightTmp = _mm_max_ss(pointLightTmp, _mm_movehdup_ps(pointLightTmp));

        float pointLight_1;
        _mm_store_ss(&pointLight_1, pointLightTmp);

        // Go through all remaining lamps, individually
        for (; l < lampCount; ++l)
        {
            if (pointPlaneId <= lampPlaneIds[l])
            {
                float const distance = (pointPosition - lampPositions[l]).length();

                float const newLight =
                    lampDistanceCoeffs[l]
                    * (lampSpreadMaxDistances[l] - distance); // If negative, max(.) below will clamp down to 0.0

                // Point's light is just max, to avoid having to normalize everything to 1.0
                pointLight_1 = std::max(
                    newLight,
                    pointLight_1);
            }
        }

        outLightBuffer[p] = std::min(1.0f, pointLight_1);
    }
}

}