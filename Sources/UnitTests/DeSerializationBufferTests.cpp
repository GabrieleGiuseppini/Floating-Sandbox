#include <Core/DeSerializationBuffer.h>

#include <Core/Endian.h>

#include "gtest/gtest.h"

#include <cstring>
#include <string>

#pragma pack(push, 1)

struct TestElement
{
    int a;
    int b;
    int c;
};

#pragma pack(pop)

TEST(DeSerializationBufferTests, BigEndian_uint16_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    uint16_t sourceVal1 = 0x0412;
    size_t const sourceSize1 = b.Append<std::uint16_t>(sourceVal1);
    EXPECT_EQ(sourceSize1, sizeof(std::uint16_t));

    uint16_t sourceVal2 = 0xff01;
    size_t const sourceSize2 = b.Append<std::uint16_t>(sourceVal2);
    EXPECT_EQ(sourceSize2, sizeof(std::uint16_t));

    uint16_t targetVal1;
    size_t const sz1 = b.ReadAt<std::uint16_t>(0, targetVal1);
    EXPECT_EQ(sz1, sizeof(std::uint16_t));
    EXPECT_EQ(sourceVal1, targetVal1);

    uint16_t targetVal2;
    size_t const sz2 = b.ReadAt<std::uint16_t>(sz1, targetVal2);
    EXPECT_EQ(sz2, sizeof(std::uint16_t));
    EXPECT_EQ(sourceVal2, targetVal2);
}

TEST(DeSerializationBufferTests, BigEndian_uint16_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<uint16_t>();
    ASSERT_EQ(idx, 0u);

    uint16_t sourceVal = 0x0412;
    size_t const sourceSize = b.WriteAt<std::uint16_t>(sourceVal, idx);
    EXPECT_EQ(sourceSize, sizeof(std::uint16_t));

    uint16_t targetVal;
    size_t const targetSize = b.ReadAt<std::uint16_t>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(std::uint16_t));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint32_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    uint32_t sourceVal = 0xffaa0088;
    size_t const sourceSize = b.Append<std::uint32_t>(sourceVal);
    EXPECT_EQ(sourceSize, sizeof(std::uint32_t));

    uint32_t targetVal;
    size_t const targetSize = b.ReadAt<std::uint32_t>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(std::uint32_t));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint32_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<uint32_t>();
    ASSERT_EQ(idx, 0u);

    uint32_t sourceVal = 0xff001122;
    size_t const sourceSize = b.WriteAt<std::uint32_t>(sourceVal, idx);
    EXPECT_EQ(sourceSize, sizeof(std::uint32_t));

    uint32_t targetVal;
    size_t const targetSize = b.ReadAt<std::uint32_t>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(std::uint32_t));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_int32_AppendAndRead_Positive)
{
    DeSerializationBuffer<BigEndianess> b(16);

    int32_t sourceVal = 456;
    size_t const sourceSize = b.Append<std::int32_t>(sourceVal);
    EXPECT_EQ(sourceSize, sizeof(std::int32_t));

    int32_t targetVal;
    size_t const targetSize = b.ReadAt<std::int32_t>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(std::int32_t));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_int32_AppendAndRead_Negative)
{
    DeSerializationBuffer<BigEndianess> b(16);

    int32_t sourceVal = -456;
    size_t const sourceSize = b.Append<std::int32_t>(sourceVal);
    EXPECT_EQ(sourceSize, sizeof(std::int32_t));

    int32_t targetVal;
    size_t const targetSize = b.ReadAt<std::int32_t>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(std::int32_t));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint64_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(2);

    uint64_t sourceVal1 = 0x1122334455667788;
    size_t const sourceSize1 = b.Append<std::uint64_t>(sourceVal1);
    EXPECT_EQ(sourceSize1, sizeof(std::uint64_t));

    uint64_t sourceVal2 = 0xffeeddccbbaa9988;
    size_t const sourceSize2 = b.Append<std::uint64_t>(sourceVal2);
    EXPECT_EQ(sourceSize2, sizeof(std::uint64_t));

    uint64_t targetVal1;
    size_t const targetSize1 = b.ReadAt<std::uint64_t>(0, targetVal1);
    EXPECT_EQ(targetSize1, sizeof(std::uint64_t));
    EXPECT_EQ(sourceVal1, targetVal1);

    uint64_t targetVal2;
    size_t const targetSize2 = b.ReadAt<std::uint64_t>(sourceSize1, targetVal2);
    EXPECT_EQ(targetSize2, sizeof(std::uint64_t));
    EXPECT_EQ(sourceVal2, targetVal2);
}

TEST(DeSerializationBufferTests, BigEndian_float_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    float sourceVal1 = 0.125f;
    size_t const sourceSize1 = b.Append<float>(sourceVal1);
    EXPECT_EQ(sourceSize1, sizeof(float));

    float sourceVal2 = -4.0f;
    size_t const sourceSize2 = b.Append<float>(sourceVal2);
    EXPECT_EQ(sourceSize2, sizeof(float));

    float targetVal1;
    size_t const targetSize1 = b.ReadAt<float>(0, targetVal1);
    EXPECT_EQ(targetSize1, sizeof(float));
    EXPECT_EQ(sourceVal1, targetVal1);

    float targetVal2;
    size_t const targetSize2 = b.ReadAt<float>(sourceSize1, targetVal2);
    EXPECT_EQ(targetSize2, sizeof(float));
    EXPECT_EQ(sourceVal2, targetVal2);
}

TEST(DeSerializationBufferTests, BigEndian_float_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<float>();
    ASSERT_EQ(idx, 0u);

    float sourceVal = 4.25f;
    size_t const sourceSize = b.WriteAt<float>(sourceVal, idx);
    EXPECT_EQ(sourceSize, sizeof(float));

    float targetVal;
    size_t const targetSize = b.ReadAt<float>(0, targetVal);
    EXPECT_EQ(targetSize, sizeof(float));
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_string_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    std::string sourceVal1 = "Test1";
    size_t const sourceSize1 = b.Append<std::string>(sourceVal1);
    EXPECT_EQ(sourceSize1, sizeof(uint32_t) + 5);

    std::string sourceVal2 = "FloatingSandbast";
    size_t const sourceSize2 = b.Append<std::string>(sourceVal2);
    EXPECT_EQ(sourceSize2, sizeof(uint32_t) + 16);

    std::string targetVal1;
    size_t const targetSize1 = b.ReadAt<std::string>(0, targetVal1);
    EXPECT_EQ(targetSize1, sizeof(uint32_t) + 5);
    EXPECT_EQ(sourceVal1, targetVal1);

    std::string targetVal2;
    size_t const targetSize2 = b.ReadAt<std::string>(sourceSize1, targetVal2);
    EXPECT_EQ(targetSize2, sizeof(uint32_t) + 16);
    EXPECT_EQ(sourceVal2, targetVal2);
}

TEST(DeSerializationBufferTests, BigEndian_var_uint16_AppendAndRead)
{
    for (std::uint16_t sourceValue = 0; sourceValue <= std::numeric_limits<var_uint16_t>::max().value(); ++sourceValue)
    {
        DeSerializationBuffer<BigEndianess> b(16);

        size_t const writeSize = b.Append<var_uint16_t>(var_uint16_t(sourceValue));
        if (sourceValue <= 0x7f)
        {
            ASSERT_EQ(writeSize, 1u);
        }
        else
        {
            ASSERT_EQ(writeSize, 2u);
        }

        var_uint16_t readValue;
        size_t const readSize = b.ReadAt<var_uint16_t>(0, readValue);
        ASSERT_EQ(readSize, writeSize);
        EXPECT_EQ(readValue.value(), sourceValue);
    }
}

TEST(DeSerializationBufferTests, BigEndian_ReserveAndAdvance_Struct)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0u);

    size_t indexStart1 = b.ReserveAndAdvance<TestElement>();

    EXPECT_EQ(indexStart1, 0u);
    EXPECT_EQ(b.GetSize(), sizeof(TestElement));
}

TEST(DeSerializationBufferTests, BigEndian_ReserveAndAdvance_Bytes)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0u);

    size_t indexStart1 = b.ReserveAndAdvance(456);

    EXPECT_EQ(indexStart1, 0u);
    EXPECT_EQ(b.GetSize(), 456u);
}

TEST(DeSerializationBufferTests, BigEndian_Receive)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0u);

    auto const ptr = b.Receive(1024);
    ASSERT_EQ(b.GetSize(), 1024u);

    unsigned char testData[] = { 2, 3, 8, 252 };
    std::memcpy(ptr, testData, 4);

    EXPECT_EQ(2, b.GetData()[0]);
    EXPECT_EQ(3, b.GetData()[1]);
    EXPECT_EQ(8, b.GetData()[2]);
    EXPECT_EQ(252, b.GetData()[3]);
}

TEST(DeSerializationBufferTests, BigEndian_CopiesWhenGrowing)
{
    DeSerializationBuffer<BigEndianess> b(6);

    uint32_t sourceVal1 = 0xffaa0088;
    size_t const sourceSize1 = b.Append<std::uint32_t>(sourceVal1);

    uint32_t sourceVal2 = 0x12345678;
    size_t const sourceSize2 = b.Append<std::uint32_t>(sourceVal2);

    uint32_t const sourceVal3 = 0x89abcdef;
    size_t sourceSize3 = b.Append<std::uint32_t>(sourceVal3);

    uint32_t targetVal;
    b.ReadAt<std::uint32_t>(0, targetVal);
    EXPECT_EQ(sourceVal1, targetVal);

    b.ReadAt<std::uint32_t>(0 + sourceSize1, targetVal);
    EXPECT_EQ(sourceVal2, targetVal);

    b.ReadAt<std::uint32_t>(0 + sourceSize1 + sourceSize2, targetVal);
    EXPECT_EQ(sourceVal3, targetVal);

    EXPECT_EQ(sourceSize3, sizeof(std::uint32_t));
}

TEST(DeSerializationBufferTests, Append_Bytes)
{
    DeSerializationBuffer<BigEndianess> b(512);

    // 1

    unsigned char testData1[] = { 2, 3, 8, 9 };
    b.Append(testData1, 4);

    // 2

    unsigned char testData2[] = { 12, 13, 18, 19 };
    b.Append(testData2, 4);

    // Verify

    ASSERT_EQ(b.GetSize(), 8u);

    // 1

    EXPECT_EQ(b.GetData()[0], 2);
    EXPECT_EQ(b.GetData()[1], 3);
    EXPECT_EQ(b.GetData()[2], 8);
    EXPECT_EQ(b.GetData()[3], 9);

    // 2

    EXPECT_EQ(b.GetData()[4], 12);
    EXPECT_EQ(b.GetData()[5], 13);
    EXPECT_EQ(b.GetData()[6], 18);
    EXPECT_EQ(b.GetData()[7], 19);
}