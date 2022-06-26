#include <GameCore/Buffer2D.h>

#include "gtest/gtest.h"

TEST(Buffer2DTests, FillCctor_Size)
{
    Buffer2D<int, struct IntegralTag> buffer(IntegralRectSize(10, 20), 242);

    EXPECT_EQ(buffer.Size.width, 10);
    EXPECT_EQ(buffer.Size.height, 20);

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, FillCctor_Dimensions)
{
    Buffer2D<int, struct IntegralTag> buffer(10, 20, 242);

    EXPECT_EQ(buffer.Size.width, 10);
    EXPECT_EQ(buffer.Size.height, 20);

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Indexing_WithCoordinates)
{
    Buffer2D<int, struct IntegralTag> buffer(10, 20, 242);

    buffer[IntegralCoordinates(7, 9)] = 42;

    EXPECT_EQ(buffer[IntegralCoordinates(0, 0)], 242);
    EXPECT_EQ(buffer[IntegralCoordinates(7, 9)], 42);
    EXPECT_EQ(buffer[IntegralCoordinates(9, 19)], 242);
}

TEST(Buffer2DTests, Clone_Whole)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferClone = buffer.Clone();
    iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(bufferClone[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Clone_Region)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const bufferClone = buffer.CloneRegion(
        IntegralRect(
            { 1, 1 },
            { 2, 2 }));

    ASSERT_EQ(IntegralRectSize(2, 2), bufferClone.Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 1) * 4 + 1;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ(bufferClone[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Trim_Equal)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Trim(
        IntegralRect(
            { 0, 0 },
            { 4, 4 }));

    ASSERT_EQ(IntegralRectSize(4, 4), buffer.Size);

    iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Trim_Smaller_SameOrigin)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Trim(
        IntegralRect(
            { 0, 0 },
            { 2, 2 }));

    ASSERT_EQ(IntegralRectSize(2, 2), buffer.Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + y * 4;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Trim_Smaller_SameSize)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Trim(
        IntegralRect(
            { 2, 2 },
            { 2, 2 }));

    ASSERT_EQ(IntegralRectSize(2, 2), buffer.Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 2) * 4 + 2;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Trim_ProperInside)
{
    Buffer2D<int, struct IntegralTag> buffer(4, 4, 0);

    int iVal = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Trim(
        IntegralRect(
            { 1, 1 },
            { 2, 2 }));

    ASSERT_EQ(IntegralRectSize(2, 2), buffer.Size);

    for (int y = 0; y < 2; ++y)
    {
        iVal = 100 + (y + 1) * 4 + 1;
        for (int x = 0; x < 2; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_WholeSource_ToOrigin)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(2, 3);
    int val = 100;
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {0, 0}, sourceBuffer.Size },
        { 0, 0 });

    // Verify
    val = 100;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x >= 2 || y >= 3)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = val++;
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_WholeSource_ToOffset)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(2, 3);
    int val = 100;
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {0, 0}, sourceBuffer.Size },
        { 4, 7 });

    // Verify
    val = 100;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x < 4 || x >= 4 + 2 || y < 7 || y >= 7 + 3)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = val++;
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, BlitFromRegion_PortionOfSource_ToOffset)
{
    // Prepare source
    Buffer2D<int, struct IntegralTag> sourceBuffer(4, 4);
    int val = 100;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            sourceBuffer[{x, y}] = val++;
        }
    }

    // Prepare target
    Buffer2D<int, struct IntegralTag> targetBuffer(10, 20, 242);

    // Blit
    targetBuffer.BlitFromRegion(
        sourceBuffer,
        { {1, 1}, {2, 2} },
        { 4, 7 });

    // Verify
    int expectedVals[] = { 105, 106, 109, 110 };
    int iExpectedVal = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            int expectedValue;
            if (x < 4 || x >= 4 + 2 || y < 7 || y >= 7 + 2)
            {
                expectedValue = 242;
            }
            else
            {
                expectedValue = expectedVals[iExpectedVal++];
            }

            EXPECT_EQ(targetBuffer[IntegralCoordinates(x, y)], expectedValue);
        }
    }
}

TEST(Buffer2DTests, Flip_Horizontal)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Horizontal);

    iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = buffer.Size.width - 1; x >= 0; --x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}


TEST(Buffer2DTests, Flip_Vertical)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Vertical);

    iVal = 100;
    for (int y = buffer.Size.height - 1; y >= 0; --y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, Flip_HorizontalAndVertical)
{
    Buffer2D<int, struct IntegralTag> buffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < buffer.Size.height; ++y)
    {
        for (int x = 0; x < buffer.Size.width; ++x)
        {
            buffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    buffer.Flip(DirectionType::Horizontal | DirectionType::Vertical);

    iVal = 100;
    for (int y = buffer.Size.height - 1; y >= 0; --y)
    {
        for (int x = buffer.Size.width - 1; x >= 0; --x)
        {
            EXPECT_EQ(buffer[IntegralCoordinates(x, y)], iVal);
            ++iVal;
        }
    }
}

TEST(Buffer2DTests, MakeReframed_SameRect)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        sourceBuffer.Size,
        { 0, 0 },
        999999);

    ASSERT_EQ(targetBuffer.Size, sourceBuffer.Size);
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords]);
        }
    }
}

TEST(Buffer2DTests, MakeReframed_SameOrigin_SmallerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 8);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 6, 4 },
        { 0, 0 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(6, 4));
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 6; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords]);
        }
    }
}

TEST(Buffer2DTests, MakeReframed_SameOrigin_LargerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 7);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 10, 12 },
        { 0, 0 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(10, 12));
    for (int y = 0; y < 12; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            if (x < 8 && y < 7)
            {
                EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords]);
            }
            else
            {
                EXPECT_EQ(targetBuffer[coords], 999999);
            }
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_OutOrigin_SameSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 6, 5 },
        { -2, -1 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(6, 5));
    for (int y = 0; y < 5; ++y)
    {
        for (int x = 0; x < 6; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords + IntegralRectSize(2, 1)]);
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_OutOrigin_SmallerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 5, 4 },
        { -1, -2 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(5, 4));
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 5; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords + IntegralRectSize(1, 2)]);
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_OutOrigin_LargerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 8, 6 },
        { -1, -2 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(8, 6));
    for (int y = 0; y < 6; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            if (x < 7 && y < 4)
            {
                EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords + IntegralRectSize(1, 2)]);
            }
            else
            {
                EXPECT_EQ(targetBuffer[coords], 999999);
            }
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_InOrigin_SameSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 6, 5 },
        { 2, 1 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(6, 5));
    for (int y = 0; y < 5; ++y)
    {
        for (int x = 0; x < 6; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            if (x < 2 || y < 1)
            {
                EXPECT_EQ(targetBuffer[coords], 999999);
            }
            else
            {
                EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords - IntegralRectSize(2, 1)]);
            }
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_InOrigin_SmallerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 4, 3 },
        { 2, 1 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(4, 3));
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            if (x < 2 || y < 1)
            {
                EXPECT_EQ(targetBuffer[coords], 999999);
            }
            else
            {
                EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords - IntegralRectSize(2, 1)]);
            }
        }
    }
}

TEST(Buffer2DTests, MakeReframed_DifferentOrigin_InOrigin_LargerSize)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 12, 16 },
        { 2, 1 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(12, 16));
    for (int y = 0; y < 16; ++y)
    {
        for (int x = 0; x < 12; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            if (x < 2 || y < 1 || x >= 10 || y >= 7)
            {
                EXPECT_EQ(targetBuffer[coords], 999999);
            }
            else
            {
                EXPECT_EQ(targetBuffer[coords], sourceBuffer[coords - IntegralRectSize(2, 1)]);
            }
        }
    }
}

TEST(Buffer2DTests, MakeReframed_BecomesEmpty)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.MakeReframed(
        { 12, 16 },
        { -8, -6 },
        999999);

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(12, 16));
    for (int y = 0; y < 16; ++y)
    {
        for (int x = 0; x < 12; ++x)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(targetBuffer[coords], 999999);
        }
    }
}

class Rotate90CWTest : public testing::TestWithParam<std::tuple<int, int>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    Buffer2DTests,
    Rotate90CWTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 2),
        std::make_tuple(2, 1),
        std::make_tuple(2, 2),
        std::make_tuple(3, 2),
        std::make_tuple(2, 3),
        std::make_tuple(3, 3)
    ));

TEST_P(Rotate90CWTest, Rotate90CWTest)
{
    int const originalWidth = std::get<0>(GetParam());
    int const originalHeight = std::get<1>(GetParam());

    //
    // Prepare source buffer
    //

    Buffer2D<int, struct IntegralTag> buffer(originalWidth, originalHeight);

    int iVal = 0;
    for (int y = 0; y < originalHeight; ++y)
    {
        for (int x = 0; x < originalWidth; ++x)
        {
            buffer[{x, y}] = iVal++;
        }
    }

    //
    // Rotate
    //

    buffer.Rotate90(RotationDirectionType::Clockwise);

    //
    // Verify
    //

    int const newWidth = originalHeight;
    int const newHeight = originalWidth;

    ASSERT_EQ(buffer.Size.width, newWidth);
    ASSERT_EQ(buffer.Size.height, newHeight);

    iVal = 0;
    for (int x = 0; x < newWidth; ++x)
    {
        for (int y = newHeight - 1; y >= 0; --y)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(buffer[coords], iVal);

            ++iVal;
        }
    }
}

class Rotate90CCWTest : public testing::TestWithParam<std::tuple<int, int>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    Buffer2DTests,
    Rotate90CCWTest,
    ::testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(1, 2),
        std::make_tuple(2, 1),
        std::make_tuple(2, 2),
        std::make_tuple(3, 2),
        std::make_tuple(2, 3),
        std::make_tuple(3, 3)
    ));

TEST_P(Rotate90CCWTest, Rotate90CCWTest)
{
    int const originalWidth = std::get<0>(GetParam());
    int const originalHeight = std::get<1>(GetParam());

    //
    // Prepare source buffer
    //

    Buffer2D<int, struct IntegralTag> buffer(originalWidth, originalHeight);

    int iVal = 0;
    for (int y = 0; y < originalHeight; ++y)
    {
        for (int x = 0; x < originalWidth; ++x)
        {
            buffer[{x, y}] = iVal++;
        }
    }

    //
    // Rotate
    //

    buffer.Rotate90(RotationDirectionType::CounterClockwise);

    //
    // Verify
    //

    int const newWidth = originalHeight;
    int const newHeight = originalWidth;

    ASSERT_EQ(buffer.Size.width, newWidth);
    ASSERT_EQ(buffer.Size.height, newHeight);

    iVal = 0;
    for (int x = newWidth - 1; x >= 0; --x)
    {
        for (int y = 0; y < newHeight; ++y)
        {
            auto const coords = IntegralCoordinates(x, y);
            EXPECT_EQ(buffer[coords], iVal);

            ++iVal;
        }
    }
}

TEST(Buffer2DTests, MakeTransformed)
{
    Buffer2D<int, struct IntegralTag> sourceBuffer(8, 6);

    int iVal = 100;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[IntegralCoordinates(x, y)] = iVal++;
        }
    }

    auto const targetBuffer = sourceBuffer.Transform<std::tuple<float, float>>(
        [](int const & value)
        {
            return std::tuple<float, float>(static_cast<float>(value), static_cast<float>(value * 2));
        });

    ASSERT_EQ(targetBuffer.Size, IntegralRectSize(8, 6));
    
    float fVal = 100.0;
    for (int y = 0; y < targetBuffer.Size.height; ++y)
    {
        for (int x = 0; x < targetBuffer.Size.width; ++x)
        {
            EXPECT_EQ(std::get<0>(targetBuffer[IntegralCoordinates(x, y)]), fVal);
            EXPECT_EQ(std::get<1>(targetBuffer[IntegralCoordinates(x, y)]), fVal * 2.0f);

            fVal += 1.0f;
        }
    }
}
