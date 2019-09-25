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

TEST(BufferTests, Buffer_FillAtCctor)
{
    Buffer<int> buf(64, 12, 242);

    buf.emplace_back(566);
    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    EXPECT_EQ(242, buf[12]);
    EXPECT_EQ(242, buf[63]);
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

//////////////////////////////////////////////////////////////////////

TEST(BufferTests, BufferSegment_EmplaceBack)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(Buffer<int>::CalculateByteSize(64));
    BufferSegment<int> buf(sharedBuffer, 0, 64);

    EXPECT_EQ(0u, buf.GetCurrentPopulatedSize());

    buf.emplace_back(24);

    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    EXPECT_EQ(24, buf.data()[0]);
    EXPECT_EQ(24, buf[0]);
}

TEST(BufferTests, BufferSegment_Move)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(Buffer<int>::CalculateByteSize(64));
    BufferSegment<int> buf1(sharedBuffer, 0, 64);

    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    BufferSegment<int> buf2(std::move(buf1));

    EXPECT_EQ(3u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf2[0]);
    EXPECT_EQ(13, buf2[1]);
    EXPECT_EQ(41, buf2[2]);
}

TEST(BufferTests, BufferSegment_CopyFrom)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(Buffer<int>::CalculateByteSize(64));
    BufferSegment<int> buf1(sharedBuffer, 0, 64);

    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    BufferSegment<int> buf2(sharedBuffer, 0, 64);

    buf2.copy_from(buf1);

    EXPECT_EQ(3u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf2[0]);
    EXPECT_EQ(13, buf2[1]);
    EXPECT_EQ(41, buf2[2]);
}

TEST(BufferTests, BufferSegment_Fill)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(Buffer<int>::CalculateByteSize(64));
    BufferSegment<int> buf(sharedBuffer, 0, 64);

    buf.fill(242);

    EXPECT_EQ(242, buf[0]);
    EXPECT_EQ(242, buf[63]);
}

TEST(BufferTests, BufferSegment_FillAtCctor)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(Buffer<int>::CalculateByteSize(64));
    BufferSegment<int> buf(sharedBuffer, 0, 64, 12, 242);

    buf.emplace_back(566);
    EXPECT_EQ(1u, buf.GetCurrentPopulatedSize());

    EXPECT_EQ(242, buf[12]);
    EXPECT_EQ(242, buf[63]);
}

TEST(BufferTests, BufferSegment_TwoSegments)
{
    auto sharedBuffer = make_shared_buffer_aligned_to_vectorization_word<std::uint8_t>(
        Buffer<int>::CalculateByteSize(64)
        + Buffer<vec2f>::CalculateByteSize(10));

    BufferSegment<int> buf1(sharedBuffer, 0, 64);

    buf1.emplace_back(24);
    buf1.emplace_back(13);
    buf1.emplace_back(41);

    BufferSegment<vec2f> buf2(sharedBuffer, Buffer<int>::CalculateByteSize(64), 10);

    buf2.emplace_back(1.0f, 2.0f);
    buf2.emplace_back(10.0f, 20.0f);

    EXPECT_EQ(3u, buf1.GetCurrentPopulatedSize());
    EXPECT_EQ(24, buf1[0]);
    EXPECT_EQ(13, buf1[1]);
    EXPECT_EQ(41, buf1[2]);

    EXPECT_EQ(2u, buf2.GetCurrentPopulatedSize());
    EXPECT_EQ(vec2f(1.0f, 2.0f), buf2[0]);
    EXPECT_EQ(vec2f(10.0f, 20.0f), buf2[1]);
}