#include "Utils.h"

#include <GameCore/SysSpecifics.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

static void VectorNormalization_Naive_NoLength(benchmark::State& state)
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
BENCHMARK(VectorNormalization_Naive_NoLength);

static void VectorNormalization_Naive_NoLength_RestrictPointers(benchmark::State& state)
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
BENCHMARK(VectorNormalization_Naive_NoLength_RestrictPointers);

static void VectorNormalization_Naive_AndLength_RestrictPointers(benchmark::State& state)
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
BENCHMARK(VectorNormalization_Naive_AndLength_RestrictPointers);

////////////////////////////////////////////////////////////////////////////////////////

// TODO: move to GameLib's LibSimdPp.h
#define SIMDPP_ARCH_X86_SSSE3
#include "simdpp/simd.h"

static void VectorNormalization_Vectorized_AndLength_Load1(benchmark::State& state)
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
        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> pAx = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].x,
                pointData[springData[s + 1].PointAIndex].x,
                pointData[springData[s + 2].PointAIndex].x,
                pointData[springData[s + 3].PointAIndex].x);

            simdpp::float32<4> pAy = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].y,
                pointData[springData[s + 1].PointAIndex].y,
                pointData[springData[s + 2].PointAIndex].y,
                pointData[springData[s + 3].PointAIndex].y);

            simdpp::float32<4> pBx = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].x,
                pointData[springData[s + 1].PointBIndex].x,
                pointData[springData[s + 2].PointBIndex].x,
                pointData[springData[s + 3].PointBIndex].x);

            simdpp::float32<4> pBy = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].y,
                pointData[springData[s + 1].PointBIndex].y,
                pointData[springData[s + 2].PointBIndex].y,
                pointData[springData[s + 3].PointBIndex].y);

            simdpp::float32<4> displacementX = pBx - pAx;
            simdpp::float32<4> displacementY = pBy - pAy;

            simdpp::float32<4> _dx = displacementX * displacementX;
            simdpp::float32<4> _dy = displacementY * displacementY;

            simdpp::float32<4> _dxy = _dx + _dy;

            simdpp::float32<4> springLength = simdpp::sqrt(_dxy);

            displacementX = displacementX / springLength;
            displacementY = displacementY / springLength;

            simdpp::mask_float32<4> validMask = (springLength != 0.0f);

            displacementX = simdpp::bit_and(displacementX, validMask);
            displacementY = simdpp::bit_and(displacementY, validMask);

            simdpp::store(lengthData + s, springLength);
            simdpp::store_packed2(resultData + s, displacementX, displacementY);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_Load1);

static void VectorNormalization_Vectorized_AndLength_Load2(benchmark::State& state)
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
        for (size_t s = 0; s < size; s += 4)
        {
            float pAx_[4] = {
                pointData[springData[s + 0].PointAIndex].x,
                pointData[springData[s + 1].PointAIndex].x,
                pointData[springData[s + 2].PointAIndex].x,
                pointData[springData[s + 3].PointAIndex].x
            };

            simdpp::float32<4> pAx = simdpp::load(pAx_);

            float pAy_[4] = {
                pointData[springData[s + 0].PointAIndex].y,
                pointData[springData[s + 1].PointAIndex].y,
                pointData[springData[s + 2].PointAIndex].y,
                pointData[springData[s + 3].PointAIndex].y
            };

            simdpp::float32<4> pAy = simdpp::load(pAy_);

            float pBx_[4] = {
                pointData[springData[s + 0].PointBIndex].x,
                pointData[springData[s + 1].PointBIndex].x,
                pointData[springData[s + 2].PointBIndex].x,
                pointData[springData[s + 3].PointBIndex].x
            };

            simdpp::float32<4> pBx = simdpp::load(pBx_);

            float pBy_[4] = {
                pointData[springData[s + 0].PointBIndex].y,
                pointData[springData[s + 1].PointBIndex].y,
                pointData[springData[s + 2].PointBIndex].y,
                pointData[springData[s + 3].PointBIndex].y
            };

            simdpp::float32<4> pBy = simdpp::load(pBy_);



            simdpp::float32<4> displacementX = pBx - pAx;
            simdpp::float32<4> displacementY = pBy - pAy;

            simdpp::float32<4> _dx = displacementX * displacementX;
            simdpp::float32<4> _dy = displacementY * displacementY;

            simdpp::float32<4> _dxy = _dx + _dy;

            simdpp::float32<4> springLength = simdpp::sqrt(_dxy);

            displacementX = displacementX / springLength;
            displacementY = displacementY / springLength;

            simdpp::mask_float32<4> validMask = (springLength != 0.0f);

            displacementX = simdpp::bit_and(displacementX, validMask);
            displacementY = simdpp::bit_and(displacementY, validMask);

            simdpp::store(lengthData + s, springLength);
            simdpp::store_packed2(resultData + s, displacementX, displacementY);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_Load2);

static void VectorNormalization_Vectorized_AndLength_LoadInstrinsics(benchmark::State& state)
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
        for (size_t s = 0; s < size; s += 4)
        {
            __m128 vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 0].PointAIndex)));
            __m128 vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 1].PointAIndex)));
            __m128 vecA01 = _mm_movelh_ps(vecA0, vecA1);

            __m128 vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 0].PointBIndex)));
            __m128 vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 1].PointBIndex)));
            __m128 vecB01 = _mm_movelh_ps(vecB0, vecB1);

            __m128 vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 2].PointAIndex)));
            __m128 vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 3].PointAIndex)));
            __m128 vecA23 = _mm_movelh_ps(vecA2, vecA3);

            __m128 vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 2].PointBIndex)));
            __m128 vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 3].PointBIndex)));
            __m128 vecB23 = _mm_movelh_ps(vecB2, vecB3);

            simdpp::float32<4> displacement01 = simdpp::float32<4>(vecB01) - simdpp::float32<4>(vecA01); // x0,y0,x1,y1
            simdpp::float32<4> displacement23 = simdpp::float32<4>(vecB23) - simdpp::float32<4>(vecA23); // x2,y2,x3,y3

            simdpp::float32<4> displacementX = simdpp::unzip4_lo(displacement01, displacement23); // x0,x1,x2,x3
            simdpp::float32<4> displacementY = simdpp::unzip4_hi(displacement01, displacement23); // y0,y1,y2,y3


            simdpp::float32<4> const springLength = simdpp::sqrt(displacementX * displacementX + displacementY * displacementY);

            displacementX = displacementX / springLength;
            displacementY = displacementY / springLength;

            simdpp::mask_float32<4> const validMask = (springLength != 0.0f);

            displacementX = simdpp::bit_and(displacementX, validMask);
            displacementY = simdpp::bit_and(displacementY, validMask);

            simdpp::store(lengthData + s, springLength);
            simdpp::store_packed2(resultData + s, displacementX, displacementY);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_LoadInstrinsics);

static void VectorNormalization_Vectorized_AndLength_LoadInstrinsics2(benchmark::State& state)
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
        for (size_t s = 0; s < size; s += 4)
        {
            __m128 vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 0].PointAIndex)));
            __m128 vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 0].PointBIndex)));
            __m128 vecD0 = _mm_sub_ps(vecB0, vecA0);

            __m128 vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 1].PointAIndex)));
            __m128 vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 1].PointBIndex)));
            __m128 vecD1 = _mm_sub_ps(vecB1, vecA1);

            __m128 vecD01 = _mm_movelh_ps(vecD0, vecD1);

            __m128 vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 2].PointAIndex)));
            __m128 vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 2].PointBIndex)));
            __m128 vecD2 = _mm_sub_ps(vecB2, vecA2);

            __m128 vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 3].PointAIndex)));
            __m128 vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const *>(pointData + springData[s + 3].PointBIndex)));
            __m128 vecD3 = _mm_sub_ps(vecB3, vecA3);

            __m128 vecD23 = _mm_movelh_ps(vecD2, vecD3);

            simdpp::float32<4> displacementX = simdpp::unzip4_lo(simdpp::float32<4>(vecD01), simdpp::float32<4>(vecD23)); // x0,x1,x2,x3
            simdpp::float32<4> displacementY = simdpp::unzip4_hi(simdpp::float32<4>(vecD01), simdpp::float32<4>(vecD23)); // y0,y1,y2,y3


            simdpp::float32<4> const springLength = simdpp::sqrt(displacementX * displacementX + displacementY * displacementY);

            displacementX = displacementX / springLength;
            displacementY = displacementY / springLength;

            simdpp::mask_float32<4> const validMask = (springLength != 0.0f);

            displacementX = simdpp::bit_and(displacementX, validMask);
            displacementY = simdpp::bit_and(displacementY, validMask);

            simdpp::store(lengthData + s, springLength);
            simdpp::store_packed2(resultData + s, displacementX, displacementY);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_LoadInstrinsics2);

static void VectorNormalization_Vectorized_AndLength_FullInstrinsics(benchmark::State& state)
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
BENCHMARK(VectorNormalization_Vectorized_AndLength_FullInstrinsics);

static void VectorNormalization_Vectorized_AndLength_Reciprocal_FullInstrinsics(benchmark::State & state)
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
BENCHMARK(VectorNormalization_Vectorized_AndLength_Reciprocal_FullInstrinsics);

static void VectorNormalization_Vectorized_AndLength_Reciprocal_LibSimDpp(benchmark::State & state)
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
        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> const pAx = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].x,
                pointData[springData[s + 1].PointAIndex].x,
                pointData[springData[s + 2].PointAIndex].x,
                pointData[springData[s + 3].PointAIndex].x);

            simdpp::float32<4> const pAy = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].y,
                pointData[springData[s + 1].PointAIndex].y,
                pointData[springData[s + 2].PointAIndex].y,
                pointData[springData[s + 3].PointAIndex].y);

            simdpp::float32<4> const pBx = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].x,
                pointData[springData[s + 1].PointBIndex].x,
                pointData[springData[s + 2].PointBIndex].x,
                pointData[springData[s + 3].PointBIndex].x);

            simdpp::float32<4> const pBy = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].y,
                pointData[springData[s + 1].PointBIndex].y,
                pointData[springData[s + 2].PointBIndex].y,
                pointData[springData[s + 3].PointBIndex].y);

            simdpp::float32<4> displacementX = pBx - pAx;
            simdpp::float32<4> displacementY = pBy - pAy;

            simdpp::float32<4> const _dx = displacementX * displacementX; // x^2
            simdpp::float32<4> const _dy = displacementY * displacementY; // y^2

            simdpp::float32<4> const _dxy = _dx + _dy;

            simdpp::mask_float32<4> const validMask = (_dxy != 0.0f);

            simdpp::float32<4> reciprocalSpringLength = simdpp::rsqrt_e(_dxy);

            // L==0 => 1/L == 0, to maintain normal == (0, 0) from vec2f
            reciprocalSpringLength = simdpp::bit_and(reciprocalSpringLength, validMask);

            displacementX = displacementX * reciprocalSpringLength;
            displacementY = displacementY * reciprocalSpringLength;

            simdpp::store(lengthData + s, reciprocalSpringLength);

            simdpp::store_packed2(resultData + s, displacementX, displacementY);
        }
    }

    benchmark::DoNotOptimize(results);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_Reciprocal_LibSimDpp);

static void VectorNormalization_Vectorized_AndLength_Reciprocal_LibSimDpp_ExplodedResults(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    std::vector<float> dirX;
    dirX.resize(size);

    std::vector<float> dirY;
    dirY.resize(size);

    std::vector<float> lengths;
    lengths.resize(size);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();
    float * restrict dirXData = dirX.data();
    float * restrict dirYData = dirY.data();
    float * restrict lengthData = lengths.data();

    for (auto _ : state)
    {
        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> const pAx = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].x,
                pointData[springData[s + 1].PointAIndex].x,
                pointData[springData[s + 2].PointAIndex].x,
                pointData[springData[s + 3].PointAIndex].x);

            simdpp::float32<4> const pAy = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].y,
                pointData[springData[s + 1].PointAIndex].y,
                pointData[springData[s + 2].PointAIndex].y,
                pointData[springData[s + 3].PointAIndex].y);

            simdpp::float32<4> const pBx = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].x,
                pointData[springData[s + 1].PointBIndex].x,
                pointData[springData[s + 2].PointBIndex].x,
                pointData[springData[s + 3].PointBIndex].x);

            simdpp::float32<4> const pBy = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].y,
                pointData[springData[s + 1].PointBIndex].y,
                pointData[springData[s + 2].PointBIndex].y,
                pointData[springData[s + 3].PointBIndex].y);

            simdpp::float32<4> const displacementX = pBx - pAx;
            simdpp::float32<4> const displacementY = pBy - pAy;

            simdpp::float32<4> const _dx = displacementX * displacementX; // x^2
            simdpp::float32<4> const _dy = displacementY * displacementY; // y^2

            simdpp::float32<4> const _dxy = _dx + _dy;

            simdpp::mask_float32<4> const validMask = (_dxy != 0.0f);

            simdpp::float32<4> reciprocalSpringLength = simdpp::rsqrt_e(_dxy);

            // L==0 => 1/L == 0, to maintain normal == (0, 0) from vec2f
            reciprocalSpringLength = simdpp::bit_and(reciprocalSpringLength, validMask);

            simdpp::store(lengthData + s, reciprocalSpringLength);

            simdpp::store(dirXData + s, displacementX * reciprocalSpringLength);
            simdpp::store(dirYData + s, displacementY * reciprocalSpringLength);
        }
    }

    benchmark::DoNotOptimize(dirX);
    benchmark::DoNotOptimize(dirY);
    benchmark::DoNotOptimize(lengths);
}
BENCHMARK(VectorNormalization_Vectorized_AndLength_Reciprocal_LibSimDpp_ExplodedResults);

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

static void VectorNormalization_TwoPass_VectorizedSecondPass(benchmark::State & state)
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

        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> xx = simdpp::load<simdpp::float32<4>>(xBuffer + s);
            simdpp::float32<4> yy = simdpp::load<simdpp::float32<4>>(yBuffer + s);
                
            simdpp::float32<4> const ll = simdpp::sqrt(xx * xx + yy * yy);

            xx = xx / ll;
            yy = yy / ll;

            simdpp::mask_float32<4> const validMask = (ll != 0.0f);

            xx = simdpp::bit_and(xx, validMask);
            yy = simdpp::bit_and(yy, validMask);

            simdpp::store(xBuffer + s, xx);
            simdpp::store(yBuffer + s, yy);
            simdpp::store(resultLengthBuffer + s, ll);
        }
    }

    benchmark::DoNotOptimize(x);
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(resultLengths);
}
BENCHMARK(VectorNormalization_TwoPass_VectorizedSecondPass);

static void VectorNormalization_TwoPass_UnrolledFirstPass_VectorizedSecondPass(benchmark::State & state)
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

        for (size_t s = 0; s < size; s += 4)
        {
            xBuffer[s] = points[springs[s].PointBIndex].x - points[springs[s].PointAIndex].x;
            yBuffer[s] = points[springs[s].PointBIndex].y - points[springs[s].PointAIndex].y;

            xBuffer[s + 1] = points[springs[s + 1].PointBIndex].x - points[springs[s + 1].PointAIndex].x;
            yBuffer[s + 1] = points[springs[s + 1].PointBIndex].y - points[springs[s + 1].PointAIndex].y;

            xBuffer[s + 2] = points[springs[s + 2].PointBIndex].x - points[springs[s + 2].PointAIndex].x;
            yBuffer[s + 2] = points[springs[s + 2].PointBIndex].y - points[springs[s + 2].PointAIndex].y;

            xBuffer[s + 3] = points[springs[s + 3].PointBIndex].x - points[springs[s + 3].PointAIndex].x;
            yBuffer[s + 3] = points[springs[s + 3].PointBIndex].y - points[springs[s + 3].PointAIndex].y;

        }

        //
        // Pass 2
        //

        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> xx = simdpp::load<simdpp::float32<4>>(xBuffer + s);
            simdpp::float32<4> yy = simdpp::load<simdpp::float32<4>>(yBuffer + s);

            simdpp::float32<4> const ll = simdpp::sqrt(xx * xx + yy * yy);

            xx = xx / ll;
            yy = yy / ll;

            simdpp::mask_float32<4> const validMask = (ll != 0.0f);

            xx = simdpp::bit_and(xx, validMask);
            yy = simdpp::bit_and(yy, validMask);

            simdpp::store(xBuffer + s, xx);
            simdpp::store(yBuffer + s, yy);
            simdpp::store(resultLengthBuffer + s, ll);
        }
    }

    benchmark::DoNotOptimize(x);
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(resultLengths);
}
BENCHMARK(VectorNormalization_TwoPass_UnrolledFirstPass_VectorizedSecondPass);
*/

static void VectorNormalization_ExplodedResults(benchmark::State & state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> points;
    std::vector<SpringEndpoints> springs;
    MakeGraph(size, points, springs);

    vec2f const * restrict pointData = points.data();
    SpringEndpoints const * restrict springData = springs.data();


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
        for (size_t s = 0; s < size; s += 4)
        {
            simdpp::float32<4> pAx = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].x,
                pointData[springData[s + 1].PointAIndex].x,
                pointData[springData[s + 2].PointAIndex].x,
                pointData[springData[s + 3].PointAIndex].x);

            simdpp::float32<4> pAy = simdpp::make_float(
                pointData[springData[s + 0].PointAIndex].y,
                pointData[springData[s + 1].PointAIndex].y,
                pointData[springData[s + 2].PointAIndex].y,
                pointData[springData[s + 3].PointAIndex].y);

            simdpp::float32<4> pBx = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].x,
                pointData[springData[s + 1].PointBIndex].x,
                pointData[springData[s + 2].PointBIndex].x,
                pointData[springData[s + 3].PointBIndex].x);

            simdpp::float32<4> pBy = simdpp::make_float(
                pointData[springData[s + 0].PointBIndex].y,
                pointData[springData[s + 1].PointBIndex].y,
                pointData[springData[s + 2].PointBIndex].y,
                pointData[springData[s + 3].PointBIndex].y);

            simdpp::float32<4> dirX = pBx - pAx;
            simdpp::float32<4> dirY = pBy - pAy;

            simdpp::float32<4> _dx = dirX * dirX;
            simdpp::float32<4> _dy = dirY * dirY;

            simdpp::float32<4> _dxy = _dx + _dy;

            simdpp::float32<4> springLength = simdpp::sqrt(_dxy);

            dirX = dirX / springLength;
            dirY = dirY / springLength;

            simdpp::mask_float32<4> validMask = (springLength != 0.0f);

            dirX = simdpp::bit_and(dirX, validMask);
            dirY = simdpp::bit_and(dirY, validMask);

            simdpp::store(xBuffer + s, dirX);
            simdpp::store(yBuffer + s, dirY);
            simdpp::store(resultLengthBuffer + s, springLength);
        }
    }

    benchmark::DoNotOptimize(x);
    benchmark::DoNotOptimize(y);
    benchmark::DoNotOptimize(resultLengths);
}
BENCHMARK(VectorNormalization_ExplodedResults);
