#include <Simulation/RopeBuffer.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

TEST(RopeBufferTests, HasEndpointAt)
{
    auto const material1 = MakeTestStructuralMaterial("mat1", rgbColor(1, 2, 3));

    RopeBuffer buffer({ 400, 400 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        &material1,
        rgbaColor(1, 2, 3, 4));

    EXPECT_TRUE(buffer.HasEndpointAt({ 4, 5 }));
    EXPECT_TRUE(buffer.HasEndpointAt({ 10, 10 }));
    EXPECT_FALSE(buffer.HasEndpointAt({ 4, 6 }));
}

TEST(RopeBufferTests, SampleAt)
{
    auto const material1 = MakeTestStructuralMaterial("mat1", rgbColor(1, 2, 3));
    auto const material2 = MakeTestStructuralMaterial("mat2", rgbColor(1, 2, 3));

    RopeBuffer buffer({ 400, 400 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        &material1,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 7),
        ShipSpaceCoordinates(11, 11),
        &material2,
        rgbaColor(1, 2, 3, 4));

    auto const * material = buffer.SampleMaterialEndpointAt({ 4, 5 });
    EXPECT_EQ(material, &material1);

    material = buffer.SampleMaterialEndpointAt({ 11, 11 });
    EXPECT_EQ(material, &material2);

    material = buffer.SampleMaterialEndpointAt({ 4, 4 });
    EXPECT_EQ(material, nullptr);
}

TEST(RopeBufferTests, Clone)
{
    RopeBuffer buffer({ 400, 300 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopeBuffer clone = buffer.Clone();

    EXPECT_EQ(ShipSpaceSize(400, 300), clone.GetSize());
    ASSERT_EQ(1u, clone.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(4, 5), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(1, 2, 3, 4), clone[0].RenderColor);
}

TEST(RopeBufferTests, CloneRegion)
{
    RopeBuffer buffer({ 400, 300 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(3, 3, 3, 3));

    RopeBuffer clone = buffer.CloneRegion(ShipSpaceRect(ShipSpaceCoordinates(3, 4), ShipSpaceSize(4, 4)));

    EXPECT_EQ(ShipSpaceSize(4, 4), clone.GetSize());
    ASSERT_EQ(1u, clone.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(2, 2), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(3, 3), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), clone[0].RenderColor);
}

TEST(RopeBufferTests, CopyRegion)
{
    RopeBuffer buffer({ 400, 400 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(3, 3, 3, 3));

    RopeBuffer clone = buffer.CopyRegion(ShipSpaceRect(ShipSpaceCoordinates(3, 4), ShipSpaceSize(4, 4)));

    EXPECT_EQ(ShipSpaceSize(4, 4), clone.GetSize());
    ASSERT_EQ(2u, clone.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(7, 6), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), clone[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(2, 2), clone[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(3, 3), clone[1].EndCoords);
    EXPECT_EQ(nullptr, clone[1].Material);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), clone[1].RenderColor);
}

TEST(RopeBufferTests, BlitFromRegion_Opaque_AllEndUpInTargetRegion_NoConflicts)
{
    ShipSpaceRect sourceRegion(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceSize(15, 20));

    ShipSpaceCoordinates const targetPos(10, 15);

    ShipSpaceSize const targetSize(100, 200);

    // Source region: [1, 1] -> (1 + 15, 1 + 20) == [15, 20]
    // Target paste region: [10, 15] -> (10 + 15, 15 + 20) == [24, 34]
    // Target region: [0, 0] -> [100, 200]

    //
    // Prepare target
    //

    RopeBuffer targetBuffer(targetSize);

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 17), // In target paste region
        nullptr,
        rgbaColor(3, 3, 3, 3));

    //
    // Prepare source
    //

    RopeBuffer sourceBuffer(sourceRegion.size);

    // Both endpoints in source region
    // End up at: (1-1+10==10, 2-1+15==16), (15-1+10==24, 20-1+15==34) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 2),
        ShipSpaceCoordinates(15, 20),
        nullptr,
        rgbaColor(4, 4, 4, 4));

    // One endpoint in source region
    // End up at: (2-1+10==11, 3-1+15==17), (16-1+10==25, 20-1+15==34) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(2, 3),
        ShipSpaceCoordinates(16, 20),
        nullptr,
        rgbaColor(5, 5, 5, 5));

    // Both endpoints outside of source region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1),
        ShipSpaceCoordinates(15, 21),
        nullptr,
        rgbaColor(6, 6, 6, 6));

    //
    // Test
    //

    targetBuffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetPos,
        false);

    //
    // Verify
    //

    ASSERT_EQ(2u + 2u, targetBuffer.GetElementCount());

    // Orig

    EXPECT_EQ(ShipSpaceCoordinates(4, 5), targetBuffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), targetBuffer[0].EndCoords);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), targetBuffer[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(5, 6), targetBuffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6, 7), targetBuffer[1].EndCoords);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), targetBuffer[1].RenderColor);

    // New

    EXPECT_EQ(ShipSpaceCoordinates(1 - 1 + 10, 2 - 1 + 15), targetBuffer[2].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(15 - 1 + 10, 20 - 1 + 15), targetBuffer[2].EndCoords);
    EXPECT_EQ(rgbaColor(4, 4, 4, 4), targetBuffer[2].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(2 - 1 + 10, 3 - 1 + 15), targetBuffer[3].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(16 - 1 + 10, 20 - 1 + 15), targetBuffer[3].EndCoords);
    EXPECT_EQ(rgbaColor(5, 5, 5, 5), targetBuffer[3].RenderColor);
}

TEST(RopeBufferTests, BlitFromRegion_Opaque_EndsUpOutsideTargetRegion_NoConflicts)
{
    ShipSpaceRect sourceRegion(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceSize(15, 21));

    ShipSpaceCoordinates const targetPos(10, 15);

    ShipSpaceSize const targetSize(24, 34);

    // Source region: [1, 1] -> (1 + 15, 1 + 21) == [15, 21]
    // Target paste region: [10, 15] -> (10 + 15, 15 + 21) == [24, 35]
    // Target region: [0, 0] -> [23, 33]

    //
    // Prepare target
    //

    RopeBuffer targetBuffer(targetSize);

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 17), // In target paste region
        nullptr,
        rgbaColor(3, 3, 3, 3));

    //
    // Prepare source
    //

    RopeBuffer sourceBuffer(sourceRegion.size);

    // Both endpoints in source region
    // End up at: (1-1+10==10, 2-1+15==16), (14-1+10==23, 19-1+15==33) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 2),
        ShipSpaceCoordinates(14, 19),
        nullptr,
        rgbaColor(4, 4, 4, 4));

    // Both endpoints in source region
    // End up at: (1-1+10==10, 2-1+15==16), (15-1+10==24, 20-1+15==34) - i.e. outside target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 2),
        ShipSpaceCoordinates(15, 20),
        nullptr,
        rgbaColor(5, 5, 5, 5));

    //
    // Test
    //

    targetBuffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetPos,
        false);

    //
    // Verify
    //

    ASSERT_EQ(2u + 1u, targetBuffer.GetElementCount());

    // Orig

    EXPECT_EQ(ShipSpaceCoordinates(4, 5), targetBuffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), targetBuffer[0].EndCoords);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), targetBuffer[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(5, 6), targetBuffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6, 7), targetBuffer[1].EndCoords);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), targetBuffer[1].RenderColor);

    // New

    EXPECT_EQ(ShipSpaceCoordinates(1 - 1 + 10, 2 - 1 + 15), targetBuffer[2].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(14 - 1 + 10, 19 - 1 + 15), targetBuffer[2].EndCoords);
    EXPECT_EQ(rgbaColor(4, 4, 4, 4), targetBuffer[2].RenderColor);
}

TEST(RopeBufferTests, BlitFromRegion_Transparent_AllEndUpInTargetRegion_NoConflicts)
{
    ShipSpaceRect sourceRegion(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceSize(15, 20));

    ShipSpaceCoordinates const targetPos(10, 15);

    ShipSpaceSize const targetSize(100, 200);

    // Source region: [1, 1] -> (1 + 15, 1 + 20) == [15, 20]
    // Target paste region: [10, 15] -> (10 + 15, 15 + 20) == [24, 34]
    // Target region: [0, 0] -> [100, 200]

    //
    // Prepare target
    //

    RopeBuffer targetBuffer(targetSize);

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 16), // In target paste region
        nullptr,
        rgbaColor(3, 3, 3, 3));

    //
    // Prepare source
    //

    RopeBuffer sourceBuffer(sourceRegion.size);

    // Both endpoints in source region
    // End up at: (1-1+10==10, 2-1+15==16), (15-1+10==24, 20-1+15==34) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 2),
        ShipSpaceCoordinates(15, 20),
        nullptr,
        rgbaColor(4, 4, 4, 4));

    // One endpoint in source region
    // End up at: (2-1+10==11, 3-1+15==17), (16-1+10==25, 20-1+15==34) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(2, 3),
        ShipSpaceCoordinates(16, 20),
        nullptr,
        rgbaColor(5, 5, 5, 5));

    // Both endpoints outside of source region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1),
        ShipSpaceCoordinates(15, 21),
        nullptr,
        rgbaColor(6, 6, 6, 6));

    //
    // Test
    //

    targetBuffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetPos,
        true);

    //
    // Verify
    //

    ASSERT_EQ(3u + 2u, targetBuffer.GetElementCount());

    // Orig

    EXPECT_EQ(ShipSpaceCoordinates(4, 5), targetBuffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), targetBuffer[0].EndCoords);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), targetBuffer[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(5, 6), targetBuffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6, 7), targetBuffer[1].EndCoords);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), targetBuffer[1].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), targetBuffer[2].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 16), targetBuffer[2].EndCoords);
    EXPECT_EQ(rgbaColor(3, 3, 3, 3), targetBuffer[2].RenderColor);

    // New

    EXPECT_EQ(ShipSpaceCoordinates(1 - 1 + 10, 2 - 1 + 15), targetBuffer[3].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(15 - 1 + 10, 20 - 1 + 15), targetBuffer[3].EndCoords);
    EXPECT_EQ(rgbaColor(4, 4, 4, 4), targetBuffer[3].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(2 - 1 + 10, 3 - 1 + 15), targetBuffer[4].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(16 - 1 + 10, 20 - 1 + 15), targetBuffer[4].EndCoords);
    EXPECT_EQ(rgbaColor(5, 5, 5, 5), targetBuffer[4].RenderColor);
}

TEST(RopeBufferTests, BlitFromRegion_Transparent_EndsUpOutsideTargetRegion_NoConflicts)
{
    ShipSpaceRect sourceRegion(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceSize(15, 21));

    ShipSpaceCoordinates const targetPos(10, 15);

    ShipSpaceSize const targetSize(24, 34);

    // Source region: [1, 1] -> (1 + 15, 1 + 21) == [15, 21]
    // Target paste region: [10, 15] -> (10 + 15, 15 + 21) == [24, 35]
    // Target region: [0, 0] -> [23, 33]

    //
    // Prepare target
    //

    RopeBuffer targetBuffer(targetSize);

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 17), // In target paste region
        nullptr,
        rgbaColor(3, 3, 3, 3));

    //
    // Prepare source
    //

    RopeBuffer sourceBuffer(sourceRegion.size);

    // Both endpoints in source region
    // End up at: (1-1+10==10, 2-1+15==16), (14-1+10==23, 19-1+15==33) - i.e. in target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 2),
        ShipSpaceCoordinates(14, 19),
        nullptr,
        rgbaColor(4, 4, 4, 4));

    // Both endpoints in source region
    // End up at: (1-1+10==10, 1-1+15==15), (15-1+10==24, 20-1+15==34) - i.e. outside target region
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(15, 20),
        nullptr,
        rgbaColor(5, 5, 5, 5));

    //
    // Test
    //

    targetBuffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetPos,
        true);

    //
    // Verify
    //

    ASSERT_EQ(3u + 1u, targetBuffer.GetElementCount());

    // Orig

    EXPECT_EQ(ShipSpaceCoordinates(4, 5), targetBuffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), targetBuffer[0].EndCoords);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), targetBuffer[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(5, 6), targetBuffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6, 7), targetBuffer[1].EndCoords);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), targetBuffer[1].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), targetBuffer[2].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 17), targetBuffer[2].EndCoords);
    EXPECT_EQ(rgbaColor(3, 3, 3, 3), targetBuffer[2].RenderColor);

    // New

    EXPECT_EQ(ShipSpaceCoordinates(1 - 1 + 10, 2 - 1 + 15), targetBuffer[3].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(14 - 1 + 10, 19 - 1 + 15), targetBuffer[3].EndCoords);
    EXPECT_EQ(rgbaColor(4, 4, 4, 4), targetBuffer[3].RenderColor);
}

TEST(RopeBufferTests, BlitFromRegion_Transparent_Conflict)
{
    ShipSpaceRect sourceRegion(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceSize(150, 200));

    ShipSpaceCoordinates const targetPos(2, 5);

    ShipSpaceSize const targetSize(100, 200);

    //
    // Prepare target
    //

    RopeBuffer targetBuffer(targetSize);

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    targetBuffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    //
    // Prepare source
    //

    RopeBuffer sourceBuffer(sourceRegion.size);

    // Start endpoint conflicts: (9-1+2==10, 6-1+5==10)
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(9, 6),
        ShipSpaceCoordinates(15, 20),
        nullptr,
        rgbaColor(3, 3, 3, 3));

    // Start endpoint does not conflict: (10-1+2==11, 6-1+5==10)
    sourceBuffer.EmplaceBack(
        ShipSpaceCoordinates(10, 6),
        ShipSpaceCoordinates(16, 20),
        nullptr,
        rgbaColor(4, 4, 4, 4));

    //
    // Test
    //

    targetBuffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetPos,
        true);

    //
    // Verify
    //

    ASSERT_EQ(1u + 2u, targetBuffer.GetElementCount());

    // Orig

    EXPECT_EQ(ShipSpaceCoordinates(5, 6), targetBuffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6, 7), targetBuffer[0].EndCoords);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), targetBuffer[0].RenderColor);

    // New

    EXPECT_EQ(ShipSpaceCoordinates(9 - 1 + 2, 6 - 1 + 5), targetBuffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(15 - 1 + 2, 20 - 1 + 5), targetBuffer[1].EndCoords);
    EXPECT_EQ(rgbaColor(3, 3, 3, 3), targetBuffer[1].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(10 - 1 + 2, 6 - 1 + 5), targetBuffer[2].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(16 - 1 + 2, 20 - 1 + 5), targetBuffer[2].EndCoords);
    EXPECT_EQ(rgbaColor(4, 4, 4, 4), targetBuffer[2].RenderColor);
}

TEST(RopeBufferTests, EraseRegion_Smaller)
{
    RopeBuffer buffer({400, 400});

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(3, 3, 3, 3));

    buffer.EraseRegion(ShipSpaceRect(ShipSpaceCoordinates(3, 4), ShipSpaceSize(4, 4)));

    EXPECT_EQ(ShipSpaceSize(400, 400), buffer.GetSize());
    ASSERT_EQ(1u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 11), buffer[0].EndCoords);
    EXPECT_EQ(nullptr, buffer[0].Material);
    EXPECT_EQ(rgbaColor(3, 3, 3, 3), buffer[0].RenderColor);
}

TEST(RopeBufferTests, EraseRegion_Equals)
{
    RopeBuffer buffer({ 400, 400 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 1, 1, 1));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 6),
        ShipSpaceCoordinates(6, 7),
        nullptr,
        rgbaColor(2, 2, 2, 2));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(1, 1),
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(3, 3, 3, 3));

    buffer.EraseRegion(ShipSpaceRect(ShipSpaceCoordinates(0, 0), ShipSpaceSize(11, 11)));

    ASSERT_EQ(0u, buffer.GetElementCount());
}

TEST(RopeBufferTests, Flip_Horizontal)
{
    RopeBuffer buffer({ 12, 20 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(11, 19),
        ShipSpaceCoordinates(0, 0),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Flip(DirectionType::Horizontal);

    EXPECT_EQ(ShipSpaceSize(12, 20), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(7, 5), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 10), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 19), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 0), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Flip_Vertical)
{
    RopeBuffer buffer({ 12, 20 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(11, 19),
        ShipSpaceCoordinates(0, 0),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Flip(DirectionType::Vertical);

    EXPECT_EQ(ShipSpaceSize(12, 20), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(4, 14), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 9), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(11, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(0, 19), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Flip_HorizontalAndVertical)
{
    RopeBuffer buffer({ 12, 20 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(11, 19),
        ShipSpaceCoordinates(0, 0),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Flip(DirectionType::Horizontal | DirectionType::Vertical);

    EXPECT_EQ(ShipSpaceSize(12, 20), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(7, 14), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 9), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 19), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Rotate90_Clockwise)
{
    RopeBuffer buffer(ShipSpaceSize(12, 20));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(11, 19),
        ShipSpaceCoordinates(0, 4),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Rotate90(RotationDirectionType::Clockwise);

    EXPECT_EQ(ShipSpaceSize(20, 12), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(5, 7), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 1), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(19, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(4, 11), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Rotate90_CounterClockwise)
{
    RopeBuffer buffer(ShipSpaceSize(12, 20));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(11, 19),
        ShipSpaceCoordinates(0, 4),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Rotate90(RotationDirectionType::CounterClockwise);

    EXPECT_EQ(ShipSpaceSize(20, 12), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(14, 4), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(9, 10), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 11), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(15, 0), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Trim)
{
    RopeBuffer buffer({400, 200});

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(7, 19),
        ShipSpaceCoordinates(6, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(14, 15),
        ShipSpaceCoordinates(1, 1),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Trim(
        { 5, 6 },
        { 100, 200 });

    EXPECT_EQ(ShipSpaceSize(100, 200), buffer.GetSize());
    ASSERT_EQ(1u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(7 - 5, 19 - 6), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6 - 5, 15 - 6), buffer[0].EndCoords);
}

TEST(RopeBufferTests, Trim_BecomesEmpty)
{
    RopeBuffer buffer({ 400, 200 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(7, 19),
        ShipSpaceCoordinates(6, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(14, 15),
        ShipSpaceCoordinates(1, 1),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Trim(
        { 35, 36 },
        { 100, 200 });

    EXPECT_EQ(ShipSpaceSize(100, 200), buffer.GetSize());
    ASSERT_EQ(0u, buffer.GetElementCount());
}

TEST(RopeBufferTests, Reframe)
{
    RopeBuffer buffer({400, 200});

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(7, 18),
        ShipSpaceCoordinates(6, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(14, 15),
        ShipSpaceCoordinates(1, 1),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(5, 14),
        ShipSpaceCoordinates(6, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(8, 14),
        ShipSpaceCoordinates(6, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Reframe(
        { 3, 5 },
        { -5, -14 });

    EXPECT_EQ(ShipSpaceSize(3, 5), buffer.GetSize());
    ASSERT_EQ(2u, buffer.GetElementCount());

    EXPECT_EQ(ShipSpaceCoordinates(2, 4), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Reframe_BecomesEmpty)
{
    RopeBuffer buffer({400, 200});

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(9, 19),
        ShipSpaceCoordinates(8, 15),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(14, 15),
        ShipSpaceCoordinates(1, 1),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.Reframe(
        { 2, 1 },
        { -5, -14 });

    EXPECT_EQ(ShipSpaceSize(2, 1), buffer.GetSize());
    ASSERT_EQ(0u, buffer.GetElementCount());
}