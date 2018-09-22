#include <GameLib/GameTypes.h>
#include <GameLib/SysSpecifics.h>
#include <GameLib/Vectors.h>

#include <iostream>

// TODO: move to GameLib's LibSimdPp.h
#define SIMDPP_ARCH_X86_SSSE3
#include "simdpp/simd.h"

#include "gtest/gtest.h"

#include <vector>

TEST(LibSimdPpTests, MulConstant)
{
    std::vector<vec2f> vectors{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 2.0f},
        {0.1f, 0.2f}
    };

    std::vector<vec2f> results{
        vec2f::zero(),
        vec2f::zero(),
        vec2f::zero(),
        vec2f::zero()
    };

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 2; // Vec has two components

    vec2f * restrict vectorsData = vectors.data();
    vec2f * restrict resultsData = results.data();

    for (size_t i = 0; i < vectors.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(vectorsData + i);
        block = block * simdpp::make_float<simdpp::float32v>(2.0f);
        simdpp::store(resultsData + i, block);
    }    

    EXPECT_EQ(vec2f(0.0f, 0.0f), results[0]);
    EXPECT_EQ(vec2f(2.0f, 2.0f), results[1]);
    EXPECT_EQ(vec2f(8.0f, 4.0f), results[2]);
    EXPECT_EQ(vec2f(0.2f, 0.4f), results[3]);
}

TEST(LibSimdPpTests, ReciprocalSquareRoot)
{
    std::vector<float> values{
        1.0f, 2.0, 4.0f, 9.0, 81.0f, 0.0f, 100.0f, 10000.0f
    };

    std::vector<float> results;
    results.resize(values.size());

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 1;

    for (size_t i = 0; i < values.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(values.data() + i);
        block = simdpp::rsqrt_e(block);
        simdpp::store(results.data() + i, block);
    }

    ////EXPECT_EQ(1.0f/1.0f, results[0]);
    ////EXPECT_EQ(1.0f / 2.0f, results[2]);
    ////EXPECT_EQ(1.0f / 3.0f, results[3]);
    ////EXPECT_EQ(1.0f / 9.0f, results[4]);
    ////EXPECT_EQ(1.0f / 10.0f, results[6]);
    ////EXPECT_EQ(1.0f / 100.0f, results[7]);
}

TEST(LibSimdPpTests, ReciprocalSquareRoot_WithMask)
{
    std::vector<float> values{
        1.0f, 2.0, 4.0f, 9.0, 81.0f, 0.0f, 100.0f, 10000.0f
    };

    std::vector<float> results;
    results.resize(values.size());

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 1;

    float * restrict valuesData = values.data();
    float * restrict resultsData = results.data();

    for (size_t i = 0; i < values.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(valuesData + i);

        simdpp::mask_float32v validMask = (block != 0.0f);
        block = simdpp::rsqrt_e(block);
        simdpp::float32v result = simdpp::blend(block, simdpp::make_float<simdpp::float32v>(0.0f), validMask);

        simdpp::store(resultsData + i, result);
    }

    EXPECT_EQ(0.0f, results[5]);
}

TEST(LibSimdPpTests, VectorNormalization)
{
    std::vector<vec2f> points{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 82.0f},
        {0.00001f, 0.00002f}
    };

    struct Spring
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
    };

    std::vector<Spring> springs{
        {0, 1},
        {0, 2},
        {0, 3},
        {1, 2},
        {1, 3},
        {2, 3},
        {0, 0},
        {0, 0}
    };


    //
    // Test
    //

    vec2f * const restrict pointData = points.data();
    Spring * const restrict springData = springs.data();

    std::vector<vec2f> springDirs;
    springDirs.resize(springs.size());
    vec2f * restrict springDirsData = springDirs.data();

    std::vector<float> springLengths;
    springLengths.resize(springs.size());
    float * restrict springLengthsData = springLengths.data();

    // TODOHERE: make vectorization-size agnostic
    for (size_t s = 0; s < springs.size(); s += 4)
    {
        float pAx_[4] = {
            pointData[springData[s + 0].PointAIndex].x,
            pointData[springData[s + 1].PointAIndex].x,
            pointData[springData[s + 2].PointAIndex].x,
            pointData[springData[s + 3].PointAIndex].x
        };

        simdpp::float32<4> pAx = simdpp::load(pAx_);

        simdpp::float32<4> pAy = simdpp::make_float(
            pointData[springData[s + 0].PointAIndex].y,
            pointData[springData[s + 1].PointAIndex].y,
            pointData[springData[s + 2].PointAIndex].y,
            pointData[springData[s + 3].PointAIndex].y);

        simdpp::float32<4> pBx = simdpp::make_float(
            pointData[springData[s + 0].PointBIndex].x,
            pointData[springData[s + 1].PointBIndex].x,
            pointData[springData[s + 2].PointBIndex].x,
            pointData[springData[s + 3].PointBIndex].x);

        simdpp::float32<4> pBy = simdpp::make_float(
            pointData[springData[s + 0].PointBIndex].y,
            pointData[springData[s + 1].PointBIndex].y,
            pointData[springData[s + 2].PointBIndex].y,
            pointData[springData[s + 3].PointBIndex].y);

        simdpp::float32<4> displacementX = pBx - pAx;
        simdpp::float32<4> displacementY = pBy - pAy;

        simdpp::float32<4> _dx = displacementX * displacementX;
        simdpp::float32<4> _dy = displacementY * displacementY;

        simdpp::float32<4> _dxy = _dx + _dy;

        simdpp::float32<4> springLength = simdpp::sqrt(_dxy);
        
        displacementX = displacementX / springLength;
        displacementY = displacementY / springLength;

        simdpp::mask_float32<4> validMask = (springLength != 0.0f);

        displacementX = simdpp::bit_and(displacementX, validMask);
        displacementY = simdpp::bit_and(displacementY, validMask);

        simdpp::store(springLengthsData + s, springLength);
        simdpp::store_packed2(springDirsData + s, displacementX, displacementY);
    }

    //
    // Results
    //

    for (size_t s = 0; s < springs.size(); s++)
    {
        vec2f const displacement = points[springs[s].PointBIndex] - points[springs[s].PointAIndex];
        float const springLength = displacement.length();
        vec2f const springDir = displacement.normalise(springLength);

        EXPECT_FLOAT_EQ(springLength, springLengths[s]);
        EXPECT_FLOAT_EQ(springDir.x, springDirs[s].x);
        EXPECT_FLOAT_EQ(springDir.y, springDirs[s].y);
    }
}