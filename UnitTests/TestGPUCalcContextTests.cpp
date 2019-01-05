#include <GPUCalc/GPUCalcContextFactory.h>
#include <GPUCalc/TestGPUCalcContext.h>

#include "gtest/gtest.h"

TEST(TestGPUCalcContextTests, Validate_Basic)
{
    vec2f a[5] = {
        {1.0f, 1.0f},
        {2.0f, 1.0f},
        {3.0f, 1.0f},
        {4.0f, 1.0f},
        {5.0f, 1.0f}
    };

    vec2f b[5] = {
        {0.1f, 10.0f},
        {0.2f, 20.0f},
        {0.4f, 30.0f},
        {0.8f, 40.0f},
        {1.0f, 50.0f}
    };

    vec2f result[5] = {
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f}
    };


    //
    // Register OpenGL context factory
    //

    // TODO


    //
    // Create calc context and calculate
    //

    auto calcContext = GPUCalcContextFactory::GetInstance().CreateTestContext(5);

    calcContext->Add(a, b, result);


    //
    // Validate
    //

    EXPECT_EQ(result[0], vec2f(1.1f, 11.0f));
    EXPECT_EQ(result[1], vec2f(2.2f, 21.0f));
    EXPECT_EQ(result[2], vec2f(3.4f, 31.0f));
    EXPECT_EQ(result[3], vec2f(4.8f, 41.0f));
    EXPECT_EQ(result[4], vec2f(6.0f, 51.0f));
}