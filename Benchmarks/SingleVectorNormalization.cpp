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

inline vec2f normalise_SSEx1(vec2f const & v) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 x = _mm_load_ss(&(v.x));
    __m128 y = _mm_load_ss(&(v.y));

    __m128 len = _mm_sqrt_ss(
        _mm_add_ss(
            _mm_mul_ss(x, x),
            _mm_mul_ss(y, y)));

    __m128 invLen = _mm_div_ss(One, len);
    __m128 validMask = _mm_cmpneq_ss(invLen, Zero);
    invLen = _mm_and_ps(invLen, validMask);

    x = _mm_mul_ss(x, invLen);
    y = _mm_mul_ss(y, invLen);

    return vec2f(_mm_cvtss_f32(x), _mm_cvtss_f32(y));
}

inline vec2f normalise_SSEx1(vec2f const & v, float length) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 _l = _mm_set_ss(length);
    __m128 _revl = _mm_div_ss(One, _l);
    __m128 validMask = _mm_cmpneq_ss(_l, Zero);
    _revl = _mm_and_ps(_revl, validMask);

    __m128 _x = _mm_mul_ss(_mm_load_ss(&(v.x)), _revl);
    __m128 _y = _mm_mul_ss(_mm_load_ss(&(v.y)), _revl);

    return vec2f(_mm_cvtss_f32(_x), _mm_cvtss_f32(_y));
}

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
            vec2f norm = normalise_SSEx1(vectors[i], lengths[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_PreLength_ResultDiscarded);

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
            vec2f norm = normalise_SSEx1(vectors[i], lengths[i]);

            ptrResults[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_PreLength_ResultStored);

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

static void SingleVectorNormalization_SSEX1_NoLength_ResultDiscarded(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<bool> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_SSEx1(vectors[i]);

            results[i] = (norm.x > norm.y);
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_NoLength_ResultDiscarded);

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

static void SingleVectorNormalization_SSEX1_NoLength_ResultStored(benchmark::State& state)
{
    auto vectors = MakeVectors(SampleSize);

    std::vector<vec2f> results;
    results.resize(SampleSize);

    for (auto _ : state)
    {
        for (size_t i = 0; i < SampleSize; ++i)
        {
            vec2f norm = normalise_SSEx1(vectors[i]);

            results[i] = norm;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSEX1_NoLength_ResultStored);