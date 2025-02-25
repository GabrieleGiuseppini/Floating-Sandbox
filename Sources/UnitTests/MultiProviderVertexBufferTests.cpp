#define MULTI_PROVIDER_VERTEX_BUFFER_TEST

#include <OpenGLCore/MultiProviderVertexBuffer.h>

#include "gtest/gtest.h"

struct TestVertexAttributes
{
    float foo1;
    float foo2;
};

TEST(MultiProviderVertexBufferTests, OneProvider_Initd)
{
    MultiProviderVertexBuffer<TestVertexAttributes, 1> buffer;

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    EXPECT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Cleaned)
{
    MultiProviderVertexBuffer<TestVertexAttributes, 1> buffer;

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    EXPECT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, {1.0f, 10.0f});
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Cleaned)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Changes)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 3.0f, 30.0f });
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));
    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Grows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 3.0f, 30.0f });
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));
    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Shrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadVertex(0, { 3.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));
    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Empties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadVertex(0, { 3.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Initd)
{
    MultiProviderVertexBuffer<TestVertexAttributes, 2> buffer;

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    EXPECT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstChanges)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstGrows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadVertex(0, { 6.0f, 60.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 4u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 4u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 6.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstShrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 1);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstEmpties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 1u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondChanges)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondGrows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadVertex(1, { 5.0f, 50.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 4u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 4u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondShrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 5.0f, 50.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondEmpties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 0);
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothChange)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 6.0f, 60.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothGrow)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 4.0f, 40.0f });
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadVertex(0, { 6.0f, 60.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 7.0f, 70.0f });
    buffer.UploadVertex(1, { 8.0f, 80.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 5u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 6.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 8.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothShrink)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 1);
    buffer.UploadVertex(0, { 5.0f, 50.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 6.0f, 60.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothEmpty)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 0);
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Elements)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 6u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstChanges)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    // Not uploaded
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstGrows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 3);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadVertex(0, { 9.0f, 90.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 7u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 7u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 9.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[6].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstShrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 1);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 5u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstEmpties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 0);
    buffer.UploadEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 4u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 4u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondChanges)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 7.0f, 70.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondGrows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 7.0f, 70.0f });
    buffer.UploadVertex(1, { 8.0f, 80.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 7u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 7u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[6].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondShrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadVertex(2, { 7.0f, 70.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 8.0f, 80.0f });
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 4u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 6.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 7.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondEmpties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(1, 0);
    buffer.UploadEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdChanges)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 7.0f, 70.0f });
    buffer.UploadVertex(2, { 8.0f, 80.0f });
    buffer.UploadVertex(2, { 9.0f, 90.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 3u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 9.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdGrows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(2, 4);
    buffer.UploadVertex(2, { 7.0f, 70.0f });
    buffer.UploadVertex(2, { 8.0f, 80.0f });
    buffer.UploadVertex(2, { 9.0f, 90.0f });
    buffer.UploadVertex(2, { 10.0f, 100.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 7u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 7u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 9.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[6].foo1, 10.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdShrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 2);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadVertex(1, { 4.0f, 40.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadVertex(2, { 7.0f, 70.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(2, 1);
    buffer.UploadVertex(2, { 8.0f, 80.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 4u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 8.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdEmpties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(2, 0);
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Change)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 9.0f, 90.0f });
    buffer.UploadVertex(2, { 10.0f, 100.0f });
    buffer.UploadVertex(2, { 11.0f, 110.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 6u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 6u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 9.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 11.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Grows)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(2, 4);
    buffer.UploadVertex(2, { 9.0f, 90.0f });
    buffer.UploadVertex(2, { 10.0f, 100.0f });
    buffer.UploadVertex(2, { 11.0f, 110.0f });
    buffer.UploadVertex(2, { 12.0f, 120.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 7u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 7u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 9.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[5].foo1, 11.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[6].foo1, 12.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Shrinks)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(2, 2);
    buffer.UploadVertex(2, { 9.0f, 90.0f });
    buffer.UploadVertex(2, { 10.0f, 100.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 5u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 9.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 10.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Empties)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 1.0f, 10.0f });
    buffer.UploadVertex(0, { 2.0f, 20.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(1, 1);
    buffer.UploadVertex(1, { 3.0f, 30.0f });
    buffer.UploadEnd(1);

    buffer.UploadStart(2, 3);
    buffer.UploadVertex(2, { 4.0f, 40.0f });
    buffer.UploadVertex(2, { 5.0f, 50.0f });
    buffer.UploadVertex(2, { 6.0f, 60.0f });
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UploadStart(0, 2);
    buffer.UploadVertex(0, { 7.0f, 70.0f });
    buffer.UploadVertex(0, { 8.0f, 80.0f });
    buffer.UploadEnd(0);

    buffer.UploadStart(2, 0);
    buffer.UploadEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
}
