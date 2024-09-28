/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "GameTypes.h"
#include "SysSpecifics.h"
#include "Vectors.h"

#include <cassert>
#include <cmath>

namespace Geometry {

class Segment
{
public:

    /*
     * Tests whether the two segments (p1->p2 and q1->q2) intersect. Touching segments
     * might be considered intersecting, depending on the order their points are given.
     * Collinear segments are not considered intersecting, no matter what.
     */
    inline static bool ProperIntersectionTest(
        vec2f const & p1,
        vec2f const & p2,
        vec2f const & q1,
        vec2f const & q2)
    {
        //
        // Check whether p1p2 is between p1q1 and p1q2;
        // i.e. whether p1p2Vp1q1 angle has a different sign than p1p2Vp1q2
        //

        vec2f const p1p2 = p2 - p1;
        vec2f const p1q1 = q1 - p1;
        vec2f const p1q2 = q2 - p1;

        if ((p1p2.cross(p1q1) < 0.0f) == (p1p2.cross(p1q2) < 0.0f))
        {
            return false;
        }


        //
        // Do the opposite now: check whether q1q2 is between q1p1 and q1p2;
        // i.e. whether q1q2Vq1p1 angle has a different sign than q1q2Vq1p2
        //

        vec2f const q1q2 = q2 - q1;
        vec2f const q1p2 = p2 - q1;

        return (q1q2.cross(p1q1) > 0.0f) != (q1q2.cross(q1p2) < 0.0f);
    }

    /*
     * Returns the distance between a point and a segment.
     */
    inline static float DistanceToPoint(
        vec2f const & segmentP1,
        vec2f const & segmentP2,
        vec2f const & point)
    {
        return std::sqrt(
            SquareDistanceToPoint(
                segmentP1,
                segmentP2,
                point));
    }

    /*
     * Returns the distance between a point and a segment.
     */
    inline static float SquareDistanceToPoint(
        vec2f const & segmentP1,
        vec2f const & segmentP2,
        vec2f const & point)
    {
        // From https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment

        float const segmentSquaredLength = (segmentP2 - segmentP1).squareLength();
        if (segmentSquaredLength == 0.0f)
        {
            // Overlapping endpoints
            return (segmentP2 - point).squareLength();
        }

        // Consider the line extending the segment, parameterized as P1 + t (P2 - P1).
        // We find projection of point P onto the line.
        // It falls where t = [(P-P1) . (P2-P)] / |P2-P1|^2
        // We clamp t from [0,1] to handle points outside the segment P1-P2.
        const float t = std::max(0.0f, std::min(1.0f, (point - segmentP1).dot(segmentP2 - segmentP1) / segmentSquaredLength));
        vec2f const projection = segmentP1 + (segmentP2 - segmentP1) * t;  // Projection falls on the segment
        return (projection - point).squareLength();
    }
};

/*
 * Returns the octant opposite to the specified octant.
 */
inline Octant OppositeOctant(Octant octant)
{
    assert(octant >= 0 && octant <= 7);

    Octant oppositeOctant = octant + 4;

    if (oppositeOctant >= 8)
        oppositeOctant -= 8;

    return oppositeOctant;
}

/*
 * Returns the angle, in CW radiants starting from E, for the specified octant.
 */
inline float OctantToCWAngle(Octant octant)
{
    assert(octant >= 0 && octant <= 7);

    return 2.0f * Pi<float> * static_cast<float>(octant) / 8.0f;
}

/*
 * Returns the angle, in CCW radiants starting from E, for the specified octant.
 */
inline float OctantToCCWAngle(Octant octant)
{
    assert(octant >= 0 && octant <= 7);

    if (octant == 0)
        return 0.0f;
    else
        return 2.0f * Pi<float> * (1.0f - static_cast<float>(octant) / 8.0f);
}

inline bool IsPointInTriangle(
    vec2f const & pPosition,
    vec2f const & aPosition,
    vec2f const & bPosition,
    vec2f const & cPosition)
{
    return (pPosition - aPosition).cross(bPosition - aPosition) >= 0.0f
        && (pPosition - bPosition).cross(cPosition - bPosition) >= 0.0f
        && (pPosition - cPosition).cross(aPosition - cPosition) >= 0.0f;
}

/*
 * Calculates a line path between (and including) the specified endpoints, going
 * through integral coordinates.
 */

enum class IntegralLineType
{
    Minimal,
    WithAdjacentSteps
};

template<IntegralLineType TType, typename _TIntegralTag, typename TVisitor>
inline void GenerateIntegralLinePath(
    _IntegralCoordinates<_TIntegralTag> const & startPoint,
    _IntegralCoordinates<_TIntegralTag> const & endPoint,
    TVisitor const & visitor)
{
    //
    // Visit starting point
    //

    visitor(startPoint);

    // Check whether we are done
    if (startPoint == endPoint)
    {
        return;
    }

    //
    // "Draw" line from start position to end position
    //
    // Go along widest of Dx and Dy, in steps of 1.0, until we're very close to end position
    //

    // W = wide, N = narrow

    int const dx = endPoint.x - startPoint.x;
    int const dy = endPoint.y - startPoint.y;
    bool widestIsX;
    float slope;
    float startW, startN;
    float endW;
    float stepW; // +1.0/-1.0
    if (std::abs(dx) > std::abs(dy))
    {
        widestIsX = true;
        slope = static_cast<float>(dy) / static_cast<float>(dx);
        startW = static_cast<float>(startPoint.x);
        startN = static_cast<float>(startPoint.y);
        endW = static_cast<float>(endPoint.x);
        stepW = static_cast<float>(dx) / static_cast<float>(std::abs(dx));
    }
    else
    {
        widestIsX = false;
        slope = static_cast<float>(dx) / static_cast<float>(dy);
        startW = static_cast<float>(startPoint.y);
        startN = static_cast<float>(startPoint.x);
        endW = static_cast<float>(endPoint.y);
        stepW = static_cast<float>(dy) / static_cast<float>(std::abs(dy));
    }

    float curW = startW;
    float curN = startN;

    auto const makePosition = [&]() ->_IntegralCoordinates<_TIntegralTag>
    {
        vec2f newPosition;
        if (widestIsX)
        {
            newPosition = vec2f(curW, curN);
        }
        else
        {
            newPosition = vec2f(curN, curW);
        }

        return _IntegralCoordinates<_TIntegralTag>(
            static_cast<typename _IntegralCoordinates<_TIntegralTag>::integral_type>(std::round(newPosition.x)),
            static_cast<typename _IntegralCoordinates<_TIntegralTag>::integral_type>(std::round(newPosition.y)));
    };

    //
    // Visit all other points
    //

    _IntegralCoordinates<_TIntegralTag> oldPosition = startPoint;

    while (true)
    {
        curW += stepW;

        if constexpr (TType == IntegralLineType::WithAdjacentSteps)
        {
            auto const newPosition = makePosition();
            if (newPosition != oldPosition)
            {
                visitor(newPosition);

                oldPosition = newPosition;
            }
        }

        curN += slope * stepW;

        auto const newPosition = makePosition();
        if (newPosition != oldPosition)
        {
            visitor(newPosition);

            oldPosition = newPosition;
        }

        // Check if done
        if (fabs(endW - curW) <= 0.5f)
        {
            // Reached destination
            break;
        }
    }
}

inline void MakeQuadInto(
    vec2f const & centerTop,
    vec2f const & centerBottom,
    vec2f const & hDir,
    float halfWidth,
    Quad & quad)
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

    __m128d vd = _mm_shuffle_pd(
        _mm_load_pd(reinterpret_cast<double const *>(&centerTop)),
        _mm_load_pd(reinterpret_cast<double const *>(&centerBottom)),
        0);

    __m128d hd0 = _mm_load_pd(reinterpret_cast<double const *>(&hDir));
    __m128 hd = _mm_movelh_ps(_mm_castpd_ps(hd0), _mm_castpd_ps(hd0));

    __m128 h = _mm_mul_ps(hd, _mm_load1_ps(&halfWidth));

    __m128 left = _mm_sub_ps(_mm_castpd_ps(vd), h);
    __m128 right = _mm_add_ps(_mm_castpd_ps(vd), h);
    _mm_store_ps(&(quad.fptr[0]), left);
    _mm_store_ps(&(quad.fptr[4]), right);

#else

    quad.V.TopLeft = vec2f(centerTop - hDir * halfWidth);
    quad.V.BottomLeft = vec2f(centerBottom - hDir * halfWidth);
    quad.V.TopRight = vec2f(centerTop + hDir * halfWidth);
    quad.V.BottomRight = vec2f(centerBottom + hDir * halfWidth);

#endif
}

inline Quad MakeQuad(
    vec2f const & centerTop,
    vec2f const & centerBottom,
    vec2f const & hDir,
    float halfWidth)
{
    Quad quad;
    MakeQuadInto(
        centerTop,
        centerBottom,
        hDir,
        halfWidth,
        quad);

    return quad;
}

}
