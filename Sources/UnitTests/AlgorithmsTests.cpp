#include <Core/Algorithms.h>

#include <Core/GameGeometry.h>
#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <array>
#include <cmath>

#include "TestingUtils.h"

#include "gtest/gtest.h"

#ifdef _MSC_VER
#pragma warning(disable : 4324) // std::optional of StateType gets padded because of alignment requirements
#endif

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

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, DiffuseLight_NeonVectorized_4Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_NeonVectorized(
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

TEST(AlgorithmsTests, DiffuseLight_NeonVectorized_8Lamps)
{
    aligned_to_vword vec2f pointPositions[] = { { 1.0f, 2.0f}, {2.0f, 4.0f}, {10.0f, 5.0f}, {3.0f, 4.0f} };
    aligned_to_vword PlaneId pointPlaneIds[] = { 1, 1, 2, 3 };

    aligned_to_vword vec2f lampPositions[] = { {100.0f, 100.0f}, {200.0f, 200.0f}, {100.0f, 100.0f}, {200.0f, 200.0f}, { 4.0f, 2.0f}, {1.0f, 2.0f}, {100.0f, 100.0f}, {200.0f, 200.0f} };
    aligned_to_vword PlaneId lampPlaneIds[] = { 1, 1, 1, 1, 3, 2, 10, 10 };
    aligned_to_vword float lampDistanceCoeffs[] = { 10.0f, 20.0f, 10.0f, 20.0f, 0.1f, 0.2f, 10.0f, 20.0f };
    aligned_to_vword float lampSpreadMaxDistances[] = { 4.0f, 6.0f, 1.0f, 2.0f, 4.0f, 6.0f, 1.0f, 2.0f };

    aligned_to_vword float outLightBuffer[4];

    Algorithms::DiffuseLight_NeonVectorized(
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
void RunSmoothBufferAndAddTest(Algorithm algorithm)
{
    // Prefix of buffer "body" necessary to include half of average window
    // (with initial zeroes) *and* making the buffer "body" aligned
    size_t constexpr BufferBodyPrefixSize = make_aligned_float_element_count(5 / 2); // Before "body"
    size_t constexpr BufferPrefixSize = BufferBodyPrefixSize - (5 / 2); // Before zeroes

#if !FS_IS_ARM_NEON()
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
#else
    static_assert(BufferPrefixSize == 14);

    aligned_to_vword float inBuffer[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,// BufferPrefix

        0.0f,
        0.0f,

        1.0f, 4.0f, 5.0f, 100.0f,
        200.0f, 2.0f, 5.0f,  6.0f,
        150.0f, 1000.0f, 7.0f, -5.0,
        4000.0f, 40.0f, 100.0f, 120.0,

        0.0f,
        0.0f
    };

    aligned_to_vword float outBuffer[] = {
        2.0f, 2.0f, 2.0f, 2.0f,
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
    EXPECT_FLOAT_EQ((150.0f * 1.0f + 1000.0f * 2.0f + 7.0f * 3.0f + -5.0f * 2.0f + 4000.0f * 1.0f) / 25.0f + 2.0f, outBuffer[10]);
    EXPECT_FLOAT_EQ((1000.0f * 1.0f + 7.0f * 2.0f + -5.0f * 3.0f + 4000.0f * 2.0f + 40.0f * 1.0f) / 25.0f + 2.0f, outBuffer[11]);

    EXPECT_FLOAT_EQ((7.0f * 1.0f + -5.0f * 2.0f + 4000.0f * 3.0f + 40.0f * 2.0f + 100.0f * 1.0f) / 25.0f + 2.0f, outBuffer[12]);
    EXPECT_FLOAT_EQ((-5.0f * 1.0f + 4000.0f * 2.0f + 40.0f * 3.0f + 100.0f * 2.0f + 120.0f * 1.0f) / 25.0f + 2.0f, outBuffer[13]);
    EXPECT_FLOAT_EQ((4000.0f * 1.0f + 40.0f * 2.0f + 100.0f * 3.0f + 120.0f * 2.0f + 0.0f * 1.0f) / 25.0f + 2.0f, outBuffer[14]);
    EXPECT_FLOAT_EQ((40.0f * 1.0f + 100.0f * 2.0f + 120.0f * 3.0f + 0.0f * 2.0f + 0.0f * 1.0f) / 25.0f + 2.0f, outBuffer[15]);
#endif
}

TEST(AlgorithmsTests, SmoothBufferAndAdd_12_5_Naive)
{
    RunSmoothBufferAndAddTest(Algorithms::SmoothBufferAndAdd_Naive<12, 5>);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, SmoothBufferAndAdd_12_5_SSEVectorized)
{
    RunSmoothBufferAndAddTest(Algorithms::SmoothBufferAndAdd_SSEVectorized<12, 5>);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, SmoothBufferAndAdd_16_5_NeonVectorized)
{
    RunSmoothBufferAndAddTest_16_5(Algorithms::SmoothBufferAndAdd_NeonVectorized<16, 5>);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateSpringVectors
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Algorithm>
void RunCalculateSpringVectorsTest(Algorithm algorithm)
{
    static size_t constexpr NumSprings = 5; // Plus one at end

    static std::array<vec2f, NumSprings * 2> const positions = {
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, 0.0f),
        vec2f(1.0f, 1.0f),
        vec2f(2.0f, 2.0f),
        vec2f(0.3f, 0.5f),
        vec2f(0.2f, 0.7f),
        vec2f(1000.0f, 2000.0f),
        vec2f(10000.0f, 20000.0f),
        vec2f(10.0f, 20.0f),
        vec2f(20.0f, 30.0f)
    };

    static std::array<SpringEndpoints, NumSprings> const endpoints = {
        SpringEndpoints{0, 1},
        SpringEndpoints{2, 3},
        SpringEndpoints{4, 5},
        SpringEndpoints{6, 7},
        SpringEndpoints{8, 9}
    };

    std::array<float, NumSprings> lengths;
    std::array<vec2f, NumSprings> normalizedVectors;

    algorithm(
        0,
        positions.data(),
        endpoints.data(),
        lengths.data(),
        normalizedVectors.data());

    for (size_t s = 0; s < 4; ++s)
    {
        vec2f const dis = (positions[endpoints[s].PointBIndex] - positions[endpoints[s].PointAIndex]);

        EXPECT_TRUE(ApproxEquals(lengths[s], dis.length(), 0.001f * dis.length()));
        EXPECT_TRUE(ApproxEquals(normalizedVectors[s].x, dis.normalise().x, 0.001f));
        EXPECT_TRUE(ApproxEquals(normalizedVectors[s].y, dis.normalise().y, 0.001f));
    }
}

TEST(AlgorithmsTests, CalculateSpringVectors_Naive)
{
    RunCalculateSpringVectorsTest(Algorithms::CalculateSpringVectors_Naive<SpringEndpoints>);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, CalculateSpringVectors_SSEVectorized)
{
    RunCalculateSpringVectorsTest(Algorithms::CalculateSpringVectors_SSEVectorized<SpringEndpoints>);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, CalculateSpringVectors_NeonVectorized)
{
    RunCalculateSpringVectorsTest(Algorithms::CalculateSpringVectors_NeonVectorized<SpringEndpoints>);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// IntegrateAndResetDynamicForces
///////////////////////////////////////////////////////////////////////////////////////////////////////

size_t constexpr IntegrateAndResetDynamicForcesInputSize = 4 + 16 + 8;

struct IntegrateAndResetDynamicForcesPoints
{
    float * GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(positionBuffer);
    }

    float * GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(velocityBuffer);
    }

    float const * GetStaticForceBufferAsFloat() const
    {
        return reinterpret_cast<float const *>(staticForceBuffer);
    }

    float const * GetIntegrationFactorBufferAsFloat() const
    {
        return reinterpret_cast<float const *>(integrationFactorBuffer);
    }

    aligned_to_vword vec2f positionBuffer[IntegrateAndResetDynamicForcesInputSize];
    aligned_to_vword vec2f velocityBuffer[IntegrateAndResetDynamicForcesInputSize];
    aligned_to_vword vec2f staticForceBuffer[IntegrateAndResetDynamicForcesInputSize];
    aligned_to_vword vec2f integrationFactorBuffer[IntegrateAndResetDynamicForcesInputSize];

    aligned_to_vword vec2f parallelDynamicForceBuffer0[IntegrateAndResetDynamicForcesInputSize];
    aligned_to_vword vec2f parallelDynamicForceBuffer1[IntegrateAndResetDynamicForcesInputSize];
    vec2f * parallelDynamicForceBuffers[2]{ parallelDynamicForceBuffer0, parallelDynamicForceBuffer1 };
};

template<typename Algorithm>
void RunIntegrateAndResetDynamicForcesTest_2(Algorithm algorithm)
{
    //
    // Populate
    //

    IntegrateAndResetDynamicForcesPoints points;

    for (size_t i = 0; i < IntegrateAndResetDynamicForcesInputSize; ++i)
    {
        auto const fi = static_cast<float>(i);

        points.positionBuffer[i] = vec2f(10.0f + fi, 20.0f + fi);
        points.velocityBuffer[i] = vec2f(100.0f + fi, 200.0f + fi);
        points.staticForceBuffer[i] = vec2f(1000.0f + fi, 2000.0f + fi);
        points.integrationFactorBuffer[i] = vec2f(1.0f + fi, 2.0f + fi);

        points.parallelDynamicForceBuffers[0][i] = vec2f(50.0f + fi, 500.0f + fi);
        points.parallelDynamicForceBuffers[1][i] = vec2f(70.0f + fi, 700.0f + fi);
    }

    //
    // Run test
    //

    float const dt = 1.0f / 64.0f;
    float const velocityFactor = 0.9f;

    float * const restrict dynamicForceBuffers[2] = {
        reinterpret_cast<float *>(points.parallelDynamicForceBuffers[0]),
        reinterpret_cast<float *>(points.parallelDynamicForceBuffers[1]) };

    algorithm(
        points,
        2, // Number of partitions
        4, // Start
        20, // End
        dynamicForceBuffers,
        dt,
        velocityFactor);

    //
    // Verify
    //

    for (size_t i = 0; i < IntegrateAndResetDynamicForcesInputSize; ++i)
    {
        auto const fi = static_cast<float>(i);

        if (i < 4 || i >= 20)
        {
            EXPECT_FLOAT_EQ(points.positionBuffer[i].x, 10.0f + fi);
            EXPECT_FLOAT_EQ(points.positionBuffer[i].y, 20.0f + fi);

            EXPECT_FLOAT_EQ(points.velocityBuffer[i].x, 100.0f + fi);
            EXPECT_FLOAT_EQ(points.velocityBuffer[i].y, 200.0f + fi);

            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[0][i].x, 50.0f + fi);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[0][i].y, 500.0f + fi);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[1][i].x, 70.0f + fi);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[1][i].y, 700.0f + fi);
        }
        else
        {
            vec2f const totalDynamicForce = vec2f(50.0f + fi, 500.0f + fi) + vec2f(70.0f + fi, 700.0f + fi);
            vec2f const deltaPos =
                vec2f(100.0f + fi, 200.0f + fi) * dt
                + (totalDynamicForce + points.staticForceBuffer[i]) * points.integrationFactorBuffer[i];

            EXPECT_FLOAT_EQ(points.positionBuffer[i].x, 10.0f + fi + deltaPos.x);
            EXPECT_FLOAT_EQ(points.positionBuffer[i].y, 20.0f + fi + deltaPos.y);

            EXPECT_FLOAT_EQ(points.velocityBuffer[i].x, deltaPos.x * velocityFactor);
            EXPECT_FLOAT_EQ(points.velocityBuffer[i].y, deltaPos.y * velocityFactor);

            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[0][i].x, 0.0f);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[0][i].y, 0.0f);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[1][i].x, 0.0f);
            EXPECT_FLOAT_EQ(points.parallelDynamicForceBuffers[1][i].y, 0.0f);
        }
    }
}

TEST(AlgorithmsTests, RunIntegrateAndResetDynamicForcesTest_2_Naive)
{
    RunIntegrateAndResetDynamicForcesTest_2(Algorithms::IntegrateAndResetDynamicForces_Naive<IntegrateAndResetDynamicForcesPoints>);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, RunIntegrateAndResetDynamicForcesTest_2_SSEVectorized)
{
    RunIntegrateAndResetDynamicForcesTest_2(Algorithms::IntegrateAndResetDynamicForces_SSEVectorized<IntegrateAndResetDynamicForcesPoints>);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, RunIntegrateAndResetDynamicForcesTest_2_NeonVectorized)
{
    RunIntegrateAndResetDynamicForcesTest_2(Algorithms::IntegrateAndResetDynamicForces_NeonVectorized<IntegrateAndResetDynamicForcesPoints>);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplySpringForces
///////////////////////////////////////////////////////////////////////////////////////////////////////

// Geometry:
//  - Springs:
//      - 3 perfect squares
//      - 4 springs
//      - 1 spring

//
//        09    --21--   10
//    /      |         |     \
//   17      18        19     20
//  /        |         |        \
// 05 --15-- 06 --08-- 07 --16-- 08
// |         |         |         |
// 03 01  02 04 05  06 11 09  10 12
// |         |         |         |
// 01 --13-- 02 --07-- 03 --14-- 04
//

size_t constexpr ApplySpringForcesPointCount = 1 + 10 + 4; // 1 prefix, 10 real, 4 suffix

size_t constexpr ApplySpringForcesSpringCount = 1 + 21 + 1; // 1 prefix, 21 real, 1 suffix

struct ApplySpringForcesPoints
{
    vec2f const * GetPositionBufferAsVec2() const
    {
        return positionBuffer;
    }

    vec2f const * GetVelocityBufferAsVec2() const
    {
        return velocityBuffer;
    }

    aligned_to_vword vec2f positionBuffer[ApplySpringForcesPointCount];
    aligned_to_vword vec2f velocityBuffer[ApplySpringForcesPointCount];
};

struct ApplySpringForcesSprings
{
    using Endpoints = SpringEndpoints;

    ElementCount GetPerfectSquareCount() const
    {
        return 3;
    }

    Endpoints const * GetEndpointsBuffer() const
    {
        return endpointsBuffer;
    }

    float const * GetRestLengthBuffer() const
    {
        return restLengthBuffer;
    }

    float const * GetStiffnessCoefficientBuffer() const
    {
        return stiffnessCoefficientBuffer;
    }

    float const * GetDampingCoefficientBuffer() const
    {
        return dampingCoefficientBuffer;
    }

    aligned_to_vword Endpoints endpointsBuffer[ApplySpringForcesSpringCount];
    aligned_to_vword float restLengthBuffer[ApplySpringForcesSpringCount];
    aligned_to_vword float stiffnessCoefficientBuffer[ApplySpringForcesSpringCount];
    aligned_to_vword float dampingCoefficientBuffer[ApplySpringForcesSpringCount];
};

template<typename Algorithm>
void RunApplySpringForcesTest(Algorithm algorithm)
{
    //
    // Populate
    //

    aligned_to_vword vec2f dynamicForceBuffer[ApplySpringForcesPointCount];

    ApplySpringForcesPoints points;

    for (size_t i = 0; i < ApplySpringForcesPointCount; ++i)
    {
        auto const fi = static_cast<float>(i);

        points.positionBuffer[i] = vec2f(10.0f + fi, 20.0f + fi * 2.0f);
        points.velocityBuffer[i] = vec2f(100.0f + fi, 200.0f + fi * 2.0f);

        dynamicForceBuffer[i] = vec2f(50.0f + fi, 500.0f + fi);
    }

    // Make sure there's a zero-length spring
    points.positionBuffer[5] = vec2f(5.0f, 7.0f);
    points.positionBuffer[2] = vec2f(5.0f, 7.0f);

    ApplySpringForcesSprings springs;

    springs.endpointsBuffer[0] = { 5, 2 };
    springs.endpointsBuffer[1] = { 6, 1 };
    springs.endpointsBuffer[2] = { 5, 1 };
    springs.endpointsBuffer[3] = { 6, 2 };

    springs.endpointsBuffer[4] = { 2, 7 };
    springs.endpointsBuffer[5] = { 6, 3 };
    springs.endpointsBuffer[6] = { 2, 3 };
    springs.endpointsBuffer[7] = { 6, 7 };

    springs.endpointsBuffer[8] = { 7, 4 };
    springs.endpointsBuffer[9] = { 8, 3 };
    springs.endpointsBuffer[10] = { 7, 3 };
    springs.endpointsBuffer[11] = { 8, 4 };

    springs.endpointsBuffer[12] = { 1, 2 };
    springs.endpointsBuffer[13] = { 4, 3 };
    springs.endpointsBuffer[14] = { 6, 5 };
    springs.endpointsBuffer[15] = { 8, 7 };
    springs.endpointsBuffer[16] = { 9, 5 };
    springs.endpointsBuffer[17] = { 9, 6 };
    springs.endpointsBuffer[18] = { 10, 7 };
    springs.endpointsBuffer[19] = { 10, 8 };
    springs.endpointsBuffer[20] = { 10, 9 };

    for (size_t i = 0; i < ApplySpringForcesSpringCount; ++i)
    {
        auto const fi = static_cast<float>(i);

        springs.restLengthBuffer[i] = 1.0f + fi;
        springs.stiffnessCoefficientBuffer[i] = 10.0f + fi;
        springs.dampingCoefficientBuffer[i] = 100.0f + fi;
    }

    //
    // Run test
    //

    algorithm(
        points,
        springs,
        0, // Start
        21, // End
        dynamicForceBuffer);

    //
    // Verify
    //

    auto CalculateExpectedForce = [&](int springIndex)
        {
            auto const & ep = springs.endpointsBuffer[springIndex];
            vec2f d = points.positionBuffer[ep.PointBIndex] - points.positionBuffer[ep.PointAIndex];
            float fSpring = (d.length() - springs.restLengthBuffer[springIndex]) * springs.stiffnessCoefficientBuffer[springIndex];
            vec2f v = points.velocityBuffer[ep.PointBIndex] - points.velocityBuffer[ep.PointAIndex];
            float fDamp = v.dot(d.normalise()) * springs.dampingCoefficientBuffer[springIndex];
            vec2f f = d.normalise() * (fSpring + fDamp);
            return f;
        };

    for (size_t i = 0; i < ApplySpringForcesPointCount; ++i)
    {
        auto const fi = static_cast<float>(i);

        if (i < 1 || i >= 11)
        {
            EXPECT_FLOAT_EQ(dynamicForceBuffer[i].x, 50.0f + fi);
            EXPECT_FLOAT_EQ(dynamicForceBuffer[i].y, 500.0f + fi);
        }
        else
        {
            vec2f expectedDynamicForce;
            switch (i)
            {
                case 1:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(12)
                        - CalculateExpectedForce(1)
                        - CalculateExpectedForce(2);

                    break;
                }

                case 2:
                {
                    expectedDynamicForce =
                        - CalculateExpectedForce(12)
                        - CalculateExpectedForce(0)
                        - CalculateExpectedForce(3)
                        + CalculateExpectedForce(4)
                        + CalculateExpectedForce(6);

                    break;
                }

                case 3:
                {
                    expectedDynamicForce =
                        - CalculateExpectedForce(6)
                        - CalculateExpectedForce(5)
                        - CalculateExpectedForce(10)
                        - CalculateExpectedForce(9)
                        - CalculateExpectedForce(13);

                    break;
                }

                case 4:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(13)
                        - CalculateExpectedForce(8)
                        - CalculateExpectedForce(11);

                    break;
                }

                case 5:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(2)
                        - CalculateExpectedForce(16)
                        - CalculateExpectedForce(14)
                        + CalculateExpectedForce(0);

                    break;
                }

                case 6:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(1)
                        + CalculateExpectedForce(14)
                        - CalculateExpectedForce(17)
                        + CalculateExpectedForce(7)
                        + CalculateExpectedForce(5)
                        + CalculateExpectedForce(3);

                    break;
                }

                case 7:
                {
                    expectedDynamicForce =
                        - CalculateExpectedForce(4)
                        - CalculateExpectedForce(7)
                        - CalculateExpectedForce(18)
                        - CalculateExpectedForce(15)
                        + CalculateExpectedForce(8)
                        + CalculateExpectedForce(10);

                    break;
                }

                case 8:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(9)
                        + CalculateExpectedForce(15)
                        - CalculateExpectedForce(19)
                        + CalculateExpectedForce(11);

                    break;
                }

                case 9:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(16)
                        - CalculateExpectedForce(20)
                        + CalculateExpectedForce(17);

                    break;
                }

                case 10:
                {
                    expectedDynamicForce =
                        CalculateExpectedForce(20)
                        + CalculateExpectedForce(19)
                        + CalculateExpectedForce(18);

                    break;
                }
            }

            expectedDynamicForce += vec2f(50.0f + fi, 500.0f + fi);

            EXPECT_TRUE(ApproxEquals(dynamicForceBuffer[i].x, expectedDynamicForce.x, 0.95f));
            EXPECT_TRUE(ApproxEquals(dynamicForceBuffer[i].y, expectedDynamicForce.y, 0.95f));
        }
    }
}

TEST(AlgorithmsTests, RunApplySpringForcesTest_Naive)
{
    RunApplySpringForcesTest(Algorithms::ApplySpringsForces_Naive<ApplySpringForcesPoints, ApplySpringForcesSprings>);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, RunApplySpringForcesTest_SSEVectorized)
{
    RunApplySpringForcesTest(Algorithms::ApplySpringsForces_SSEVectorized<ApplySpringForcesPoints, ApplySpringForcesSprings>);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, RunApplySpringForcesTest_NeonVectorized)
{
    RunApplySpringForcesTest(Algorithms::ApplySpringsForces_NeonVectorized<ApplySpringForcesPoints, ApplySpringForcesSprings>);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// MakeAABBWeightedUnion
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Algorithm>
void RunMakeAABBWeightedUnionTest(Algorithm algorithm)
{
    std::vector<Geometry::ShipAABB> aabbSet;

    aabbSet.emplace_back(
        -10.0f,
        7.0f,
        12.0f,
        6.0f,
        12);

    aabbSet.emplace_back(
        -1000.0f,
        7000.0f,
        12000.0f,
        6000.0f,
        2); // Ignored

    aabbSet.emplace_back(
        -10.0f,
        -7.0f,
        -1.0f,
        -6.0f,
        4);

    auto const result = algorithm(aabbSet);

    ASSERT_TRUE(result.has_value());

    // Expectation

    float const w1 = 12.0f - 3.0f;
    vec2f center1 = vec2f(-3.0f, 18.0f) / 2.0f * w1;
    float const w2 = 4.0f - 3.0f;
    vec2f center3 = vec2f(-17.0f, -7.0f) / 2.0f * w2;
    vec2f center = (center1 + center3) / (w1 + w2);

    float const maxWeight = std::max(w1, w2);

    float left = std::min(
        (-10.f - center.x) * w1 / maxWeight,
        (-10.f - center.x) * w2 / maxWeight);
    float right = std::max(
        (7.f - center.x) * w1 / maxWeight,
        (-7.f - center.x) * w2 / maxWeight);
    float top = std::max(
        (12.f - center.y) * w1 / maxWeight,
        (-1.f - center.y) * w2 / maxWeight);
    float bottom = std::min(
        (6.f - center.y) * w1 / maxWeight,
        (-6.f - center.y) * w2 / maxWeight);

    EXPECT_FLOAT_EQ(result->BottomLeft.x, center.x + left);
    EXPECT_FLOAT_EQ(result->TopRight.x, center.x + right);
    EXPECT_FLOAT_EQ(result->TopRight.y, center.y + top);
    EXPECT_FLOAT_EQ(result->BottomLeft.y, center.y + bottom);
}

template<typename Algorithm>
void RunMakeAABBWeightedUnionTest_Empty(Algorithm algorithm)
{
    std::vector<Geometry::ShipAABB> aabbSet;

    //
    // 1. Fully empty
    //

    auto const result1 = algorithm(aabbSet);

    EXPECT_FALSE(result1.has_value());

    //
    // 2. All AABBs under threshold
    //

    aabbSet.emplace_back(
        Geometry::ShipAABB(
            -10.0f,
            10.f,
            10.0f,
            -10.f,
            2));

    auto const result2 = algorithm(aabbSet);

    EXPECT_FALSE(result2.has_value());
}

TEST(AlgorithmsTests, MakeAABBWeightedUnion_Naive)
{
    RunMakeAABBWeightedUnionTest(Algorithms::MakeAABBWeightedUnion_Naive);
}

TEST(AlgorithmsTests, MakeAABBWeightedUnion_Naive_Empty)
{
    RunMakeAABBWeightedUnionTest_Empty(Algorithms::MakeAABBWeightedUnion_Naive);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, MakeAABBWeightedUnion_SSEVectorized)
{
    RunMakeAABBWeightedUnionTest(Algorithms::MakeAABBWeightedUnion_SSEVectorized);
}

TEST(AlgorithmsTests, MakeAABBWeightedUnion_SSEVectorized_Empty)
{
    RunMakeAABBWeightedUnionTest_Empty(Algorithms::MakeAABBWeightedUnion_SSEVectorized);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, MakeAABBWeightedUnion_NeonVectorized)
{
    RunMakeAABBWeightedUnionTest(Algorithms::MakeAABBWeightedUnion_NeonVectorized);
}

TEST(AlgorithmsTests, MakeAABBWeightedUnion_NeonVectorized_Empty)
{
    RunMakeAABBWeightedUnionTest_Empty(Algorithms::MakeAABBWeightedUnion_NeonVectorized);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// MixVec4f
///////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Algorithm>
void RunMixVec4fTest(Algorithm algorithm)
{
    vec4f const vec1(0.3f, 4.0f, 500.0f, 1234.0f);
    vec4f const vec2(1.7f, 7.0f, 777.0f, 13.5f);
    vec4f output;

    float const weight = 3.7f;

    algorithm(
        reinterpret_cast<float const *>(&vec1),
        reinterpret_cast<float const *>(&vec2),
        reinterpret_cast<float *>(&output),
        weight);

    EXPECT_FLOAT_EQ(output.x, vec1.x + (vec2.x - vec1.x) * weight);
    EXPECT_FLOAT_EQ(output.y, vec1.y + (vec2.y - vec1.y) * weight);
    EXPECT_FLOAT_EQ(output.z, vec1.z + (vec2.z - vec1.z) * weight);
    EXPECT_FLOAT_EQ(output.w, vec1.w + (vec2.w - vec1.w) * weight);
}

TEST(AlgorithmsTests, MixVec4f_Naive)
{
    RunMixVec4fTest(Algorithms::MixVec4f_Naive);
}

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
TEST(AlgorithmsTests, MixVec4f_SSEVectorized)
{
    RunMixVec4fTest(Algorithms::MixVec4f_SSEVectorized);
}
#endif

#if FS_IS_ARM_NEON()
TEST(AlgorithmsTests, MixVec4f_NeonVectorized)
{
    RunMixVec4fTest(Algorithms::MixVec4f_NeonVectorized);
}
#endif
