#include "Utils.h"

#include <GameCore/GameMath.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t Size = 10000000;

static void FastPow_Pow(benchmark::State& state)
{
    auto bases = MakeFloats(Size);
    auto exponents = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            results.push_back(pow(bases[i], exponents[i]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastPow_Pow);

static void FastPow_FastPow(benchmark::State& state)
{
    auto bases = MakeFloats(Size);
    auto exponents = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            results.push_back(FastPow(bases[i], exponents[i]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastPow_FastPow);

static void FastExp_Exp(benchmark::State& state)
{
    auto exponents = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            results.push_back(exp(exponents[i]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastExp_Exp);

static void FastExp_FastExp(benchmark::State& state)
{
    auto exponents = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            results.push_back(FastExp(exponents[i]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastExp_FastExp);