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

inline void DiffuseLight(
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
    // TODOHERE: this is the original implementation

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
                    std::min(
                        lampDistanceCoeffs[l] * std::max(lampSpreadMaxDistances[l] - distance, 0.0f),
                        1.0f);

                // Point's light is just max, to avoid having to normalize everything to 1.0
                pointLight = std::max(
                    newLight,
                    pointLight);
            }
        }

        outLightBuffer[p] = pointLight;
    }
}

}