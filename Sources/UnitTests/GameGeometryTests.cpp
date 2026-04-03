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
void GenerateSegmentedLandQuadTriangle(
    TLandVertex & apex1,
    TLandVertex & base1,
    TLandVertex & base2,
    float height, // Absolute
    float width, // Absolute
    float maxQuadTriangleHeight,
    float minQuadTriangleHeight,
    TVertexBuffer & vertexBuffer)
{
    //  apex
    //  |\
    //  | \
    // b1--b2

    assert(height > 0.0f);
    assert(width > 0.0f);

    for (float currentHeight = 0.0f; currentHeight < height; /* incremented in loop */)
    {
        // Calculate bottom height for this segment
        float newBottomHeight = height; // Shoot for max first
        if (newBottomHeight > currentHeight + maxQuadTriangleHeight + minQuadTriangleHeight)
        {
            newBottomHeight = currentHeight + maxQuadTriangleHeight;
        }

        // TODOHERE
    }
}

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
    //       B
    //      /|
    //     / |
    //  A /--| B'
    //    |  |
    //  A'|--|D
    //    | /
    //  C |/

    assert(leftTop.position.x < rightTop.position.x);
    assert(leftTop.position.x == leftBottom.position.x);
    assert(rightTop.position.x == rightTop.position.x);
    assert(leftTop.position.y >= leftBottom.position.y);
    assert(rightTop.position.y >= rightBottom.position.y);

    // For convenience
    size_t constexpr A = 0;
    size_t constexpr B = 1;
    size_t constexpr C = 2;
    size_t constexpr D = 3;
    std::array<TLandVertex const, 4> const vertices = { leftTop, rightTop, leftBottom, rightBottom };

    // The two segments (left and right) that we are currently scanning down along

    struct Segment
    {
        size_t Start;
        size_t End;
    };

    // Left, right
    std::array<Segment, 2> currentSegmentEndpoints;

    //
    // Initialize segments
    //

    if (leftTop.position.y > rightTop.position.y)
    {
        currentSegmentEndpoints = { Segment{A, C}, Segment{A, B} };
    }
    else if (rightTop.position.y > leftTop.position.y)
    {
        currentSegmentEndpoints = { Segment{B, A}, Segment{B, D} };
    }
    else
    {
        assert(leftTop.position.y == rightTop.position.y);
        currentSegmentEndpoints = { Segment{A, C}, Segment{B, D} };
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

        // TODO: advance
    }


    // TODOOLD 2

    //// Current vertices; at each phase we've generated up to these
    //std::array<TLandVertex, 2> currentVertices{ leftTop, rightTop };

    ////
    //// Phase 1. Do top triangle, if any
    ////

    //if (leftTop.position.y > rightTop.position.y)
    //{
    //    // Interpolate A down to A'
    //    float const dy = leftTop.position.y - rightTop.position.y
    //    float const Dy = leftTop.position.y - leftBottom.position.y;
    //    TLandVertex aPrimeVertex = {
    //        vec2f(leftTop.position.x, rightTop.position.y),
    //        Dy != 0.0f ? leftTop.depth + (leftBottom.depth - leftTop.depth) * dy / Dy : leftTop.depth,
    //        Dy != 0.0f ? leftTop.interfaceBlendDepth + (leftBottom.interfaceBlendDepth - leftTop.interfaceBlendDepth) * dy / Dy : leftTop.interfaceBlendDepth };

    //    GenerateSegmentedLandQuadTriangle(
    //        leftTop,
    //        aPrimeVertex,
    //        rightTop,
    //        leftTop.position.y - rightTop.position.y,
    //        rightTop.position.x - leftTop.position.x,
    //        maxQuadTriangleHeight,
    //        minQuadTriangleHeight,
    //        vertexBuffer);

    //    currentVertex[0] = aPrimeVertex;
    //}
    //else if (rightTop.position.y > leftTop.position.y)
    //{
    //    // TODOHERE
    //}
    //else
    //{
    //    // No triangle on top
    //    assert(leftTop.position.y == rightTop.position.y);
    //}

    ////
    //// Phase 2. Flat-top rectangle
    ////

    //assert(currentVertices[0].position.y == currentVertices[1].position.y);

    //// TODOHERE





    //// TODOOLD


    //// Start scan line from top
    //float currentY = std::max(leftTop.position.y, rightTop.position.y);

    //// Stop at last at bottom
    //float const maxBottomY = std::min(leftBottom.position.y, rightBottom.position.y);

    //// Current vertices; at each iteration we've generated up to these
    //std::array<TLandVertex, 2> currentVertices{ leftTop, rightTop };

    //// Prepare interpolations; delta's are all positive
    ////std::array<float, 2> const Dx = { rightTop.position.x - leftTop.position.x, leftTop.position.x - rightTop.position.x };
    //std::array<float, 2> const Dy = { leftTop.position.y - leftBottom.position.y, rightTop.position.y - rightBottom.position.y };
    //std::array<float, 2> const Ddepth = { leftTop.depth - leftBottom.depth, rightTop.depth - rightBottom.depth };
    //std::array<float, 2> const DinterfaceBlendDepth = { leftTop.interfaceBlendDepth - leftBottom.interfaceBlendDepth, rightTop.interfaceBlendDepth - rightBottom.interfaceBlendDepth };

    //while (currentY > maxBottomY)
    //{
    //    // We have generated quads up to currentVertices

    //    // Decide whether to do vertical interpolation (when we're sliding down the edged)
    //    // or horizontal (when we're sliding down the top or bottom triangle)

    //    int iVerticalSide = -1;

    //    // TODOHERE: condition broken for bottom triangle
    //    // Regular down if currentVertices[each].position.y == currentY; oblique otherwise
    //    if (currentVertices[0].position.y == currentY)
    //    {
    //        if (currentVertices[1].position.y == currentY)
    //        {
    //            // Do rectangle slice
    //        }
    //        else
    //        {
    //            // Do oblique side, right
    //            assert(currentY > currentVertices[1].position.y);
    //            assert(currentVertices[0].position.y == currentY);
    //            iVerticalSide = 0;
    //        }
    //    }
    //    else
    //    {
    //        // Do oblique side, left
    //        assert(currentY > currentVertices[0].position.y);
    //        assert(currentVertices[1].position.y == currentY);
    //        iVerticalSide = 1;
    //    }

    //    if (iVerticalSide >= 0)
    //    {
    //        //
    //        // Slice triangle, doing horizontal interpolation along oblique side,
    //        // and vertical interpolation along straight side
    //        //

    //        TLandVertex const verticalVertexOrigin = (iVerticalSide == 1) ? rightTop : leftTop; // We calc delta's always from origin, instead of incrementally, to reduce numeric errors

    //        // All "positive", i.e. vertical minus other
    //        // TODO: broken for bottom triangle
    //        float const Dx_Oblique = currentVertices[iVerticalSide].position.x - currentVertices[1 - iVerticalSide].position.x;
    //        float const Dy_Oblique = currentVertices[iVerticalSide].position.y - currentVertices[1 - iVerticalSide].position.y;
    //        float const Ddepth_Oblique = currentVertices[iVerticalSide].depth - currentVertices[1 - iVerticalSide].depth;
    //        float const DinterfaceBlendDepth_Oblique = currentVertices[iVerticalSide].interfaceBlendDepth - currentVertices[1 - iVerticalSide].interfaceBlendDepth;

    //        while (currentY > currentVertices[1 - iVerticalSide].position.y)
    //        {
    //            // Calculate bottom y for this segment
    //            float newBottomY = currentVertices[1 - iVerticalSide].position.y; // Shoot for max first
    //            if (newBottomY < currentY - maxQuadTriangleHeight - minQuadTriangleHeight)
    //            {
    //                newBottomY = currentY - maxQuadTriangleHeight;
    //            }

    //            //
    //            // Interpolate x, Depth, InterfaceBlendDepth horizontally for top/bottom side (oblique),
    //            // and vertically for vertical side
    //            //

    //            float const dy_Vertical = verticalVertexOrigin.position.y - newBottomY; // Positive
    //            assert(dy_Vertical >= 0.0f);

    //            // TODOHERE: need to calc dy_Oblique as delta height of triangle; depends on how we detect this case

    //            // x = x[i] + Dx * dy / Dy
    //            float const newDepth_Vertical = verticalVertexOrigin.depth + Ddepth[iVerticalSide] * dy_Vertical / Dy[iVerticalSide];
    //            float const newInterfaceBlendDepth_Vertical = verticalVertexOrigin.interfaceBlendDepth + DinterfaceBlendDepth[iVerticalSide] * dy_Vertical / Dy[iVerticalSide];
    //            float const newX_Oblique = verticalVertexOrigin.position.x + Dx_Oblique * dy_Oblique / Dy_Oblique;
    //            float const newDepth_Oblique = verticalVertexOrigin.depth + Ddepth_Oblique * dy_Oblique / Dy_Oblique;
    //            float const newInterfaceBlendDepth_Oblique = verticalVertexOrigin.interfaceBlendDepth + DinterfaceBlendDepth_Oblique * dy_Oblique / Dy_Oblique;

    //            // TODOHERE
    //        }

    //        // TODO: advance current
    //    }
    //    else
    //    {
    //        //
    //        // Slice rectangle until we reach the first (topmost) of current,
    //        // interpolating vertically along both sides
    //        //

    //        float const rectBottomY = std::max(leftBottom.position.y, rightBottom.position.y);
    //        while (currentY > rectBottomY)
    //        {
    //            // It's a flat-top rectangle
    //            assert(currentVertices[0].position.y == currentVertices[1].position.y);
    //            assert(currentVertices[0].position.y == currentY);

    //            // Calculate bottom y for this segment
    //            float newBottomY = rectBottomY; // Shoot for max first
    //            if (newBottomY < currentY - maxQuadTriangleHeight - minQuadTriangleHeight)
    //            {
    //                newBottomY = currentY - maxQuadTriangleHeight;
    //            }

    //            //
    //            // Interpolate Depth, InterfaceBlendDepth vertically, for both sides
    //            //

    //            float const dy_Left = newBottomY - leftTop.position.y; // Negative
    //            assert(dy_Left < 0.0f);
    //            // x = x[i] + Dx * dy / Dy
    //            float const newDepth_Left = leftTop.depth + Ddepth[0] * dy_Left / Dy[0];
    //            float const newInterfaceBlendDepth_Left = leftTop.interfaceBlendDepth + DinterfaceBlendDepth[0] * dy_Left / Dy[0];

    //            float const dy_Right = newBottomY - rightTop.position.y; // Negative
    //            assert(dy_Right < 0.0f);
    //            // x = x[i] + Dx * dy / Dy
    //            float const newDepth_Right = rightTop.depth + Ddepth[1] * dy_Right / Dy[1];
    //            float const newInterfaceBlendDepth_Right = rightTop.interfaceBlendDepth + DinterfaceBlendDepth[1] * dy_Right / Dy[1];

    //            //
    //            // Produce quad
    //            //
    //            //    A'--------B'
    //            //      |      |
    //            //      |      |
    //            //   A''--------B''
    //            //

    //            TLandVertex newCurrentVertex0{
    //                vec2f(leftTop.position.x, newBottomY),
    //                newDepth_Left,
    //                newInterfaceBlendDepth_Left };

    //            TLandVertex newCurrentVertex1{
    //                vec2f(rightTop.position.x, newBottomY),
    //                newDepth_Right,
    //                newInterfaceBlendDepth_Right };

    //            // A'-B'-B''
    //            vertexBuffer.emplace_back(currentVertices[0]);
    //            vertexBuffer.emplace_back(currentVertices[1]);
    //            vertexBuffer.emplace_back(newCurrentVertex1);
    //            // A'-B''-A''
    //            vertexBuffer.emplace_back(currentVertices[0]);
    //            vertexBuffer.emplace_back(newCurrentVertex1);
    //            vertexBuffer.emplace_back(newCurrentVertex0);

    //            //
    //            // Advance
    //            //

    //            currentVertices[0] = newCurrentVertex0;
    //            currentVertices[1] = newCurrentVertex1;
    //            currentY = newBottomY;
    //        }

    //        // Reset current's to reduce errors

    //        if (rectBottomY == leftBottom.position.y)
    //        {
    //            currentVertices[0] = leftBottom;
    //        }

    //        if (rectBottomY == rightBottom.position.y)
    //        {
    //            currentVertices[1] = rightBottom;
    //        }
    //    }
    //}
}

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleAboveOnly)
{
    //      B    150.0
    //     /|
    //    / |
    //   /  |
    //  AC--D    133.0

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
        // C
        TestLandVertex{ {0.0f, 150.0f - Dy}, 0.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 110.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 20.0f + rDd, 110.0f + rDibd },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 3 + 2 * 3);

    {
        // B-B'-A'

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
        // A'-B'-D

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

        // A'-D-AC

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

// TODO: TriangleOnly, other orientation

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

TEST(GeometryTests, GenerateSegmentedLandQuad_TriangleBelowOnly)
{
    //  AC--B    150.0
    //   \  |
    //    \ |
    //     \|
    //      D    133.0

    std::vector<TestLandVertex> vertexBuffer;

    float const oDx = 2.0f;
    float const Dy = 150.0f - 133.0f;
    //float const oDd = 20.0f - 0.0f;
    //float const oDibd = 110.0f - 100.0f;
    float const rDd = 60.0f;
    float const rDibd = 10.0f;

    GenerateSegmentedLandQuad(
        // A
        TestLandVertex{ {0.0f, 150.0f }, 0.0f, 100.0f },
        // C
        TestLandVertex{ {0.0f, 150.0f }, 0.0f, 100.0f },
        // B
        TestLandVertex{ {0.0f + oDx, 150.0f}, 20.0f, 110.0f },
        // D
        TestLandVertex{ {0.0f + oDx, 150.0f - Dy}, 20.0f + rDd, 110.0f + rDibd },
        10.0f,
        2.0f,
        vertexBuffer);

    ASSERT_EQ(vertexBuffer.size(), 2 * 3 + 3);

    {
        // AC-B-B'

        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.x, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].depth, 0.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[0].interfaceBlendDepth, 100.0f, 0.0000f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].position.y, 150.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].depth, 20.0f, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[1].interfaceBlendDepth, 110.0f, 0.0001f));

        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.x, 2.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].position.y, 140.0f, 0.0000f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].depth, 20.0f - rDd * 10.0f / Dy, 0.0001f));
        EXPECT_TRUE(ApproxEquals(vertexBuffer[2].interfaceBlendDepth, 110.0f - rDibd * 10.0f / Dy, 0.0001f));

        // AC-B'-A'
        // TODOHERE
    }
}

// TODO: TriangleOnly, other orientation

// TODO: full
// TODO: full - with no vertical intersections

// TODO: 3 pieces but each shorter

// TODO: exercise min
