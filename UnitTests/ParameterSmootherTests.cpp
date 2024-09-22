#include <Game/GameParameters.h>

#include <GameCore/ParameterSmoother.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(ParameterSmootherTests, CurrentValueIsTarget)
{
    float parameterValue = 0.0f;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue](float const & value)
        {
			parameterValue = value;
        },
        0.001f,
        0.0005f);

    smoother.SetValue(10.0f);

    EXPECT_FLOAT_EQ(10.0f, smoother.GetValue());
}

TEST(ParameterSmootherTests, SmoothsFromStartToTarget)
{
    float parameterValue = 0.0f;
	bool hasSetterBeenInvoked = false;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue, &hasSetterBeenInvoked](float const & value)
        {
			parameterValue = value;
			hasSetterBeenInvoked = true;
        },
        0.1f,
        0.0005f);

    smoother.SetValue(10.0f);

	EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update();
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 1.0f, 0.1f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 1.0f + 0.9f, 0.1f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 1.0f + 0.9f + 0.81f, 0.1f));
}

TEST(ParameterSmootherTests, SmoothsFromStartToTargetWithRateMultiplier)
{
    float parameterValue = 0.0f;
    bool hasSetterBeenInvoked = false;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue, &hasSetterBeenInvoked](float const & value)
        {
            parameterValue = value;
            hasSetterBeenInvoked = true;
        },
        0.1f,
        0.0005f);

    smoother.SetValue(10.0f);

    EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(2.0);
    EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 2.0f, 0.1f));

    smoother.Update(1);
    EXPECT_TRUE(ApproxEquals(parameterValue, 2.0f + 0.8f, 0.1f));

    smoother.Update(4.0);
    EXPECT_TRUE(ApproxEquals(parameterValue, 2.0f + 0.9f + 2.84f, 0.1f));
}

TEST(ParameterSmootherTests, TargetsClampedTarget)
{
    float parameterValue = 0.0f;
	bool hasSetterBeenInvoked = false;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue, &hasSetterBeenInvoked](float const & value) -> float const &
        {
			parameterValue = value;
			hasSetterBeenInvoked = true;
            return value;
        },
        [](float const & targetValue)
        {
            // Clamp to this
            return std::min(targetValue, 5.0f);
        },
        0.1f,
        0.0005f);

    smoother.SetValue(10.0f);

    // Real target is 5.0f
    EXPECT_TRUE(ApproxEquals(smoother.GetValue(), 5.0f, 0.1f));    

	EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update();
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 0.5f, 0.1f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 0.5f + 0.45f, 0.1f));
}

TEST(ParameterSmootherTests, SetValueImmediateTruncatesProgress)
{
    float parameterValue = 0.0f;
	bool hasSetterBeenInvoked = false;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue, &hasSetterBeenInvoked](float const & value)
        {
			parameterValue = value;
			hasSetterBeenInvoked = true;
        },
        0.1f,
        0.0005f);

    smoother.SetValue(10.0f);

    EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update();
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 1.0f, 0.1f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 1.0f + 0.9f, 0.1f));

    smoother.SetValueImmediate(95.0f);

    EXPECT_FLOAT_EQ(smoother.GetValue(), 95.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 95.0f, 0.1f));
}

TEST(ParameterSmootherTests, StopsAfterThreshold)
{
    float parameterValue = 0.0f;
    bool hasSetterBeenInvoked = false;

    ParameterSmoother<float> smoother(
        [&parameterValue]() -> float const &
        {
            return parameterValue;
        },
        [&parameterValue, &hasSetterBeenInvoked](float const & value)
        {
            parameterValue = value;
            hasSetterBeenInvoked = true;
        },
        0.5f,
        1.24f);

    smoother.SetValue(10.0f);

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.0001f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 7.5f, 0.0001f));

    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 8.75f, 0.0001f));
    
    smoother.Update();
    EXPECT_TRUE(ApproxEquals(parameterValue, 10.0f, 0.0f));
}