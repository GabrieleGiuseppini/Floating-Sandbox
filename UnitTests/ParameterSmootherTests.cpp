#include <GameCore/ParameterSmoother.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(ParameterSmootherTests, CurrentValueIsTarget)
{
    float valueBeingSet = 0.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 5.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
        std::chrono::milliseconds(1000));

    smoother.SetValue(10.0f, GameWallClock::GetInstance().Now());

    EXPECT_FLOAT_EQ(10.0f, smoother.GetValue());
}

TEST(ParameterSmootherTests, SmoothsFromStartToTarget)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 0.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
        std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(10.0f, startTimestamp);

    EXPECT_FLOAT_EQ(valueBeingSet, 0.0f);

    smoother.Update(startTimestamp + std::chrono::milliseconds(1));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 0.01f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 5.0f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(999));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 9.99f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(1000));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 10.0f, 0.1f));
}

TEST(ParameterSmootherTests, RestartsFromPreviousTargetValueOnInterrupted)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 0.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
        std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(10.0f, startTimestamp);

    // Now we are at 5
    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 5.0f, 0.1f));

    // Set new target
    auto startTimestamp2 = startTimestamp + std::chrono::milliseconds(500000000);
    smoother.SetValue(100.0f, startTimestamp2);

    // We jumped to target
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 10.0f, 0.1f));

    // Jump to half-way through
    smoother.Update(startTimestamp2 + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 10.0f + 90.0f * 0.5f, 0.5f));
}

TEST(ParameterSmootherTests, TargetsClampedTarget)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 0.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
            return value;
        },
        [](float targetValue)
        {
            // Clamp to this
            return targetValue + 100.0f;
        },
        std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(10.0f, startTimestamp);

    // Real target is 110.0f
    EXPECT_TRUE(ApproxEquals(smoother.GetValue(), 110.0f, 0.1f));

    EXPECT_TRUE(ApproxEquals(valueBeingSet, 0.0f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 55.0f, 0.5f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(1000));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 110.0f, 0.1f));
}

TEST(ParameterSmootherTests, NverOvershoots_Positive)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 0.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
        std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(10.0f, startTimestamp);

    EXPECT_FLOAT_EQ(valueBeingSet, 0.0f);

    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 5.0f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(2000));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 10.0f, 0.1f));
}

TEST(ParameterSmootherTests, NeverOvershoots_Negative)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 10.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
        std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(0.0f, startTimestamp);

    EXPECT_FLOAT_EQ(valueBeingSet, 10.0f);

    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 5.0f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(2000));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 0.0f, 0.1f));
}

TEST(ParameterSmootherTests, SetValueImmediateTruncatesProgress)
{
    float valueBeingSet = 1000.0f;

    ParameterSmoother<float> smoother(
        []()
        {
            return 0.0f;
        },
        [&valueBeingSet](float value)
        {
            valueBeingSet = value;
        },
            std::chrono::milliseconds(1000));

    auto startTimestamp = GameWallClock::GetInstance().Now();
    smoother.SetValue(10.0f, startTimestamp);

    EXPECT_FLOAT_EQ(valueBeingSet, 0.0f);

    smoother.Update(startTimestamp + std::chrono::milliseconds(1));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 0.01f, 0.1f));

    smoother.Update(startTimestamp + std::chrono::milliseconds(500));
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 5.0f, 0.1f));

    smoother.SetValueImmediate(95.0f);

    EXPECT_FLOAT_EQ(smoother.GetValue(), 95.0f);
    EXPECT_TRUE(ApproxEquals(valueBeingSet, 95.0f, 0.1f));
}
