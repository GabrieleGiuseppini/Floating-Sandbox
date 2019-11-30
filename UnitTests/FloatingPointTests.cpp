#include <GameCore/FloatingPoint.h>

#include "Utils.h"

#include <limits>

#include "gtest/gtest.h"

TEST(FloatingPointTests, FlusthToZero)
{
    float const f1 = std::numeric_limits<float>::min();
    EXPECT_NE(f1, 0.0f);

    float const f2 = DivideByTwo(f1);
    EXPECT_NE(f2, 0.0f);

    EnableFloatingPointFlushToZero();

    float const f3 = DivideByTwo(f1);
    EXPECT_EQ(f3, 0.0f);

    DisableFloatingPointFlushToZero();

    float const f4 = DivideByTwo(f1);
    EXPECT_NE(f4, 0.0f);
}