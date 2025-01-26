#include <Core/Matrix.h>

#include "gtest/gtest.h"

TEST(Matrix2Tests, Constructor_DefaultValue)
{
    Matrix2 m(10, 12, 242.0f);

    for (int x = 0; x < 10; ++x)
    {
        for (int y = 0; y < 12; ++y)
        {
            EXPECT_EQ(m[vec2i(x, y)], 242.0f);
        }
    }
}