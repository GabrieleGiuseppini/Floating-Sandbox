#include "Utils.h"

#include <GameLib/GameTypes.h>
#include <GameLib/SysSpecifics.h>
#include <GameLib/Vectors.h>

// TODO: move to GameLib's LibSimdPp.h
//#define SIMDPP_ARCH_X86_SSSE3
#define SIMDPP_ARCH_X86_SSE2
#include "simdpp/simd.h"
#include "intrin.h"

#include "gtest/gtest.h"

#include <iostream>
#include <vector>

TEST(LibSimdPpTests, MulConstant)
{
    std::vector<vec2f> vectors{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 2.0f},
        {0.1f, 0.2f}
    };

    std::vector<vec2f> results{
        vec2f::zero(),
        vec2f::zero(),
        vec2f::zero(),
        vec2f::zero()
    };

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 2; // Vec has two components

    vec2f * restrict vectorsData = vectors.data();
    vec2f * restrict resultsData = results.data();

    for (size_t i = 0; i < vectors.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(vectorsData + i);
        block = block * simdpp::make_float<simdpp::float32v>(2.0f);
        simdpp::store(resultsData + i, block);
    }    

    EXPECT_EQ(vec2f(0.0f, 0.0f), results[0]);
    EXPECT_EQ(vec2f(2.0f, 2.0f), results[1]);
    EXPECT_EQ(vec2f(8.0f, 4.0f), results[2]);
    EXPECT_EQ(vec2f(0.2f, 0.4f), results[3]);
}

TEST(LibSimdPpTests, ReciprocalSquareRoot)
{
    std::vector<float> values{
        1.0f, 2.0, 4.0f, 9.0, 81.0f, 0.0f, 100.0f, 10000.0f
    };

    std::vector<float> results;
    results.resize(values.size());

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 1;

    for (size_t i = 0; i < values.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(values.data() + i);
        block = simdpp::rsqrt_e(block);
        simdpp::store(results.data() + i, block);
    }

    ////EXPECT_EQ(1.0f/1.0f, results[0]);
    ////EXPECT_EQ(1.0f / 2.0f, results[2]);
    ////EXPECT_EQ(1.0f / 3.0f, results[3]);
    ////EXPECT_EQ(1.0f / 9.0f, results[4]);
    ////EXPECT_EQ(1.0f / 10.0f, results[6]);
    ////EXPECT_EQ(1.0f / 100.0f, results[7]);
}

TEST(LibSimdPpTests, ReciprocalSquareRoot_WithMask)
{
    std::vector<float> values{
        1.0f, 2.0, 4.0f, 9.0, 81.0f, 0.0f, 100.0f, 10000.0f
    };

    std::vector<float> results;
    results.resize(values.size());

    static constexpr size_t BatchSize = SIMDPP_FAST_FLOAT32_SIZE / 1;

    float * restrict valuesData = values.data();
    float * restrict resultsData = results.data();

    for (size_t i = 0; i < values.size(); i += BatchSize)
    {
        simdpp::float32v block = simdpp::load(valuesData + i);

        simdpp::mask_float32v validMask = (block != 0.0f);
        block = simdpp::rsqrt_e(block);
        simdpp::float32v result = simdpp::blend(block, simdpp::make_float<simdpp::float32v>(0.0f), validMask);

        simdpp::store(resultsData + i, result);
    }

    EXPECT_EQ(0.0f, results[5]);
}

TEST(LibSimdPpTests, VectorNormalization)
{
    std::vector<vec2f> points{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 82.0f},
        {0.00001f, 0.00002f}
    };

    struct Spring
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
    };

    std::vector<Spring> springs{
        {0, 1},
        {0, 2},
        {0, 3},
        {1, 2},
        {1, 3},
        {2, 3},
        {0, 0},
        {0, 0}
    };


    //
    // Test
    //

    vec2f * const restrict pointData = points.data();
    Spring * const restrict springData = springs.data();

    std::vector<vec2f> springDirs;
    springDirs.resize(springs.size());
    vec2f * restrict springDirsData = springDirs.data();

    std::vector<float> springLengths;
    springLengths.resize(springs.size());
    float * restrict springLengthsData = springLengths.data();




    float testtest1[4] = { 0.f, 1.f, 2.f, 3.f};
    float testtest2[4] = { 4.f, 5.f, 6.f, 7.f};

    simdpp::float32<4> v1 = simdpp::load(testtest1);
    simdpp::float32<4> v2 = simdpp::load(testtest2);

    simdpp::float32<4> vs = simdpp::shuffle2x2<0, 2>(v1, v2); // Exp: 0, 4, 2, 6

    DoSomething(vs);





    ////float testtest[4] = { 1.f, 2.f, 3.f, 4.f };

    ////simdpp::float32<1> v0 = simdpp::load(testtest);
    ////simdpp::float32<1> v1 = simdpp::load(testtest+1);
    ////simdpp::float32<1> v2 = simdpp::load(testtest+2);
    ////simdpp::float32<1> v3 = simdpp::load(testtest+3);
    ////simdpp::float32<2> v01 = simdpp::combine(v0, v1);
    ////simdpp::float32<2> v23 = simdpp::combine(v2, v3);

    ////simdpp::float32<2> v01 = simdpp::load(testtest);
    ////simdpp::float32<2> v23 = simdpp::load(testtest + 2);
    ////simdpp::float32<4> v0123 = simdpp::combine(v01, v23);
    ////
    ////simdpp::float32<4> v02 = v0123 * v0123;

    ////std::cout 
    ////        << simdpp::extract<0>(v0123) << ","
    ////        << simdpp::extract<1>(v0123) << ","
    ////        << simdpp::extract<2>(v0123) << ","
    ////        << simdpp::extract<3>(v0123) << std::endl;



    ////__m128 foo01 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(testtest)));
    ////__m128 foo23 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(testtest + 2)));
    ////__m128 foo0123 = _mm_movelh_ps(foo01, foo23);
    ////DoSomething(foo0123);

    // TODOHERE: make vectorization-size agnostic
    for (size_t s = 0; s < springs.size(); s += 4)
    {
        //simdpp::float32<2> vecA0 = simdpp::load(pointData + springData[s + 0].PointAIndex);
        //simdpp::float32<2> vecA1 = simdpp::load(pointData + springData[s + 1].PointAIndex);
        //simdpp::float32<4> vecA01 = simdpp::combine(vecA0, vecA1); // xa0,ya0,xa1,ya1

        //simdpp::float32<2> vecB0 = simdpp::load(pointData + springData[s + 0].PointBIndex);
        //simdpp::float32<2> vecB1 = simdpp::load(pointData + springData[s + 1].PointBIndex);
        //simdpp::float32<4> vecB01 = simdpp::combine(vecB0, vecB1); // xb0,yb0,xb1,yb1

        //simdpp::float32<4> displacement01 = vecB01 - vecA01; // x0,y0,x1,y1


        //simdpp::float32<2> vecA2 = simdpp::load(pointData + springData[s + 2].PointAIndex);
        //simdpp::float32<2> vecA3 = simdpp::load(pointData + springData[s + 3].PointAIndex);
        //simdpp::float32<4> vecA23 = simdpp::combine(vecA2, vecA3); // xa2,ya2,xa3,ya3

        //simdpp::float32<2> vecB2 = simdpp::load(pointData + springData[s + 2].PointBIndex);
        //simdpp::float32<2> vecB3 = simdpp::load(pointData + springData[s + 3].PointBIndex);
        //simdpp::float32<4> vecB23 = simdpp::combine(vecB2, vecB3); // xb2,yb2,xb3,yb3

        //simdpp::float32<4> displacement23 = vecB23 - vecA23; // x2,y2,x3,y3



        ////// TODO: P0P2,P1P3
        ////__m128 vecA0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 0].PointAIndex)));
        ////__m128 vecA2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 2].PointAIndex)));
        ////__m128 vecA02 = _mm_movelh_ps(vecA0, vecA2);

        ////__m128 vecB0 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 0].PointBIndex)));
        ////__m128 vecB2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 2].PointBIndex)));
        ////__m128 vecB02 = _mm_movelh_ps(vecB0, vecB2);

        ////simdpp::float32<4> displacement02 = simdpp::float32<4>(vecB02) - simdpp::float32<4>(vecA02); // x0,y0,x2,y2

        ////
        ////__m128 vecA1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 1].PointAIndex)));
        ////__m128 vecA3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 3].PointAIndex)));
        ////__m128 vecA13 = _mm_movelh_ps(vecA1, vecA3);

        ////__m128 vecB1 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 1].PointBIndex)));
        ////__m128 vecB3 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double *>(pointData + springData[s + 3].PointBIndex)));
        ////__m128 vecB13 = _mm_movelh_ps(vecB1, vecB3);

        ////simdpp::float32<4> displacement13 = simdpp::float32<4>(vecB13) - simdpp::float32<4>(vecA13); // x1,y1,x3,y3

        ////simdpp::float32<4> displacementX = simdpp::shuffle2x2<0, 2>(displacement02, displacement13); // x0,x1,x2,x3
        ////simdpp::float32<4> displacementY = simdpp::shuffle2x2<1, 3>(displacement02, displacement13); // y0,y1,y2,y3




        // TODO: P0P1,P2P3
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

        simdpp::store(springLengthsData + s, springLength);
        simdpp::store_packed2(springDirsData + s, displacementX, displacementY);
    }

    //
    // Results
    //

    for (size_t s = 0; s < springs.size(); s++)
    {
        vec2f const displacement = points[springs[s].PointBIndex] - points[springs[s].PointAIndex];
        float const springLength = displacement.length();
        vec2f const springDir = displacement.normalise(springLength);

        EXPECT_FLOAT_EQ(springLength, springLengths[s]);
        EXPECT_FLOAT_EQ(springDir.x, springDirs[s].x);
        EXPECT_FLOAT_EQ(springDir.y, springDirs[s].y);
    }
}