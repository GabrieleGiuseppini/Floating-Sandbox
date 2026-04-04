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

///////////////////////////////////////////////////////

struct TestLandVertex
{
    vec2f position; // World
    float depth;
    float interfaceBlendDepth;

    TestLandVertex(
        vec2f _position,
        float _depth,
        float _interfaceBlendDepth)
        : position(_position)
        , depth(_depth)
        , interfaceBlendDepth(_interfaceBlendDepth)
    {
    }
};


// TODOTEST: move to GameGeometry
template<typename TLandVertex, typename TVertexBuffer>
void GenerateSegmentedLandQuad(
    TLandVertex leftTop,
    TLandVertex leftBottom,
    TLandVertex rightTop,
    TLandVertex rightBottom,
    float maxQuadTriangleHeight,
    float minQuadTriangleHeight,
    TVertexBuffer & vertexBuffer)
{
    //       D
    //      /|
    //     / |
    //  A /--| D'
    //    |  |
    //  A'|--|C
    //    | /
    //  B |/

    assert(leftTop.position.x < rightTop.position.x);
    assert(leftTop.position.x == leftBottom.position.x);
    assert(rightTop.position.x == rightTop.position.x);
    assert(leftTop.position.y >= leftBottom.position.y);
    assert(rightTop.position.y >= rightBottom.position.y);

    // For convenience
    int constexpr A = 0;
    int constexpr B = 1;
    int constexpr C = 2;
    int constexpr D = 3;
    std::array<TLandVertex const, 4> const vertices = { leftTop, leftBottom, rightBottom, rightTop };

    // The two segments (left and right) that we are currently scanning down along

    struct Segment
    {
        int Start;
        int End;
    };

    // Left, right
    std::array<Segment, 2> currentSegmentEndpoints;

    //
    // Initialize segments
    //

    if (leftTop.position.y > rightTop.position.y)
    {
        currentSegmentEndpoints = { Segment{A, B}, Segment{A, D} };
    }
    else if (rightTop.position.y > leftTop.position.y)
    {
        currentSegmentEndpoints = { Segment{D, A}, Segment{D, C} };
    }
    else
    {
        assert(leftTop.position.y == rightTop.position.y);
        currentSegmentEndpoints = { Segment{A, B}, Segment{D, C} };
    }

    //
    // Scan line down
    //

    // The current left and right vertices of this scan line;
    // we've created quads up to here.
    //
    // Not always at real vertices (might be halfway through a real edge)
    std::array<TLandVertex, 2> currentScanLineVertices = { vertices[currentSegmentEndpoints[0].Start], vertices[currentSegmentEndpoints[1].Start] };

    float currentY = std::max(leftTop.position.y, rightTop.position.y);
    assert(currentY == std::max(currentScanLineVertices[0].position.y, currentScanLineVertices[1].position.y));
    float const maxBottomY = std::min(leftBottom.position.y, rightBottom.position.y);

    while (currentY > maxBottomY)
    {
        //
        // Scan current segments down until we meet the first one
        //

        // Prepare interpolations

        // Slopes on entire quad sides: end-start; left, right
        std::array<float, 2> Dx = {
            vertices[currentSegmentEndpoints[0].End].position.x - vertices[currentSegmentEndpoints[0].Start].position.x,
            vertices[currentSegmentEndpoints[1].End].position.x - vertices[currentSegmentEndpoints[1].Start].position.x };
        std::array<float, 2> Dy = {
            vertices[currentSegmentEndpoints[0].End].position.y - vertices[currentSegmentEndpoints[0].Start].position.y,
            vertices[currentSegmentEndpoints[1].End].position.y - vertices[currentSegmentEndpoints[1].Start].position.y };
        std::array<float, 2> Ddepth = {
            vertices[currentSegmentEndpoints[0].End].depth - vertices[currentSegmentEndpoints[0].Start].depth,
            vertices[currentSegmentEndpoints[1].End].depth - vertices[currentSegmentEndpoints[1].Start].depth };
        std::array<float, 2> DinterfaceBlendDepth = {
            vertices[currentSegmentEndpoints[0].End].interfaceBlendDepth - vertices[currentSegmentEndpoints[0].Start].interfaceBlendDepth,
            vertices[currentSegmentEndpoints[1].End].interfaceBlendDepth - vertices[currentSegmentEndpoints[1].Start].interfaceBlendDepth };

        // This is where we stop
        float const currentBottomY = std::max(vertices[currentSegmentEndpoints[0].End].position.y, vertices[currentSegmentEndpoints[1].End].position.y);

        while (currentY > currentBottomY)
        {
            // The current vertices are at the same y
            assert(currentScanLineVertices[0].position.y == currentScanLineVertices[1].position.y);

            // Calculate bottom y for this segment
            float newBottomY = currentBottomY; // Shoot for max first
            if (newBottomY < currentY - maxQuadTriangleHeight - minQuadTriangleHeight)
            {
                newBottomY = currentY - maxQuadTriangleHeight;
            }

            //
            // Interpolate x, depth, interface blend depth, for both sides
            //
            // Always refer to the origins, to reduce error
            //

            float const dyDy_Left = Dy[0] != 0.0f
                ? (newBottomY - vertices[currentSegmentEndpoints[0].Start].position.y) / Dy[0]
                : 0.0f;
            float const dyDy_Right = Dy[1] != 0.0f
                ? (newBottomY - vertices[currentSegmentEndpoints[1].Start].position.y) / Dy[1]
                : 0.0f;

            TLandVertex newLeftVertex = {
                vec2f(
                    vertices[currentSegmentEndpoints[0].Start].position.x + Dx[0] * dyDy_Left,
                    //vertices[currentSegmentEndpoints[0].Start].position.y + Dy[0] * dyDy_Left),
                    newBottomY),
                vertices[currentSegmentEndpoints[0].Start].depth + Ddepth[0] * dyDy_Left,
                vertices[currentSegmentEndpoints[0].Start].interfaceBlendDepth + DinterfaceBlendDepth[0] * dyDy_Left };

            TLandVertex newRightVertex = {
                vec2f(
                    vertices[currentSegmentEndpoints[1].Start].position.x + Dx[1] * dyDy_Right,
                    //vertices[currentSegmentEndpoints[1].Start].position.y + Dy[1] * dyDy_Right),
                    newBottomY),
                vertices[currentSegmentEndpoints[1].Start].depth + Ddepth[1] * dyDy_Right,
                vertices[currentSegmentEndpoints[1].Start].interfaceBlendDepth + DinterfaceBlendDepth[1] * dyDy_Right };

            //
            // Produce quad
            //
            //    J------K
            //    |      |
            //    |      |
            //   J'------K'
            //

            if (currentScanLineVertices[0].position.x < currentScanLineVertices[1].position.x) // J != K
            {
                // J-K-K'
                vertexBuffer.emplace_back(currentScanLineVertices[0]);
                vertexBuffer.emplace_back(currentScanLineVertices[1]);
                vertexBuffer.emplace_back(newRightVertex);
            }

            if (newLeftVertex.position.x < newRightVertex.position.x) // J' != K'
            {
                // J-K'-J'
                vertexBuffer.emplace_back(currentScanLineVertices[0]);
                vertexBuffer.emplace_back(newRightVertex);
                vertexBuffer.emplace_back(newLeftVertex);
            }

            //
            // Advance
            //

            currentScanLineVertices[0] = newLeftVertex;
            currentScanLineVertices[1] = newRightVertex;
            currentY = newBottomY;
        }

        //
        // We have reached one or both ends; generate next segment(s)
        //

        assert(currentY == vertices[currentSegmentEndpoints[0].End].position.y || currentY == vertices[currentSegmentEndpoints[1].End].position.y);

        if (currentY == vertices[currentSegmentEndpoints[0].End].position.y)
        {
            currentSegmentEndpoints[0].Start = currentSegmentEndpoints[0].End;
            currentSegmentEndpoints[0].End = currentSegmentEndpoints[0].End + 1;
            if (currentSegmentEndpoints[0].End >= 4)
            {
                currentSegmentEndpoints[0].End -= 4;
            }
        }

        if (currentY == vertices[currentSegmentEndpoints[1].End].position.y)
        {
            currentSegmentEndpoints[1].Start = currentSegmentEndpoints[1].End;
            currentSegmentEndpoints[1].End = currentSegmentEndpoints[1].End - 1;
            if (currentSegmentEndpoints[1].End < 0)
            {
                currentSegmentEndpoints[1].End += 4;
            }
        }
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleAboveOnly_Orientation1)
{
    //      D    150.0
    //     /|
    //    / |
    //   /  |
    //  AB--C    133.0

    std::vector<TestLandVertex> vertexBuffer;

    float const oDx = 2.0f;
    float const Dy = 150.0f - 133.0f;
    float const oDd = 20.0f - 0.0f;
    float const oDibd = 110.0f - 100.0f;
    float const rDd = 60.0f;
    float const rDibd = 10.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f - Dy}, 0.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 150.0f - Dy}, 0.0f, 100.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 110.0f },
        // C
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 20.0f + rDd, 110.0f + rDibd },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 3 + 2 * 3);

    {
        // D-D'-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 110.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 110.0f - oDibd * 10.0f / Dy, 0.0001f));
    }

    {
        // A'-D'-C

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 110.0f - oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 20.0f + rDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 110.0f + rDibd, 0.0000f));

        // A'-C-AB

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 110.0f - oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 20.0f + rDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 110.0f + rDibd, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 100.0f, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleAboveOnly_Orientation2)
{
    //  A       150.0
    //  |\
    //  | \
    //  |  \
    //  B---CD  133.0

    std::vector<TestLandVertex> vertexBuffer;

    float const oDx = 2.0f;
    float const Dy = 150.0f - 133.0f;
    float const oDd = 50.0f - 10.0f;
    float const oDibd = 500.0f - 100.0f;
    float const lDd = 70.0f;
    float const lDibd = 20.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f}, 10.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 150.0f - Dy}, 80.0f, 120.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 50.0f, 500.0f },
        // C
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 50.0f, 500.0f },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 3 + 2 * 3);

    {
        // A-C'-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 0.0f + oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 10.0f + oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 100.0f + oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 10.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));
    }

    {
        // A'-C'-CD

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 10.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 0.0f + oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 10.0f + oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 100.0f + oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 50.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 500.0f, 0.0000f));

        // A'-CD-B

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 10.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 50.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 500.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 80.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 120.0f, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_RectOnly)
{
    //   A--B    150.0
    //   |  |
    //   |  |
    //   |  |
    //   C--D    103.0

    std::vector<TestLandVertex> vertexBuffer;

    float const Dy = 150.0f - 103.0f;
    float const lDd = 40.0f;
    float const lDibd = 50.0f;
    float const rDd = 60.0f;
    float const rDibd = 10.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f}, 0.0f, 100.0f },
        // C
        TestLandVertex{ {0.0f, 150.0f - Dy}, 0.0f + lDd, 100.0f + lDibd },
        // B
        TestLandVertex{ {1.0f, 150.0f}, 20.0f, 110.0f },
        // D
        TestLandVertex{ {1.0f, 150.0f - Dy}, 20.0f + rDd, 110.0f + rDibd },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 5 * 2 * 3);

    {
        // A-B-B'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 110.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        // A-B'-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 0.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));
    }

    {
        // A`-B'-B''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 0.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 20.0f + rDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 110.0f + rDibd * 20.0f / Dy, 0.0001f));

        // A`-B''-A''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].depth, 0.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.x, 1.0, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].depth, 20.0f + rDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].interfaceBlendDepth, 110.0f + rDibd * 20.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].depth, 0.0f + lDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].interfaceBlendDepth, 100.0f + lDibd * 20.0f / Dy, 0.0001f));
    }

    {
        // A''-B''-B'''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].depth, 0.0f + lDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].interfaceBlendDepth, 100.0f + lDibd * 20.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].depth, 20.0f + rDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].interfaceBlendDepth, 110.0f + rDibd * 20.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].depth, 20.0f + rDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].interfaceBlendDepth, 110.0f + rDibd * 30.0f / Dy, 0.0001f));

        // A''-B'''-A'''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].depth, 0.0f + lDd * 20.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].interfaceBlendDepth, 100.0f + lDibd * 20.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].depth, 20.0f + rDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].interfaceBlendDepth, 110.0f + rDibd * 30.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].depth, 0.0f + lDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].interfaceBlendDepth, 100.0f + lDibd * 30.0f / Dy, 0.0001f));
    }

    {
        // A'''-B'''-B''''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].depth, 0.0f + lDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].interfaceBlendDepth, 100.0f + lDibd * 30.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].depth, 20.0f + rDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].interfaceBlendDepth, 110.0f + rDibd * 30.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].depth, 20.0f + rDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].interfaceBlendDepth, 110.0f + rDibd * 40.0f / Dy, 0.0001f));

        // A'''-B''''-A''''

        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].depth, 0.0f + lDd * 30.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].interfaceBlendDepth, 100.0f + lDibd * 30.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].depth, 20.0f + rDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].interfaceBlendDepth, 110.0f + rDibd * 40.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].depth, 0.0f + lDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].interfaceBlendDepth, 100.0f + lDibd * 40.0f / Dy, 0.0001f));
    }

    {
        // A'''-B''''-D

        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].depth, 0.0f + lDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].interfaceBlendDepth, 100.0f + lDibd * 40.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].depth, 20.0f + rDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].interfaceBlendDepth, 110.0f + rDibd * 40.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].position.y, 103.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].depth, 20.0f + rDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].interfaceBlendDepth, 110.0f + rDibd, 0.0000f));

        // A'''-D-C

        EXPECT_TRUE(ApproxEquals(vertexBuffer[27].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[27].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[27].depth, 0.0f + lDd * 40.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[27].interfaceBlendDepth, 100.0f + lDibd * 40.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[28].position.x, 1.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[28].position.y, 103.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[28].depth, 20.0f + rDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[28].interfaceBlendDepth, 110.0f + rDibd, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[29].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[29].position.y, 103.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[29].depth, 0.0f + lDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[29].interfaceBlendDepth, 100.0f + lDibd, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleBelowOnly_Orientation1)
{
    //  AB--D    150.0
    //   \  |
    //    \ |
    //     \|
    //      C    133.0

    std::vector<TestLandVertex> vertexBuffer;

    float const Dy = 150.0f - 133.0f;
    float const rDd = 60.0f;
    float const rDibd = 10.0f;

    float const oDx = 2.0f;
    float const oDd = (20.0f + rDd) - 0.0f;
    float const oDibd = (110.0f + rDibd) - 100.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f }, 0.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 150.0f }, 0.0f, 100.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 110.0f },
        // C
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 20.0f + rDd, 110.0f + rDibd },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 2 * 3 + 3);

    {
        // AB-D-D'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 110.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        // AB-D'-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 0.0f + oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 0.0f + oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 100.0f + oDibd * 10.0f / Dy, 0.0001f));
    }

    {
        // A'-D'-C

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 0.0f + oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 0.0f + oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 100.0f + oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 20.0f + rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 110.0f + rDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 20.0f + rDd, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 110.0f + rDibd, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleBelowOnly_Orientation2)
{
    // A---CD    150.0
    // |  /
    // | /
    // |/
    // B         133.0

    std::vector<TestLandVertex> vertexBuffer;

    float const Dy = 150.0f - 133.0f;
    float const lDd = 60.0f;
    float const lDibd = 10.0f;

    float const oDx = 2.0f;
    float const oDd = 20.0f - 60.0f;
    float const oDibd = 120.0f - 110.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f }, 0.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 150.0f - Dy }, 60.0f, 110.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 120.0f },
        // C
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 120.0f },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 2 * 3 + 3);

    {
        // A-CD-CD'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 120.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 120.0f - oDibd * 10.0f / Dy, 0.0001f));

        // A-CD'-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 120.0f - oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 0.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));
    }

    {
        // A'-CD'-B

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 0.0f + lDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 100.0f + lDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f - oDx * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 20.0f - oDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 120.0f - oDibd * 10.0f / Dy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 133.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 60.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 110.0f, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_Full)
{
    //        D    150.0
    //       /|
    //   A1 / | D1
    //     /  |
    //    A---D'   130.0
    //    |   |
    // A2 |   | D2
    //    |   |
    //    A'--C    110.0
    //    |  /
    // A3 | / C2
    //    |/
    //    B         10.0


    std::vector<TestLandVertex> vertexBuffer;

    float const lDy = 130.0f - 10.0f;
    float const lDd = 10.0f - 20.0f;
    float const lDibd = 100.0f - 200.0f;

    float const rDy = 150.0f - 110.0f;
    float const rDd = 100.0f - 200.0f;
    float const rDibd = 1000.0f - 2000.0f;

    float const tDd = 100.0f - 10.0f;
    float const tDibd = 1000.0f - 100.0f;
    float const bDd = 200.0f - 20.0f;
    float const bDibd = 2000.0f - 200.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 130.0f}, 10.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 10.0f}, 20.0f, 200.0f },
        // D
        TestLandVertex{ {2.0f, 150.0f}, 100.0f, 1000.0f },
        // C
        TestLandVertex{ {2.0f, 110.0f}, 200.0f, 2000.0f },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 3 * 3 + 4 * 3 + 19 * 3);

    // Top

    {
        // D-D1-A1

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 100.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 1000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 100.0f - rDd * 10.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 1000.0f - rDibd * 10.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 2.0f - 2.0f * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 100.0f - tDd * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 1000.0f - tDibd * 10.0f / 20.0f, 0.0001f));
    }

    {
        // A1-D1-D'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 2.0f - 2.0f * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 100.0f - tDd * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 1000.0f - tDibd * 10.0f / 20.0f, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 100.0f - rDd * 10.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 1000.0f - rDibd * 10.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 100.0f - rDd * 20.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 1000.0f - rDibd * 20.0f / rDy, 0.0001f));

        // A1-D'-A

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 2.0f - 2.0f * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 100.0f - tDd * 10.0f / 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 1000.0f - tDibd * 10.0f / 20.0f, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 100.0f - rDd * 20.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 1000.0f - rDibd * 20.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 100.0f, 0.0000f));
    }

    // Rect

    {
        // A-D'-D2

        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].depth, 100.0f - rDd * 20.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].interfaceBlendDepth, 1000.0f - rDibd * 20.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].depth, 100.0f - rDd * 30.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].interfaceBlendDepth, 1000.0f - rDibd * 30.0f / rDy, 0.0001f));

        // A-D2-A2

        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].position.y, 130.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[12].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].depth, 100.0f - rDd * 30.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[13].interfaceBlendDepth, 1000.0f - rDibd * 30.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].depth, 10.0f - lDd * 10.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[14].interfaceBlendDepth, 100.0f - lDibd * 10.0f / lDy, 0.0001f));
    }

    {
        // A2-D2-C

        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].depth, 10.0f - lDd * 10.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[15].interfaceBlendDepth, 100.0f - lDibd * 10.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].depth, 100.0f - rDd * 30.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[16].interfaceBlendDepth, 1000.0f - rDibd * 30.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[17].interfaceBlendDepth, 2000.0f, 0.0000f));

        // A2-C-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].position.y, 120.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].depth, 10.0f - lDd * 10.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[18].interfaceBlendDepth, 100.0f - lDibd * 10.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[19].interfaceBlendDepth, 2000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].depth, 10.0f - lDd * 20.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[20].interfaceBlendDepth, 100.0f - lDibd * 20.0f / lDy, 0.0001f));
    }

    // Bottom

    {
        // A'-C-C2

        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].depth, 10.0f - lDd * 20.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[21].interfaceBlendDepth, 100.0f - lDibd * 20.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[22].interfaceBlendDepth, 2000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].position.x, 2.0f - 2.0f * 10.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].position.y, 100.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].depth, 200.0f - bDd * 10.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[23].interfaceBlendDepth, 2000.0f - bDibd * 10.0f / 100.0f, 0.0001f));

        // A'-C2-A3

        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].position.y, 110.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].depth, 10.0f - lDd * 20.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[24].interfaceBlendDepth, 100.0f - lDibd * 20.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].position.x, 2.0f - 2.0f * 10.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].position.y, 100.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].depth, 200.0f - bDd * 10.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[25].interfaceBlendDepth, 2000.0f - bDibd * 10.0f / 100.0f, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].position.y, 100.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].depth, 10.0f - lDd * 30.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[26].interfaceBlendDepth, 100.0f - lDibd * 30.0f / lDy, 0.0001f));
    }

    // ...

    {
        // An-Cn-B

        EXPECT_TRUE(ApproxEquals(vertexBuffer[75].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[75].position.y, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[75].depth, 10.0f - lDd * 110.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[75].interfaceBlendDepth, 100.0f - lDibd * 110.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[76].position.x, 2.0f - 2.0f * 90.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[76].position.y, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[76].depth, 200.0f - bDd * 90.0f / 100.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[76].interfaceBlendDepth, 2000.0f - bDibd * 90.0f / 100.0f, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[77].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[77].position.y, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[77].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[77].interfaceBlendDepth, 200.0f, 0.0000f));
    }
}

TEST(GeometryTests, GenerateSegmentedLandQuad_Full_NoneSegmented)
{
    //        D    150.0
    //       /|
    //      / |
    //     /  |
    //    A---D'   139.0
    //    |   |
    //    |   |
    //    |   |
    //    A'--C    137.0
    //    |  /
    //    | /
    //    |/
    //    B         135.0


    std::vector<TestLandVertex> vertexBuffer;

    float const lDy = 139.0f - 135.0f;
    float const lDd = 10.0f - 20.0f;
    float const lDibd = 100.0f - 200.0f;

    float const rDy = 150.0f - 137.0f;
    float const rDd = 100.0f - 200.0f;
    float const rDibd = 1000.0f - 2000.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 139.0f}, 10.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f, 135.0f}, 20.0f, 200.0f },
        // D
        TestLandVertex{ {2.0f, 150.0f}, 100.0f, 1000.0f },
        // C
        TestLandVertex{ {2.0f, 137.0f}, 200.0f, 2000.0f },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 4 * 3);

    // Top

    {
        // D-D'-A

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 100.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 1000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 139.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 100.0f - rDd * 11.0f / rDy, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 1000.0f - rDibd * 11.0f / rDy, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 139.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 100.0f, 0.0000f));
    }

    // Rect

    {
        // A-D'-C

        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].position.y, 139.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[3].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].position.y, 139.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].depth, 100.0f - rDd * 11.0f / rDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[4].interfaceBlendDepth, 1000.0f - rDibd * 11.0f / rDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].position.y, 137.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[5].interfaceBlendDepth, 2000.0f, 0.0000f));

        // A-C-A'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].position.y, 139.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].depth, 10.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[6].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].position.y, 137.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[7].interfaceBlendDepth, 2000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].position.y, 137.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].depth, 10.0f - lDd * 2.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[8].interfaceBlendDepth, 100.0f - lDibd * 2.0f / lDy, 0.0001f));
    }

    // Bottom

    {
        // A'-C-B

        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].position.y, 137.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].depth, 10.0f - lDd * 2.0f / lDy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[9].interfaceBlendDepth, 100.0f - lDibd * 2.0f / lDy, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].position.y, 137.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].depth, 200.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[10].interfaceBlendDepth, 2000.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].position.y, 135.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].depth, 20.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[11].interfaceBlendDepth, 200.0f, 0.0000f));
    }
}

// TODO: full - with no vertical intersections

