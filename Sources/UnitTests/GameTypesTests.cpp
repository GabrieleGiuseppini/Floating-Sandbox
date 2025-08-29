#include <Core/GameTypes.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

TEST(GameTypesTests, FloatSize_fit_Equal)
{
    FloatSize sz1 = { 10.0f, 15.0f };
    FloatSize sz2 = { 10.0f, 15.0f };
    FloatSize ret = sz1.fit(sz2);

    EXPECT_EQ(ret.width, 10.0f);
    EXPECT_EQ(ret.height, 15.0f);
}

TEST(GameTypesTests, FloatSize_fit_Smaller_H_wins)
{
    FloatSize sz1 = { 10.0f, 15.0f };
    FloatSize sz2 = { 20.0f, 25.0f };
    FloatSize ret = sz1.fit(sz2);

    EXPECT_TRUE(ApproxEquals(ret.width, 16.6666f, 0.001f));
    EXPECT_EQ(ret.height, 25.0f);
}

TEST(GameTypesTests, FloatSize_fit_Smaller_W_wins)
{
    FloatSize sz1 = { 10.0f, 15.0f };
    FloatSize sz2 = { 20.0f, 100.0f };
    FloatSize ret = sz1.fit(sz2);

    EXPECT_EQ(ret.width, 20.0f);
    EXPECT_EQ(ret.height, 30.0f);
}

TEST(GameTypesTests, FloatSize_fit_Larger_W_wins)
{
    FloatSize sz1 = { 100.0f, 150.0f };
    FloatSize sz2 = { 20.0f, 31.0f };
    FloatSize ret = sz1.fit(sz2);

    EXPECT_EQ(ret.width, 20.0f);
    EXPECT_EQ(ret.height, 30.0f);
}

TEST(GameTypesTests, FloatSize_fit_Larger_H_wins)
{
    FloatSize sz1 = { 100.0f, 150.0f };
    FloatSize sz2 = { 20.0f, 20.0f };
    FloatSize ret = sz1.fit(sz2);

    EXPECT_TRUE(ApproxEquals(ret.width, 13.3333f, 0.001f));
    EXPECT_EQ(ret.height, 20.0f);
}
