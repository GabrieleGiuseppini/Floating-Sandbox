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

TEST(AlgorithmsTests, DiffuseLight)
{
    vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    vec2f lampPositions[] = { { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    PlaneId lampPlaneIds[] = { 3, 2, 10, 10 };
    float lampDistanceCoeffs[] = { 0.1f, 0.2f, 10.0f, 20.0f };
    float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f };

    float outLightBuffer[4];

    Algorithms::DiffuseLight(
        pointPositions,
        pointPlaneIds,
        4,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        4,
        outLightBuffer);

    // Point1:
    //  - Lamp1: D=3 NewLight=0.1*(4-3) = 0.1
    //  - Lamp2: D=0 NewLight=0.2*(6-0) = 1.2 // Truncated

    EXPECT_FLOAT_EQ(1.0f, outLightBuffer[0]);

    // Point2:
    //  - Lamp1: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.11715
    //  - Lamp2: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.75278

    EXPECT_FLOAT_EQ(0.7527864f, outLightBuffer[1]);

    // Point3:
    //  - Lamp1: D=sqrt(45) NewLight=0.1*(4-sqrt(45)) = 0.0
    //  - Lamp2: D=sqrt(90) NewLight=0.2*(6-sqrt(90)) = 0.0

    EXPECT_FLOAT_EQ(0.0f, outLightBuffer[2]);

    // Point4:
    //  - Lamp1: D=sqrt(5) NewLight=0.1*(4-sqrt(5)) = 0.17639320225
    //  - Lamp2: D=sqrt(8) NewLight=0.2*(6-sqrt(8)) = 0.63431457505 // Excluded by planeID

    EXPECT_FLOAT_EQ(0.17639320225f, outLightBuffer[3]);
}