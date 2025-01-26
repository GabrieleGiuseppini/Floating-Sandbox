#include <Core/PrecalculatedFunction.h>

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

TEST(PrecalculatedFunctionTests, NearestPeriodic)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    for (float offset = -3.0f; offset < 4.0f; offset += 1.0f)
    {
        EXPECT_NEAR(0.0f, pf.GetNearestPeriodic(0.0f + offset), 0.001f);
        EXPECT_NEAR(-1.0f, pf.GetNearestPeriodic(-0.25f + offset), 0.001f);
        EXPECT_NEAR(1.0f, pf.GetNearestPeriodic(0.25f + offset), 0.001f);
        EXPECT_NEAR(0.0f, pf.GetNearestPeriodic(-0.50f + offset), 0.001f);
        EXPECT_NEAR(0.0f, pf.GetNearestPeriodic(0.50f + offset), 0.001f);
        EXPECT_NEAR(1.0f, pf.GetNearestPeriodic(-0.75f + offset), 0.001f);
        EXPECT_NEAR(-1.0f, pf.GetNearestPeriodic(0.75f + offset), 0.001f);

        EXPECT_NEAR(sin(2.0f * Pi<float> * 0.17f), pf.GetNearestPeriodic(0.17f + offset), 0.001f);
        EXPECT_NEAR(sin(2.0f * Pi<float> * 0.67f), pf.GetNearestPeriodic(0.67f + offset), 0.001f);
    }
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