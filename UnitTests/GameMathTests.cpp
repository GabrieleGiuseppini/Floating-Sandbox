#include <GameLib/GameMath.h>

#include "gtest/gtest.h"

TEST(CeilPowerOfTwo, Basic)
{
    EXPECT_EQ(CeilPowerOfTwo(0), 1);
    EXPECT_EQ(CeilPowerOfTwo(1), 1);
    EXPECT_EQ(CeilPowerOfTwo(2), 2);
    EXPECT_EQ(CeilPowerOfTwo(3), 4);
    EXPECT_EQ(CeilPowerOfTwo(4), 4);
    EXPECT_EQ(CeilPowerOfTwo(5), 8);
    EXPECT_EQ(CeilPowerOfTwo(6), 8);
    EXPECT_EQ(CeilPowerOfTwo(7), 8);
    EXPECT_EQ(CeilPowerOfTwo(8), 8);
    EXPECT_EQ(CeilPowerOfTwo(9), 16);
}



::testing::AssertionResult ApproxEquals(float a, float b, float tolerance)
{
    if (abs(a - b) < tolerance)
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure() << "Result " << a << " too different than expected value " << b;
    }
}

TEST(FastPowTest, Basic)
{
    float result = FastPow(0.1f, 2.0f);
    float expectedResult = 0.01f;

    EXPECT_TRUE(ApproxEquals(result, expectedResult, 0.0001f));
}

class FastPowTest : public testing::TestWithParam<std::tuple<float, float, float>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    TestCases,
    FastPowTest,
    ::testing::Values(
        std::make_tuple(0.0f, 0.0f, 0.001f),
        std::make_tuple(0.0f, 0.1f, 0.001f),
        std::make_tuple(0.0f, 0.5f, 0.001f),
        std::make_tuple(0.0f, 1.0f, 0.001f),
        std::make_tuple(0.0f, 2.0f, 0.001f),

        std::make_tuple(1.0f, 0.0f, 0.001f),
        std::make_tuple(1.0f, 0.1f, 0.001f),
        std::make_tuple(1.0f, 0.5f, 0.001f),
        std::make_tuple(1.0f, 1.0f, 0.001f),
        std::make_tuple(1.0f, 2.0f, 0.001f),

        std::make_tuple(1.5f, 0.0f, 0.001f),
        std::make_tuple(1.5f, 0.1f, 0.001f),
        std::make_tuple(1.5f, 0.5f, 0.001f),
        std::make_tuple(1.5f, 1.0f, 0.001f),
        std::make_tuple(1.5f, 2.0f, 0.001f),
        std::make_tuple(1.5f, 2.1f, 0.001f),
        std::make_tuple(1.5f, 4.1f, 0.001f),

        std::make_tuple(2.0f, 0.0f, 0.001f),
        std::make_tuple(2.0f, 0.1f, 0.001f),
        std::make_tuple(2.0f, 0.5f, 0.001f),
        std::make_tuple(2.0f, 1.0f, 0.001f),
        std::make_tuple(2.0f, 2.0f, 0.001f),

        std::make_tuple(10.0f, 0.0f, 0.001f),
        std::make_tuple(10.0f, 0.001f, 0.001f),
        std::make_tuple(10.0f, 0.1f, 0.001f),
        std::make_tuple(10.0f, 0.99f, 0.001f),
        std::make_tuple(10.0f, 1.0f, 0.001f),
        std::make_tuple(10.0f, 1.001f, 0.001f),
        std::make_tuple(10.0f, 1.1f, 0.001f),
        std::make_tuple(10.0f, 1.5f, 0.1f),
        std::make_tuple(10.0f, 1.99f, 0.1f),
        std::make_tuple(10.0f, 2.0f, 1.0f),
        std::make_tuple(10.0f, 3.0f, 1.0f)
    ));

TEST_P(FastPowTest, FastPowTest)
{
    float result = FastPow(std::get<0>(GetParam()), std::get<1>(GetParam()));
    float expectedResult = pow(std::get<0>(GetParam()), std::get<1>(GetParam()));

    EXPECT_TRUE(ApproxEquals(result, expectedResult, std::get<2>(GetParam())));
}

class FastExpTest : public testing::TestWithParam<std::tuple<float, float>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    TestCases,
    FastExpTest,
    ::testing::Values(
        std::make_tuple(-5.0f, 0.001f),
        std::make_tuple(-4.0f, 0.001f),
        std::make_tuple(-1.001f, 0.001f),
        std::make_tuple(-1.0f, 0.001f),
        std::make_tuple(-0.9f, 0.001f),
        std::make_tuple(-0.1f, 0.001f),
        std::make_tuple(-0.001f, 0.001f),
        std::make_tuple(0.0f, 0.001f),
        std::make_tuple(0.001f, 0.001f),
        std::make_tuple(0.1f, 0.001f),
        std::make_tuple(0.9f, 0.001f),
        std::make_tuple(1.0f, 0.001f),
        std::make_tuple(1.001f, 0.001f),
        std::make_tuple(1.1f, 0.001f),
        std::make_tuple(4.0f, 0.001f),
        std::make_tuple(5.0f, 0.001f)
    ));

TEST_P(FastExpTest, FastExpTest)
{
    float result = FastExp(std::get<0>(GetParam()));
    float expectedResult = exp(std::get<0>(GetParam()));

    EXPECT_TRUE(ApproxEquals(result, expectedResult, std::get<1>(GetParam())));
}