#include <Core/UniqueBuffer.h>

#include "gtest/gtest.h"

TEST(UniqueBufferTests, Constructor)
{
    unique_buffer<float> b(8);

    EXPECT_EQ(8u, b.size());
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

    EXPECT_EQ(8u, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[7]);

    b1[0] = 0.0f;
    b1[7] = 0.0f;

    ASSERT_EQ(8u, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, MoveConstructor)
{
    unique_buffer<float> b1(8);

    b1[0] = 123.0f;
    b1[7] = 999.0f;

    unique_buffer<float> b2(std::move(b1));

    EXPECT_EQ(0u, b1.size());

    ASSERT_EQ(8u, b2.size());
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

    EXPECT_EQ(8u, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[7]);

    b1[0] = 0.0f;
    b1[7] = 0.0f;

    ASSERT_EQ(8u, b2.size());
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

    EXPECT_EQ(0u, b1.size());

    ASSERT_EQ(8u, b2.size());
    EXPECT_EQ(123.0f, b2[0]);
    EXPECT_EQ(999.0f, b2[7]);
}

TEST(UniqueBufferTests, Comparison)
{
    unique_buffer<float> b1(3);
    b1[0] = 4.0f;
    b1[1] = 8.0f;
    b1[2] = 16.0f;

    unique_buffer<float> b2(3);
    b2[0] = 4.0f;
    b2[1] = 8.0f;
    b2[2] = 16.0f;

    unique_buffer<float> b3(3);
    b3[0] = 4.0f;
    b3[1] = 8.1f;
    b3[2] = 16.0f;

    unique_buffer<float> b4(4);
    b4[0] = 4.0f;
    b4[1] = 8.0f;
    b4[2] = 16.0f;
    b4[3] = 32.0f;

    EXPECT_EQ(b1, b2);
    EXPECT_NE(b1, b3);
    EXPECT_NE(b1, b4);
    EXPECT_NE(b4, b1);
}

TEST(UniqueBufferTests, UnaryAddition)
{
	unique_buffer<float> b1(3);
	b1[0] = 4.0f;
	b1[1] = 8.0f;
	b1[2] = 16.0f;

	unique_buffer<float> b2(3);
	b2[0] = 14.0f;
	b2[1] = 18.0f;
	b2[2] = 116.0f;

	b1 += b2;

	EXPECT_FLOAT_EQ(18.0f, b1[0]);
	EXPECT_FLOAT_EQ(26.0f, b1[1]);
	EXPECT_FLOAT_EQ(132.0f, b1[2]);
}

TEST(UniqueBufferTests, UnarySubtraction)
{
	unique_buffer<float> b1(3);
	b1[0] = 4.0f;
	b1[1] = 8.0f;
	b1[2] = 116.0f;

	unique_buffer<float> b2(3);
	b2[0] = 14.0f;
	b2[1] = 18.0f;
	b2[2] = 16.0f;

	b1 -= b2;

	EXPECT_FLOAT_EQ(-10.0f, b1[0]);
	EXPECT_FLOAT_EQ(-10.0f, b1[1]);
	EXPECT_FLOAT_EQ(100.0f, b1[2]);
}

TEST(UniqueBufferTests, ScalarUnaryMultiplication)
{
	unique_buffer<float> b1(3);
	b1[0] = 4.0f;
	b1[1] = 8.0f;
	b1[2] = 116.0f;

	b1 *= 2.0f;

	EXPECT_FLOAT_EQ(8.0f, b1[0]);
	EXPECT_FLOAT_EQ(16.0f, b1[1]);
	EXPECT_FLOAT_EQ(232.0f, b1[2]);
}

TEST(UniqueBufferTests, ScalarUnaryDivision)
{
	unique_buffer<float> b1(3);
	b1[0] = 4.0f;
	b1[1] = 8.0f;
	b1[2] = 116.0f;

	b1 /= 2.0f;

	EXPECT_FLOAT_EQ(2.0f, b1[0]);
	EXPECT_FLOAT_EQ(4.0f, b1[1]);
	EXPECT_FLOAT_EQ(58.0f, b1[2]);
}

TEST(UniqueBufferTests, ConvertCopy_LargerToSmaller)
{
    unique_buffer<float> b1(3);

    b1[0] = 123.0f;
    b1[2] = 999.0f;

    unique_buffer<unsigned char> b2 = b1.convert_copy<unsigned char>();

    EXPECT_EQ(3u, b1.size());
    EXPECT_EQ(123.0f, b1[0]);
    EXPECT_EQ(999.0f, b1[2]);

    b1[0] = 0.0f;
    b1[2] = 0.0f;

    ASSERT_EQ(3u * sizeof(float), b2.size());
    EXPECT_EQ(123.0f, (reinterpret_cast<float *>(b2.get()))[0]);
    EXPECT_EQ(999.0f, (reinterpret_cast<float *>(b2.get()))[2]);
}

TEST(UniqueBufferTests, ConvertMove_LargerToSmaller)
{
    unique_buffer<float> b1(3);

    b1[0] = 123.0f;
    b1[2] = 999.0f;

    unique_buffer<unsigned char> b2 = b1.convert_move<unsigned char>();

    EXPECT_EQ(0u, b1.size());

    ASSERT_EQ(3u * sizeof(float), b2.size());
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

    EXPECT_EQ(4u, b1.size());

    ASSERT_EQ(1u, b2.size());
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

    EXPECT_EQ(0u, b1.size());

    ASSERT_EQ(1u, b2.size());
    EXPECT_EQ(uint32_t(0x01010101), b2[0]);

}