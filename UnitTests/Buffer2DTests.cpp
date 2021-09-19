#include <GameCore/Buffer2D.h>

#include "gtest/gtest.h"

TEST(Buffer2DTests, FillCctor)
{
    Buffer2D<int, Integral2DSize> buffer(10, 20, 242);

    EXPECT_EQ(buffer.Size.Width, 10);
    EXPECT_EQ(buffer.Size.Height, 20);

    EXPECT_EQ(buffer[vec2i(0, 0)], 242);
    EXPECT_EQ(buffer[vec2i(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing)
{
    Buffer2D<int, Integral2DSize> buffer(10, 20, 242);

    buffer[vec2i(7, 9)] = 42;

    EXPECT_EQ(buffer[vec2i(0, 0)], 242);
    EXPECT_EQ(buffer[vec2i(7, 9)], 42);
    EXPECT_EQ(buffer[vec2i(9, 19)], 242);
}

TEST(Buffer2DTests, MakeCopy_Whole)
{
    Buffer2D<int, Integral2DSize> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[vec2i(x, y)] = iVal++;
        }
    }

    auto const bufferCopy = buffer.MakeCopy();
    iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(buffer[vec2i(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, MakeCopy_Region)
{
    Buffer2D<int, Integral2DSize> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[vec2i(x, y)] = iVal++;
        }
    }

    auto const bufferCopy = buffer.MakeCopy(vec2i(1, 1), Integral2DSize(2, 2));

    ASSERT_EQ(Integral2DSize(2, 2), bufferCopy->Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 1) * 4 + 1;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ((*bufferCopy)[vec2i(x, y)], iVal);
            ++iVal;
        }
    }
}