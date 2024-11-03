#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/FixedSetSliderCore.h>
#include <GameCore/FixedTickSliderCore.h>
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

INSTANTIATE_TEST_SUITE_P(
    LinearSliderCoreTests,
    LinearSliderCoreTest,
    ::testing::Values(
        std::make_tuple(0.0f, 0.5f, 60 + 1),
        std::make_tuple(0.001f, 0.5f, 60 + 1),
        std::make_tuple(0.0f, 1.0f, 60 + 1),
        std::make_tuple(0.001f, 1.0f, 60 + 1),
        std::make_tuple(0.001f, 2.4f, 60 + 1),
        std::make_tuple(0.0f, 5.0f, 60 + 1),
        std::make_tuple(0.0f, 20.0f, 60 + 1),
        std::make_tuple(0.1f, 20.0f, 60 + 1),
        std::make_tuple(0.0f, 1000.0f, 60 + 1),
        std::make_tuple(0.0001f, 1000.0f, 60 + 1),
        std::make_tuple(900.0f, 1000.0f, 60 + 1),
        std::make_tuple(20.0f, 500.0f, 60 + 1)
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
    EXPECT_LT(core.TickToValue(core.GetNumberOfTicks() - 2), maxValue);
    EXPECT_EQ(core.TickToValue(core.GetNumberOfTicks() - 1), maxValue);
    EXPECT_EQ(core.TickToValue(core.GetNumberOfTicks()), maxValue);
}

TEST_F(LinearSliderCoreTest, AlmostZeroToTen)
{
    LinearSliderCore core(0.1f, 10.0f); // Step=0.125

    EXPECT_EQ(core.GetNumberOfTicks(), 80 + 1);

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

    EXPECT_EQ(core.TickToValue(81), 10.0f);
}

TEST_F(LinearSliderCoreTest, NegativeMin)
{
    LinearSliderCore core(-10.0f, 10.0f); // Step=0.25

    EXPECT_EQ(core.GetNumberOfTicks(), 80 + 1);

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

    EXPECT_EQ(core.TickToValue(81), 10.0f);
}

TEST_F(LinearSliderCoreTest, TwentyToFiveHundred)
{
    LinearSliderCore core(20.0f, 500.0f); // Step=8

    EXPECT_EQ(core.GetNumberOfTicks(), 60 + 1);

    EXPECT_EQ(core.TickToValue(0), 20.0f);
    EXPECT_EQ(core.ValueToTick(20.f), 0);

    EXPECT_EQ(core.TickToValue(1), 24.0f);
    EXPECT_EQ(core.ValueToTick(24.f), 1);

    EXPECT_EQ(core.TickToValue(59), 488.f);
    EXPECT_EQ(core.ValueToTick(488.f), 59);

    EXPECT_EQ(core.TickToValue(60), 500.0f);
    EXPECT_EQ(core.ValueToTick(500.0f), 60);

    EXPECT_EQ(core.TickToValue(61), 500.0f);
}

TEST_F(LinearSliderCoreTest, EmptyRange)
{
    LinearSliderCore core(10.0f, 10.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 1);

    EXPECT_EQ(core.TickToValue(0), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.0f), 0);
}

TEST(IntegralLinearSliderCoreTest, MoreDeltaThanTicks)
{
    IntegralLinearSliderCore<size_t> core(100, 1000); // TickSize = 16

    EXPECT_EQ(core.GetNumberOfTicks(), 57 + 1);

    EXPECT_EQ(core.TickToValue(0), 100u);
    EXPECT_EQ(core.ValueToTick(100), 0);

    EXPECT_EQ(core.TickToValue(1), 112u); // 96 + 16
    EXPECT_EQ(core.ValueToTick(111), 0);
    EXPECT_EQ(core.ValueToTick(112), 1);

    EXPECT_EQ(core.TickToValue(2), 128u); // 96 + 32
    EXPECT_EQ(core.ValueToTick(127), 1);
    EXPECT_EQ(core.ValueToTick(128), 2);

    EXPECT_EQ(core.TickToValue(57), 1000u);
    EXPECT_EQ(core.ValueToTick(999), 56);
    EXPECT_EQ(core.ValueToTick(1000), 57);

    EXPECT_EQ(core.TickToValue(58), 1000u);
}

TEST(IntegralLinearSliderCoreTest, MoreTicksThanDelta)
{
    IntegralLinearSliderCore<size_t> core(100, 110); // TickSize = 1

    EXPECT_EQ(core.GetNumberOfTicks(), 11);

    EXPECT_EQ(core.TickToValue(0), 100u);
    EXPECT_EQ(core.ValueToTick(100), 0);

    EXPECT_EQ(core.TickToValue(1), 101u);
    EXPECT_EQ(core.ValueToTick(101), 1);

    EXPECT_EQ(core.TickToValue(9), 109u);
    EXPECT_EQ(core.ValueToTick(109), 9);

    EXPECT_EQ(core.TickToValue(10), 110u);
    EXPECT_EQ(core.ValueToTick(110), 10);

    EXPECT_EQ(core.TickToValue(11), 110u);
}

TEST(IntegralLinearSliderCoreTest, EmptyRange)
{
    IntegralLinearSliderCore<size_t> core(10, 10);

    EXPECT_EQ(core.GetNumberOfTicks(), 1);

    EXPECT_EQ(core.TickToValue(0), 10);
    EXPECT_EQ(core.ValueToTick(10), 0);
}

TEST(ExponentialSliderCoreTest, PositiveEdges)
{
    ExponentialSliderCore core(0.01f, 1.0f, 1000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 99);

    EXPECT_TRUE(ApproxEquals(core.TickToValue(0), 0.01f, 0.001f));
    EXPECT_EQ(core.ValueToTick(0.01f), 0);

    EXPECT_EQ(core.TickToValue((99 - 1) / 2), 1.0f);
    EXPECT_EQ(core.ValueToTick(1.0f), (99 - 1) / 2);

    EXPECT_EQ(core.TickToValue(99 - 1), 1000.0f);
    EXPECT_EQ(core.ValueToTick(1000.0f), (99 - 1));
}

TEST(ExponentialSliderCoreTest, NegativeEdges)
{
    ExponentialSliderCore core(-50.0f, 1.0f, 100000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 99);

    EXPECT_EQ(core.TickToValue(0), -50.0f);
    EXPECT_EQ(core.ValueToTick(-50.0f), 0);

    EXPECT_EQ(core.TickToValue((99 - 1) / 2), 1.0f);
    EXPECT_EQ(core.ValueToTick(1.0f), (99 - 1) / 2);

    EXPECT_EQ(core.TickToValue(99 - 1), 100000.0f);
    EXPECT_EQ(core.ValueToTick(100000.0f), 99 - 1);
}

TEST(ExponentialSliderCoreTest, NegativeEdges_ArbitraryMidpoint)
{
    ExponentialSliderCore core(-50.0f, 300.0f, 100000.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 99);

    EXPECT_EQ(core.TickToValue(0), -50.0f);
    EXPECT_EQ(core.ValueToTick(-50.0f), 0);

    EXPECT_EQ(core.TickToValue((99 - 1) / 2), 300.0f);
    EXPECT_EQ(core.ValueToTick(300.0f), (99 - 1) / 2);

    EXPECT_EQ(core.TickToValue(99 - 1), 100000.0f);
    EXPECT_EQ(core.ValueToTick(100000.0f), 99 - 1);
}

TEST(FixedTickSliderCoreTest, FractionalTickSize)
{
    FixedTickSliderCore core(0.5f, 10.0f, 20.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 21);

    EXPECT_EQ(core.TickToValue(0), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.0f), 0);

    EXPECT_EQ(core.TickToValue(1), 10.5f);
    EXPECT_EQ(core.ValueToTick(10.5f), 1);

    EXPECT_EQ(core.TickToValue(2), 11.0f);
    EXPECT_EQ(core.ValueToTick(11.0f), 2);

    EXPECT_EQ(core.TickToValue(9), 14.5f);
    EXPECT_EQ(core.ValueToTick(14.5f), 9);

    EXPECT_EQ(core.TickToValue(10), 15.0f);
    EXPECT_EQ(core.ValueToTick(15.0f), 10);

    EXPECT_EQ(core.TickToValue(11), 15.5f);
    EXPECT_EQ(core.ValueToTick(15.5f), 11);

    EXPECT_EQ(core.TickToValue(19), 19.5f);
    EXPECT_EQ(core.ValueToTick(19.5f), 19);

    EXPECT_EQ(core.TickToValue(20), 20.0f);
    EXPECT_EQ(core.ValueToTick(20.0f), 20);

    EXPECT_EQ(core.TickToValue(21), 20.0f);
}

TEST(FixedTickSliderCoreTest, IntegralTickSize)
{
    FixedTickSliderCore core(2.0f, 10.0f, 20.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 6);

    EXPECT_EQ(core.TickToValue(0), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.0f), 0);

    EXPECT_EQ(core.TickToValue(1), 12.0f);
    EXPECT_EQ(core.ValueToTick(12.0f), 1);

    EXPECT_EQ(core.TickToValue(4), 18.0f);
    EXPECT_EQ(core.ValueToTick(18.0f), 4);

    EXPECT_EQ(core.TickToValue(5), 20.0f);
    EXPECT_EQ(core.ValueToTick(20.0f), 5);

    EXPECT_EQ(core.TickToValue(6), 20.0f);
}

TEST(FixedTickSliderCoreTest, EmptyRange)
{
    FixedTickSliderCore core(2.0f, 10.0f, 10.0f);

    EXPECT_EQ(core.GetNumberOfTicks(), 1);

    EXPECT_EQ(core.TickToValue(0), 10.0f);
    EXPECT_EQ(core.ValueToTick(10.0f), 0);
}

TEST(FixedSetSliderCoreTest, FixedSet_Integral)
{
    FixedSetSliderCore<int> core({1, 40, 90, 117});

    EXPECT_EQ(core.GetNumberOfTicks(), 4);

    EXPECT_EQ(core.TickToValue(0), 1);
    EXPECT_EQ(core.TickToValue(1), 40);
    EXPECT_EQ(core.TickToValue(2), 90);
    EXPECT_EQ(core.TickToValue(3), 117);

    EXPECT_EQ(core.ValueToTick(-100), 0);
    EXPECT_EQ(core.ValueToTick(0), 0);
    EXPECT_EQ(core.ValueToTick(1), 0);
    EXPECT_EQ(core.ValueToTick(2), 0);
    EXPECT_EQ(core.ValueToTick(19), 0);
    EXPECT_EQ(core.ValueToTick(20), 0);
    EXPECT_EQ(core.ValueToTick(21), 1);
    EXPECT_EQ(core.ValueToTick(22), 1);
    EXPECT_EQ(core.ValueToTick(39), 1);
    EXPECT_EQ(core.ValueToTick(40), 1);
    EXPECT_EQ(core.ValueToTick(41), 1);
    EXPECT_EQ(core.ValueToTick(89), 2);
    EXPECT_EQ(core.ValueToTick(90), 2);
    EXPECT_EQ(core.ValueToTick(91), 2);
    EXPECT_EQ(core.ValueToTick(100), 2);
    EXPECT_EQ(core.ValueToTick(104), 3);
    EXPECT_EQ(core.ValueToTick(117), 3);
    EXPECT_EQ(core.ValueToTick(118), 3);
    EXPECT_EQ(core.ValueToTick(1000), 3);

    EXPECT_EQ(core.GetMinValue(), 1);
    EXPECT_EQ(core.GetMaxValue(), 117);
}

TEST(FixedSetSliderCoreTest, FixedSet_Float)
{
    FixedSetSliderCore<float> core({ 1.0f, 40.0f, 90.0f, 117.0f });

    EXPECT_EQ(core.GetNumberOfTicks(), 4);

    EXPECT_EQ(core.TickToValue(0), 1.0f);
    EXPECT_EQ(core.TickToValue(1), 40.0f);
    EXPECT_EQ(core.TickToValue(2), 90.0f);
    EXPECT_EQ(core.TickToValue(3), 117.0f);

    EXPECT_EQ(core.ValueToTick(-100.0f), 0);
    EXPECT_EQ(core.ValueToTick(0.0f), 0);
    EXPECT_EQ(core.ValueToTick(1.0f), 0);
    EXPECT_EQ(core.ValueToTick(2.0f), 0);
    EXPECT_EQ(core.ValueToTick(19.0f), 0);
    EXPECT_EQ(core.ValueToTick(20.0f), 0);
    EXPECT_EQ(core.ValueToTick(21.0f), 1);
    EXPECT_EQ(core.ValueToTick(22.0f), 1);
    EXPECT_EQ(core.ValueToTick(39.0f), 1);
    EXPECT_EQ(core.ValueToTick(40.0f), 1);
    EXPECT_EQ(core.ValueToTick(41.0f), 1);
    EXPECT_EQ(core.ValueToTick(89.0f), 2);
    EXPECT_EQ(core.ValueToTick(90.0f), 2);
    EXPECT_EQ(core.ValueToTick(91.0f), 2);
    EXPECT_EQ(core.ValueToTick(100.0f), 2);
    EXPECT_EQ(core.ValueToTick(104.0f), 3);
    EXPECT_EQ(core.ValueToTick(117.0f), 3);
    EXPECT_EQ(core.ValueToTick(118.0f), 3);
    EXPECT_EQ(core.ValueToTick(1000.0f), 3);

    EXPECT_EQ(core.GetMinValue(), 1.0f);
    EXPECT_EQ(core.GetMaxValue(), 117.0f);
}

TEST(FixedSetSliderCoreTest, FixedSet_FromPowersOfTwo)
{
    auto const core = FixedSetSliderCore<int>::FromPowersOfTwo(8, 128);

    EXPECT_EQ(core->GetNumberOfTicks(), 5);

    EXPECT_EQ(core->TickToValue(0), 8);
    EXPECT_EQ(core->TickToValue(1), 16);
    EXPECT_EQ(core->TickToValue(2), 32);
    EXPECT_EQ(core->TickToValue(3), 64);
    EXPECT_EQ(core->TickToValue(4), 128);

    EXPECT_EQ(core->ValueToTick(-100), 0);
    EXPECT_EQ(core->ValueToTick(0), 0);
    EXPECT_EQ(core->ValueToTick(7), 0);
    EXPECT_EQ(core->ValueToTick(8), 0);
    EXPECT_EQ(core->ValueToTick(9), 0);
    EXPECT_EQ(core->ValueToTick(12), 1);
    EXPECT_EQ(core->ValueToTick(13), 1);
    EXPECT_EQ(core->ValueToTick(15), 1);
    EXPECT_EQ(core->ValueToTick(16), 1);
    EXPECT_EQ(core->ValueToTick(17), 1);
    EXPECT_EQ(core->ValueToTick(31), 2);
    EXPECT_EQ(core->ValueToTick(32), 2);
    EXPECT_EQ(core->ValueToTick(33), 2);
    EXPECT_EQ(core->ValueToTick(63), 3);
    EXPECT_EQ(core->ValueToTick(64), 3);
    EXPECT_EQ(core->ValueToTick(65), 3);
    EXPECT_EQ(core->ValueToTick(127), 4);
    EXPECT_EQ(core->ValueToTick(128), 4);
    EXPECT_EQ(core->ValueToTick(129), 4);

    EXPECT_EQ(core->GetMinValue(), 8);
    EXPECT_EQ(core->GetMaxValue(), 128);
}