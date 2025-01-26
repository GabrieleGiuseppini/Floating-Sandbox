#include <Core/Colors.h>

#include "gtest/gtest.h"

TEST(ColorsTests, Rgb_ToString)
{
    rgbColor foo1(11, 22, 33);
    EXPECT_EQ("0b1621", foo1.toString());

    rgbColor foo2(0, 0, 0);
    EXPECT_EQ("000000", foo2.toString());

    rgbColor foo3(255, 255, 255);
    EXPECT_EQ("ffffff", foo3.toString());
}

TEST(ColorsTests, Rgb_FromString)
{
    rgbColor foo1 = rgbColor::fromString("0b1621");
    EXPECT_EQ(rgbColor(11, 22, 33), foo1);

    rgbColor foo2 = rgbColor::fromString("000000");
    EXPECT_EQ(rgbColor(0, 0, 0), foo2);

    rgbColor foo3 = rgbColor::fromString("ffffff");
    EXPECT_EQ(rgbColor(255, 255, 255), foo3);
}

TEST(ColorsTests, Rgb_FromString_Legacy)
{
    rgbColor foo1 = rgbColor::fromString("00 8 11");
    EXPECT_EQ(rgbColor(0, 8, 17), foo1);

    rgbColor foo2 = rgbColor::fromString("40 40 40");
    EXPECT_EQ(rgbColor(64, 64, 64), foo2);

    rgbColor foo3 = rgbColor::fromString("f ff ff");
    EXPECT_EQ(rgbColor(15, 255, 255), foo3);
}

TEST(ColorsTests, Rgba_ToString)
{
    rgbaColor foo1(11, 22, 33, 44);
    EXPECT_EQ("0b16212c", foo1.toString());

    rgbaColor foo2(0, 0, 0, 0);
    EXPECT_EQ("00000000", foo2.toString());

    rgbaColor foo3(255, 255, 255, 255);
    EXPECT_EQ("ffffffff", foo3.toString());
}

TEST(ColorsTests, Rgba_FromString)
{
    rgbaColor foo1 = rgbaColor::fromString("0b16212c");
    EXPECT_EQ(rgbaColor(11, 22, 33, 44), foo1);

    rgbaColor foo2 = rgbaColor::fromString("00000000");
    EXPECT_EQ(rgbaColor(0, 0, 0, 0), foo2);

    rgbaColor foo3 = rgbaColor::fromString("ffffffff");
    EXPECT_EQ(rgbaColor(255, 255, 255, 255), foo3);
}

TEST(ColorsTests, Rgba_FromString_Legacy)
{
    rgbaColor foo1 = rgbaColor::fromString("00 8 11 7");
    EXPECT_EQ(rgbaColor(0, 8, 17, 7), foo1);

    rgbaColor foo2 = rgbaColor::fromString("40 40 40 40");
    EXPECT_EQ(rgbaColor(64, 64, 64, 64), foo2);

    rgbaColor foo3 = rgbaColor::fromString("f ff ff ff");
    EXPECT_EQ(rgbaColor(15, 255, 255, 255), foo3);
}