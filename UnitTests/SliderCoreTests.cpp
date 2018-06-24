#include <UILib/LinearSliderCore.h>

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
        std::make_tuple(900.0f, 1000.0f, 60)
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
