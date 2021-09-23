#include <GameCore/Colors.h>

#include "gtest/gtest.h"

TEST(ColorsTests, ToString)
{
    rgbColor foo1(11, 22, 33);
    EXPECT_EQ("0b1621", foo1.toString());

    rgbColor foo2(0, 0, 0);
    EXPECT_EQ("000000", foo2.toString());

    rgbColor foo3(255, 255, 255);
    EXPECT_EQ("ffffff", foo3.toString());
}

TEST(ColorsTests, FromString)
{
    rgbColor foo1 = rgbColor::fromString("0b1621");
    EXPECT_EQ(rgbColor(11, 22, 33), foo1);

    rgbColor foo2 = rgbColor::fromString("000000");
    EXPECT_EQ(rgbColor(0, 0, 0), foo2);

    rgbColor foo3 = rgbColor::fromString("ffffff");
    EXPECT_EQ(rgbColor(255, 255, 255), foo3);
}