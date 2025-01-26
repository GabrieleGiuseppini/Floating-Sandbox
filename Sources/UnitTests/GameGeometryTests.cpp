#include <Core/GameGeometry.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

#include <cmath>

class SegmentIntersectionTest : public testing::TestWithParam<std::tuple<vec2f, vec2f, vec2f, vec2f, bool>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
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

class IsPointInTriangleTest : public testing::TestWithParam<std::tuple<vec2f, vec2f, vec2f, vec2f, bool>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    IsPointInTriangleTests,
    IsPointInTriangleTest,
    ::testing::Values(
        std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, true),
        std::make_tuple(vec2f{ 1.1f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, true),
        std::make_tuple(vec2f{ 2.0f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, true),
        std::make_tuple(vec2f{ 1.0f, 3.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, false),
        std::make_tuple(vec2f{ 1.0f, 2.1f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, false),
        std::make_tuple(vec2f{ 0.9f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, vec2f{ 3.0f, 1.0f }, false)
    ));

TEST_P(IsPointInTriangleTest, PositiveAndNegativeTests)
{
    bool result = Geometry::IsPointInTriangle(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        std::get<2>(GetParam()),
        std::get<3>(GetParam()));

    EXPECT_EQ(result, std::get<4>(GetParam()));
}

///////////////////////////////////////////////////////

class Segment_DistanceToPointTest : public testing::TestWithParam<std::tuple<vec2f, vec2f, vec2f, float>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    Segment_DistanceToPointTests,
    Segment_DistanceToPointTest,
    ::testing::Values(
    // Empty segment
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 1.0f, 2.5f }, 0.5f),
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 1.0f, 2.0f }, vec2f{ 0.5f, 2.0f }, 0.5f),
    // Matching endpoint
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 4.0f }, vec2f{ 1.0f, 2.0f }, 0.0f),
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 4.0f }, vec2f{ 3.0f, 4.0f }, 0.0f),
    // Within endpoints
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 2.0f, 2.0f }, 0.0f),

    // Proper - within
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 2.0f, 3.0f }, 1.0f),

    // Proper - outside - in-line
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 0.0f, 2.0f }, 1.0f),
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 2.0f }, vec2f{ 4.0f, 2.0f }, 1.0f),

    // Proper - outside - out-line
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 7.0f }, vec2f{ 0.0f, 1.0f }, std::sqrt(2.0f)),
    std::make_tuple(vec2f{ 1.0f, 2.0f }, vec2f{ 3.0f, 7.0f }, vec2f{ 4.0f, 8.0f }, std::sqrt(2.0f))
));

TEST_P(Segment_DistanceToPointTest, Segment_DistanceToPointTests)
{
    float result = Geometry::Segment::DistanceToPoint(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        std::get<2>(GetParam()));

    EXPECT_TRUE(ApproxEquals(result, std::get<3>(GetParam()), 0.0001f));
}

///////////////////////////////////////////////////////

TEST(GeometryTests, GenerateLinePath_Minimal_Distance0)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 1u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance1)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(4, 6),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 2u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(4, 6));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance2_Diagonal)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(5, 7),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(4, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(5, 7));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance2_VerticalDown)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 7),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(3, 7));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance2_VerticalUp)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 3),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 4));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(3, 3));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance2_HorizontalLeft)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(1, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(2, 5));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(1, 5));
}

TEST(GeometryTests, GenerateLinePath_Minimal_Distance2_HorizontalRight)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::Minimal>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(5, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(4, 5));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(5, 5));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance0)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 1u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance1)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(4, 6),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(4, 6));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_Diagonal_Equal)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(5, 7),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 5u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(4, 6));
    EXPECT_EQ(generatedCoordinates[3], IntegralCoordinates(4, 7));
    EXPECT_EQ(generatedCoordinates[4], IntegralCoordinates(5, 7));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_Diagonal_MoreX)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(5, 6),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 4u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(4, 5));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(4, 6));
    EXPECT_EQ(generatedCoordinates[3], IntegralCoordinates(5, 6));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_Diagonal_MoreY)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(4, 7),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 4u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(4, 6));
    EXPECT_EQ(generatedCoordinates[3], IntegralCoordinates(4, 7));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_VerticalDown)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 7),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 6));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(3, 7));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_VerticalUp)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(3, 3),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(3, 4));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(3, 3));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_HorizontalLeft)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(1, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(2, 5));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(1, 5));
}

TEST(GeometryTests, GenerateLinePath_WithAdjacentSteps_Distance2_HorizontalRight)
{
    std::vector<IntegralCoordinates> generatedCoordinates;

    Geometry::GenerateIntegralLinePath<Geometry::IntegralLineType::WithAdjacentSteps>(
        IntegralCoordinates(3, 5),
        IntegralCoordinates(5, 5),
        [&generatedCoordinates](IntegralCoordinates const & pt)
        {
            generatedCoordinates.push_back(pt);
        });

    ASSERT_EQ(generatedCoordinates.size(), 3u);
    EXPECT_EQ(generatedCoordinates[0], IntegralCoordinates(3, 5));
    EXPECT_EQ(generatedCoordinates[1], IntegralCoordinates(4, 5));
    EXPECT_EQ(generatedCoordinates[2], IntegralCoordinates(5, 5));
}