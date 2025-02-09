#include "Utils.h"

#include <Core/Algorithms.h>
#include <Core/SysSpecifics.h>
#include <Core/Vectors.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 100000;

/* TODOTEST
*
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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

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

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

static void SingleVectorNormalization_SSEX1_NoLength_Algorithms_ResultStored(benchmark::State& state)
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
BENCHMARK(SingleVectorNormalization_SSEX1_NoLength_Algorithms_ResultStored);

#endif
*/

/////////////////////////////////////////////////////////////////////////

static void SingleVectorNormalization_Simple_Original(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
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
BENCHMARK(SingleVectorNormalization_Simple_Original);

static void SingleVectorNormalization_Simple_Original_WithLength(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<float> results_len;
    results_len.resize(size);
    std::vector<vec2f> results_vec;
    results_vec.resize(size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
        {
            vec2f const posA = points[springs[i].PointAIndex];
            vec2f const posB = points[springs[i].PointBIndex];
            vec2f v(posB - posA);
            float const length = v.length();
            results_len[i] = length;
            vec2f const normalizedVector = v.normalise(length);

            results_vec[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results_len);
    benchmark::DoNotOptimize(results_vec);
}
BENCHMARK(SingleVectorNormalization_Simple_Original_WithLength);

inline vec2f normalise_with_mul(vec2f v) noexcept
{
    float const squareLength = v.x * v.x + v.y * v.y;
    if (squareLength != 0)
    {
        // Note: this is also how the "original" normalise gets compiled by MSVC2019...
        return v * (1.0f / std::sqrt(squareLength));
    }
    else
    {
        return vec2f(0.0f, 0.0f);
    }
}

static void SingleVectorNormalization_Simple_MulInsteadOfDiv(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
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
            vec2f const normalizedVector = normalise_with_mul(v);

            results[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_Simple_MulInsteadOfDiv);

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

inline vec2f NormalizeVector_SSE_1_Precise(vec2f const & v) noexcept
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

static void SingleVectorNormalization_SSE_1_Precise(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
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
            vec2f const normalizedVector = NormalizeVector_SSE_1_Precise(v);

            results[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSE_1_Precise);

#endif


#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

static void SingleVectorNormalization_SSE_2_Approx(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
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
            vec2f const normalizedVector = v.normalise_approx();

            results[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSE_2_Approx);

#endif

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

static void SingleVectorNormalization_SSE_3_Approx_WithLength(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<float> results_len;
    results_len.resize(size);
    std::vector<vec2f> results_vec;
    results_vec.resize(size);

    for (auto _ : state)
    {
        for (size_t i = 0; i < size; ++i)
        {
            vec2f const posA = points[springs[i].PointAIndex];
            vec2f const posB = points[springs[i].PointBIndex];
            vec2f v(posB - posA);
            float const length = v.length();
            results_len[i] = length;
            vec2f const normalizedVector = v.normalise_approx(length);

            results_vec[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results_len);
    benchmark::DoNotOptimize(results_vec);
}
BENCHMARK(SingleVectorNormalization_SSE_3_Approx_WithLength);

#endif


/*
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

inline vec2f NormalizeVector_SSE_4_Approx_Packed(vec2f const & v) noexcept
{
    __m128 xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(&(v.x))));
    __m128 xy2 = _mm_mul_ps(xy, xy);

    __m128 const sqrArg = _mm_hadd_ps(xy2, xy2);

    __m128 const validMask = _mm_cmpneq_ss(sqrArg, _mm_setzero_ps());

    __m128 const invLen_or_zero = _mm_and_ps(
        _mm_rsqrt_ss(sqrArg),
        validMask);

    __m128 const invLen_or_zero2 = _mm_moveldup_ps(invLen_or_zero);
    xy = _mm_mul_ps(xy, invLen_or_zero2);

    vec2f result;
    _mm_store_sd(reinterpret_cast<double *>(&(result.x)), _mm_castps_pd(xy));
    return result;
}

static void SingleVectorNormalization_SSE_4_Approx_Packed(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
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
            vec2f const normalizedVector = NormalizeVector_SSE_4_Approx_Packed(v);
            results[i] = normalizedVector;
        }
    }

    benchmark::DoNotOptimize(results);
}
BENCHMARK(SingleVectorNormalization_SSE_4_Approx_Packed);

#endif
*/