#include <Core/MemoryStreams.h>

#include <istream>
#include <ostream>

#include "gtest/gtest.h"

TEST(MemoryStreamsTests, BackingOutputStream_write)
{
    memory_streambuf ms;

    std::ostream os(&ms);

    unsigned char testData[] = { 0x05, 0x00, 0x7f, 0x80, 0x81, 0xff };

    os.write(reinterpret_cast<char *>(testData), 3);

    EXPECT_EQ(ms.size(), 3u);

    os.write(reinterpret_cast<char *>(testData) + 3, 3);

    EXPECT_EQ(ms.size(), 6u);

    EXPECT_EQ(0x05, static_cast<unsigned char>(ms.data()[0]));
    EXPECT_EQ(0x00, static_cast<unsigned char>(ms.data()[1]));
    EXPECT_EQ(0x7f, static_cast<unsigned char>(ms.data()[2]));
    EXPECT_EQ(0x80, static_cast<unsigned char>(ms.data()[3]));
    EXPECT_EQ(0x81, static_cast<unsigned char>(ms.data()[4]));
    EXPECT_EQ(0xff, static_cast<unsigned char>(ms.data()[5]));
}

TEST(MemoryStreamsTests, BackingOutputStream_streaming)
{
    memory_streambuf ms;

    std::ostream os(&ms);

    os << "foo" << "bar";

    ASSERT_EQ(ms.size(), 6u);

    EXPECT_EQ('f', ms.data()[0]);
    EXPECT_EQ('o', ms.data()[1]);
    EXPECT_EQ('o', ms.data()[2]);
    EXPECT_EQ('b', ms.data()[3]);
    EXPECT_EQ('a', ms.data()[4]);
    EXPECT_EQ('r', ms.data()[5]);
}

TEST(MemoryStreamsTests, BackingOutputStream_put)
{
    memory_streambuf ms;

    std::ostream os(&ms);

    os.put('h');
    os.put('o');
    os.put('i');

    ASSERT_EQ(ms.size(), 3u);

    EXPECT_EQ('h', ms.data()[0]);
    EXPECT_EQ('o', ms.data()[1]);
    EXPECT_EQ('i', ms.data()[2]);
}

TEST(MemoryStreamsTests, BackingInputStream_read_whole)
{
    memory_streambuf ms("hello");

    std::istream is(&ms);

    char localBuf[5];
    is.read(localBuf, 5);

    EXPECT_EQ(5, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    EXPECT_EQ('h', localBuf[0]);
    EXPECT_EQ('e', localBuf[1]);
    EXPECT_EQ('l', localBuf[2]);
    EXPECT_EQ('l', localBuf[3]);
    EXPECT_EQ('o', localBuf[4]);
}

TEST(MemoryStreamsTests, BackingInputStream_read_less)
{
    memory_streambuf ms("hello");

    std::istream is(&ms);

    char localBuf[5];
    is.read(localBuf, 3);

    EXPECT_EQ(3, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    EXPECT_EQ('h', localBuf[0]);
    EXPECT_EQ('e', localBuf[1]);
    EXPECT_EQ('l', localBuf[2]);
}

TEST(MemoryStreamsTests, BackingInputStream_read_more)
{
    memory_streambuf ms("hello");

    std::istream is(&ms);

    char localBuf[5];
    is.read(localBuf, 6);

    EXPECT_EQ(5, is.gcount());
    EXPECT_TRUE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_TRUE(is.eof());

    EXPECT_EQ('h', localBuf[0]);
    EXPECT_EQ('e', localBuf[1]);
    EXPECT_EQ('l', localBuf[2]);
    EXPECT_EQ('l', localBuf[3]);
    EXPECT_EQ('o', localBuf[4]);
}

TEST(MemoryStreamsTests, BackingInputStream_rewind)
{
    memory_streambuf ms("hello");

    std::istream is(&ms);

    char localBuf[5];
    is.read(localBuf, 5);

    EXPECT_EQ(5, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ms.rewind();

    is.read(localBuf, 5);

    EXPECT_EQ(5, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());
}

TEST(MemoryStreamsTests, BackingInputStream_seek)
{
    memory_streambuf ms("hello");

    std::istream is(&ms);

    EXPECT_EQ(0, is.tellg());

    char localBuf[5];
    is.read(localBuf, 5);

    EXPECT_EQ(5, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());
    EXPECT_EQ(5, is.tellg());

    is.seekg(2, std::ios_base::beg);
    EXPECT_EQ(2, is.tellg());
    is.read(localBuf, 3);

    EXPECT_EQ(3, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());
    EXPECT_EQ('l', localBuf[0]);
    EXPECT_EQ('l', localBuf[1]);
    EXPECT_EQ('o', localBuf[2]);
    EXPECT_EQ(5, is.tellg());

    is.seekg(1, std::ios_base::beg);
    is.seekg(2, std::ios_base::cur);
    EXPECT_EQ(3, is.tellg());
    is.read(localBuf, 2);

    EXPECT_EQ(2, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());
    EXPECT_EQ('l', localBuf[0]);
    EXPECT_EQ('o', localBuf[1]);

    is.seekg(-2, std::ios_base::end);
    EXPECT_EQ(3, is.tellg());
    is.read(localBuf, 2);

    EXPECT_EQ(2, is.gcount());
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());
    EXPECT_EQ('l', localBuf[0]);
    EXPECT_EQ('o', localBuf[1]);
    EXPECT_EQ(5, is.tellg());
}

TEST(MemoryStreamsTests, BackingInputStream_get)
{
    unsigned char initData[] = { 0x00, 0x7f, 0x80, 0x81, 0xff };
    memory_streambuf ms(reinterpret_cast<char *>(initData), 5);

    std::istream is(&ms);

    int ch = is.get();
    EXPECT_EQ(0x00, ch);
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ch = is.get();
    EXPECT_EQ(0x7f, ch);
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ch = is.get();
    EXPECT_EQ(0x80, ch);
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ch = is.get();
    EXPECT_EQ(0x81, ch);
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ch = is.get();
    EXPECT_EQ(0xff, ch);
    EXPECT_FALSE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_FALSE(is.eof());

    ch = is.get();
    EXPECT_EQ(EOF, ch);
    EXPECT_TRUE(is.fail());
    EXPECT_FALSE(is.bad());
    EXPECT_TRUE(is.eof());
}

TEST(MemoryStreamsTests, BackingInputStream_streaming)
{
    memory_streambuf ms("Hello");

    std::istream is(&ms);

    std::string content;
    is >> content;

    EXPECT_EQ(std::string("Hello"), content);
}

TEST(MemoryStreamsTests, BackingOutputAndInputStream_10K_streaming)
{
    memory_streambuf ms;

    std::ostream os(&ms);
    for (size_t i = 0; i < 1024; ++i)
        os << "aaaaaaaaaa";


    std::istream is(&ms);

    std::string content;
    is >> content;

    EXPECT_EQ(std::string(10 * 1024, 'a'), content);
}