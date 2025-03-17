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

/////////////////////////////////////////////////////////////////////////////////////////////////

TEST(MultiProviderVertexBufferTests, OneProvider_Clean_Append)
{
    MultiProviderVertexBuffer<TestVertexAttributes, 1> buffer;

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    EXPECT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, {1.0f, 10.0f});
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Cleaned_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Changes_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 3.0f, 30.0f });
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));
    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Grows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 3.0f, 30.0f });
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Shrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 3.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));
    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_Empties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 3.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Initd_Append)
{
    MultiProviderVertexBuffer<TestVertexAttributes, 2> buffer;

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    EXPECT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 3);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstGrows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendVertex(0, { 6.0f, 60.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstShrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 1);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstEmpties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 1u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_FirstEmpties_SecondChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 1u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 4.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondGrows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendVertex(1, { 5.0f, 50.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondShrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 5.0f, 50.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_SecondEmpties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 0);
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothChange_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 20);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 10);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 20);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 10);
    buffer.AppendVertex(1, { 6.0f, 60.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_BothGrow_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 4.0f, 40.0f });
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendVertex(0, { 6.0f, 60.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 7.0f, 70.0f });
    buffer.AppendVertex(1, { 8.0f, 80.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, TwoProviders_BothShrink_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 1);
    buffer.AppendVertex(0, { 5.0f, 50.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 6.0f, 60.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 6.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_BothEmpty_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 0);
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 0u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Elements_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 4);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstGrows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 3);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendVertex(0, { 9.0f, 90.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstShrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 1);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_FirstEmpties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 0);
    buffer.AppendEnd(0);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 7.0f, 70.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondGrows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 7.0f, 70.0f });
    buffer.AppendVertex(1, { 8.0f, 80.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondShrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendVertex(2, { 7.0f, 70.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 8.0f, 80.0f });
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_SecondEmpties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(1, 0);
    buffer.AppendEnd(1);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdChanges_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 7.0f, 70.0f });
    buffer.AppendVertex(2, { 8.0f, 80.0f });
    buffer.AppendVertex(2, { 9.0f, 90.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdGrows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(2, 4);
    buffer.AppendVertex(2, { 7.0f, 70.0f });
    buffer.AppendVertex(2, { 8.0f, 80.0f });
    buffer.AppendVertex(2, { 9.0f, 90.0f });
    buffer.AppendVertex(2, { 10.0f, 100.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdShrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 2);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 4.0f, 40.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendVertex(2, { 7.0f, 70.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(2, 1);
    buffer.AppendVertex(2, { 8.0f, 80.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 4u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 8.0f);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_ThirdEmpties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(2, 0);
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Change_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 9.0f, 90.0f });
    buffer.AppendVertex(2, { 10.0f, 100.0f });
    buffer.AppendVertex(2, { 11.0f, 110.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Grows_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(2, 4);
    buffer.AppendVertex(2, { 9.0f, 90.0f });
    buffer.AppendVertex(2, { 10.0f, 100.0f });
    buffer.AppendVertex(2, { 11.0f, 110.0f });
    buffer.AppendVertex(2, { 12.0f, 120.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Shrinks_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(2, 2);
    buffer.AppendVertex(2, { 9.0f, 90.0f });
    buffer.AppendVertex(2, { 10.0f, 100.0f });
    buffer.AppendEnd(2);

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

TEST(MultiProviderVertexBufferTests, ThreeProviders_Change_NoDirty_Empties_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 3>;
    TBuf buffer;

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 1);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.AppendStart(2, 3);
    buffer.AppendVertex(2, { 4.0f, 40.0f });
    buffer.AppendVertex(2, { 5.0f, 50.0f });
    buffer.AppendVertex(2, { 6.0f, 60.0f });
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.AppendStart(0, 2);
    buffer.AppendVertex(0, { 7.0f, 70.0f });
    buffer.AppendVertex(0, { 8.0f, 80.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(2, 0);
    buffer.AppendEnd(2);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 3u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 7.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 8.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
}

TEST(MultiProviderVertexBufferTests, Large_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 9);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 7);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.AppendStart(1, 7);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 16u);
}

TEST(MultiProviderVertexBufferTests, NotDirty_Append)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.AppendStart(0, 9);
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 1.0f, 10.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendVertex(0, { 2.0f, 20.0f });
    buffer.AppendEnd(0);

    buffer.AppendStart(1, 7);
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendVertex(1, { 3.0f, 30.0f });
    buffer.AppendEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 16u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

TEST(MultiProviderVertexBufferTests, OneProvider_UpdateStart_FromInitToSize_UpdateElements_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_NoUpdate_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_UpdateElements_Prefix_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 10.0f, 100.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_UpdateElements_Suffix_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 1u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 20.0f); // This is after offset
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_UpdateStart_FromSizeToSizeLarger_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 3);
    buffer.UpdateVertex(0, 2, { 3.0f, 30.0f });
    buffer.UpdateEnd(0);

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

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_UpdateStart_FromSizeToSizeSmaller_NoUpdate_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 1);
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 1u);
    ASSERT_EQ(buffer.TestActions.size(), 0u);
}

TEST(MultiProviderVertexBufferTests, OneProvider_Elements_UpdateStart_FromSizeToSizeSmaller_WithUpdate_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 1>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 1);
    buffer.UpdateVertex(0, 0, { 10.0f, 1000.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 1u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_UpdateStart_FromInitToSize_UpdateElements_First_SecondEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_UpdateStart_FromInitToSize_UpdateElements_First_SecondNonEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(1, 2);
    buffer.UpdateVertex(1, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(1, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 3);
    buffer.UpdateVertex(0, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(0, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(0, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 5u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 5.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements_UpdateElements_First_SecondEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 0);
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 10.0f, 100.0f });
    buffer.UpdateVertex(0, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 20.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements_UpdateElements_First_SecondNonEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 10.0f, 100.0f });
    buffer.UpdateVertex(0, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 20.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_UpdateStart_FromInitToSize_UpdateElements_Second_FirstEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(1, 2);
    buffer.UpdateVertex(1, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(1, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_UpdateStart_FromInitToSize_UpdateElements_Second_FirstNonEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, {3.0f, 30.0f});
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::AllocateAndUploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 5u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 1.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 4.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[4].foo1, 5.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements_UpdateElements_Second_FirstEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 0);
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 2);
    buffer.UpdateVertex(1, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(1, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(1, 2);
    buffer.UpdateVertex(1, 0, { 10.0f, 100.0f });
    buffer.UpdateVertex(1, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 2u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 20.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_Elements_UpdateElements_Second_FirstNonEmpty_Update)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(1, 2);
    buffer.UpdateVertex(1, 0, { 30.0f, 300.0f });
    buffer.UpdateVertex(1, 1, { 40.0f, 400.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 4u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 2u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 30.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 40.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_PrefixOfFirst)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 10.0f, 100.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_SuffixOfFirst)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(0);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 1u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 20.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_PrefixOfSecond)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 30.0f, 300.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 2u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 30.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_SuffixOfSecond)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 2, { 50.0f, 500.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 4u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 1u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 50.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_PrefixOfFirst_PrefixOfSecond)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 10.0f, 100.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 30.0f, 300.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 0u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 3u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 10.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 2.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 30.0f);
}

TEST(MultiProviderVertexBufferTests, TwoProviders_PartialUpdates_SuffixOfFirst_SuffixOfSecond)
{
    using TBuf = MultiProviderVertexBuffer<TestVertexAttributes, 2>;
    TBuf buffer;

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 0, { 1.0f, 10.0f });
    buffer.UpdateVertex(0, 1, { 2.0f, 20.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 0, { 3.0f, 30.0f });
    buffer.UpdateVertex(1, 1, { 4.0f, 40.0f });
    buffer.UpdateVertex(1, 2, { 5.0f, 50.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    buffer.TestActions.clear();

    buffer.UpdateStart(0, 2);
    buffer.UpdateVertex(0, 1, { 20.0f, 200.0f });
    buffer.UpdateEnd(0);

    buffer.UpdateStart(1, 3);
    buffer.UpdateVertex(1, 1, { 40.0f, 400.0f });
    buffer.UpdateVertex(1, 2, { 50.0f, 500.0f });
    buffer.UpdateEnd(1);

    buffer.RenderUpload();

    EXPECT_EQ(buffer.GetTotalVertexCount(), 5u);
    ASSERT_EQ(buffer.TestActions.size(), 1u);

    EXPECT_EQ(buffer.TestActions[0].Action, TBuf::TestAction::ActionKind::UploadVBO);
    EXPECT_EQ(buffer.TestActions[0].Offset, 1u * sizeof(TestVertexAttributes));
    ASSERT_EQ(buffer.TestActions[0].Size, 4u * sizeof(TestVertexAttributes));

    EXPECT_EQ(buffer.TestActions[0].Pointer[0].foo1, 20.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[1].foo1, 3.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[2].foo1, 40.0f);
    EXPECT_EQ(buffer.TestActions[0].Pointer[3].foo1, 50.0f);
}

// TODO:

// TwoProviders_Elements_UpdateStart_FromInitToSize_First_Update
// TwoProviders_Elements_UpdateStart_FromSizeToSizeLarger_First_Update // Ensure everything following is uploaded
// TwoProviders_Elements_UpdateStart_FromSizeToSizeSmaller_First_Update // Ensure everything following is uploaded
// TwoProviders_Elements_UpdateStart_FromInitToSize_Second_Update
// TwoProviders_Elements_UpdateStart_FromSizeToSizeLarger_Second_Update
// TwoProviders_Elements_UpdateStart_FromSizeToSizeSmaller_Second_Update

/////////////////////////////

// TwoProviders: first appends, second updates
// TwoProviders: first updates, second appends
