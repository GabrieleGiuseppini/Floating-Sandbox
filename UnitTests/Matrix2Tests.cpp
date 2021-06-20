#include <GameCore/Matrix.h>

#include "gtest/gtest.h"

TEST(Matrix2Tests, Constructor_DefaultValue)
{
    Matrix2 m(10, 12, 242.0f);

    for (int x = 0; x < 10; ++x)
    {
        for (int y = 0; y < 12; ++y)
        {
            EXPECT_EQ(m[Matrix2Index(x, y)], 242.0f);
        }
    }
}

TEST(Matrix2Tests, Index_Plus)
{
    Matrix2Index i(10, 12);
    Matrix2Index j = i + Matrix2Index(3, -5);

    EXPECT_EQ(j.X, 13);
    EXPECT_EQ(j.Y, 7);
}