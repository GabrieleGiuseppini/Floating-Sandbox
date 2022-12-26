#include <GameCore/Buffer.h>

#include <GameCore/Vectors.h>

#include "gtest/gtest.h"

TEST(BufferTests, Buffer_EmplaceBack)
{
    Buffer<int> buf(64);

    EXPECT_EQ(0u, buf.GetCurrentPopulatedSize());

    buf.emplace_back(24);

    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    EXPECT_EQ(24, buf.data()[0]);
    EXPECT_EQ(24, buf[0]);
}

TEST(BufferTests, Buffer_Clear)
{
    Buffer<int> buf(64);

    EXPECT_EQ(0u, buf.GetCurrentPopulatedSize());

    buf.emplace_back(24);

    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    buf.clear();

    EXPECT_EQ(0u, buf.GetCurrentPopulatedSize());
}

TEST(BufferTests, Buffer_Move)
{
    Buffer<int> buf1(64);

    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    Buffer<int> buf2(std::move(buf1));

    EXPECT_EQ(3u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf2[0]);
    EXPECT_EQ(13, buf2[1]);
    EXPECT_EQ(41, buf2[2]);
}

TEST(BufferTests, Buffer_CopyFrom)
{
    Buffer<int> buf1(64);

    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    Buffer<int> buf2(64);

    buf2.copy_from(buf1);

    EXPECT_EQ(3u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf2[0]);
    EXPECT_EQ(13, buf2[1]);
    EXPECT_EQ(41, buf2[2]);
}

TEST(BufferTests, Buffer_Fill)
{
    Buffer<int> buf(64);

    buf.fill(242);

    EXPECT_EQ(242, buf[0]);
    EXPECT_EQ(242, buf[63]);
}

TEST(BufferTests, Buffer_FillAtCctor_Constant)
{
    Buffer<int> buf(64, 12, 242);

    buf.emplace_back(566);
    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    EXPECT_EQ(242, buf[12]);
    EXPECT_EQ(242, buf[63]);
}

TEST(BufferTests, Buffer_FillAtCctor_Function)
{
	Buffer<int> buf(64, 12, [](size_t i) { return static_cast<int>(i); });

	buf.emplace_back(566);
	EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

	EXPECT_EQ(12, buf[12]);
	EXPECT_EQ(63, buf[63]);
}

TEST(BufferTests, Buffer_Swap)
{
    Buffer<int> buf1(64);
    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    Buffer<int> buf2(10);
    buf2.emplace_back(2);
    buf2.emplace_back(1);

    buf1.swap(buf2);

    ASSERT_EQ(2u, buf1.GetCurrentPopulatedSize());
    EXPECT_EQ(2, buf1[0]);
    EXPECT_EQ(1, buf1[1]);

    ASSERT_EQ(3u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf2[0]);
    EXPECT_EQ(13, buf2[1]);
    EXPECT_EQ(41, buf2[2]);
}

