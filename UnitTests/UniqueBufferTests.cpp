#include <GameCore/UniqueBuffer.h>

#include "gtest/gtest.h"

TEST(UniqueBufferTests, Constructor)
{
    unique_buffer<float> b(8);

    EXPECT_EQ(8, b.size());
}

TEST(UniqueBufferTests, NonConstAccess)
{
    unique_buffer<float> b(8);

    b[0] = 123.0f;
    b[7] = 999.0f;

    EXPECT_EQ(123.0f, b[0]);
    EXPECT_EQ(999.0f, b[7]);

    float * buf = b.get();
    EXPECT_EQ(123.0f, buf[0]);
    EXPECT_EQ(999.0f, buf[7]);
}

TEST(UniqueBufferTests, CopyConstructor)
{
    unique_buffer<float> b1(8);

    b1[0] = 123.0f;
    b1[7] = 999.0f;

    unique_buffer<float> b2(b1);

    EXPECT_EQ(8, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[7]);

    b1[0] = 0.0f;
    b1[7] = 0.0f;

    ASSERT_EQ(8, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, MoveConstructor)
{
    unique_buffer<float> b1(8);

    b1[0] = 123.0f;
    b1[7] = 999.0f;

    unique_buffer<float> b2(std::move(b1));

    EXPECT_EQ(0, b1.size());

    ASSERT_EQ(8, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, Assignment)
{
    unique_buffer<float> b1(8);

    b1[0] = 123.0f;
    b1[7] = 999.0f;

    unique_buffer<float> b2(5);
    
    b2 = b1;

    EXPECT_EQ(8, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[7]);

    b1[0] = 0.0f;
    b1[7] = 0.0f;

    ASSERT_EQ(8, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, MoveAssignment)
{
    unique_buffer<float> b1(8);

    b1[0] = 123.0f;
    b1[7] = 999.0f;

    unique_buffer<float> b2(5);

    b2 = std::move(b1);

    EXPECT_EQ(0, b1.size());

    ASSERT_EQ(8, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, ConvertCopy_LargerToSmaller)
{
    unique_buffer<float> b1(3);

    b1[0] = 123.0f;
    b1[2] = 999.0f;

    unique_buffer<unsigned char> b2 = b1.convert_copy<unsigned char>();

    EXPECT_EQ(3, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[2]);

    b1[0] = 0.0f;
    b1[2] = 0.0f;

    ASSERT_EQ(3 * sizeof(float), b2.size());
    EXPECT_EQ(123.0f, (reinterpret_cast<float *>(b2.get()))[0]);
    EXPECT_EQ(999.0f, (reinterpret_cast<float *>(b2.get()))[2]);
}

TEST(UniqueBufferTests, ConvertMove_LargerToSmaller)
{
    unique_buffer<float> b1(3);

    b1[0] = 123.0f;
    b1[2] = 999.0f;

    unique_buffer<unsigned char> b2 = b1.convert_move<unsigned char>();

    EXPECT_EQ(0, b1.size());

    ASSERT_EQ(3 * sizeof(float), b2.size());
    EXPECT_EQ(123.0f, (reinterpret_cast<float *>(b2.get()))[0]);
    EXPECT_EQ(999.0f, (reinterpret_cast<float *>(b2.get()))[2]);
}

TEST(UniqueBufferTests, ConvertCopy_SmallerToLarger)
{
    unique_buffer<unsigned char> b1(4);

    b1[0] = 1;
    b1[1] = 1;
    b1[2] = 1;
    b1[3] = 1;
    
    unique_buffer<uint32_t> b2 = b1.convert_copy<uint32_t>();

    EXPECT_EQ(4, b1.size());

    ASSERT_EQ(1, b2.size());
    EXPECT_EQ(uint32_t(0x01010101), b2[0]);
}

TEST(UniqueBufferTests, ConvertMove_SmallerToLarger)
{
    unique_buffer<unsigned char> b1(4);

    b1[0] = 1;
    b1[1] = 1;
    b1[2] = 1;
    b1[3] = 1;

    unique_buffer<uint32_t> b2 = b1.convert_move<uint32_t>();

    EXPECT_EQ(0, b1.size());

    ASSERT_EQ(1, b2.size());
    EXPECT_EQ(uint32_t(0x01010101), b2[0]);

}
