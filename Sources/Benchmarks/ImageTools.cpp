#include "Utils.h"

#include <Core/ImageTools.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <limits>

static constexpr size_t SizeLow = 1024;
static constexpr size_t SizeHigh = 4096;

static void ImageTools_Resize_Bilinear_Up(benchmark::State& state)
{
    auto srcImage = MakeRgbaImageData(ImageSize(SizeLow, SizeLow));
    ImageSize newSize(SizeHigh, SizeHigh);

    int result = 0;
    for (auto _ : state)
    {
        auto const newImage = ImageTools::Resize(srcImage, newSize, ImageTools::FilterKind::Bilinear);
        result += newImage.Size.width;
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(ImageTools_Resize_Bilinear_Up);

static void ImageTools_Resize_Bilinear_Down(benchmark::State& state)
{
    auto srcImage = MakeRgbaImageData(ImageSize(SizeHigh, SizeHigh));
    ImageSize newSize(SizeLow, SizeLow);

    int result = 0;
    for (auto _ : state)
    {
        auto const newImage = ImageTools::Resize(srcImage, newSize, ImageTools::FilterKind::Bilinear);
        result += newImage.Size.width;
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(ImageTools_Resize_Bilinear_Down);

static void ImageTools_ResizeNicer_Up(benchmark::State& state)
{
    auto srcImage = MakeRgbaImageData(ImageSize(SizeLow, SizeLow));
    ImageSize newSize(SizeHigh, SizeHigh);

    int result = 0;
    for (auto _ : state)
    {
        auto const newImage = ImageTools::ResizeNicer(srcImage, newSize);
        result += newImage.Size.width;
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(ImageTools_ResizeNicer_Up);

static void ImageTools_ResizeNicer_Down_1(benchmark::State& state)
{
    auto srcImage = MakeRgbaImageData(ImageSize(SizeHigh, SizeHigh));
    ImageSize newSize(3000, 3000);

    int result = 0;
    for (auto _ : state)
    {
        auto const newImage = ImageTools::ResizeNicer(srcImage, newSize);
        result += newImage.Size.width;
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(ImageTools_ResizeNicer_Down_1);

static void ImageTools_ResizeNicer_Down_2(benchmark::State& state)
{
    auto srcImage = MakeRgbaImageData(ImageSize(SizeHigh, SizeHigh));
    ImageSize newSize(SizeLow, SizeLow);

    int result = 0;
    for (auto _ : state)
    {
        auto const newImage = ImageTools::ResizeNicer(srcImage, newSize);
        result += newImage.Size.width;
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(ImageTools_ResizeNicer_Down_2);
