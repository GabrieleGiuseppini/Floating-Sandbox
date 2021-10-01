#include <GameCore/DeSerializationBuffer.h>

#include "gtest/gtest.h"

#include <cstring>

#pragma pack(push, 1)

struct TestElement
{
    int a;
    int b;
    int c;
};

#pragma pack(pop)

// TODO: copies with growth

TEST(DeSerializationBufferTests, ReserveAndAdvance)
{
    DeSerializationBuffer b(4);

    ASSERT_EQ(b.GetSize(), 0);

    // Simulate writing a tag

    // 1

    size_t indexStart1 = b.ReserveAndAdvance<TestElement>();

    ASSERT_EQ(b.GetSize(), sizeof(TestElement));

    TestElement & targetElement1 = b.GetAs<TestElement>(indexStart1);
    targetElement1.a = 4;
    targetElement1.b = 5;
    targetElement1.c = 8193;

    // 2

    size_t indexStart2 = b.ReserveAndAdvance<TestElement>();

    ASSERT_EQ(b.GetSize(), 2 * sizeof(TestElement));

    TestElement & targetElement2 = b.GetAs<TestElement>(indexStart2);
    targetElement2.a = 14;
    targetElement2.b = 15;
    targetElement2.c = 18193;

    // Verify

    // 1

    TestElement const * verifyElement1 = reinterpret_cast<TestElement const *>(b.GetData());

    EXPECT_EQ(verifyElement1->a, 4);
    EXPECT_EQ(verifyElement1->b, 5);
    EXPECT_EQ(verifyElement1->c, 8193);

    // 2

    TestElement const * verifyElement2 = reinterpret_cast<TestElement const *>(b.GetData() + sizeof(TestElement));

    EXPECT_EQ(verifyElement2->a, 14);
    EXPECT_EQ(verifyElement2->b, 15);
    EXPECT_EQ(verifyElement2->c, 18193);
}

TEST(DeSerializationBufferTests, Append_Struct)
{
    DeSerializationBuffer b(512);

    // Simulate writing an image

    // 1

    TestElement element1{ 2, 3, 109 };

    b.Append(element1);

    // 2

    TestElement element2{ 12, 13, 1109 };

    b.Append(element2);

    ASSERT_EQ(b.GetSize(), 2 * sizeof(TestElement));

    // Verify

    // 1

    TestElement const * verifyElement1 = reinterpret_cast<TestElement const *>(b.GetData());

    EXPECT_EQ(verifyElement1->a, 2);
    EXPECT_EQ(verifyElement1->b, 3);
    EXPECT_EQ(verifyElement1->c, 109);

    // 2

    TestElement const * verifyElement2 = reinterpret_cast<TestElement const *>(b.GetData() + sizeof(TestElement));

    EXPECT_EQ(verifyElement2->a, 12);
    EXPECT_EQ(verifyElement2->b, 13);
    EXPECT_EQ(verifyElement2->c, 1109);
}

TEST(DeSerializationBufferTests, Append_Bytes)
{
    DeSerializationBuffer b(512);

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

TEST(DeSerializationBufferTests, Receive)
{
    DeSerializationBuffer b(512);

    // Simulate writing an image

    // 1

    unsigned char testData1[] = { 2, 3, 8, 9 };
    std::memcpy(b.Receive(4), testData1, 4);

    // 2

    unsigned char testData2[] = { 22, 23, 28, 29 };
    std::memcpy(b.Receive(4), testData2, 4);

    // Verify

    ASSERT_EQ(b.GetSize(), 8);

    // 1

    EXPECT_EQ(b.GetData()[0], 2);
    EXPECT_EQ(b.GetData()[1], 3);
    EXPECT_EQ(b.GetData()[2], 8);
    EXPECT_EQ(b.GetData()[3], 9);

    // 2

    EXPECT_EQ(b.GetData()[4], 22);
    EXPECT_EQ(b.GetData()[5], 23);
    EXPECT_EQ(b.GetData()[6], 28);
    EXPECT_EQ(b.GetData()[7], 29);
}