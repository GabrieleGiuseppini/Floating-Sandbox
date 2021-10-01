#include <GameCore/Endian.h>

#include <GameCore/SysSpecifics.h>

#include "gtest/gtest.h"

#include <limits>

#if FS_IS_ARCHITECTURE_X86_64() || FS_IS_ARCHITECTURE_X86_32()

TEST(EndianTests, uint16_t_Nativex86_vs_LittleEndianess)
{
    {
        unsigned char bigEndianBuffer[] = { 0x01, 0x04 };
        std::uint16_t value = *(reinterpret_cast<std::uint16_t *>(bigEndianBuffer));
        EXPECT_EQ(value, uint16_t(0x0401));
    }

    {
        std::uint16_t sourceValue = 0x0401;
        std::uint16_t value = LittleEndian<std::uint16_t>::Read(reinterpret_cast<unsigned char const *>(&sourceValue));
        EXPECT_EQ(value, sourceValue);
    }
}

#endif

TEST(EndianTests, uint16_t_Read_Big)
{
    {
        unsigned char bigEndianBuffer[] = { 0x01, 0x04 };
        std::uint16_t value = BigEndian<std::uint16_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint16_t(0x0104));
    }

    {
        unsigned char bigEndianBuffer[] = { 0xff, 0x00 };
        std::uint16_t value = BigEndian<std::uint16_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint16_t(0xff00));
    }

    {
        unsigned char bigEndianBuffer[] = { 0x00, 0xff };
        std::uint16_t value = BigEndian<std::uint16_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint16_t(0x00ff));
    }
}

TEST(EndianTests, uint16_t_Write)
{
    unsigned char bigEndianBuffer[4];

    {
        std::uint16_t value = 0x0104;
        BigEndian<std::uint16_t>::Write(value, bigEndianBuffer);

        EXPECT_EQ(bigEndianBuffer[0], 0x01);
        EXPECT_EQ(bigEndianBuffer[1], 0x04);
    }

    {
        std::uint16_t value = 0xff00;
        BigEndian<std::uint16_t>::Write(value, bigEndianBuffer);

        EXPECT_EQ(bigEndianBuffer[0], 0xff);
        EXPECT_EQ(bigEndianBuffer[1], 0x00);
    }

    {
        std::uint16_t value = 0x00ff;
        BigEndian<std::uint16_t>::Write(value, bigEndianBuffer);

        EXPECT_EQ(bigEndianBuffer[0], 0x00);
        EXPECT_EQ(bigEndianBuffer[1], 0xff);
    }
}

TEST(EndianTests, uint32_t_Read)
{
    {
        unsigned char bigEndianBuffer[] = { 0x01, 0x04, 0xff, 0x0a };
        std::uint32_t value = BigEndian<std::uint32_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint32_t(0x0104ff0a));
    }

    {
        unsigned char bigEndianBuffer[] = { 0xff, 0x00, 0x01, 0x02 };
        std::uint32_t value = BigEndian<std::uint32_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint32_t(0xff000102));
    }

    {
        unsigned char bigEndianBuffer[] = { 0x00, 0x01, 0x02, 0xff };
        std::uint32_t value = BigEndian<std::uint32_t>::Read(bigEndianBuffer);
        EXPECT_EQ(value, uint32_t(0x000102ff));
    }
}

class EndianFloatTest : public testing::TestWithParam<float>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    EndianTests,
    EndianFloatTest,
    ::testing::Values(
        1.0f,
        -1.0f,
        0.0f,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::lowest()
    ));

TEST_P(EndianFloatTest, float_WriteRead)
{
    unsigned char bigEndianBuffer[4];

    {
        float sourceVal = GetParam();
        BigEndian<float>::Write(sourceVal, bigEndianBuffer);
        float targetVal = BigEndian<float>::Read(bigEndianBuffer);
        EXPECT_EQ(targetVal, sourceVal);
    }
}