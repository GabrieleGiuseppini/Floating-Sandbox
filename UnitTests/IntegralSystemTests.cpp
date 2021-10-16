#include <GameCore/GameTypes.h>

#include "gtest/gtest.h"

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Left)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 2, 10 });
    EXPECT_EQ(foo.origin.x, 2);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 7);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Left_ByZero)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 5, 10 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Left_ByOne)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 4, 10 });
    EXPECT_EQ(foo.origin.x, 4);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 5);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Right)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 10, 10 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 6);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Right_ByZero)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 8, 10 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Right_ByOne)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 9, 10 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 5);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Top)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 6, 8 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 8);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 10);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Top_ByZero)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 6, 10 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 8);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Top_ByOne)
{
    IntegralRect foo({ 5, 10 }, { 4, 8 });
    foo.UpdateWith({ 6, 9 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 9);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 9);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Bottom)
{
    IntegralRect foo({ 5, 10 }, { 4, 2 });
    foo.UpdateWith({ 6, 20 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 11);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Bottom_ByZero)
{
    IntegralRect foo({ 5, 10 }, { 4, 2 });
    foo.UpdateWith({ 6, 11 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 2);
}

TEST(IntegralSystemTests, IntegralRect_UpdateWith_Bottom_ByOne)
{
    IntegralRect foo({ 5, 10 }, { 4, 2 });
    foo.UpdateWith({ 6, 12 });
    EXPECT_EQ(foo.origin.x, 5);
    EXPECT_EQ(foo.origin.y, 10);
    EXPECT_EQ(foo.size.width, 4);
    EXPECT_EQ(foo.size.height, 3);
}

class IntersectionTest_NonEmpty : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntersectionTest_NonEmpty,
    ::testing::Values(
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, {4, 4} }, { {4, 5}, {2, 2} }, { {4, 5},  {2, 2} }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {4, 5}, {2, 2} }, { {3, 4}, {4, 4} }, { {4, 5},  {2, 2} }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {4, 5}, {2, 2} }, { {4, 5},  {1, 1} })
    ));

TEST_P(IntersectionTest_NonEmpty, IntersectionTest_NonEmpty)
{
    auto const result = std::get<0>(GetParam()).IntersectWith(std::get<1>(GetParam()));

    ASSERT_TRUE(result);
    EXPECT_EQ(*result, std::get<2>(GetParam()));
}

class IntersectionTest_Empty : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntersectionTest_Empty,
    ::testing::Values(
        // new x at size
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {5, 5}, {1, 2} }),
        // new y at size
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {6, 6}, {1, 2} }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {5, 5}, {1, 2} }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {0, 0} }, { {5, 5}, {1, 2} }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {5, 6}, {1, 2} }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, {2, 2} }, { {15, 15}, {1, 2} })
    ));

TEST_P(IntersectionTest_Empty, IntersectionTest_NonEmpty)
{
    auto const result = std::get<0>(GetParam()).IntersectWith(std::get<1>(GetParam()));

    EXPECT_FALSE(result);
}