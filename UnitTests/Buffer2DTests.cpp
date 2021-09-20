#include <GameCore/Buffer2D.h>

#include "gtest/gtest.h"

TEST(Buffer2DTests, FillCctor)
{
    Buffer2D<int, IntegralTag> buffer(10, 20, 242);

    EXPECT_EQ(buffer.Size.width, 10);
    EXPECT_EQ(buffer.Size.height, 20);

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing_WithCoordinates)
{
    Buffer2D<int, IntegralTag> buffer(10, 20, 242);

    buffer[IntegralCoordinates(7, 9)] = 42;

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(7, 9)], 42);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing_DoubleIndex)
{
    Buffer2D<int, IntegralTag> buffer(10, 20, 242);

    buffer[7][9] = 42;

    EXPECT_EQ(buffer[0][0], 242);
    EXPECT_EQ(buffer[7][9], 42);
    EXPECT_EQ(buffer[9][19], 242);
}

TEST(Buffer2DTests, MakeCopy_Whole)
{
    Buffer2D<int, IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferCopy = buffer.MakeCopy();
    iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, MakeCopy_Region)
{
    Buffer2D<int, IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferCopy = buffer.MakeCopy(IntegralCoordinates(1, 1), IntegralRectSize(2, 2));

    ASSERT_EQ(IntegralRectSize(2, 2), bufferCopy->Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 1) * 4 + 1;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ((*bufferCopy)[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}