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

#include <array>
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
     * If the point projects outside of the segment, the distance
     * returned is that from the closest endpoint.
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

    /*
     * Returns the distance between a point and a segment, or +INF
     * if the point does not project onto the segment.
     */
    inline static float SquareDistanceToPointUncapped(
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
        const float t = (point - segmentP1).dot(segmentP2 - segmentP1) / segmentSquaredLength;
        if (t < 0.0f || t > 1.0f)
            return std::numeric_limits<float>::max();
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

inline bool AreVerticesInCwOrder(
    vec2f const & aPosition,
    vec2f const & bPosition,
    vec2f const & cPosition)
{
    return (bPosition.x - aPosition.x) * (cPosition.y - aPosition.y) - (cPosition.x - aPosition.x) * (bPosition.y - aPosition.y) < 0;
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

#if defined(__GNUC__)
/* Test for GCC >= 7.3.1 */
#if __GNUC__ > 7 || \
    (__GNUC__ == 7 && (__GNUC_MINOR__ > 3 || \
                       (__GNUC_MINOR__ == 3 && \
                        __GNUC_PATCHLEVEL__ >= 1)))
#pragma GCC diagnostic push
// We load a vec2f below (2 floats) into a 4-float register
#pragma GCC diagnostic ignored "-Warray-bounds="
#define HAS_DISABLED_ARRAY_BOUNDS
#endif
#endif

inline void MakeQuadInto(
    vec2f const & centerTop,
    vec2f const & centerBottom,
    vec2f const & hDir,
    float halfWidth,
    Quad & quad)
{
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

    assert(is_aligned_to_vectorization_word(&quad));

    __m128d vd = _mm_shuffle_pd(
        _mm_loadu_pd(reinterpret_cast<double const *>(&centerTop)),
        _mm_loadu_pd(reinterpret_cast<double const *>(&centerBottom)),
        0);

    __m128d hd0 = _mm_loadu_pd(reinterpret_cast<double const *>(&hDir));
    __m128 hd = _mm_movelh_ps(_mm_castpd_ps(hd0), _mm_castpd_ps(hd0));

    __m128 h = _mm_mul_ps(hd, _mm_load1_ps(&halfWidth));

    __m128 left = _mm_sub_ps(_mm_castpd_ps(vd), h);
    __m128 right = _mm_add_ps(_mm_castpd_ps(vd), h);
    _mm_store_ps(&(quad.fptr[0]), left);
    _mm_store_ps(&(quad.fptr[4]), right);

#elif FS_IS_ARM_NEON() // Implies ARM anyways

    //assert(is_aligned_to_vectorization_word(&quad)); // Not necessarily, we are aligned to 4*4 but struct is 4*2

    float32x4_t vd = vcombine_f32(
        vld1_f32(reinterpret_cast<float const *>(&centerTop)),
        vld1_f32(reinterpret_cast<float const *>(&centerBottom)));

    float32x2_t hd0 = vld1_f32(reinterpret_cast<float const *>(&hDir));
    float32x4_t hd = vcombine_f32(hd0, hd0);
    float32x4_t h = vmulq_f32(hd, vld1q_dup_f32(&halfWidth));

    vst1q_f32(reinterpret_cast<float *>(&(quad.fptr[0])), vsubq_f32(vd, h));
    vst1q_f32(reinterpret_cast<float *>(&(quad.fptr[4])), vaddq_f32(vd, h));

#else

    quad.V.TopLeft = vec2f(centerTop - hDir * halfWidth);
    quad.V.BottomLeft = vec2f(centerBottom - hDir * halfWidth);
    quad.V.TopRight = vec2f(centerTop + hDir * halfWidth);
    quad.V.BottomRight = vec2f(centerBottom + hDir * halfWidth);

#endif
}

#if defined(__GNUC__)
#ifdef HAS_DISABLED_ARRAY_BOUNDS
#pragma GCC diagnostic pop
#endif
#endif

inline Quad MakeQuad(
    vec2f const & centerTop,
    vec2f const & centerBottom,
    vec2f const & hDir,
    float halfWidth)
{
    FS_ALIGN16_BEG Quad quad FS_ALIGN16_END;
    MakeQuadInto(
        centerTop,
        centerBottom,
        hDir,
        halfWidth,
        quad);

    return quad;
}

template<typename TLandVertex, typename TVertexBuffer>
void GenerateSegmentedLandQuad(
    TLandVertex leftTop,
    TLandVertex leftBottom,
    TLandVertex rightTop,
    TLandVertex rightBottom,
    float maxQuadTriangleHeight,
    float minQuadTriangleHeight,
    TVertexBuffer & vertexBuffer)
{
    //       D
    //      /|
    //     / |
    //  A /--| D'
    //    |  |
    //  A'|--|C
    //    | /
    //  B |/

    assert(leftTop.position.x < rightTop.position.x);
    assert(leftTop.position.x == leftBottom.position.x);
    assert(rightTop.position.x == rightTop.position.x);
    assert(leftTop.position.y >= leftBottom.position.y);
    assert(rightTop.position.y >= rightBottom.position.y);

    // For convenience
    int constexpr A = 0;
    int constexpr B = 1;
    int constexpr C = 2;
    int constexpr D = 3;
    std::array<TLandVertex const, 4> const vertices = { leftTop, leftBottom, rightBottom, rightTop };

    // The two segments (left and right) that we are currently scanning down along

    struct Segment
    {
        int Start;
        int End;
    };

    // Left, right
    std::array<Segment, 2> currentSegmentEndpoints;

    //
    // Initialize segments
    //

    if (leftTop.position.y > rightTop.position.y)
    {
        currentSegmentEndpoints = { Segment{A, B}, Segment{A, D} };
    }
    else if (rightTop.position.y > leftTop.position.y)
    {
        currentSegmentEndpoints = { Segment{D, A}, Segment{D, C} };
    }
    else
    {
        assert(leftTop.position.y == rightTop.position.y);
        currentSegmentEndpoints = { Segment{A, B}, Segment{D, C} };
    }

    //
    // Scan line down
    //

    // The current left and right vertices of this scan line;
    // we've created quads up to here.
    //
    // Not always at real vertices (might be halfway through a real edge)
    std::array<TLandVertex, 2> currentScanLineVertices = { vertices[currentSegmentEndpoints[0].Start], vertices[currentSegmentEndpoints[1].Start] };

    float currentY = std::max(leftTop.position.y, rightTop.position.y);
    assert(currentY == std::max(currentScanLineVertices[0].position.y, currentScanLineVertices[1].position.y));
    float const maxBottomY = std::min(leftBottom.position.y, rightBottom.position.y);

    while (currentY > maxBottomY)
    {
        //
        // Scan current segments down until we meet the first one
        //

        // Prepare interpolations

        // Slopes on entire quad sides: end-start; left, right
        std::array<float, 2> Dx = {
            vertices[currentSegmentEndpoints[0].End].position.x - vertices[currentSegmentEndpoints[0].Start].position.x,
            vertices[currentSegmentEndpoints[1].End].position.x - vertices[currentSegmentEndpoints[1].Start].position.x };
        std::array<float, 2> Dy = {
            vertices[currentSegmentEndpoints[0].End].position.y - vertices[currentSegmentEndpoints[0].Start].position.y,
            vertices[currentSegmentEndpoints[1].End].position.y - vertices[currentSegmentEndpoints[1].Start].position.y };
        std::array<float, 2> Ddepth = {
            vertices[currentSegmentEndpoints[0].End].depth - vertices[currentSegmentEndpoints[0].Start].depth,
            vertices[currentSegmentEndpoints[1].End].depth - vertices[currentSegmentEndpoints[1].Start].depth };
        std::array<float, 2> DinterfaceBlendDepth = {
            vertices[currentSegmentEndpoints[0].End].interfaceBlendDepth - vertices[currentSegmentEndpoints[0].Start].interfaceBlendDepth,
            vertices[currentSegmentEndpoints[1].End].interfaceBlendDepth - vertices[currentSegmentEndpoints[1].Start].interfaceBlendDepth };

        // This is where we stop
        float const currentBottomY = std::max(vertices[currentSegmentEndpoints[0].End].position.y, vertices[currentSegmentEndpoints[1].End].position.y);

        while (currentY > currentBottomY)
        {
            // The current vertices are at the same y
            assert(currentScanLineVertices[0].position.y == currentScanLineVertices[1].position.y);

            // Calculate bottom y for this segment
            float newBottomY = currentBottomY; // Shoot for max first
            if (newBottomY < currentY - maxQuadTriangleHeight - minQuadTriangleHeight)
            {
                newBottomY = currentY - maxQuadTriangleHeight;
            }

            //
            // Calculate next scan line vertices, after traveling down to newBottomY
            //

            std::array<TLandVertex, 2> newScanLineVertices = { TLandVertex{vec2f(), 0.0f, 0.0f}, TLandVertex{vec2f(), 0.0f, 0.0f} };

            if (vertices[currentSegmentEndpoints[0].End].position.y == newBottomY)
            {
                // We've reached this one, use it verbatim to reduce error accumulation
                newScanLineVertices[0] = vertices[currentSegmentEndpoints[0].End];
            }
            else
            {
                // Interpolate

                float const dyDy = Dy[0] != 0.0f
                    ? (newBottomY - vertices[currentSegmentEndpoints[0].Start].position.y) / Dy[0]
                    : 0.0f;

                newScanLineVertices[0] = {
                    vec2f(
                        vertices[currentSegmentEndpoints[0].Start].position.x + Dx[0] * dyDy,
                        newBottomY),
                    vertices[currentSegmentEndpoints[0].Start].depth + Ddepth[0] * dyDy,
                    vertices[currentSegmentEndpoints[0].Start].interfaceBlendDepth + DinterfaceBlendDepth[0] * dyDy };
            }

            if (vertices[currentSegmentEndpoints[1].End].position.y == newBottomY)
            {
                // We've reached this one, use it verbatim to reduce error accumulation
                newScanLineVertices[1] = vertices[currentSegmentEndpoints[1].End];
            }
            else
            {
                // Interpolate

                float const dyDy = Dy[1] != 0.0f
                    ? (newBottomY - vertices[currentSegmentEndpoints[1].Start].position.y) / Dy[1]
                    : 0.0f;

                newScanLineVertices[1] = {
                    vec2f(
                        vertices[currentSegmentEndpoints[1].Start].position.x + Dx[1] * dyDy,
                        newBottomY),
                    vertices[currentSegmentEndpoints[1].Start].depth + Ddepth[1] * dyDy,
                    vertices[currentSegmentEndpoints[1].Start].interfaceBlendDepth + DinterfaceBlendDepth[1] * dyDy };
            }

            //
            // Produce quad
            //
            //    J------K
            //    |      |
            //    |      |
            //   J'------K'
            //

            if (currentScanLineVertices[0].position.x < currentScanLineVertices[1].position.x) // J != K
            {
                // J-K-K'
                vertexBuffer.emplace_back(currentScanLineVertices[0]);
                vertexBuffer.emplace_back(currentScanLineVertices[1]);
                vertexBuffer.emplace_back(newScanLineVertices[1]);
            }

            if (newScanLineVertices[0].position.x < newScanLineVertices[1].position.x) // J' != K'
            {
                // J-K'-J'
                vertexBuffer.emplace_back(currentScanLineVertices[0]);
                vertexBuffer.emplace_back(newScanLineVertices[1]);
                vertexBuffer.emplace_back(newScanLineVertices[0]);
            }

            //
            // Advance
            //

            currentScanLineVertices = newScanLineVertices;
            currentY = newBottomY;
        }

        //
        // We have reached one or both ends; generate next segment(s)
        //

        assert(currentY == vertices[currentSegmentEndpoints[0].End].position.y || currentY == vertices[currentSegmentEndpoints[1].End].position.y);

        if (currentY == vertices[currentSegmentEndpoints[0].End].position.y)
        {
            currentSegmentEndpoints[0].Start = currentSegmentEndpoints[0].End;
            currentSegmentEndpoints[0].End = currentSegmentEndpoints[0].End + 1;
            if (currentSegmentEndpoints[0].End >= 4)
            {
                currentSegmentEndpoints[0].End -= 4;
            }
        }

        if (currentY == vertices[currentSegmentEndpoints[1].End].position.y)
        {
            currentSegmentEndpoints[1].Start = currentSegmentEndpoints[1].End;
            currentSegmentEndpoints[1].End = currentSegmentEndpoints[1].End - 1;
            if (currentSegmentEndpoints[1].End < 0)
            {
                currentSegmentEndpoints[1].End += 4;
            }
        }
    }
}

}
