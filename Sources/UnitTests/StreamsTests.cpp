#include <Core/Streams.h>
#include <Core/MemoryStreams.h>

#include "gtest/gtest.h"

TEST(Streams, MemoryBinaryReadStream)
{
    std::vector<std::uint8_t> data{
        0x00, 0x01, 0x02, 0x03
    };

    MemoryBinaryReadStream stream(std::move(data));

    EXPECT_EQ(stream.GetCurrentPosition(), 0u);
    EXPECT_EQ(stream.GetSize(), 4u);

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

    stream.SetPosition(1u);
    EXPECT_EQ(stream.GetCurrentPosition(), 1u);

    szRead = stream.Read(buffer, 2u);

    EXPECT_EQ(szRead, 2u);
    EXPECT_EQ(stream.GetCurrentPosition(), 3u);
    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x02);
}

TEST(Streams, MemoryTextReadStream_ReadAll)
{
    std::string data = " Hello\nWorld\r\nOut There! ";

    std::string copy = data;
    MemoryTextReadStream stream(std::move(copy));

    std::string read = stream.ReadAll();

    EXPECT_EQ(read, data);
}

TEST(Streams, MemoryTextReadStream_ReadAllLines)
{
    std::string data = " Hello\nWorld\r\nOut There! ";

    std::string copy = data;
    MemoryTextReadStream stream(std::move(copy));

    std::vector<std::string> read = stream.ReadAllLines();

    ASSERT_EQ(read.size(), 3u);
    EXPECT_EQ(read[0], " Hello");
    EXPECT_EQ(read[1], "World\r");
    EXPECT_EQ(read[2], "Out There! ");
}

TEST(Streams, MemoryBinaryWriteStream)
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

TEST(Streams, MemoryTextWriteStream)
{
    MemoryTextWriteStream stream;

    stream.Write(" Hello\n");
    stream.Write(" World\r\n! ");

    EXPECT_EQ(stream.GetData(), " Hello\n World\r\n! ");
}