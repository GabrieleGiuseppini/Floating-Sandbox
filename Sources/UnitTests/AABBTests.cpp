#include <Core/AABB.h>
#include <Core/AABBSet.h>

#include "gtest/gtest.h"

TEST(AABBTests, AABB_Contains)
{
    Geometry::AABB t(10.0f, 20.0f, 100.0f, 90.0f);

    EXPECT_FALSE(t.Contains(vec2f(5.0f, 95.0f)));
    EXPECT_FALSE(t.Contains(vec2f(15.0f, 85.0f)));
    EXPECT_TRUE(t.Contains(vec2f(15.0f, 95.0f)));
}

TEST(AABBTests, AABB_ContainsWithMargin)
{
    Geometry::AABB t(10.0f, 20.0f, 100.0f, 90.0f);

    EXPECT_FALSE(t.Contains(vec2f(5.0f, 95.0f), 2.0f));
    EXPECT_FALSE(t.Contains(vec2f(15.0f, 85.0f), 2.0f));
    EXPECT_TRUE(t.Contains(vec2f(9.0f, 95.0f), 2.0f));
    EXPECT_TRUE(t.Contains(vec2f(15.0f, 89.0f), 2.0f));
}

TEST(AABBTests, AABBSet_Contains)
{
    Geometry::AABBSet t;
    t.Add(Geometry::AABB(10.0f, 20.0f, 100.0f, 90.0f));
    t.Add(Geometry::AABB(1000.0f, 2000.0f, 10000.0f, 9000.0f));

    EXPECT_FALSE(t.Contains(vec2f(5.0f, 95.0f)));
    EXPECT_FALSE(t.Contains(vec2f(500.0f, 9500.0f)));
    EXPECT_FALSE(t.Contains(vec2f(15.0f, 85.0f)));
    EXPECT_FALSE(t.Contains(vec2f(1500.0f, 8500.0f)));
    EXPECT_TRUE(t.Contains(vec2f(15.0f, 95.0f)));
    EXPECT_TRUE(t.Contains(vec2f(1500.0f, 9500.0f)));
}

TEST(AABBTests, AABBSet_MakeUnion)
{
    Geometry::AABBSet t;
    t.Add(Geometry::AABB(10.0f, 20.0f, 100.0f, 80.0f));
    t.Add(Geometry::AABB(15.0f, 25.0f, 91.0f, 70.0f));

    auto const res = t.MakeUnion();

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->TopRight, vec2f(25.0f, 100.0f));
    EXPECT_EQ(res->BottomLeft, vec2f(10.0f, 70.0f));
}
