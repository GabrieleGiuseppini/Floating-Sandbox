#include "Utils.h"

#include <GameLib/SysSpecifics.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

static void VectorNormalization_Naive_NoLength(benchmark::State& state) 
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<Spring> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);
    
    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
        {
            vec2f const posA = points[springs[i].PointAIndex];
            vec2f const posB = points[springs[i].PointBIndex];
            vec2f v(posB - posA);
            vec2f const normalizedVector = v.normalise();

            results[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(VectorNormalization_Naive_NoLength);

static void VectorNormalization_Naive_NoLength_RestrictPointers(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<Spring> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    vec2f const * restrict pointData = points.data();
    Spring const * restrict springData = springs.data();
    vec2f * restrict resultData = results.data();

    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
        {
            vec2f const posA = pointData[springData[i].PointAIndex];
            vec2f const posB = pointData[springData[i].PointBIndex];
            vec2f v(posB - posA);
            vec2f const normalizedVector = v.normalise();

            resultData[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(VectorNormalization_Naive_NoLength_RestrictPointers);

static void VectorNormalization_Naive_AndLength_RestrictPointers(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<Spring> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    std::vector<float> lengths;
    lengths.resize(size);

    vec2f const * restrict pointData = points.data();
    Spring const * restrict springData = springs.data();
    vec2f * restrict resultData = results.data();
    float * restrict lengthData = lengths.data();

    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
        {
            vec2f const posA = pointData[springData[i].PointAIndex];
            vec2f const posB = pointData[springData[i].PointBIndex];
            vec2f v(posB - posA);
            float length = v.length();
            vec2f const normalizedVector = v.normalise(length);

            resultData[i] = normalizedVector;
            lengthData[i] = length;
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Naive_AndLength_RestrictPointers);
