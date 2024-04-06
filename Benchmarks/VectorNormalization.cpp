#include "Utils.h"

#include <GameCore/Algorithms.h>
#include <GameCore/SysSpecifics.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

static void VectorNormalization_Naive_NoLengthStorage(benchmark::State& state)
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
BENCHMARK(VectorNormalization_Naive_NoLengthStorage);

static void VectorNormalization_Naive_NoLengthStorage_RestrictPointers(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();
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
BENCHMARK(VectorNormalization_Naive_NoLengthStorage_RestrictPointers);

static void VectorNormalization_Naive_AndLengthStorage_RestrictPointers(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    std::vector<float> lengths;
    lengths.resize(size);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();
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
BENCHMARK(VectorNormalization_Naive_AndLengthStorage_RestrictPointers);

////////////////////////////////////////////////////////////////////////////////////////

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

static void VectorNormalization_Vectorized_AndLengthStorage_FullInstrinsics(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    std::vector<float> lengths;
    lengths.resize(size);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();
    vec2f * restrict resultData = results.data();
    float * restrict lengthData = lengths.data();

    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);

    for (auto _ : state)
    {
        for (size_t s = 0; s < size; s += 4)
        {
            __m128 vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 0].PointAIndex)));
            __m128 vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 0].PointBIndex)));
            __m128 vecD0 = _mm_sub_ps(vecB0, vecA0);

            __m128 vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 1].PointAIndex)));
            __m128 vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 1].PointBIndex)));
            __m128 vecD1 = _mm_sub_ps(vecB1, vecA1);

            __m128 vecD01 = _mm_movelh_ps(vecD0, vecD1); //x0,y0,x1,y1

            __m128 vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 2].PointAIndex)));
            __m128 vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 2].PointBIndex)));
            __m128 vecD2 = _mm_sub_ps(vecB2, vecA2);

            __m128 vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 3].PointAIndex)));
            __m128 vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 3].PointBIndex)));
            __m128 vecD3 = _mm_sub_ps(vecB3, vecA3);

            __m128 vecD23 = _mm_movelh_ps(vecD2, vecD3); //x2,y2,x3,y3

            __m128 displacementX = _mm_shuffle_ps(vecD01, vecD23, 0x88); // x0,x1,x2,x3
            __m128 displacementY = _mm_shuffle_ps(vecD01, vecD23, 0xDD); // y0,y1,y2,y3

            __m128 displacementX2 = _mm_mul_ps(displacementX, displacementX);
            __m128 displacementY2 = _mm_mul_ps(displacementY, displacementY);

            __m128 displacementXY = _mm_add_ps(displacementX2, displacementY2);
            __m128 springLength = _mm_sqrt_ps(displacementXY);

            displacementX = _mm_div_ps(displacementX, springLength);
            displacementY = _mm_div_ps(displacementY, springLength);

            __m128 validMask = _mm_cmpneq_ps(springLength, Zero);

            displacementX = _mm_and_ps(displacementX, validMask);
            displacementY = _mm_and_ps(displacementY, validMask);

            _mm_store_ps(lengthData + s, springLength);

            __m128 s01 = _mm_unpacklo_ps(displacementX, displacementY);
            __m128 s23 = _mm_unpackhi_ps(displacementX, displacementY);

            _mm_store_ps(reinterpret_cast<float * restrict>(resultData + s), s01);
            _mm_store_ps(reinterpret_cast<float * restrict>(resultData + s + 2), s23);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLengthStorage_FullInstrinsics);

#endif

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

static void VectorNormalization_Vectorized_AndLengthStorage_Reciprocal_FullInstrinsics(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<vec2f> results;
    results.resize(size);

    std::vector<float> lengths;
    lengths.resize(size);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();
    vec2f * restrict resultData = results.data();
    float * restrict lengthData = lengths.data();

    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);

    for (auto _ : state)
    {
        for (size_t s = 0; s < size; s += 4)
        {
            __m128 vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 0].PointAIndex)));
            __m128 vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 0].PointBIndex)));
            __m128 vecD0 = _mm_sub_ps(vecB0, vecA0);

            __m128 vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 1].PointAIndex)));
            __m128 vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 1].PointBIndex)));
            __m128 vecD1 = _mm_sub_ps(vecB1, vecA1);

            __m128 vecD01 = _mm_movelh_ps(vecD0, vecD1); //x0,y0,x1,y1

            __m128 vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 2].PointAIndex)));
            __m128 vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 2].PointBIndex)));
            __m128 vecD2 = _mm_sub_ps(vecB2, vecA2);

            __m128 vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 3].PointAIndex)));
            __m128 vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointData + springData[s + 3].PointBIndex)));
            __m128 vecD3 = _mm_sub_ps(vecB3, vecA3);

            __m128 vecD23 = _mm_movelh_ps(vecD2, vecD3); //x2,y2,x3,y3

            __m128 displacementX = _mm_shuffle_ps(vecD01, vecD23, 0x88); // x0,x1,x2,x3
            __m128 displacementY = _mm_shuffle_ps(vecD01, vecD23, 0xDD); // y0,y1,y2,y3

            __m128 displacementX2 = _mm_mul_ps(displacementX, displacementX);
            __m128 displacementY2 = _mm_mul_ps(displacementY, displacementY);

            __m128 displacementXY = _mm_add_ps(displacementX2, displacementY2); // x^2 + y^2

            __m128 validMask = _mm_cmpneq_ps(displacementXY, Zero);
            __m128 rspringLength = _mm_rsqrt_ps(displacementXY);
            rspringLength = _mm_and_ps(rspringLength, validMask);   // L==0 => 1/L == 0, to maintain normal == (0, 0) from vec2f

            displacementX = _mm_mul_ps(displacementX, rspringLength);
            displacementY = _mm_mul_ps(displacementY, rspringLength);

            _mm_store_ps(lengthData + s, rspringLength);

            __m128 s01 = _mm_unpacklo_ps(displacementX, displacementY);
            __m128 s23 = _mm_unpackhi_ps(displacementX, displacementY);

            _mm_store_ps(reinterpret_cast<float * restrict>(resultData + s), s01);
            _mm_store_ps(reinterpret_cast<float * restrict>(resultData + s + 2), s23);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLengthStorage_Reciprocal_FullInstrinsics);

#endif

//////////////////////////////////////////////////////////////////////////////

/* Two passes make it considerably worse
static void VectorNormalization_TwoPass_Naive(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<float> x;
    x.resize(size);
    float * restrict xBuffer = x.data();

    std::vector<float> y;
    y.resize(size);
    float * restrict yBuffer = y.data();

    std::vector<float> resultLengths;
    resultLengths.resize(size);
    float * restrict resultLengthBuffer = resultLengths.data();

    for (auto _ : state)
    {
        //
        // Pass 1
        //

        for (size_t s = 0; s < size; ++s)
        {
            xBuffer[s] = points[springs[s].PointBIndex].x - points[springs[s].PointAIndex].x;
            yBuffer[s] = points[springs[s].PointBIndex].y - points[springs[s].PointAIndex].y;
        }

        //
        // Pass 2
        //

        for (size_t s = 0; s < size; ++s)
        {
            float const d = std::sqrt(xBuffer[s] * xBuffer[s] + yBuffer[s] + yBuffer[s]);
            xBuffer[s] /= d;
            yBuffer[s] /= d;
            resultLengths[s] = d;
        }
    }

    benchmark::DoNotOptimize(x);
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(resultLengths);
}
BENCHMARK(VectorNormalization_TwoPass_Naive);
*/