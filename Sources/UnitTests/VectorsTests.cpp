#include <Core/Vectors.h>

#include <Core/SysSpecifics.h>

#include "TestingUtils.h"

#include <cmath>

#include "gtest/gtest.h"

TEST(VectorsTests, Sum_2f)
{
	vec2f a(1.0f, 5.0f);
	vec2f b(2.0f, 4.0f);
	vec2f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
	EXPECT_EQ(c.y, 9.0f);
}

TEST(VectorsTests, Sum_4f)
{
    vec4f a(1.0f, 5.0f, 20.0f, 100.4f);
    vec4f b(2.0f, 4.0f, 40.0f, 200.0f);
    vec4f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 9.0f);
    EXPECT_EQ(c.z, 60.0f);
    EXPECT_EQ(c.w, 300.4f);
}

TEST(VectorsTests, Sum_2i)
{
    vec2i a(1, 5);
    vec2i b(2, 4);
    vec2i c = a + b;

    EXPECT_EQ(c.x, 3);
    EXPECT_EQ(c.y, 9);
}

TEST(VectorsTests, Scale_2f)
{
    vec2f a(1.0f, 5.0f);
    vec2f b(2.0f, 4.0f);
    vec2f c = a.scale(b);

    EXPECT_EQ(c.x, 2.0f);
    EXPECT_EQ(c.y, 20.0f);
}

TEST(VectorsTests, Float_to_Integral_Round_2f)
{
    vec2f a(1.4f, 5.8f);
    vec2i const b = a.to_vec2i_round();

    EXPECT_EQ(b.x, 1);
    EXPECT_EQ(b.y, 6);
}

class Normalization2fTest : public testing::TestWithParam<std::tuple<vec2f, vec2f>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    VectorTests,
    Normalization2fTest,
    ::testing::Values(
        std::make_tuple(vec2f{ 1.0f, 0.0f }, vec2f{ 1.0f, 0.0f }),
        std::make_tuple(vec2f{ 1.0f, 1.0f }, vec2f{ 1.0f / std::sqrt(2.0f), 1.0f / std::sqrt(2.0f) }),
        std::make_tuple(vec2f{ 3.0f, 4.0f }, vec2f{ 3.0f / 5.0f, 4.0f / 5.0f}),
        std::make_tuple(vec2f{ 0.0f, 0.0f }, vec2f{ 0.0f, 0.0f }),
        std::make_tuple(vec2f{ 3000.0f, 4000.0f }, vec2f{ 3.0f / 5.0f, 4.0f / 5.0f })
    ));

TEST_P(Normalization2fTest, Normalization2fTest)
{
    vec2f input = std::get<0>(GetParam());
    vec2f expected = std::get<1>(GetParam());

    vec2f calcd = input.normalise();
    EXPECT_TRUE(ApproxEquals(calcd.x, expected.x, 0.0001f));
    EXPECT_TRUE(ApproxEquals(calcd.y, expected.y, 0.0001f));
}

TEST_P(Normalization2fTest, NormalizationWithLength2fTest)
{
    vec2f input = std::get<0>(GetParam());
    vec2f expected = std::get<1>(GetParam());

    float len = input.length();
    vec2f calcd = input.normalise(len);
    EXPECT_TRUE(ApproxEquals(calcd.x, expected.x, 0.0001f));
    EXPECT_TRUE(ApproxEquals(calcd.y, expected.y, 0.0001f));
}

TEST_P(Normalization2fTest, NormalizationApprox2fTest)
{
    vec2f input = std::get<0>(GetParam());
    vec2f expected = std::get<1>(GetParam());

    vec2f calcd = input.normalise_approx();
    EXPECT_TRUE(ApproxEquals(calcd.x, expected.x, 0.001f));
    EXPECT_TRUE(ApproxEquals(calcd.y, expected.y, 0.001f));
}

TEST_P(Normalization2fTest, NormalizationApproxWithLength2fTest)
{
    vec2f input = std::get<0>(GetParam());
    vec2f expected = std::get<1>(GetParam());

    float len = input.length();
    vec2f calcd = input.normalise_approx(len);
    EXPECT_TRUE(ApproxEquals(calcd.x, expected.x, 0.001f));
    EXPECT_TRUE(ApproxEquals(calcd.y, expected.y, 0.001f));
}