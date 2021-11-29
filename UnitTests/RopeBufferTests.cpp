#include <Game/RopeBuffer.h>

#include "gtest/gtest.h"

TEST(RopeBufferTests, Clone)
{
    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5),
        ShipSpaceCoordinates(10, 10),
        nullptr, 
        rgbaColor(1, 2, 3, 4));

    RopeBuffer clone = buffer.Clone();

    ASSERT_EQ(1, clone.GetSize());
    EXPECT_EQ(ShipSpaceCoordinates(4, 5), clone[0].StartCoords);
    EXPECT_EQ(ShipSpaceCoordinates(10, 10), clone[0].EndCoords);
    EXPECT_EQ(nullptr, clone[0].Material);
    EXPECT_EQ(rgbaColor(1, 2, 3, 4), clone[0].RenderColor);
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
