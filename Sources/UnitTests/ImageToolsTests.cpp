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
