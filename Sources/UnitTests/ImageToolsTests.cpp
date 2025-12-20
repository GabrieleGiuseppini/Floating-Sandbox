#include <Core/ImageTools.h>

#include "gtest/gtest.h"

TEST(ImageToolsTests, Resize_Smaller_Nearest_1)
{
    RgbImageData sourceImage(2, 2);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(1, 1),
        ImageTools::FilterKind::Nearest);

    ASSERT_EQ(destImage.Size.width, 1);
    ASSERT_EQ(destImage.Size.height, 1);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(1, 1, 4));
}

TEST(ImageToolsTests, Resize_Smaller_Nearest_Any)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(3, 3),
        ImageTools::FilterKind::Nearest);

    ASSERT_EQ(destImage.Size.width, 3);
    ASSERT_EQ(destImage.Size.height, 3);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(0, 0, 4));
    c = destImage[{1, 0}];
    EXPECT_EQ(c, rgbColor(2, 0, 4));
    c = destImage[{2, 0}];
    EXPECT_EQ(c, rgbColor(3, 0, 4));

    c = destImage[{0, 1}];
    EXPECT_EQ(c, rgbColor(0, 2, 4));
}

TEST(ImageToolsTests, Resize_Larger_Nearest)
{
    RgbImageData sourceImage(2, 2);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(4, 4),
        ImageTools::FilterKind::Nearest);

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 4);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(0, 0, 4));
    c = destImage[{1, 0}];
    EXPECT_EQ(c, rgbColor(0, 0, 4));
    c = destImage[{2, 0}];
    EXPECT_EQ(c, rgbColor(1, 0, 4));
    c = destImage[{3, 0}];
    EXPECT_EQ(c, rgbColor(1, 0, 4));

    c = destImage[{0, 1}];
    EXPECT_EQ(c, rgbColor(0, 0, 4));
    c = destImage[{0, 2}];
    EXPECT_EQ(c, rgbColor(0, 1, 4));
    c = destImage[{0, 3}];
    EXPECT_EQ(c, rgbColor(0, 1, 4));
}

TEST(ImageToolsTests, Resize_Idempotent_Nearest)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(4, 4),
        ImageTools::FilterKind::Nearest);

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 4);

    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            auto cd = destImage[{x, y}];
            auto cs = sourceImage[{x, y}];
            EXPECT_EQ(cd, cs);
        }
    }
}

TEST(ImageToolsTests, Resize_Smaller_Bilinear_1)
{
    RgbImageData sourceImage(2, 2);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x * 10, y * 100, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(1, 1),
        ImageTools::FilterKind::Bilinear);

    ASSERT_EQ(destImage.Size.width, 1);
    ASSERT_EQ(destImage.Size.height, 1);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(5, 50, 4));
}

TEST(ImageToolsTests, Resize_Smaller_Bilinear_2)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x * 10, y * 40, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(3, 3),
        ImageTools::FilterKind::Bilinear);

    ASSERT_EQ(destImage.Size.width, 3);
    ASSERT_EQ(destImage.Size.height, 3);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(2, 7, 4));

    c = destImage[{1, 1}];
    EXPECT_EQ(c, rgbColor(15, 60, 4));

    c = destImage[{2, 2}];
    EXPECT_EQ(c, rgbColor(28, 113, 4));
}

TEST(ImageToolsTests, Resize_Larger_Bilinear)
{
    RgbImageData sourceImage(2, 2);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x * 10, y * 100, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(4, 4),
        ImageTools::FilterKind::Bilinear);

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 4);

    auto c = destImage[{0, 0}];
    EXPECT_EQ(c, rgbColor(0, 0, 4));
    c = destImage[{1, 0}];
    EXPECT_EQ(c, rgbColor(3, 0, 4));
    c = destImage[{2, 0}];
    EXPECT_EQ(c, rgbColor(8, 0, 4));
    c = destImage[{3, 0}];
    EXPECT_EQ(c, rgbColor(10, 0, 4));

    c = destImage[{0, 1}];
    EXPECT_EQ(c, rgbColor(0, 25, 4));
    c = destImage[{1, 1}];
    EXPECT_EQ(c, rgbColor(3, 25, 4));
    c = destImage[{2, 1}];
    EXPECT_EQ(c, rgbColor(8, 25, 4));
    c = destImage[{3, 1}];
    EXPECT_EQ(c, rgbColor(10, 25, 4));

    c = destImage[{0, 2}];
    EXPECT_EQ(c, rgbColor(0, 75, 4));
    c = destImage[{1, 2}];
    EXPECT_EQ(c, rgbColor(3, 75, 4));
    c = destImage[{2, 2}];
    EXPECT_EQ(c, rgbColor(8, 75, 4));
    c = destImage[{3, 2}];
    EXPECT_EQ(c, rgbColor(10, 75, 4));

    c = destImage[{0, 3}];
    EXPECT_EQ(c, rgbColor(0, 100, 4));
    c = destImage[{1, 3}];
    EXPECT_EQ(c, rgbColor(3, 100, 4));
    c = destImage[{2, 3}];
    EXPECT_EQ(c, rgbColor(8, 100, 4));
    c = destImage[{3, 3}];
    EXPECT_EQ(c, rgbColor(10, 100, 4));
}

TEST(ImageToolsTests, Resize_Idempotent_Bilinear)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x * 10, y * 40, 4);
        }
    }

    RgbImageData destImage = ImageTools::Resize(
        sourceImage,
        ImageSize(4, 4),
        ImageTools::FilterKind::Bilinear);

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 4);

    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            auto cd = destImage[{x, y}];
            auto cs = sourceImage[{x, y}];
            EXPECT_EQ(cd, cs);
        }
    }
}

/////////////////////////// Nicer

TEST(ImageToolsTests, ResizeNicer_IdempotentBothDirs)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(4, 4));

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 4);

    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            auto cd = destImage[{x, y}];
            auto cs = sourceImage[{x, y}];
            EXPECT_EQ(cd, cs);
        }
    }
}

TEST(ImageToolsTests, ResizeNicer_IdempotentW)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(4, 2));

    ASSERT_EQ(destImage.Size.width, 4);
    ASSERT_EQ(destImage.Size.height, 2);

    for (uint8_t y = 0; y < destImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < destImage.Size.width; ++x)
        {
            auto cd = destImage[{x, y}];
            auto cs1 = sourceImage[{x, y * 2}];
            auto cs2 = sourceImage[{x, y * 2 + 1}];
            EXPECT_EQ(cd, rgbColor((cs1.toVec() + cs2.toVec()) / 2.0f));
        }
    }
}

TEST(ImageToolsTests, ResizeNicer_IdempotentH)
{
    RgbImageData sourceImage(4, 4);
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbColor(x, y, 4);
        }
    }

    RgbImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(2, 4));

    ASSERT_EQ(destImage.Size.width, 2);
    ASSERT_EQ(destImage.Size.height, 4);

    for (uint8_t y = 0; y < destImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < destImage.Size.width; ++x)
        {
            auto cd = destImage[{x, y}];
            auto cs1 = sourceImage[{x * 2, y}];
            auto cs2 = sourceImage[{x * 2 + 1, y}];
            EXPECT_EQ(cd, rgbColor((cs1.toVec() + cs2.toVec()) / 2.0f));
        }
    }
}

TEST(ImageToolsTests, ResizeNicer_LargerW_LargerH)
{
    RgbaImageData sourceImage(4, 4);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(8, 8));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        10);

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            10 * 0.75f * 0.25f + 11 * 0.25f * 0.25f
            + 10 * 0.75f * 0.75f + 11 * 0.25f * 0.75f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            10 * 0.75f * 0.75f + 11 * 0.25f * 0.75f
            + 14 * 0.75f * 0.25f + 15 * 0.25f * 0.25f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(6, 6)].r),
        std::roundf(
            20 * 0.25f * 0.25f + 21 * 0.75f * 0.25f
            + 24 * 0.25f * 0.75f + 25 * 0.75f * 0.75f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(7, 7)].r),
        25);
}

TEST(ImageToolsTests, ResizeNicer_Smaller1W_LargerH)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(9, 24));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            (10 * 0.833333313f + 11 * 0.166666687f) * 0.25f
            + (10 * 0.833333313f + 11 * 0.166666687f) * 0.75f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            (11 * 0.5f + 12 * 0.5f) * 0.5f
            + (11 * 0.5f + 12 * 0.5f) * 0.5f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            (11 * 0.5f + 12 * 0.5f) * 0.75f
            + (23 * 0.5f + 24 * 0.5f) * 0.25f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(7, 22)].r),
        std::roundf(
            (139 * 0.5f + 140 * 0.5f) * 0.25f
            + (151 * 0.5f + 152 * 0.5f) * 0.75f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(8, 23)].r),
        std::roundf(
            (152 * 0.166f + 153 * 0.833f) * 0.75f
            + (152 * 0.166f + 153 * 0.833f) * 0.25f
        ));
}

TEST(ImageToolsTests, ResizeNicer_Smaller2W_LargerH)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(3, 24));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            (10 + 11 + 12 + 13) / 4.0f * 0.75f
            + (10 + 11 + 12 + 13) / 4.0f * 0.25f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            (14 + 15 + 16 + 17) / 4.0f * 0.75f
            + (14 + 15 + 16 + 17) / 4.0f * 0.25f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            (14 + 15 + 16 + 17) / 4.0f * 0.75f
            + (26 + 27 + 28 + 29) / 4.0f * 0.25f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 22)].r),
        std::roundf(
            (134 + 135 + 136 + 137) / 4.0f * 0.25f
            + (146 + 147 + 148 + 149) / 4.0f * 0.75f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(2, 23)].r),
        std::roundf(
            (150 + 151 + 152 + 153) / 4.0f * 0.75f
            + (150 + 151 + 152 + 153) / 4.0f * 0.25f
        ));
}

TEST(ImageToolsTests, ResizeNicer_LargerW_Smaller1H)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(24, 9));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            (10 * 0.75f + 10 * 0.25f) * 0.8333f
            + (22 * 0.75f + 23 * 0.25f) * 0.1666f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            (10 * 0.75f + 11 * 0.25f) * 0.8333f
            + (22 * 0.75f + 23 * 0.25f) * 0.1666f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            (22 * 0.75f + 23 * 0.25f) * 0.5f
            + (34 * 0.75f + 35 * 0.25f) * 0.5f
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(22, 7)].r),
        std::roundf(
            (128 * 0.25f + 129 * 0.75f) * 0.5f // 10, 11 - @9
            + (140 * 0.25f + 141 * 0.75f) * 0.5f //  10, 11 - @10
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(23, 8)].r),
        std::roundf(
            (141 * 0.75f + 141 * 0.25f) * 0.1666f // 11, 11 - @10
            + (153 * 0.75f + 153 * 0.25f) * 0.8333f //  11, 11 - @11
        ));
}

TEST(ImageToolsTests, ResizeNicer_Smaller1W_Smaller1H)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(9, 9));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            (10 * 0.833333313f + 11 * 0.166666687f) * 0.833333313f // 0, 1 @ 0
            + (22 * 0.833333313f + 23 * 0.166666687f) * 0.166666687f // 0, 1 @ 1
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            (11 * 0.5f + 12 * 0.5f) * 0.833333313f // 1, 2 @ 0
            + (23 * 0.5f + 24 * 0.5f) * 0.166666687f // 1, 2 @ 1
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            (23 * 0.5f + 24 * 0.5f) * 0.5f // 1, 2 @ 1
            + (35 * 0.5f + 36 * 0.5f) * 0.5f // 1, 2 @ 2
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(7, 7)].r),
        std::roundf(
            (127 * 0.5f + 128 * 0.5f) * 0.5f // 9, 10 - @9
            + (139 * 0.5f + 140 * 0.5f) * 0.5f //  9, 10 - @10
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(8, 8)].r),
        std::roundf(
            (140 * 0.166666687f + 141 * 0.833333313f) * 0.166666687f // 10, 11 - @10
            + (152 * 0.166666687f + 153 * 0.833333313f) * 0.833333313f //  10, 11 - @11
        ));
}

TEST(ImageToolsTests, ResizeNicer_Smaller2W_Smaller1H)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(3, 9));

    // Verify

    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            (10 + 11 + 12 + 13) / 4.0f * 0.833333313f // 0-3 @ 0
            + (22 + 23 + 24 + 25) / 4.0f * 0.166666687f // 0-3 @ 1
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            (14 + 15 + 16 + 17) / 4.0f * 0.833333313f // 4-7 @ 0
            + (26 + 27 + 28 + 29) / 4.0f * 0.166666687f // 4-7 @ 1
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            (26 + 27 + 28 + 29) / 4.0f * 0.5f // 4-7 @ 1
            + (38 + 39 + 40 + 41) / 4.0f * 0.5f // 4-7 @ 2
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 7)].r),
        std::roundf(
            (122 + 123 + 124 + 125) / 4.0f * 0.5f // 4-7 @ 9
            + (134 + 135 + 136 + 137) / 4.0f * 0.5f // 4-7 @ 10
        ));

    EXPECT_EQ(
        int(destImage[ImageCoordinates(2, 8)].r),
        std::roundf(
            (138 + 139 + 140 + 141) / 4.0f * 0.166666687f // 8-11 - @10
            + (150 + 151 + 152 + 153) / 4.0f * 0.833333313f // 8-11 - @11
        ));
}

TEST(ImageToolsTests, ResizeNicer_LargerW_Smaller2H)
{
    RgbaImageData sourceImage(12, 12);
    uint8_t c = 10;
    for (uint8_t y = 0; y < sourceImage.Size.height; ++y)
    {
        for (uint8_t x = 0; x < sourceImage.Size.width; ++x)
        {
            sourceImage[{x, y}] = rgbaColor(c, c, c, c);
            ++c;
        }
    }

    RgbaImageData destImage = ImageTools::ResizeNicer(
        sourceImage,
        ImageSize(24, 3));

    // Verify

    // (0 * 0.75, 0 * 0.25) @ 0-3
    EXPECT_EQ(
        int(destImage[ImageCoordinates(0, 0)].r),
        std::roundf(
            ((10 * 0.75f + 10 * 0.25f)
            + (22 * 0.75f + 22 * 0.25f)
            + (34 * 0.75f + 34 * 0.25f)
            + (46 * 0.75f + 46 * 0.25f)) / 4.0f
        ));

    // (0 * 0.75f + 1 * 0.25f) @ 0-3
    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 0)].r),
        std::roundf(
            ((10 * 0.75f + 11 * 0.25f)
            + (22 * 0.75f + 23 * 0.25f)
            + (34 * 0.75f + 35 * 0.25f)
            + (46 * 0.75f + 47 * 0.25f)) / 4.0f
        ));

    // (0 * 0.75f + 1 * 0.25f) @ 4-7
    EXPECT_EQ(
        int(destImage[ImageCoordinates(1, 1)].r),
        std::roundf(
            ((58 * 0.75f + 59 * 0.25f)
            + (70 * 0.75f + 71 * 0.25f)
            + (82 * 0.75f + 83 * 0.25f)
            + (94 * 0.75f + 95 * 0.25f)) / 4.0f
        ));

    // 10 * 0.25 + 11 * 0.75 @ 4-7
    EXPECT_EQ(
        int(destImage[ImageCoordinates(22, 1)].r),
        std::roundf(
            ((68 * 0.25f + 69 * 0.75f)
            + (80 * 0.25f + 81 * 0.75f)
            + (92 * 0.75f + 93 * 0.25f)
            + (104 * 0.75f + 105 * 0.25f)) / 4.0f
        ));

    // 11 * 0.75 + 11 * 0.25 @ 8-11
    EXPECT_EQ(
        int(destImage[ImageCoordinates(23, 2)].r),
        std::roundf(
            ((117 * 0.75f + 117 * 0.25f)
            + (129 * 0.75f + 129 * 0.25f)
            + (141 * 0.75f + 141 * 0.25f)
            + (153 * 0.75f + 153 * 0.25f)) / 4.0f
        ));
}

// TODO: ResizeNicer_Smaller1W_Smaller2H
// TODO: ResizeNicer_Smaller2W_Smaller2H