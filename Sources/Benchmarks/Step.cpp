#include "Utils.h"

#include <Core/GameMath.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>

static constexpr size_t Size = 1000000000;

static void Step_TernaryOperator(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);
    results.resize(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = floats[i] < 0.0f ? 1.0f : 0.0f;

            results[i] = result;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Step_TernaryOperator);

static void Step_ComparisonCast(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = static_cast<float>(floats[i] <= 0.0f);

            results[i] = result;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Step_ComparisonCast);

static void Step_TernaryOperator2(benchmark::State & state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);
    results.resize(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float const result =
                SmoothStep(-30.0f, 30.0f, floats[i])
                * (floats[i] <= 0.0f ? 12.0f : 3.0f);

            results[i] = result;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Step_TernaryOperator2);

static void Step_ComparisonCast2(benchmark::State & state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float const result =
                SmoothStep(-30.0f, 30.0f, floats[i])
                * (3.0f + 9.0f * static_cast<float>(floats[i] <= 0.0f));

            results[i] = result;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Step_ComparisonCast2);