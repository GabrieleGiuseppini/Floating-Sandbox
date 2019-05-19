#include <GameCore/PrecalculatedFunction.h>

#include <cmath>

#include "gtest/gtest.h"

TEST(PrecalculatedFunctionTests, Nearest)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    EXPECT_NEAR(0.0f, pf.GetNearest(0.0f), 0.001f);
    EXPECT_NEAR(1.0f, pf.GetNearest(0.25f), 0.001f);
    EXPECT_NEAR(0.0f, pf.GetNearest(0.50f), 0.001f);
    EXPECT_NEAR(-1.0f, pf.GetNearest(0.75f), 0.001f);

    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.17f), pf.GetNearest(0.17f), 0.001f);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetNearest(0.67f), 0.001f);
}

TEST(PrecalculatedFunctionTests, LinearlyInterpolated)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolated(0.0f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolated(0.25f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolated(0.50f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolated(0.75f), 0.0001);

    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.17f), pf.GetLinearlyInterpolated(0.17f), 0.0001f);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetLinearlyInterpolated(0.67f), 0.0001f);
}

TEST(PrecalculatedFunctionTests, LinearlyInterpolatedPeriodic)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(0.0f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(1.0f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(2.0f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(100.0f), 0.0001);

    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(-1.0f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(-2.0f), 0.0001);
    EXPECT_NEAR(0.0f, pf.GetLinearlyInterpolatedPeriodic(-100.0f), 0.0001);



    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(0.25f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(1.25f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(2.25f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(100.25f), 0.0001);

    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(-0.25f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(-1.25f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(-2.25f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(-100.25f), 0.0001);



    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(0.75f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(1.75f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(2.75f), 0.0001);
    EXPECT_NEAR(-1.0f, pf.GetLinearlyInterpolatedPeriodic(100.75f), 0.0001);

    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(-0.75f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(-1.75f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(-2.75f), 0.0001);
    EXPECT_NEAR(1.0f, pf.GetLinearlyInterpolatedPeriodic(-100.75f), 0.0001);



    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.05f), pf.GetLinearlyInterpolatedPeriodic(0.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.05f), pf.GetLinearlyInterpolatedPeriodic(1.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.05f), pf.GetLinearlyInterpolatedPeriodic(2.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.05f), pf.GetLinearlyInterpolatedPeriodic(100.05f), 0.0001);

    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.05f), pf.GetLinearlyInterpolatedPeriodic(-0.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.05f), pf.GetLinearlyInterpolatedPeriodic(-1.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.05f), pf.GetLinearlyInterpolatedPeriodic(-2.05f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.05f), pf.GetLinearlyInterpolatedPeriodic(-100.05f), 0.0001);


    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetLinearlyInterpolatedPeriodic(0.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetLinearlyInterpolatedPeriodic(1.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetLinearlyInterpolatedPeriodic(2.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetLinearlyInterpolatedPeriodic(100.67f), 0.0001);

    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.67f), pf.GetLinearlyInterpolatedPeriodic(-0.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.67f), pf.GetLinearlyInterpolatedPeriodic(-1.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.67f), pf.GetLinearlyInterpolatedPeriodic(-2.67f), 0.0001);
    EXPECT_NEAR(sin(2.0f * Pi<float> * -0.67f), pf.GetLinearlyInterpolatedPeriodic(-100.67f), 0.0001);
}