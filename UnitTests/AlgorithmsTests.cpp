#include <GameCore/Algorithms.h>
#include <GameCore/Vectors.h>

#include "Utils.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <cmath>

#include "gtest/gtest.h"

struct SpringEndpoints
{
    ElementIndex PointAIndex;
    ElementIndex PointBIndex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffuseLight
///////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(AlgorithmsTests, DiffuseLight_Naive_1Lamp)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { { 4.0f, 2.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 3 };
    aligned_to_vword float lampDistanceCoeffs[] = { 0.1f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_Naive(
        pointPositions,
        pointPlaneIds,
        4,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        1,
        outLightBuffer);

    // Point1:
    //  - Lamp1: D=3 NewLight=0.1*(4-3) = 0.1

    EXPECT_FLOAT_EQ(0.1f, outLightBuffer[0]);

    // Point2:
    //  - Lamp1: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573

    EXPECT_FLOAT_EQ(0.1171573f, outLightBuffer[1]);

    // Point3:
    //  - Lamp1: D=sqrt(45) NewLight=0.1*(4-sqrt(45)) = 0.0

    EXPECT_FLOAT_EQ(0.0f, outLightBuffer[2]);

    // Point4:
    //  - Lamp1: D=sqrt(5) NewLight=0.1*(4-sqrt(5)) = 0.17639320225

    EXPECT_FLOAT_EQ(0.17639320225f, outLightBuffer[3]);
}

TEST(AlgorithmsTests, DiffuseLight_Naive_3Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { {1.0f, 2.0f}, {100.0f, 100.0f}, { 4.0f, 2.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 2, 10, 3 };
    aligned_to_vword float lampDistanceCoeffs[] = { 0.2f, 10.0f, 0.1f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 6.0f, 1.0f, 4.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_Naive(
        pointPositions,
        pointPlaneIds,
        4,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        3,
        outLightBuffer);

    // Point1:
    //  - Lamp1: D=3 NewLight=0.1*(4-3) = 0.1
    //  - Lamp2: D=0 NewLight=0.2*(6-0) = 1.2 // Truncated

    EXPECT_FLOAT_EQ(1.0f, outLightBuffer[0]);

    // Point2:
    //  - Lamp1: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573
    //  - Lamp2: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.7527864

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

TEST(AlgorithmsTests, DiffuseLight_Vectorized_4Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_Vectorized(
        0,
        4,
        pointPositions,
        pointPlaneIds,
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
    //  - Lamp1: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573
    //  - Lamp2: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.7527864

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

TEST(AlgorithmsTests, DiffuseLight_Vectorized_8Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { {100.0f, 100.0f}, {200.0f, 200.0f}, {100.0f, 100.0f}, {200.0f, 200.0f}, { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 1, 1, 1, 1, 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 10.0f, 20.0f, 10.0f, 20.0f, 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f, 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_Vectorized(
        0,
        4,
        pointPositions,
        pointPlaneIds,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        8,
        outLightBuffer);

    // Point1:
    //  - Lamp5: D=3 NewLight=0.1*(4-3) = 0.1
    //  - Lamp6: D=0 NewLight=0.2*(6-0) = 1.2 // Truncated

    EXPECT_FLOAT_EQ(1.0f, outLightBuffer[0]);

    // Point2:
    //  - Lamp5: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573
    //  - Lamp6: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.7527864

    EXPECT_FLOAT_EQ(0.7527864f, outLightBuffer[1]);

    // Point3:
    //  - Lamp5: D=sqrt(45) NewLight=0.1*(4-sqrt(45)) = 0.0
    //  - Lamp6: D=sqrt(90) NewLight=0.2*(6-sqrt(90)) = 0.0

    EXPECT_FLOAT_EQ(0.0f, outLightBuffer[2]);

    // Point4:
    //  - Lamp5: D=sqrt(5) NewLight=0.1*(4-sqrt(5)) = 0.17639320225
    //  - Lamp6: D=sqrt(8) NewLight=0.2*(6-sqrt(8)) = 0.63431457505 // Excluded by planeID

    EXPECT_FLOAT_EQ(0.17639320225f, outLightBuffer[3]);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, DiffuseLight_SSEVectorized_4Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_SSEVectorized(
        0,
        4,
        pointPositions,
        pointPlaneIds,
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
    //  - Lamp1: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573
    //  - Lamp2: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.7527864

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

TEST(AlgorithmsTests, DiffuseLight_SSEVectorized_8Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { {100.0f, 100.0f}, {200.0f, 200.0f}, {100.0f, 100.0f}, {200.0f, 200.0f}, { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 1, 1, 1, 1, 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 10.0f, 20.0f, 10.0f, 20.0f, 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f, 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_SSEVectorized(
        0,
        4,
        pointPositions,
        pointPlaneIds,
        lampPositions,
        lampPlaneIds,
        lampDistanceCoeffs,
        lampSpreadMaxDistances,
        8,
        outLightBuffer);

    // Point1:
    //  - Lamp5: D=3 NewLight=0.1*(4-3) = 0.1
    //  - Lamp6: D=0 NewLight=0.2*(6-0) = 1.2 // Truncated

    EXPECT_FLOAT_EQ(1.0f, outLightBuffer[0]);

    // Point2:
    //  - Lamp5: D=sqrt(8) NewLight=0.1*(4-sqrt(8)) = 0.1171573
    //  - Lamp6: D=sqrt(5) NewLight=0.2*(6-sqrt(5)) = 0.7527864

    EXPECT_FLOAT_EQ(0.7527864f, outLightBuffer[1]);

    // Point3:
    //  - Lamp5: D=sqrt(45) NewLight=0.1*(4-sqrt(45)) = 0.0
    //  - Lamp6: D=sqrt(90) NewLight=0.2*(6-sqrt(90)) = 0.0

    EXPECT_FLOAT_EQ(0.0f, outLightBuffer[2]);

    // Point4:
    //  - Lamp5: D=sqrt(5) NewLight=0.1*(4-sqrt(5)) = 0.17639320225
    //  - Lamp6: D=sqrt(8) NewLight=0.2*(6-sqrt(8)) = 0.63431457505 // Excluded by planeID

    EXPECT_FLOAT_EQ(0.17639320225f, outLightBuffer[3]);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferSmoothing
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Algorithm>
void RunSmoothBufferAndAddTest_12_5(Algorithm algorithm)
{
    // Prefix of buffer "body" necessary to include half of average window
    // (with initial zeroes) *and* making the buffer "body" aligned
    size_t constexpr BufferBodyPrefixSize = make_aligned_float_element_count(5 / 2); // Before "body"
    size_t constexpr BufferPrefixSize = BufferBodyPrefixSize - (5 / 2); // Before zeroes
    static_assert(BufferPrefixSize == 2);

    aligned_to_vword float inBuffer[] = {
        0.0f, 0.0f, // BufferPrefix

        0.0f,
        0.0f,

        1.0f, 4.0f, 5.0f, 100.0f,
        200.0f, 2.0f, 5.0f,  6.0f,
        150.0f, 1000.0f, 7.0f, -5.0,

        0.0f,
        0.0f
    };

    aligned_to_vword float outBuffer[] = {
        2.0f, 2.0f, 2.0f, 2.0f,
        2.0f, 2.0f, 2.0f, 2.0f,
        2.0f, 2.0f, 2.0f, 2.0f
    };

    algorithm(
        inBuffer + BufferPrefixSize + 2,
        outBuffer);

    EXPECT_FLOAT_EQ((0.0f * 1.0f + 0.0f * 2.0f + 1.0f * 3.0f + 4.0f * 2.0f + 5.0f * 1.0f) / 25.0f + 2.0f, outBuffer[0]);
    EXPECT_FLOAT_EQ((0.0f * 1.0f + 1.0f * 2.0f + 4.0f * 3.0f + 5.0f * 2.0f + 100.0f * 1.0f) / 25.0f + 2.0f, outBuffer[1]);
    EXPECT_FLOAT_EQ((1.0f * 1.0f + 4.0f * 2.0f + 5.0f * 3.0f + 100.0f * 2.0f + 200.0f * 1.0f) / 25.0f + 2.0f, outBuffer[2]);
    EXPECT_FLOAT_EQ((4.0f * 1.0f + 5.0f * 2.0f + 100.0f * 3.0f + 200.0f * 2.0f + 2.0f * 1.0f) / 25.0f + 2.0f, outBuffer[3]);
    EXPECT_FLOAT_EQ((5.0f * 1.0f + 100.0f * 2.0f + 200.0f * 3.0f + 2.0f * 2.0f + 5.0f * 1.0f) / 25.0f + 2.0f, outBuffer[4]);
    EXPECT_FLOAT_EQ((100.0f * 1.0f + 200.0f * 2.0f + 2.0f * 3.0f + 5.0f * 2.0f + 6.0f * 1.0f) / 25.0f + 2.0f, outBuffer[5]);
    EXPECT_FLOAT_EQ((200.0f * 1.0f + 2.0f * 2.0f + 5.0f * 3.0f + 6.0f * 2.0f + 150.0f * 1.0f) / 25.0f + 2.0f, outBuffer[6]);
    EXPECT_FLOAT_EQ((2.0f * 1.0f + 5.0f * 2.0f + 6.0f * 3.0f + 150.0f * 2.0f + 1000.0f * 1.0f) / 25.0f + 2.0f, outBuffer[7]);
    EXPECT_FLOAT_EQ((5.0f * 1.0f + 6.0f * 2.0f + 150.0f * 3.0f + 1000.0f * 2.0f + 7.0f * 1.0f) / 25.0f + 2.0f, outBuffer[8]);
    EXPECT_FLOAT_EQ((6.0f * 1.0f + 150.0f * 2.0f + 1000.0f * 3.0f + 7.0f * 2.0f + -5.0f * 1.0f) / 25.0f + 2.0f, outBuffer[9]);
    EXPECT_FLOAT_EQ((150.0f * 1.0f + 1000.0f * 2.0f + 7.0f * 3.0f + -5.0f * 2.0f + 0.0f * 1.0f) / 25.0f + 2.0f, outBuffer[10]);
    EXPECT_FLOAT_EQ((1000.0f * 1.0f + 7.0f * 2.0f + -5.0f * 3.0f + 0.0f * 2.0f + 0.0f * 1.0f) / 25.0f + 2.0f, outBuffer[11]);
}

TEST(AlgorithmsTests, SmoothBufferAndAdd_12_5_Naive)
{
    RunSmoothBufferAndAddTest_12_5(Algorithms::SmoothBufferAndAdd_Naive<12, 5>);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, SmoothBufferAndAdd_12_5_SSEVectorized)
{
    RunSmoothBufferAndAddTest_12_5(Algorithms::SmoothBufferAndAdd_SSEVectorized<12, 5>);
}
#endif