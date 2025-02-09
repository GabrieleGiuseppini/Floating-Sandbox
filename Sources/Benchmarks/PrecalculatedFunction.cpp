#include "Utils.h"

#include <Core/PrecalculatedFunction.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t Size = 200000000;


static void PrecalculatedFunction_PureSin(benchmark::State& state)
{
    auto floats = MakeFloats(Size);

    float result = 0.0;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            result += sin(floats[i]);
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(PrecalculatedFunction_PureSin);

static void PrecalculatedFunction_LinearlyInterpolatedPeriodic_8k(benchmark::State& state)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    auto floats = MakeFloats(Size);

    float result = 0.0;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            result += pf.GetLinearlyInterpolatedPeriodic(floats[i]);
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(PrecalculatedFunction_LinearlyInterpolatedPeriodic_8k);

static void PrecalculatedFunction_LinearlyInterpolatedPeriodic_2k(benchmark::State& state)
{
    PrecalculatedFunction<2048> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    auto floats = MakeFloats(Size);

    float result = 0.0;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            result += pf.GetLinearlyInterpolatedPeriodic(floats[i]);
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(PrecalculatedFunction_LinearlyInterpolatedPeriodic_2k);

static void PrecalculatedFunction_LinearlyInterpolatedPeriodic_256(benchmark::State& state)
{
    PrecalculatedFunction<256> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    auto floats = MakeFloats(Size);

    float result = 0.0;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            result += pf.GetLinearlyInterpolatedPeriodic(floats[i]);
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(PrecalculatedFunction_LinearlyInterpolatedPeriodic_256);

static void PrecalculatedFunction_LinearlyInterpolatedPeriodic_WithPhaseArgAdjustment(benchmark::State& state)
{
    PrecalculatedFunction<8192> pf(
        [](float x)
        {
            return sin(2.0f * Pi<float> * x);
        });

    auto floats = MakeFloats(Size);

    float result = 0.0;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            result += pf.GetLinearlyInterpolatedPeriodic(floats[i] / (2.0f * Pi<float>));
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(PrecalculatedFunction_LinearlyInterpolatedPeriodic_WithPhaseArgAdjustment);