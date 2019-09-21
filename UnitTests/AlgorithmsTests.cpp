#include <GameCore/Algorithms.h>

#include "Utils.h"

#include <GameCore/GameTypes.h>

#include <cmath>

#include "gtest/gtest.h"

struct SpringEndpoints
{
    ElementIndex PointAIndex;
    ElementIndex PointBIndex;
};

TEST(AlgorithmsTests, CalculateVectorDirsAndReciprocalLengths)
{
    vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    SpringEndpoints springs[] = { {0, 1}, {1, 2}, {0, 3}, {2, 3} };
    vec2f outDirs[4];
    float outReciprocalLengths[4];

    Algorithms::CalculateVectorDirsAndReciprocalLengths(
        pointPositions,
        springs,
        outDirs,
        outReciprocalLengths,
        4);

    float constexpr Tolerance = 0.001f;

    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(5.0f), outReciprocalLengths[0], Tolerance));
    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(5.0f), outDirs[0].x, Tolerance));
    EXPECT_TRUE(ApproxEquals(2.0f / std::sqrt(5.0f), outDirs[0].y, Tolerance));

    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(65.0f), outReciprocalLengths[1], Tolerance));
    EXPECT_TRUE(ApproxEquals(8.0f / std::sqrt(65.0f), outDirs[1].x, Tolerance));
    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(65.0f), outDirs[1].y, Tolerance));

    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(8.0f), outReciprocalLengths[2], Tolerance));
    EXPECT_TRUE(ApproxEquals(2.0f / std::sqrt(8.0f), outDirs[2].x, Tolerance));
    EXPECT_TRUE(ApproxEquals(2.0f / std::sqrt(8.0f), outDirs[2].y, Tolerance));

    EXPECT_TRUE(ApproxEquals(1.0f / std::sqrt(50.0f), outReciprocalLengths[3], Tolerance));
    EXPECT_TRUE(ApproxEquals(-7.0f / std::sqrt(50.0f), outDirs[3].x, Tolerance));
    EXPECT_TRUE(ApproxEquals(-1.0f / std::sqrt(50.0f), outDirs[3].y, Tolerance));
}
