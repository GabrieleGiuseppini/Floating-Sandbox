#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/IntegralLinearSliderCore.h>
#include <GameCore/LinearSliderCore.h>

#include "Utils.h"

#include "gtest/gtest.h"

class LinearSliderCoreTest : public testing::TestWithParam<std::tuple<float, float, int>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    TestCases,
    LinearSliderCoreTest,
    ::testing::Values(
        std::make_tuple(0.0f, 0.5f, 60),
        std::make_tuple(0.001f, 0.5f, 60),
        std::make_tuple(0.0f, 1.0f, 60),
        std::make_tuple(0.001f, 1.0f, 60),
        std::make_tuple(0.001f, 2.4f, 60),
        std::make_tuple(0.0f, 5.0f, 60),
        std::make_tuple(0.0f, 20.0f, 60),
        std::make_tuple(0.1f, 20.0f, 60),
        std::make_tuple(0.0f, 1000.0f, 60),
        std::make_tuple(0.0001f, 1000.0f, 60),
        std::make_tuple(900.0f, 1000.0f, 60),
        std::make_tuple(20.0f, 500.0f, 60)
    ));

TEST_P(LinearSliderCoreTest, LinearTestCases)
{
    float const minValue = std::get<0>(GetParam());
    float const maxValue = std::get<1>(GetParam());
    int leastNumberOfTicks = std::get<2>(GetParam());

    LinearSliderCore core(minValue, maxValue);

    EXPECT_GE(core.GetNumberOfTicks(), leastNumberOfTicks);

    EXPECT_EQ(core.TickToValue(0), minValue);
    EXPECT_GT(core.TickToValue(1), minValue);
    EXPECT_LT(core.TickToValue(core.GetNumberOfTicks() - 1), maxValue);
    EXPECT_EQ(core.TickToValue(core.GetNumberOfTicks()), maxValue);
}

TEST_F(LinearSliderCoreTest, AlmostZeroToTen)
{
    LinearSliderCore core(0.1f, 10.0f); // Step=0.125

    EXPECT_EQ(core.GetNumberOfTicks(), 80);

    EXPECT_EQ(core.TickToValue(0), 0.1f);
    EXPECT_EQ(core.ValueToTick(0.1f), 0);

    EXPECT_EQ(core.TickToValue(1), 0.125f);
    EXPECT_EQ(core.ValueToTick(0.125f), 1);

    EXPECT_EQ(core.TickToValue(2), 0.25f);
    EXPECT_EQ(core.ValueToTick(0.25f), 2);

    EXPECT_EQ(core.TickToValue(4), 0.5f);
    EXPECT_EQ(core.ValueToTick(0.5f), 4);

    EXPECT_EQ(core.TickToValue(79), 9.875f);
    EXPECT_EQ(core.ValueToTick(9.875f), 79);

    EXPECT_EQ(core.TickToValue(80), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.f), 80);
}

TEST_F(LinearSliderCoreTest, NegativeMin)
{
    LinearSliderCore core(-10.0f, 10.0f); // Step=0.25

    EXPECT_EQ(core.GetNumberOfTicks(), 80);

    EXPECT_EQ(core.TickToValue(0), -10.0f);
    EXPECT_EQ(core.ValueToTick(-10.0f), 0);

    EXPECT_EQ(core.TickToValue(1), -9.75f);
    EXPECT_EQ(core.ValueToTick(-9.75f), 1);

    EXPECT_EQ(core.TickToValue(2), -9.5f);
    EXPECT_EQ(core.ValueToTick(-9.5f), 2);

    EXPECT_EQ(core.TickToValue(4), -9.0f);
    EXPECT_EQ(core.ValueToTick(-9.0f), 4);

    EXPECT_EQ(core.TickToValue(79), 9.75f);
    EXPECT_EQ(core.ValueToTick(9.75f), 79);

    EXPECT_EQ(core.TickToValue(80), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.f), 80);
}

TEST_F(LinearSliderCoreTest, TwentyToFiveHundred)
{
    LinearSliderCore core(20.0f, 500.0f); // Step=8

    EXPECT_EQ(core.GetNumberOfTicks(), 60);

    EXPECT_EQ(core.TickToValue(0), 20.0f);
    EXPECT_EQ(core.ValueToTick(20.f), 0);

    EXPECT_EQ(core.TickToValue(1), 24.0f);
    EXPECT_EQ(core.ValueToTick(24.f), 1);

    EXPECT_EQ(core.TickToValue(59), 488.f);
    EXPECT_EQ(core.ValueToTick(488.f), 59);

    EXPECT_EQ(core.TickToValue(60), 500.0f);
    EXPECT_EQ(core.ValueToTick(500.0f), 60);
}

TEST(IntegralLinearSliderCoreTest, ZeroToTen)
{
    IntegralLinearSliderCore<size_t> core(100, 1000); // TickSize = 16

    EXPECT_EQ(core.GetNumberOfTicks(), 57);

    EXPECT_EQ(core.TickToValue(0), 100);
    EXPECT_EQ(core.ValueToTick(100), 0);

    EXPECT_EQ(core.TickToValue(1), 112); // 96 + 16
    EXPECT_EQ(core.ValueToTick(111), 0);
    EXPECT_EQ(core.ValueToTick(112), 1);

    EXPECT_EQ(core.TickToValue(2), 128); // 96 + 32
    EXPECT_EQ(core.ValueToTick(127), 1);
    EXPECT_EQ(core.ValueToTick(128), 2);

    EXPECT_EQ(core.TickToValue(57), 1000);
    EXPECT_EQ(core.ValueToTick(999), 56);
    EXPECT_EQ(core.ValueToTick(1000), 57);
}

TEST(ExponentialSliderCoreTest, PositiveEdges)
{
    ExponentialSliderCore core(0.01f, 1.0f, 1000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 100);

    EXPECT_TRUE(ApproxEquals(core.TickToValue(0), 0.01f, 0.001f));
    EXPECT_EQ(core.ValueToTick(0.01f), 0);

    EXPECT_EQ(core.TickToValue(50), 1.0f);
    EXPECT_EQ(core.ValueToTick(1.0f), 50);

    EXPECT_EQ(core.TickToValue(100), 1000.0f);
    EXPECT_EQ(core.ValueToTick(1000.0f), 100);
}

TEST(ExponentialSliderCoreTest, NegativeEdges)
{
    ExponentialSliderCore core(-50.0f, 1.0f, 100000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 100);

    EXPECT_EQ(core.TickToValue(0), -50.0f);
    EXPECT_EQ(core.ValueToTick(-50.0f), 0);

    EXPECT_EQ(core.TickToValue(50), 1.0f);
    EXPECT_EQ(core.ValueToTick(1.0f), 50);

    EXPECT_EQ(core.TickToValue(100), 100000.0f);
    EXPECT_EQ(core.ValueToTick(100000.0f), 100);
}

TEST(ExponentialSliderCoreTest, NegativeEdges_ArbitraryMidpoint)
{
    ExponentialSliderCore core(-50.0f, 300.0f, 100000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 100);

    EXPECT_EQ(core.TickToValue(0), -50.0f);
    EXPECT_EQ(core.ValueToTick(-50.0f), 0);

    EXPECT_EQ(core.TickToValue(50), 300.0f);
    EXPECT_EQ(core.ValueToTick(300.0f), 50);

    EXPECT_EQ(core.TickToValue(100), 100000.0f);
    EXPECT_EQ(core.ValueToTick(100000.0f), 100);
}