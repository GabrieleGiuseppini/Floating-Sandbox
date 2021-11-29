#include <GameCore/Buffer2D.h>

#include "gtest/gtest.h"

TEST(Buffer2DTests, FillCctor_Size)
{
    Buffer2D<int, struct IntegralTag> buffer(IntegralRectSize(10, 20), 242);

    EXPECT_EQ(buffer.Size.width, 10);
    EXPECT_EQ(buffer.Size.height, 20);

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, FillCctor_Dimensions)
{
    Buffer2D<int, struct IntegralTag> buffer(10, 20, 242);

    EXPECT_EQ(buffer.Size.width, 10);
    EXPECT_EQ(buffer.Size.height, 20);

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing_WithCoordinates)
{
    Buffer2D<int, struct IntegralTag> buffer(10, 20, 242);

    buffer[IntegralCoordinates(7, 9)] = 42;

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(7, 9)], 42);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Clone_Whole)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferClone = buffer.Clone();
    iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(bufferClone[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Clone_Region)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferClone = buffer.CloneRegion(
        IntegralRect(
            { 1, 1 },
            { 2, 2 }));

    ASSERT_EQ(IntegralRectSize(2, 2), bufferClone.Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 1) * 4 + 1;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ(bufferClone[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_WholeSource_ToOrigin)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(2, 3);
    int val = 100;
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {0, 0}, sourceBuffer.Size },
        { 0, 0 });

    // Verify
    val = 100;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x >= 2 || y >= 3)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = val++;
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_WholeSource_ToOffset)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(2, 3);
    int val = 100;
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {0, 0}, sourceBuffer.Size },
        { 4, 7 });

    // Verify
    val = 100;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x < 4 || x >= 4 + 2 || y < 7 || y >= 7 + 3)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = val++;
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_PortionOfSource_ToOffset)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(4, 4);
    int val = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {1, 1}, {2, 2} },
        { 4, 7 });

    // Verify
    int expectedVals[] = { 105, 106, 109, 110 };
    int iExpectedVal = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x < 4 || x >= 4 + 2 || y < 7 || y >= 7 + 2)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = expectedVals[iExpectedVal++];
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, Flip_Horizontal)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Horizontal);
    
    iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = buffer.Size.width - 1; x >= 0; --x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}


TEST(Buffer2DTests, Flip_Vertical)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Vertical);

    iVal = 100;
    for (int y = buffer.Size.height - 1; y >= 0; --y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Flip_HorizontalAndVertical)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Horizontal | DirectionType::Vertical);

    iVal = 100;
    for (int y = buffer.Size.height - 1; y >= 0; --y)
    {
        for (int x = buffer.Size.width - 1; x >= 0; --x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}
