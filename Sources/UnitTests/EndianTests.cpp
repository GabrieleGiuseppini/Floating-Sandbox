#include <Core/Endian.h>

#include <Core/SysSpecifics.h>

#include "gtest/gtest.h"

#include <limits>

#if FS_IS_ARCHITECTURE_X86_64() || FS_IS_ARCHITECTURE_X86_32()

TEST(EndianTests, uint16_t_Nativex86_vs_LittleEndianess)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04 };
        std::uint16_t value = *(reinterpret_cast<std::uint16_t *>(endianBuffer));
        EXPECT_EQ(value, uint16_t(0x0401));
    }

    {
        std::uint16_t sourceValue = 0x0401;
        std::uint16_t value;
        size_t sz = LittleEndian<std::uint16_t>::Read(reinterpret_cast<unsigned char const *>(&sourceValue), value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, sourceValue);
    }
}

#endif

TEST(EndianTests, uint16_t_Read_Big)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04 };
        std::uint16_t value;
        size_t sz = BigEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0x0104));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00 };
        std::uint16_t value;
        size_t sz = BigEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0xff00));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0xff };
        std::uint16_t value;
        size_t sz = BigEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0x00ff));
    }
}

TEST(EndianTests, uint16_t_Read_Little)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04 };
        std::uint16_t value;
        size_t sz = LittleEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0x0401));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00 };
        std::uint16_t value;
        size_t sz = LittleEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0x00ff));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0xff };
        std::uint16_t value;
        size_t sz = LittleEndian<std::uint16_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint16_t));
        EXPECT_EQ(value, uint16_t(0xff00));
    }
}

TEST(EndianTests, uint16_t_Write_Big)
{
    unsigned char endianBuffer[4];

    {
        std::uint16_t value = 0x0104;
        BigEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0x01);
        EXPECT_EQ(endianBuffer[1], 0x04);
    }

    {
        std::uint16_t value = 0xff00;
        BigEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0xff);
        EXPECT_EQ(endianBuffer[1], 0x00);
    }

    {
        std::uint16_t value = 0x00ff;
        BigEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0x00);
        EXPECT_EQ(endianBuffer[1], 0xff);
    }
}

TEST(EndianTests, uint16_t_Write_Little)
{
    unsigned char endianBuffer[4];

    {
        std::uint16_t value = 0x0104;
        LittleEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0x04);
        EXPECT_EQ(endianBuffer[1], 0x01);
    }

    {
        std::uint16_t value = 0xff00;
        LittleEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0x00);
        EXPECT_EQ(endianBuffer[1], 0xff);
    }

    {
        std::uint16_t value = 0x00ff;
        LittleEndian<std::uint16_t>::Write(value, endianBuffer);

        EXPECT_EQ(endianBuffer[0], 0xff);
        EXPECT_EQ(endianBuffer[1], 0x00);
    }
}

TEST(EndianTests, var_uint16_t_WriteRead_Big)
{
    unsigned char buffer[2]{ 0x00, 0x00 };;

    for (std::uint16_t sourceValue = 0; sourceValue <= std::numeric_limits<var_uint16_t>::max().value(); ++sourceValue)
    {
        size_t const writeSize = BigEndian<var_uint16_t>::Write(var_uint16_t(sourceValue), buffer);
        if (sourceValue <= 0x7f)
        {
            ASSERT_EQ(writeSize, 1u);
        }
        else
        {
            ASSERT_EQ(writeSize, 2u);
        }

        var_uint16_t readValue;
        size_t const readSize = BigEndian<var_uint16_t>::Read(buffer, readValue);
        ASSERT_EQ(readSize, writeSize);
        EXPECT_EQ(readValue.value(), sourceValue);
    }
}

TEST(EndianTests, var_uint16_t_WriteRead_Little)
{
    unsigned char buffer[2]{ 0x00, 0x00 };;

    for (std::uint16_t sourceValue = 0; sourceValue <= std::numeric_limits<var_uint16_t>::max().value(); ++sourceValue)
    {
        size_t const writeSize = LittleEndian<var_uint16_t>::Write(var_uint16_t(sourceValue), buffer);
        if (sourceValue <= 0x7f)
        {
            ASSERT_EQ(writeSize, 1u);
        }
        else
        {
            ASSERT_EQ(writeSize, 2u);
        }

        var_uint16_t readValue;
        size_t const readSize = LittleEndian<var_uint16_t>::Read(buffer, readValue);
        ASSERT_EQ(readSize, writeSize);
        EXPECT_EQ(readValue.value(), sourceValue);
    }
}

TEST(EndianTests, uint32_t_Read_Big)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a };
        std::uint32_t value;
        size_t sz = BigEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0x0104ff0a));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02 };
        std::uint32_t value;
        size_t sz = BigEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0xff000102));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0xff };
        std::uint32_t value;
        size_t sz = BigEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0x000102ff));
    }
}

TEST(EndianTests, uint32_t_Read_Little)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a };
        std::uint32_t value;
        size_t sz = LittleEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0x0aff0401));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02 };
        std::uint32_t value;
        size_t sz = LittleEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0x020100ff));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0xff };
        std::uint32_t value;
        size_t sz = LittleEndian<std::uint32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint32_t));
        EXPECT_EQ(value, uint32_t(0xff020100));
    }
}

TEST(EndianTests, int32_t_Read_Big)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a };
        std::int32_t value;
        size_t sz = BigEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0x0104ff0a));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02 };
        std::int32_t value;
        size_t sz = BigEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0xff000102));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0xff };
        std::int32_t value;
        size_t sz = BigEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0x000102ff));
    }
}

TEST(EndianTests, int32_t_Read_Little)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a };
        std::int32_t value;
        size_t sz = LittleEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0x0aff0401));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02 };
        std::int32_t value;
        size_t sz = LittleEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0x020100ff));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0xff };
        std::int32_t value;
        size_t sz = LittleEndian<std::int32_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::int32_t));
        EXPECT_EQ(value, int32_t(0xff020100));
    }
}

TEST(EndianTests, uint64_t_Read_Big)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a, 0x02, 0x09, 0xaa, 0x04 };
        std::uint64_t value;
        size_t sz = BigEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0x0104ff0a0209aa04));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02, 0xaa, 0xbb, 0xcc, 0xdd };
        std::uint64_t value;
        size_t sz = BigEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0xff000102aabbccdd));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0x0f, 0x05, 0x77, 0xaa, 0xff };
        std::uint64_t value;
        size_t sz = BigEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0x0001020f0577aaff));
    }
}

TEST(EndianTests, uint64_t_Read_Little)
{
    {
        unsigned char endianBuffer[] = { 0x01, 0x04, 0xff, 0x0a, 0x02, 0x09, 0xaa, 0x04 };
        std::uint64_t value;
        size_t sz = LittleEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0x04aa09020aff0401));
    }

    {
        unsigned char endianBuffer[] = { 0xff, 0x00, 0x01, 0x02, 0xaa, 0xbb, 0xcc, 0xdd };
        std::uint64_t value;
        size_t sz = LittleEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0xddccbbaa020100ff));
    }

    {
        unsigned char endianBuffer[] = { 0x00, 0x01, 0x02, 0x0f, 0x05, 0x77, 0xaa, 0xff };
        std::uint64_t value;
        size_t sz = LittleEndian<std::uint64_t>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(std::uint64_t));
        EXPECT_EQ(value, uint64_t(0xffaa77050f020100));
    }
}

TEST(EndianTests, bool_Read_Big)
{
    {
        unsigned char endianBuffer[] = { 0x01 };
        bool value;
        size_t sz = BigEndian<bool>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(unsigned char));
        EXPECT_EQ(value, true);
    }

    {
        unsigned char endianBuffer[] = { 0x00 };
        bool value;
        size_t sz = BigEndian<bool>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(unsigned char));
        EXPECT_EQ(value, false);
    }
}

TEST(EndianTests, bool_Read_Little)
{
    {
        unsigned char endianBuffer[] = { 0x01 };
        bool value;
        size_t sz  = BigEndian<bool>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(unsigned char));
        EXPECT_EQ(value, true);
    }

    {
        unsigned char endianBuffer[] = { 0x00 };
        bool value;
        size_t sz = BigEndian<bool>::Read(endianBuffer, value);
        EXPECT_EQ(sz, sizeof(unsigned char));
        EXPECT_EQ(value, false);
    }
}

class EndianFloatTest_Big : public testing::TestWithParam<float>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    EndianTests,
    EndianFloatTest_Big,
    ::testing::Values(
        1.0f,
        -1.0f,
        0.0f,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::lowest()
    ));

TEST_P(EndianFloatTest_Big, float_WriteRead_Big)
{
    unsigned char endianBuffer[4];

    {
        float sourceVal = GetParam();
        BigEndian<float>::Write(sourceVal, endianBuffer);
        float targetVal;
        size_t sz = BigEndian<float>::Read(endianBuffer, targetVal);
        EXPECT_EQ(sz, sizeof(float));
        EXPECT_EQ(targetVal, sourceVal);
    }
}

class EndianFloatTest_Little : public testing::TestWithParam<float>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    EndianTests,
    EndianFloatTest_Little,
    ::testing::Values(
        1.0f,
        -1.0f,
        0.0f,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::lowest()
    ));

TEST_P(EndianFloatTest_Little, float_WriteRead_Little)
{
    unsigned char endianBuffer[4];

    {
        float sourceVal = GetParam();
        LittleEndian<float>::Write(sourceVal, endianBuffer);
        float targetVal;
        size_t sz = LittleEndian<float>::Read(endianBuffer, targetVal);
        EXPECT_EQ(sz, sizeof(float));
        EXPECT_EQ(targetVal, sourceVal);
    }
}