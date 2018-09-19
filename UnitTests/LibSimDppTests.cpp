#include <GameLib/Vectors.h>

#include <iostream>

// TODO: move to GameLib's LibSimdPp.h
#define SIMDPP_ARCH_X86_SSSE3
#include "simdpp/simd.h"

#include "gtest/gtest.h"

#include <vector>

TEST(LibSimDppTests, MulConstant)
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

    for (size_t i = 0; i < vectors.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(vectors.data() + i);
        block = block * simdpp::make_float<simdpp::float32v>(2.0f);
        simdpp::store(results.data() + i, block);
    }    

    EXPECT_EQ(vec2f(0.0f, 0.0f), results[0]);
    EXPECT_EQ(vec2f(2.0f, 2.0f), results[1]);
    EXPECT_EQ(vec2f(8.0f, 4.0f), results[2]);
    EXPECT_EQ(vec2f(0.2f, 0.4f), results[3]);
}

TEST(LibSimDppTests, ReciprocalSquareRoot)
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

TEST(LibSimDppTests, ReciprocalSquareRoot_WithMask)
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
        simdpp::mask_float32v validMask = (block != 0.0f);

        block = simdpp::rsqrt_e(block);
        simdpp::float32v result = simdpp::blend(block, simdpp::make_float<simdpp::float32v>(0.0f), validMask);
        simdpp::store(results.data() + i, result);
    }

    EXPECT_EQ(0.0f, results[5]);
}

TEST(LibSimDppTests, VectorNormalization)
{
    std::vector<vec2f> vectors{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 2.0f},
        {0.01f, 0.02f}
    };

    std::vector<vec2f> results;
    results.resize(vectors.size());

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 2; // Vec has two components

    for (size_t i = 0; i < vectors.size(); i += BatchSize)
    {
        // TODOHERE
        simdpp::float32v block = simdpp::load(vectors.data() + i);
        block = block * simdpp::make_float<simdpp::float32v>(2.0f);
        simdpp::store(results.data() + i, block);
    }

    EXPECT_EQ(vec2f(0.0f, 0.0f), results[0]);
    EXPECT_EQ(vec2f(0.707107f, 0.707107f), results[1]);
    EXPECT_EQ(vec2f(0.894427, 0.447214), results[2]);
    EXPECT_EQ(vec2f(0.447214, 0.894427), results[3]);
}