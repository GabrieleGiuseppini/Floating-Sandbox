#include <GameCore/Version.h>

#include "gtest/gtest.h"

TEST(VersionTest, CurrentVersion)
{
    EXPECT_EQ(Version::CurrentVersion().ToString(), APPLICATION_VERSION_LONG_STR);
}

TEST(VersionTest, Operators)
{
    Version v1(5, 6, 7, 8);
    Version v2(5, 6, 7, 0);

    EXPECT_TRUE(v1 == Version(5, 6, 7, 8));
    EXPECT_FALSE(v1 == Version(5, 6, 7, 0));
    EXPECT_FALSE(v1 == Version(4, 6, 7, 8));
    EXPECT_FALSE(v1 == Version(5, 6, 7, 9));

    EXPECT_TRUE(v2 == Version(5, 6, 7, 0));
    EXPECT_FALSE(v2 == Version(5, 6, 7, 8));
    EXPECT_FALSE(v2 == Version(4, 6, 7, 8));
    EXPECT_FALSE(v2 == Version(5, 6, 7, 9));

    EXPECT_TRUE(v1 != Version(4, 6, 7, 8));
    EXPECT_FALSE(v1 != Version(5, 6, 7, 8));

    EXPECT_TRUE(v2 != Version(4, 6, 7, 8));
    EXPECT_FALSE(v2 != Version(5, 6, 7, 0));

    EXPECT_FALSE(v1 < Version(5, 6, 7, 8));
    EXPECT_TRUE(v1 < Version(5, 6, 7, 0)); // beta 8 < beta 0
    EXPECT_TRUE(v1 < Version(6, 6, 7, 8));
    EXPECT_TRUE(v1 < Version(5, 7, 7, 8));
    EXPECT_TRUE(v1 < Version(5, 6, 8, 8));
    EXPECT_FALSE(v1 < Version(5, 6, 7, 7));
    EXPECT_TRUE(v1 < Version(5, 6, 7, 9));

    EXPECT_FALSE(v2 < Version(5, 6, 7, 0));
    EXPECT_FALSE(v2 < Version(5, 6, 7, 8)); // beta 0 >beta 8
    EXPECT_TRUE(v2 < Version(6, 6, 7, 8));
    EXPECT_TRUE(v2 < Version(5, 7, 7, 8));
    EXPECT_TRUE(v2 < Version(5, 6, 8, 8));
    EXPECT_FALSE(v2 < Version(5, 6, 7, 9));

    EXPECT_TRUE(v1 <= Version(5, 6, 7, 8));
    EXPECT_TRUE(v1 <= Version(5, 6, 7, 0)); // beta 8 < beta 0
    EXPECT_TRUE(v1 <= Version(6, 6, 7, 8));
    EXPECT_TRUE(v1 <= Version(5, 7, 7, 8));
    EXPECT_TRUE(v1 <= Version(5, 6, 8, 8));
    EXPECT_FALSE(v1 <= Version(5, 6, 7, 7));
    EXPECT_TRUE(v1 <= Version(5, 6, 7, 9));

    EXPECT_TRUE(v2 <= Version(5, 6, 7, 0));
    EXPECT_FALSE(v2 <= Version(5, 6, 7, 8)); // beta 0 > beta 8
    EXPECT_TRUE(v2 <= Version(6, 6, 7, 8));
    EXPECT_TRUE(v2 <= Version(5, 7, 7, 8));
    EXPECT_TRUE(v2 <= Version(5, 6, 8, 8));
    EXPECT_FALSE(v2 <= Version(5, 6, 7, 9));

    EXPECT_TRUE(v1 >= Version(5, 6, 7, 8));
    EXPECT_FALSE(v1 >= Version(5, 6, 7, 0)); // beta 8 < beta 0
    EXPECT_TRUE(v1 >= Version(4, 6, 7, 8));
    EXPECT_TRUE(v1 >= Version(5, 5, 7, 8));
    EXPECT_TRUE(v1 >= Version(5, 6, 6, 8));
    EXPECT_TRUE(v1 >= Version(5, 6, 7, 7));
    EXPECT_FALSE(v1 >= Version(5, 6, 7, 9));

    EXPECT_TRUE(v2 >= Version(5, 6, 7, 0));
    EXPECT_TRUE(v2 >= Version(5, 6, 7, 8)); // beta 0 > beta 8
    EXPECT_TRUE(v2 >= Version(4, 6, 7, 8));
    EXPECT_TRUE(v2 >= Version(5, 5, 7, 8));
    EXPECT_TRUE(v2 >= Version(5, 6, 6, 8));
    EXPECT_TRUE(v2 >= Version(5, 6, 7, 7));
}

TEST(VersionTest, ToString)
{
    Version v1(5, 6, 0, 8);
    EXPECT_EQ(v1.ToString(), "5.6.0.beta8");

    Version v2(5, 6, 2, 0);
    EXPECT_EQ(v2.ToString(), "5.6.2");
}

TEST(VersionTest, FromString_Good)
{
    EXPECT_EQ(Version(5, 6, 8, 0), Version::FromString("5.6.8"));
    EXPECT_EQ(Version(5, 6, 0, 8), Version::FromString("5.6.0.beta8"));
    EXPECT_EQ(Version(5, 6, 0, 8), Version::FromString("5.6.0.beta8\n"));
    EXPECT_EQ(Version(1, 0, 0, 0), Version::FromString("1.0.0.beta0"));
    EXPECT_EQ(Version(1, 0, 0, 0), Version::FromString("1.0.0"));
    EXPECT_EQ(Version(500, 60, 40, 80), Version::FromString("500.60.40.beta80"));
}

TEST(VersionTest, FromString_Bad)
{
    EXPECT_THROW(
        Version::FromString(""),
        std::exception);

    EXPECT_THROW(
        Version::FromString("5.6."),
        std::exception);

    EXPECT_THROW(
        Version::FromString("5.6.0.8"),
        std::exception);

    EXPECT_THROW(
        Version::FromString("5.6.0,8"),
        std::exception);

    EXPECT_THROW(
        Version::FromString("5.a.0.8"),
        std::exception);

    EXPECT_THROW(
        Version::FromString("5.6.0.8h"),
        std::exception);
}