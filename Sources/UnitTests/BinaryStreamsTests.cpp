#include <Core/BinaryStreams.h>
#include <Core/MemoryBinaryStreams.h>

#include "gtest/gtest.h"

TEST(BinaryStreams, MemoryBinaryReadStream)
{
    std::vector<std::uint8_t> data{
        0x00, 0x01, 0x02, 0x03
    };

    MemoryBinaryReadStream stream(std::move(data));

    EXPECT_EQ(stream.GetCurrentPosition(), 0u);

    std::uint8_t buffer[5] = { 0xff, 0xff, 0xff, 0xff, 0xff };

    size_t szRead = stream.Read(buffer, 3u);

    EXPECT_EQ(szRead, 3u);
    EXPECT_EQ(stream.GetCurrentPosition(), 3u);
    EXPECT_EQ(buffer[0], 0x00);
    EXPECT_EQ(buffer[1], 0x01);
    EXPECT_EQ(buffer[2], 0x02);
    EXPECT_EQ(buffer[3], 0xff);

    szRead = stream.Read(buffer, 2u);

    EXPECT_EQ(szRead, 1u);
    EXPECT_EQ(stream.GetCurrentPosition(), 4u);
    EXPECT_EQ(buffer[0], 0x03);
    EXPECT_EQ(buffer[1], 0x01);
}

TEST(BinaryStreams, MemoryBinaryWriteStream)
{
    MemoryBinaryWriteStream stream;

    std::uint8_t buffer[4] = { 0x00, 0x01, 0x02, 0x03 };

    EXPECT_EQ(stream.GetSize(), 0u);

    stream.Write(buffer, 3u);

    EXPECT_EQ(stream.GetSize(), 3u);
    EXPECT_EQ(stream.GetData()[0], 0x00);
    EXPECT_EQ(stream.GetData()[1], 0x01);
    EXPECT_EQ(stream.GetData()[2], 0x02);

    stream.Write(buffer + 1, 1u);

    EXPECT_EQ(stream.GetSize(), 4u);
    EXPECT_EQ(stream.GetData()[0], 0x00);
    EXPECT_EQ(stream.GetData()[1], 0x01);
    EXPECT_EQ(stream.GetData()[2], 0x02);
    EXPECT_EQ(stream.GetData()[3], 0x01);
}