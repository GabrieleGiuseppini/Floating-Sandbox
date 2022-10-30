#include <Game/RopeBuffer.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(RopeBufferTests, SampleAt)
{
    auto const material1 = MakeTestStructuralMaterial("mat1", rgbColor(1, 2, 3));
    auto const material2 = MakeTestStructuralMaterial("mat2", rgbColor(1, 2, 3));

    RopeBuffer buffer;

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
    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopeBuffer clone = buffer.Clone();

    ASSERT_EQ(clone.GetSize(), 1u);
    EXPECT_EQ(ShipSpaceCoordinates(4, 5), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(1, 2, 3, 4), clone[0].RenderColor);
}

TEST(RopeBufferTests, CloneRegion)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(clone.GetSize(), 1u);

    EXPECT_EQ(ShipSpaceCoordinates(2, 2), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(3, 3), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), clone[0].RenderColor);
}

TEST(RopeBufferTests, CopyRegion)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(clone.GetSize(), 2u);

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(7, 6), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(1, 1, 1, 1), clone[0].RenderColor);

    EXPECT_EQ(ShipSpaceCoordinates(2, 2), clone[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(3, 3), clone[1].EndCoords);
    EXPECT_EQ(nullptr, clone[1].Material);
    EXPECT_EQ(rgbaColor(2, 2, 2, 2), clone[1].RenderColor);
}

TEST(RopeBufferTests, EraseRegion_Smaller)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 1u);

    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 11), buffer[0].EndCoords);
    EXPECT_EQ(nullptr, buffer[0].Material);
    EXPECT_EQ(rgbaColor(3, 3, 3, 3), buffer[0].RenderColor);
}

TEST(RopeBufferTests, EraseRegion_Equals)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 0u);
}

TEST(RopeBufferTests, Flip_Horizontal)
{
    RopeBuffer buffer;

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

    buffer.Flip(DirectionType::Horizontal, ShipSpaceSize(12, 20));

    EXPECT_EQ(ShipSpaceCoordinates(7, 5), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 10), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 19), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 0), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Flip_Vertical)
{
    RopeBuffer buffer;

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

    buffer.Flip(DirectionType::Vertical, ShipSpaceSize(12, 20));

    EXPECT_EQ(ShipSpaceCoordinates(4, 14), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 9), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(11, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(0, 19), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Flip_HorizontalAndVertical)
{
    RopeBuffer buffer;

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

    buffer.Flip(DirectionType::Horizontal | DirectionType::Vertical, ShipSpaceSize(12, 20));

    EXPECT_EQ(ShipSpaceCoordinates(7, 14), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 9), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(11, 19), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Rotate90_Clockwise)
{
    RopeBuffer buffer;

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

    buffer.Rotate90(RotationDirectionType::Clockwise, ShipSpaceSize(12, 20));

    EXPECT_EQ(ShipSpaceCoordinates(5, 7), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 1), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(19, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(4, 11), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Rotate90_CounterClockwise)
{
    RopeBuffer buffer;

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

    buffer.Rotate90(RotationDirectionType::CounterClockwise, ShipSpaceSize(12, 20));

    EXPECT_EQ(ShipSpaceCoordinates(14, 4), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(9, 10), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 11), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(15, 0), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Trim)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 1u);

    EXPECT_EQ(ShipSpaceCoordinates(7 - 5, 19 - 6), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(6 - 5, 15 - 6), buffer[0].EndCoords);
}

TEST(RopeBufferTests, Trim_BecomesEmpty)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 0u);
}

TEST(RopeBufferTests, Reframe)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 2u);

    EXPECT_EQ(ShipSpaceCoordinates(2, 4), buffer[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[0].EndCoords);

    EXPECT_EQ(ShipSpaceCoordinates(0, 0), buffer[1].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(1, 1), buffer[1].EndCoords);
}

TEST(RopeBufferTests, Reframe_BecomesEmpty)
{
    RopeBuffer buffer;

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

    ASSERT_EQ(buffer.GetSize(), 0u);
}