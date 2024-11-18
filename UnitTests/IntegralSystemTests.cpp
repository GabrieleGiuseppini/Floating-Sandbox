#include <GameCore/GameTypes.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(IntegralSystemTests, Algebra_CoordsMinusSize)
{
    IntegralCoordinates coords = { 10, 15 };
    IntegralRectSize offset = { 2, 3 };

    IntegralCoordinates result = coords - offset;
    EXPECT_EQ(result, IntegralCoordinates(8, 12));
}

TEST(IntegralSystemTests, Scale)
{
    IntegralCoordinates coords = { 10, 15 };
    IntegralCoordinates scaler = { 2, 3 };

    IntegralCoordinates result = coords.scale(scaler);
    EXPECT_EQ(result, IntegralCoordinates(20, 45));
}

class IntegralRect_IsContainedInRect : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect, bool>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntegralRect_IsContainedInRect,
    ::testing::Values(
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(5, 5), IntegralRectSize(1, 1) }, { IntegralCoordinates(3, 4), IntegralRectSize(3, 2) }, true),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(5, 5), IntegralRectSize(2, 2) }, { IntegralCoordinates(5, 5), IntegralRectSize(2, 2) }, true),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(5, 5), IntegralRectSize(0, 0) }, { IntegralCoordinates(4, 4), IntegralRectSize(2, 2) }, true),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(2, 3), IntegralRectSize(1, 2) }, { IntegralCoordinates(0, 2), IntegralRectSize(4, 4) }, true),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(2, 3), IntegralRectSize(2, 2) }, { IntegralCoordinates(3, 3), IntegralRectSize(4, 4) }, false),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(2, 3), IntegralRectSize(2, 2) }, { IntegralCoordinates(2, 4), IntegralRectSize(4, 4) }, false),
        std::make_tuple<IntegralRect, IntegralRect, bool>({ IntegralCoordinates(2, 3), IntegralRectSize(2, 2) }, { IntegralCoordinates(2, 3), IntegralRectSize(1, 1) }, false)
    ));

TEST_P(IntegralRect_IsContainedInRect, IntegralRect_IsContainedInRect)
{
    auto const result = std::get<0>(GetParam()).IsContainedInRect(std::get<1>(GetParam()));

    EXPECT_EQ(result, std::get<2>(GetParam()));
}

class IntegralRect_UnionWithCoords : public testing::TestWithParam<std::tuple<IntegralRect, IntegralCoordinates, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntegralRect_UnionWithCoords,
    ::testing::Values(
        std::make_tuple<IntegralRect, IntegralCoordinates, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, {3, 4}, { {3, 4}, IntegralRectSize(4, 4) }),
        std::make_tuple<IntegralRect, IntegralCoordinates, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, {2, 3}, { {2, 3}, IntegralRectSize(5, 5) }),
        std::make_tuple<IntegralRect, IntegralCoordinates, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, {7, 8}, { {3, 4}, IntegralRectSize(5, 5) })
    ));

TEST_P(IntegralRect_UnionWithCoords, IntegralRect_UnionWithCoords)
{
    auto rect1 = std::get<0>(GetParam());
    rect1.UnionWith(std::get<1>(GetParam()));
    EXPECT_EQ(rect1, std::get<2>(GetParam()));
}

class IntegralRect_UnionWithRect : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntegralRect_UnionWithRect,
    ::testing::Values(
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {3, 4}, IntegralRectSize(4, 4) }, { {3, 4}, IntegralRectSize(4, 4) }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {4, 5}, IntegralRectSize(1, 1) }, { {3, 4}, IntegralRectSize(4, 4) }),

        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {2, 3}, IntegralRectSize(2, 2) }, { {2, 3}, IntegralRectSize(5, 5) }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {2, 3}, IntegralRectSize(1, 1) }, { {2, 3}, IntegralRectSize(5, 5) }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {2, 3}, IntegralRectSize(8, 8) }, { {2, 3}, IntegralRectSize(8, 8) })
    ));

TEST_P(IntegralRect_UnionWithRect, IntegralRect_UnionWithRect)
{
    auto rect1 = std::get<0>(GetParam());
    rect1.UnionWith(std::get<1>(GetParam()));
    EXPECT_EQ(rect1, std::get<2>(GetParam()));

    auto rect2 = std::get<1>(GetParam());
    rect2.UnionWith(std::get<0>(GetParam()));
    EXPECT_EQ(rect2, std::get<2>(GetParam()));
}

class IntegralRect_MakeIntersectionWith_NonEmpty : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntegralRect_MakeIntersectionWith_NonEmpty,
    ::testing::Values(
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(4, 4) }, { {4, 5}, IntegralRectSize(2, 2) }, { {4, 5}, IntegralRectSize(2, 2) }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {4, 5}, IntegralRectSize(2, 2) }, { {3, 4}, IntegralRectSize(4, 4) }, { {4, 5}, IntegralRectSize(2, 2) }),
        std::make_tuple<IntegralRect, IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {4, 5}, IntegralRectSize(2, 2) }, { {4, 5}, IntegralRectSize(1, 1) })
    ));

TEST_P(IntegralRect_MakeIntersectionWith_NonEmpty, IntegralRect_MakeIntersectionWith_NonEmpty)
{
    auto const result = std::get<0>(GetParam()).MakeIntersectionWith(std::get<1>(GetParam()));

    ASSERT_TRUE(result);
    EXPECT_EQ(*result, std::get<2>(GetParam()));
}

class IntegralRect_MakeIntersectionWith_Empty : public testing::TestWithParam<std::tuple<IntegralRect, IntegralRect>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    IntegralRect_MakeIntersectionWith_Empty,
    ::testing::Values(
        // new x at size
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {5, 5}, IntegralRectSize(1, 2) }),
        // new y at size
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {6, 6}, IntegralRectSize(1, 2) }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {5, 5}, IntegralRectSize(1, 2) }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(0, 0) }, { {5, 5}, IntegralRectSize(1, 2) }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {5, 6}, IntegralRectSize(1, 2) }),
        std::make_tuple<IntegralRect, IntegralRect>({ {3, 4}, IntegralRectSize(2, 2) }, { {15, 15}, IntegralRectSize(1, 2) })
    ));

TEST_P(IntegralRect_MakeIntersectionWith_Empty, IntegralRect_MakeIntersectionWith_Empty)
{
    auto const result = std::get<0>(GetParam()).MakeIntersectionWith(std::get<1>(GetParam()));

    EXPECT_FALSE(result);
}

class CoordsRatio : public testing::TestWithParam<std::tuple<ShipSpaceCoordinates, ShipSpaceToWorldSpaceCoordsRatio, vec2f>>
{
};

INSTANTIATE_TEST_SUITE_P(
    IntegralSystemTests,
    CoordsRatio,
    ::testing::Values(
        std::make_tuple<ShipSpaceCoordinates, ShipSpaceToWorldSpaceCoordsRatio, vec2f>({ 1, 7 }, { 1.0f, 2.0f }, { 2.0f, 14.0f }),
        std::make_tuple<ShipSpaceCoordinates, ShipSpaceToWorldSpaceCoordsRatio, vec2f>({ 1, 7 }, { 2.0f, 1.0f }, { 0.5f, 3.5f }),
        std::make_tuple<ShipSpaceCoordinates, ShipSpaceToWorldSpaceCoordsRatio, vec2f>({ 4, 6 }, { 2.0f, 3.0f }, { 6.0f, 9.0f })
    ));

TEST_P(CoordsRatio, CoordsRatio)
{
    auto const result = std::get<0>(GetParam()).ToFractionalCoords(std::get<1>(GetParam()));

    vec2f const expected = std::get<2>(GetParam());
    EXPECT_TRUE(ApproxEquals(result.x, expected.x, 0.00001f));
    EXPECT_TRUE(ApproxEquals(result.y, expected.y, 0.00001f));
}

TEST(IntegralSystemTests, Rect_Center)
{
    IntegralRect rect(
        IntegralCoordinates(10, 8),
        IntegralRectSize(4, 12));

    EXPECT_EQ(rect.Center(), IntegralCoordinates(12, 14));
}