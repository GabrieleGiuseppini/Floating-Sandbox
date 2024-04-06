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

////////////////////////////////////////////////////////////////

inline float Clamp1(
    float x,
    float lLimit,
    float rLimit)
{
    if (x < lLimit)
        return lLimit;
    else if (x < rLimit)
        return x;
    else
        return rLimit;
}

inline float SmoothStep1(
    float lEdge,
    float rEdge,
    float x)
{
    assert(lEdge < rEdge);

    x = Clamp1((x - lEdge) / (rEdge - lEdge), 0.0f, 1.0f);

    return x * x * (3.0f - 2.0f * x); // 3x^2 -2x^3, Cubic Hermite
}

static void Smoothstep1(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size - 2; ++i)
        {
            results.push_back(SmoothStep1(vals[i], vals[i + 1], vals[i + 3]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Smoothstep1);

inline float Clamp2(
    float x,
    float lLimit,
    float rLimit)
{
    return std::min(std::max(lLimit, x), rLimit);
}

inline float SmoothStep2(
    float lEdge,
    float rEdge,
    float x)
{
    assert(lEdge < rEdge);

    x = Clamp2((x - lEdge) / (rEdge - lEdge), 0.0f, 1.0f);

    return x * x * (3.0f - 2.0f * x); // 3x^2 -2x^3, Cubic Hermite
}

static void Smoothstep2(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    std::vector<float> results(Size);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size - 2; ++i)
        {
            results.push_back(SmoothStep2(vals[i], vals[i + 1], vals[i + 3]));
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(Smoothstep2);

static void SinCos4_Base(benchmark::State & state)
{
    auto x = MakeFloats(Size * 4);
    std::vector<float> s(Size * 4);
    std::vector<float> c(Size * 4);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            s[i * 4 + 0] = std::sinf(x[i * 4 + 0]);
            s[i * 4 + 1] = std::sinf(x[i * 4 + 1]);
            s[i * 4 + 2] = std::sinf(x[i * 4 + 2]);
            s[i * 4 + 3] = std::sinf(x[i * 4 + 3]);

            c[i * 4 + 0] = std::cosf(x[i * 4 + 0]);
            c[i * 4 + 1] = std::cosf(x[i * 4 + 1]);
            c[i * 4 + 2] = std::cosf(x[i * 4 + 2]);
            c[i * 4 + 3] = std::cosf(x[i * 4 + 3]);
        }
    }

    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(c);
}
BENCHMARK(SinCos4_Base);

static void SinCos4(benchmark::State & state)
{
    auto x = MakeFloats(Size * 4);
    std::vector<float> s(Size * 4);
    std::vector<float> c(Size * 4);
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            SinCos4(&(x[i * 4]), &(s[i * 4]), &(c[i * 4]));
        }
    }

    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(c);
}
BENCHMARK(SinCos4);
