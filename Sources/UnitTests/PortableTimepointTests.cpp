#include <Core/PortableTimepoint.h>

#include <thread>

#include "gtest/gtest.h"

TEST(PortableTimepointTests, Now)
{
    auto const pt1 = PortableTimepoint::Now();

    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto const pt2 = PortableTimepoint::Now();

    EXPECT_GT(pt2.Value(), pt1.Value());
}

TEST(PortableTimepointTests, Operators)
{
    auto const pt1 = PortableTimepoint::Now();
    auto const pt1b = PortableTimepoint(pt1.Value());
    auto const ptLess = PortableTimepoint(pt1.Value() - 1);
    auto const ptMore = PortableTimepoint(pt1.Value() + 1);

    EXPECT_TRUE(pt1 == pt1b);
    EXPECT_FALSE(pt1 == ptLess);
    EXPECT_FALSE(pt1 == ptMore);

    EXPECT_FALSE(pt1 != pt1b);
    EXPECT_TRUE(pt1 != ptLess);
    EXPECT_TRUE(pt1 != ptMore);

    EXPECT_FALSE(pt1 < pt1b);
    EXPECT_FALSE(pt1 < ptLess);
    EXPECT_TRUE(pt1 < ptMore);

    EXPECT_TRUE(pt1 <= pt1b);
    EXPECT_FALSE(pt1 < ptLess);
    EXPECT_TRUE(pt1 < ptMore);

    EXPECT_FALSE(pt1 > pt1b);
    EXPECT_TRUE(pt1 > ptLess);
    EXPECT_FALSE(pt1 > ptMore);

    EXPECT_TRUE(pt1 >= pt1b);
    EXPECT_TRUE(pt1 > ptLess);
    EXPECT_FALSE(pt1 > ptMore);
}