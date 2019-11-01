#include <Game/GameParameters.h>
#include <Game/OceanFloorTerrain.h>

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
        std::chrono::milliseconds(1000));

    smoother.SetValue(10.0f, 3600.0f);

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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

	EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(startTimestamp + 0.001f);
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 0.01f, 0.1f));

    smoother.Update(startTimestamp + 0.5f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    smoother.Update(startTimestamp + 0.999f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 9.99f, 0.1f));

    smoother.Update(startTimestamp + 1.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 10.0f, 0.1f));
}

TEST(ParameterSmootherTests, SmoothsFromStartToTarget_OceanFloorTerrain)
{
	OceanFloorTerrain parameterValue;
	bool hasSetterBeenInvoked = false;

	OceanFloorTerrain targetValue;
	targetValue[0] = 10.0f;
	targetValue[1] = 0.0f;
	targetValue[2] = 100.0f;
	targetValue[3] = 1000.0f;

	ParameterSmoother<OceanFloorTerrain> smoother(
		[&parameterValue]() -> OceanFloorTerrain const &
		{
			return parameterValue;
		},
		[&parameterValue, &hasSetterBeenInvoked](OceanFloorTerrain const & value)
		{
			parameterValue = value;
			hasSetterBeenInvoked = true;
		},
		std::chrono::milliseconds(1000));

	auto startTimestamp = 3600.0f;
	smoother.SetValue(targetValue, startTimestamp);

	EXPECT_FALSE(hasSetterBeenInvoked);

	smoother.Update(startTimestamp + 0.5f);
	EXPECT_TRUE(hasSetterBeenInvoked);
	EXPECT_TRUE(ApproxEquals(parameterValue[0], 5.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[1], 0.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[2], 50.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[3], 500.0f, 0.1f));

	smoother.Update(startTimestamp + 0.999f);
	EXPECT_TRUE(ApproxEquals(parameterValue[0], 9.99f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[1], 0.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[2], 99.99f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[3], 999.99f, 0.1f));

	smoother.Update(startTimestamp + 1.0f);
	EXPECT_TRUE(ApproxEquals(parameterValue[0], 10.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[1], 0.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[2], 100.0f, 0.1f));
	EXPECT_TRUE(ApproxEquals(parameterValue[3], 1000.0f, 0.1f));
}

TEST(ParameterSmootherTests, SetValueDuringSmoothing_MaintainsValue)
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

    // Now we are at 0.5
    smoother.Update(startTimestamp + 0.5f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    // Set new target
    auto startTimestamp2 = startTimestamp + 0.5001f;
    smoother.SetValue(100.0f, startTimestamp2);
    EXPECT_FLOAT_EQ(smoother.GetValue(), 100.0f);

    // Value has remained more or less the same
    smoother.Update(startTimestamp2 + 0.0002f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.01f, 0.1f));
}

TEST(ParameterSmootherTests, SetValueDuringSmoothing_ExtendsTime)
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

    // Now we are at 0.5
    smoother.Update(startTimestamp + 0.5f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    // Set new target
    auto startTimestamp2 = startTimestamp + 0.5001f;
    smoother.SetValue(100.0f, startTimestamp2);
    EXPECT_FLOAT_EQ(smoother.GetValue(), 100.0f);

    // Jump close to end
    smoother.Update(startTimestamp2 + 0.999f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 100.0f, 0.1f));

    // Jump to end
    smoother.Update(startTimestamp2 + 1.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 100.0f, 0.0001f));
}

TEST(ParameterSmootherTests, SetValueDuringSmoothing_RemainsStable)
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

    // Now we are at 0.5
    smoother.Update(startTimestamp + 0.5f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    // Set new target, close to the end
    auto startTimestamp2 = startTimestamp + 0.5001f;
    smoother.SetValue(100.0f, startTimestamp2);
    EXPECT_FLOAT_EQ(smoother.GetValue(), 100.0f);

    // Jump to new value, being very close to end
    smoother.Update(startTimestamp2 + 0.999f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 99.9f, 0.1f));
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

    // Real target is 5.0f
    EXPECT_TRUE(ApproxEquals(smoother.GetValue(), 5.0f, 0.1f));    

	EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(startTimestamp + 0.5f);
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 2.5f, 0.5f));

    smoother.Update(startTimestamp + 1.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));
}

TEST(ParameterSmootherTests, NeverOvershoots_Positive)
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

	EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(startTimestamp + 0.5f);
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    smoother.Update(startTimestamp + 2.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 10.0f, 0.1f));
}

TEST(ParameterSmootherTests, NeverOvershoots_Negative)
{
    float parameterValue = 10.0f;
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(0.0f, startTimestamp);

    EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(startTimestamp + 0.5f);
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    smoother.Update(startTimestamp + 2.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 0.0f, 0.1f));
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
        std::chrono::milliseconds(1000));

    auto startTimestamp = 3600.0f;
    smoother.SetValue(10.0f, startTimestamp);

    EXPECT_FALSE(hasSetterBeenInvoked);

    smoother.Update(startTimestamp + 0.001f);
	EXPECT_TRUE(hasSetterBeenInvoked);
    EXPECT_TRUE(ApproxEquals(parameterValue, 0.01f, 0.1f));

    smoother.Update(startTimestamp + 0.5f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 5.0f, 0.1f));

    smoother.SetValueImmediate(95.0f);

    EXPECT_FLOAT_EQ(smoother.GetValue(), 95.0f);
    EXPECT_TRUE(ApproxEquals(parameterValue, 95.0f, 0.1f));
}
