#include <Core/ProgressCallback.h>

#include "gtest/gtest.h"

class ProgressCallbackTests : public testing::Test
{
protected:

    std::vector<float> mProgressCalls;
};

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_NoRange)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        });

    // Test

    pc(0.0f, ProgressMessageType::InitializingNoise);
    pc(0.4f, ProgressMessageType::InitializingNoise);
    pc(1.0f, ProgressMessageType::InitializingNoise);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    EXPECT_NEAR(mProgressCalls[0], 0.0f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], 1.0f, 0.0001f);
}

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_WithRange)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        },
        0.2f,
        0.4f);

    // Test

    pc(0.0f, ProgressMessageType::InitializingNoise);
    pc(0.4f, ProgressMessageType::InitializingNoise);
    pc(1.0f, ProgressMessageType::InitializingNoise);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    EXPECT_NEAR(mProgressCalls[0], 0.2f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], 0.2f + 0.4f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], 0.6f, 0.0001f);
}

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_NoRange_SubCallback_Once)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        });

    ProgressCallback pc2 = pc.MakeSubCallback(0.2f, 0.4f);

    // Test

    pc2(0.0f, ProgressMessageType::InitializingNoise);
    pc2(0.4f, ProgressMessageType::InitializingNoise);
    pc2(1.0f, ProgressMessageType::InitializingNoise);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    EXPECT_NEAR(mProgressCalls[0], 0.2f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], 0.2f + 0.4f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], 0.6f, 0.0001f);
}

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_WithRange_SubCallback_Once)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        },
        0.2f,
        0.4f);

    ProgressCallback pc2 = pc.MakeSubCallback(0.3f, 0.2f);

    // Test

    pc2(0.0f, ProgressMessageType::InitializingNoise);
    pc2(0.4f, ProgressMessageType::InitializingNoise);
    pc2(1.0f, ProgressMessageType::InitializingNoise);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    EXPECT_NEAR(mProgressCalls[0], 0.2f + 0.3f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], 0.2f + 0.3f * 0.4f + 0.4f * 0.2f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], 0.2f + 0.3f * 0.4f + 1.0f * 0.2f * 0.4f, 0.0001f);
}

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_WithRange_SubCallback_Twice)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        },
        0.2f,
        0.4f);

    ProgressCallback pc2 = pc.MakeSubCallback(0.3f, 0.2f);

    ProgressCallback pc3 = pc2.MakeSubCallback(0.1f, 0.8f);

    // Test

    pc3(0.0f, ProgressMessageType::InitializingNoise);
    pc3(0.4f, ProgressMessageType::InitializingNoise);
    pc3(1.0f, ProgressMessageType::InitializingNoise);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    float constexpr Pc2Left = 0.2f + 0.3f * 0.4f;
    float constexpr Pc3Left = Pc2Left + 0.1f * 0.2f * 0.4f;
    EXPECT_NEAR(mProgressCalls[0], Pc3Left, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], Pc3Left + 0.4f * 0.8f * 0.2f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], Pc3Left + 1.0f * 0.8f * 0.2f * 0.4f, 0.0001f);
}

TEST_F(ProgressCallbackTests, ProgressCallback_Direct_WithRange_SubCallback_Cloned)
{
    ProgressCallback pc = ProgressCallback(
        [&](float progress, ProgressMessageType message)
        {
            mProgressCalls.push_back(progress);
            EXPECT_EQ(message, ProgressMessageType::InitializingNoise);
        },
        0.2f,
        0.4f);

    ProgressCallback pc2 = pc.MakeSubCallback(0.3f, 0.2f);

    ProgressCallback pc3 = pc2.MakeSubCallback(0.1f, 0.8f);

    SimpleProgressCallback pc4 = pc3.CloneToSimpleCallback(
        [&](float progress)
        {
            mProgressCalls.push_back(progress);
        });

    // Test

    pc4(0.0f);
    pc4(0.4f);
    pc4(1.0f);

    // Verify

    ASSERT_EQ(mProgressCalls.size(), 3u);

    float constexpr Pc2Left = 0.2f + 0.3f * 0.4f;
    float constexpr Pc3Left = Pc2Left + 0.1f * 0.2f * 0.4f;
    EXPECT_NEAR(mProgressCalls[0], Pc3Left, 0.0001f);
    EXPECT_NEAR(mProgressCalls[1], Pc3Left + 0.4f * 0.8f * 0.2f * 0.4f, 0.0001f);
    EXPECT_NEAR(mProgressCalls[2], Pc3Left + 1.0f * 0.8f * 0.2f * 0.4f, 0.0001f);
}
