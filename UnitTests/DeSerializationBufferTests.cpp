#include <GameCore/DeSerializationBuffer.h>

#include <GameCore/Endian.h>

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

    uint16_t sourceVal = 0x0412;
    size_t sz = b.Append<std::uint16_t>(sourceVal);
    uint16_t targetVal = b.ReadAt<std::uint16_t>(0);

    EXPECT_EQ(sz, 2);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint16_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<uint16_t>();
    ASSERT_EQ(idx, 0);

    uint16_t sourceVal = 0x0412;
    size_t sz = b.WriteAt<std::uint16_t>(sourceVal, idx);
    uint16_t targetVal = b.ReadAt<std::uint16_t>(0);

    EXPECT_EQ(sz, 2);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint32_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    uint32_t sourceVal = 0xffaa0088;
    size_t sz = b.Append<std::uint32_t>(sourceVal);
    uint32_t targetVal = b.ReadAt<std::uint32_t>(0);

    EXPECT_EQ(sz, 4);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_uint32_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<uint32_t>();
    ASSERT_EQ(idx, 0);

    uint32_t sourceVal = 0xff001122;
    size_t sz = b.WriteAt<std::uint32_t>(sourceVal, idx);
    uint32_t targetVal = b.ReadAt<std::uint32_t>(0);

    EXPECT_EQ(sz, 4);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_float_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    float sourceVal = 0.125f;
    size_t sz = b.Append<float>(sourceVal);
    float targetVal = b.ReadAt<float>(0);

    EXPECT_EQ(sz, 4);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_float_WriteAtAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    size_t idx = b.ReserveAndAdvance<float>();
    ASSERT_EQ(idx, 0);

    float sourceVal = 4.25f;
    size_t sz = b.WriteAt<float>(sourceVal, idx);
    float targetVal = b.ReadAt<float>(0);

    EXPECT_EQ(sz, 4);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_string_AppendAndRead)
{
    DeSerializationBuffer<BigEndianess> b(16);

    std::string sourceVal = "Test1";
    size_t sz = b.Append<std::string>(sourceVal);
    std::string targetVal = b.ReadAt<std::string>(0);

    EXPECT_EQ(sz, sizeof(uint32_t) + 5);
    EXPECT_EQ(sourceVal, targetVal);
}

TEST(DeSerializationBufferTests, BigEndian_ReserveAndAdvance_Struct)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0);

    size_t indexStart1 = b.ReserveAndAdvance<TestElement>();

    EXPECT_EQ(indexStart1, 0);
    EXPECT_EQ(b.GetSize(), sizeof(TestElement));
}

TEST(DeSerializationBufferTests, BigEndian_ReserveAndAdvance_Bytes)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0);

    size_t indexStart1 = b.ReserveAndAdvance(456);

    EXPECT_EQ(indexStart1, 0);
    EXPECT_EQ(b.GetSize(), 456);
}

TEST(DeSerializationBufferTests, BigEndian_Receive)
{
    DeSerializationBuffer<BigEndianess> b(4);

    ASSERT_EQ(b.GetSize(), 0);

    auto const ptr = b.Receive(1024);
    ASSERT_EQ(b.GetSize(), 1024);

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
    size_t sz1 = b.Append<std::uint32_t>(sourceVal1);

    uint32_t sourceVal2 = 0x12345678;
    size_t sz2 = b.Append<std::uint32_t>(sourceVal2);

    uint32_t sourceVal3 = 0x89abcdef;
    size_t sz3 = b.Append<std::uint32_t>(sourceVal3);

    uint32_t targetVal = b.ReadAt<std::uint32_t>(0);
    EXPECT_EQ(sourceVal1, targetVal);
    targetVal = b.ReadAt<std::uint32_t>(0 + sz1);
    EXPECT_EQ(sourceVal2, targetVal);
    targetVal = b.ReadAt<std::uint32_t>(0 + sz1 + sz2);
    EXPECT_EQ(sourceVal3, targetVal);

    EXPECT_EQ(sz3, 4);
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

    ASSERT_EQ(b.GetSize(), 8);

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