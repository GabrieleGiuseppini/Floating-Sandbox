#include "Utils.h"

#include <GameCore/Algorithms.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000;

inline vec2f normalise_naive(vec2f const & v) noexcept
{
    float const squareLength = v.x * v.x + v.y * v.y;
    if (squareLength > 0)
    {
        return v / sqrtf(squareLength);
    }
    else
    {
        return vec2f(0.0f, 0.0f);
    }
}

inline vec2f normalise_naive(vec2f const & v, float length) noexcept
{
    if (length > 0)
    {
        return v / length;
    }
    else
    {
        return vec2f(0.0f, 0.0f);
    }
}

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

inline vec2f normalise_SSEx2(vec2f const & v, float length) noexcept
{
    float fResult[4];

    __m128 const Zero = _mm_setzero_ps();

    __m128 _v = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&v)));
    __m128 _l = _mm_set1_ps(length);
    __m128 _r = _mm_div_ps(_v, _l);
    __m128 validMask = _mm_cmpneq_ps(_l, Zero);
    _r = _mm_and_ps(_r, validMask);

    _mm_store_ps(reinterpret_cast<float * restrict>(fResult), _r);
    return *(reinterpret_cast<vec2f *>(fResult));
}

#endif

static void SingleVectorNormalization_Naive_PreLength_ResultDiscarded(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_naive(vectors[i], lengths[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_Naive_PreLength_ResultDiscarded);

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX1_PreLength_ResultDiscarded(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = Algorithms::NormalizeVector2_SSE(vectors[i], lengths[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_PreLength_ResultDiscarded);

#endif

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX2_PreLength_ResultDiscarded(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_SSEx2(vectors[i], lengths[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX2_PreLength_ResultDiscarded);

#endif

static void SingleVectorNormalization_Naive_PreLength_ResultStored(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);
    vec2f * restrict const ptrResults = results.data();

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_naive(vectors[i], lengths[i]);

            ptrResults[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_Naive_PreLength_ResultStored);

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX1_PreLength_ResultStored(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);
    vec2f * restrict const ptrResults = results.data();

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = Algorithms::NormalizeVector2_SSE(vectors[i], lengths[i]);

            ptrResults[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_PreLength_ResultStored);

#endif

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX2_PreLength_ResultStored(benchmark::State& state)
{
    auto lengths = MakeFloats(SampleSize);
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);
    vec2f * restrict const ptrResults = results.data();

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_SSEx2(vectors[i], lengths[i]);

            ptrResults[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX2_PreLength_ResultStored);

#endif

////////////////////////////////

static void SingleVectorNormalization_Naive_NoLength_ResultDiscarded(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_naive(vectors[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_Naive_NoLength_ResultDiscarded);

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX1_NoLength_ResultDiscarded(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = Algorithms::NormalizeVector2_SSE(vectors[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_NoLength_ResultDiscarded);

#endif

static void SingleVectorNormalization_Naive_NoLength_ResultStored(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_naive(vectors[i]);

            results[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_Naive_NoLength_ResultStored);

#if defined(FS_ARCHITECTURE_X86_32) || defined(FS_ARCHITECTURE_X86_64)

static void SingleVectorNormalization_SSEX1_NoLength_ResultStored(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = Algorithms::NormalizeVector2_SSE(vectors[i]);

            results[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_NoLength_ResultStored);

#endif