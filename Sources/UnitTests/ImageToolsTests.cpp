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

// TODO: ResizeNicer_Smaller1W_LargerH
// TODO: ResizeNicer_Smaller2W_LargerH

// TODO: ResizeNicer_LargerW_Smaller1H
// TODO: ResizeNicer_Smaller1W_Smaller1H
// TODO: ResizeNicer_Smaller2W_Smaller1H

// TODO: ResizeNicer_LargerW_Smaller2H
// TODO: ResizeNicer_Smaller1W_Smaller2H
// TODO: ResizeNicer_Smaller2W_Smaller2H