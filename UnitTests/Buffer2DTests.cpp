#include <GameCore/Buffer2D.h>

#include "gtest/gtest.h"

TEST(Buffer2DTests, FillCctor)
{
    Buffer2D<int> buffer(10, 20, 242);

    EXPECT_EQ(buffer.Size.Width, 10);
    EXPECT_EQ(buffer.Size.Height, 20);

    EXPECT_EQ(buffer[vec2i(0, 0)], 242);
    EXPECT_EQ(buffer[vec2i(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing)
{
    Buffer2D<int> buffer(10, 20, 242);

    buffer[vec2i(7, 9)] = 42;

    EXPECT_EQ(buffer[vec2i(0, 0)], 242);
    EXPECT_EQ(buffer[vec2i(7, 9)], 42);
    EXPECT_EQ(buffer[vec2i(9, 19)], 242);
}