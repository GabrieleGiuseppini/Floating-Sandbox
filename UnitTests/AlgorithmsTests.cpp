#include <GameCore/Algorithms.h>

#include <cmath>

#include "gtest/gtest.h"

TEST(AlgorithmTests, CalculateVectorLengthsAndDirs)
{
    float inOutXBuffer[4] = { 1.0f, 2.0f, 10.0f, 3.0f };
    float inOutYBuffer[4] = { 2.0f, 4.0f, 5.0f, 4.0f };
    float outLengthBuffer[4];

    Algorithms::CalculateVectorLengthsAndDirs(
        inOutXBuffer,
        inOutYBuffer,
        outLengthBuffer,
        4);

    EXPECT_FLOAT_EQ(1.0f / std::sqrt(5.0f), inOutXBuffer[0]);
    EXPECT_FLOAT_EQ(2.0f / std::sqrt(5.0f), inOutYBuffer[0]);
    EXPECT_FLOAT_EQ(std::sqrt(5.0f), outLengthBuffer[0]);

    EXPECT_FLOAT_EQ(2.0f / std::sqrt(20.0f), inOutXBuffer[1]);
    EXPECT_FLOAT_EQ(4.0f / std::sqrt(20.0f), inOutYBuffer[1]);
    EXPECT_FLOAT_EQ(std::sqrt(20.0f), outLengthBuffer[1]);

    EXPECT_FLOAT_EQ(10.0f / std::sqrt(125.0f), inOutXBuffer[2]);
    EXPECT_FLOAT_EQ(5.0f / std::sqrt(125.0f), inOutYBuffer[2]);
    EXPECT_FLOAT_EQ(std::sqrt(125.0f), outLengthBuffer[2]);

    EXPECT_FLOAT_EQ(3.0f / std::sqrt(25.0f), inOutXBuffer[3]);
    EXPECT_FLOAT_EQ(4.0f / std::sqrt(25.0f), inOutYBuffer[3]);
    EXPECT_FLOAT_EQ(std::sqrt(25.0f), outLengthBuffer[3]);
}
