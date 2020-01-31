#include <GameCore/Segment.h>

#include "gtest/gtest.h"

class SegmentIntersectionTest : public testing::TestWithParam<std::tuple<vec2f, vec2f, vec2f, vec2f, bool>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    SegmentIntersectionTests,
    SegmentIntersectionTest,
    ::testing::Values(
        // Basic
        std::make_tuple(vec2f{ 0.0f, 0.0f }, vec2f{ 2.0f, 0.0f }, vec2f{ 1.0f, 1.0f }, vec2f{ 1.0f, 4.0f }, false),
        std::make_tuple(vec2f{ 0.0f, 0.0f }, vec2f{ 2.0f, 0.0f }, vec2f{ 1.0f, 1.0f }, vec2f{ 1.0f, -1.0f }, true),

        // Cross - 2
        std::make_tuple(vec2f{ 1.0f, 1.0f }, vec2f{ 3.0f, 3.0f }, vec2f{ 1.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, true),
        std::make_tuple(vec2f{ 1.0f, 1.0f }, vec2f{ 3.0f, 3.0f }, vec2f{ 1.0f, 3.0f }, vec2f{ 1.9f, 2.1f }, false),

        // Collinear - 4
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 6.0f, 2.0f }, vec2f{ 5.0f, 2.0f }, false),
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 6.0f, 2.0f }, vec2f{ 2.5f, 2.0f }, false),
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 2.5f, 2.0f }, vec2f{ 6.0f, 2.0f }, false),

        // Parallel - 7
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 2.0f, 1.0f }, vec2f{ 3.0f, 1.0f }, false),
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 1.0f, 3.0f }, false),

        // Heavy skew
        std::make_tuple(vec2f{ 4.0f, 4.0f }, vec2f{ 8.0f, 4.0f }, vec2f{ 2.0f, 5.0f }, vec2f{ 10.0f, 3.0f }, true),
        std::make_tuple(vec2f{ 4.0f, 4.0f }, vec2f{ 8.0f, 4.0f }, vec2f{ 10.0f, 3.0f }, vec2f{ 2.0f, 5.0f }, true),
        std::make_tuple(vec2f{ 4.0f, 4.0f }, vec2f{ 4.0f, 8.0f }, vec2f{ 3.0f, 10.0f }, vec2f{ 5.0f, 2.0f }, true),
        std::make_tuple(vec2f{ 4.0f, 4.0f }, vec2f{ 4.0f, 8.0f }, vec2f{ 5.0f, 2.0f }, vec2f{ 3.0f, 10.0f }, true),

        // Cross - 13
        std::make_tuple(vec2f{ -4.0f, 0.0f }, vec2f{ 4.0f, 0.0f }, vec2f{ 0.0f, 4.0f }, vec2f{ 0.0f, -4.0f }, true),
        std::make_tuple(vec2f{ -4.0f, 0.0f }, vec2f{ 4.0f, 0.0f }, vec2f{ 3.0f, 4.0f }, vec2f{ 3.0f, -4.0f }, true),
        std::make_tuple(vec2f{ -4.0f, 0.0f }, vec2f{ 4.0f, 0.0f }, vec2f{ 3.5f, 4.0f }, vec2f{ 3.5f, -4.0f }, true),
        std::make_tuple(vec2f{ -4.0f, 0.0f }, vec2f{ 4.0f, 0.0f }, vec2f{ 3.98f, 4.0f }, vec2f{ 3.98f, -4.0f }, true),
        std::make_tuple(vec2f{ -4.0f, 0.0f }, vec2f{ 4.0f, 0.0f }, vec2f{ 3.98f, 4.0f }, vec2f{ 3.98f, -4.0f }, true),
        std::make_tuple(vec2f{ 0.0f, 20.0f }, vec2f{ 0.0f, -20.0f }, vec2f{ -1.0f, 0.0f }, vec2f{ 1.0f, 0.0f }, true),
        std::make_tuple(vec2f{ -20.0f, 0.0f }, vec2f{ 20.0f, 0.0f }, vec2f{ 0.0f, -1.0f }, vec2f{ 0.0f, 1.0f }, true),
        std::make_tuple(vec2f{ -25.0f, 0.0f }, vec2f{ 20.0f, 0.0f }, vec2f{ 0.0f, -1.0f }, vec2f{ 0.0f, 1.0f }, true),
        std::make_tuple(vec2f{ -20.0f, 0.0f }, vec2f{ 25.0f, 0.0f }, vec2f{ 0.0f, -1.0f }, vec2f{ 0.0f, 1.0f }, true),
        std::make_tuple(vec2f{ 0.0f, -20.0f }, vec2f{ 0.0f, 25.0f }, vec2f{ -1.0f, 0.0f }, vec2f{ 1.0f, 0.0f }, true),

        // Micro
        std::make_tuple(vec2f{ 4.5f, 33.3f }, vec2f{ 4.5f, 33.1f }, vec2f{ 4.0f, 33.4f }, vec2f{ 5.0f, 33.4f }, false),
        std::make_tuple(vec2f{ 4.5f, 33.3f }, vec2f{ 4.5f, 33.1f }, vec2f{ 4.0f, 33.2f }, vec2f{ 5.0f, 33.2f }, true),
        std::make_tuple(vec2f{ 4.43f, 33.3f }, vec2f{ 4.43f, 33.1f }, vec2f{ 4.0f, 33.2f }, vec2f{ 5.0f, 33.2f }, true),
        std::make_tuple(vec2f{ 4.5f, 33.33f }, vec2f{ 4.5f, 33.19f }, vec2f{ 4.0f, 33.31f }, vec2f{ 5.0f, 33.31f }, true),
        std::make_tuple(vec2f{ 4.5f, 33.325f }, vec2f{ 4.5f, 33.195f }, vec2f{ 4.0f, 33.313f }, vec2f{ 5.0f, 33.313f }, true),
        std::make_tuple(vec2f{ 4.43f, 33.325f }, vec2f{ 4.43f, 33.195f }, vec2f{ 4.0f, 33.313f }, vec2f{ 5.0f, 33.313f }, true),
        std::make_tuple(vec2f{ 4.43733f, 33.3297f }, vec2f{ 4.43733f, 33.1941f }, vec2f{ 4.0f, 33.3129f }, vec2f{ 5.0f, 33.3129f }, true)
    ));

TEST_P(SegmentIntersectionTest, ProperIntersectionTest)
{
    bool result = Geometry::Segment::ProperIntersectionTest(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        std::get<2>(GetParam()),
        std::get<3>(GetParam()));

    EXPECT_EQ(result, std::get<4>(GetParam()));

    result = Geometry::Segment::ProperIntersectionTest(
        std::get<1>(GetParam()),
        std::get<0>(GetParam()),
        std::get<2>(GetParam()),
        std::get<3>(GetParam()));

    EXPECT_EQ(result, std::get<4>(GetParam()));

    result = Geometry::Segment::ProperIntersectionTest(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        std::get<3>(GetParam()),
        std::get<2>(GetParam()));

    EXPECT_EQ(result, std::get<4>(GetParam()));

    result = Geometry::Segment::ProperIntersectionTest(
        std::get<1>(GetParam()),
        std::get<0>(GetParam()),
        std::get<3>(GetParam()),
        std::get<2>(GetParam()));

    EXPECT_EQ(result, std::get<4>(GetParam()));
}