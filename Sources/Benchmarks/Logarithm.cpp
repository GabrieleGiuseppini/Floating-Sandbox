#include "Utils.h"

#include <Core/GameMath.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <limits>

static constexpr size_t Size = 100000000;

static void Log(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = log(floats[i]);

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Log);

static void FastLog2(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = FastLog2(floats[i]);

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastLog2);

static void FastLog(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = FastLog(floats[i]);

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(FastLog);

float DiscreteLog2_logb(float x)
{
    return logb(x);
}

static void DiscreteLog2_logb(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = DiscreteLog2_logb(floats[i]);

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(DiscreteLog2_logb);

float DiscreteLog2_manual(float x)
{
    typedef union {
        float f;
        struct {
            unsigned int mantissa : 23;
            unsigned int exponent : 8;
            unsigned int sign : 1;
        } parts;
    } float_cast;

    float_cast d1 = { x };

    return static_cast<float>(static_cast<int>(d1.parts.exponent) - 127);
}

static void DiscreteLog2_manual(benchmark::State& state)
{
    auto floats = MakeFloats(Size);
    std::vector<float> results(Size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            float result = DiscreteLog2_manual(floats[i]);

            results.push_back(result);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(DiscreteLog2_manual);