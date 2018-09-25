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

TEST(LibSimdPpTests, VectorNormalization_Instrinsics)
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

    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t s = 0; s < springs.size(); s += 4)
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

        _mm_store_ps(springLengthsData + s, springLength);

        __m128 s01 = _mm_unpacklo_ps(displacementX, displacementY);
        __m128 s23 = _mm_unpackhi_ps(displacementX, displacementY);

        _mm_store_ps(reinterpret_cast<float * restrict>(springDirsData + s), s01);
        _mm_store_ps(reinterpret_cast<float * restrict>(springDirsData + s + 2), s23);
    }

    //
    // Results
    //

    for (size_t s = 0; s < springs.size(); s++)
    {
        vec2f const displacement = points[springs[s].PointBIndex] - points[springs[s].PointAIndex];
        float const expectedSpringLength = displacement.length();
        vec2f const expectedSpringDir = displacement.normalise(expectedSpringLength);

        EXPECT_FLOAT_EQ(expectedSpringLength, springLengths[s]);
        EXPECT_FLOAT_EQ(expectedSpringDir.x, springDirs[s].x);
        EXPECT_FLOAT_EQ(expectedSpringDir.y, springDirs[s].y);
    }
}

TEST(LibSimdPpTests, UpdateSpringForces)
{
    std::vector<vec2f> pointsPosition{
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {4.0f, 82.0f},
        {0.00001f, 0.00002f},
    };

    std::vector<vec2f> pointsVelocity{
        {0.0f, 0.0f},
        {11.0f, 12.0f},
        {20.0f, 21.0f},
        {30.0f, 31.0f}
    };

    struct SpringEndpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
    };

    std::vector<SpringEndpoints> springsEndpoint{
        {0, 1},
        {0, 2},
        {0, 3},
        {1, 2},
        {1, 3},
        {2, 3},
        {0, 0},
        {0, 0}
    };

    std::vector<float> springsStiffnessCoefficient{
        10.f,
        11.f,
        12.f,
        13.f,
        14.f,
        15.f,
        16.f,
        17.f
    };

    std::vector<float> springsDamperCoefficient{
        0.1f,
        0.2f,
        0.3f,
        0.4f,
        0.5f,
        0.6f,
        0.7f,
        0.8f
    };

    std::vector<float> springsRestLength{
      1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f
    };


    //
    // Test
    //

    vec2f * const restrict pointsPositionData = pointsPosition.data();
    vec2f * const restrict pointsVelocityData = pointsVelocity.data();
    SpringEndpoints * const restrict springsEndpointData = springsEndpoint.data();
    float * const restrict springsStiffnessCoefficientData = springsStiffnessCoefficient.data();
    float * const restrict springsDamperCoefficientData = springsDamperCoefficient.data();
    float * const restrict springsRestLengthData = springsRestLength.data();

    std::vector<vec2f> pointsForce;
    pointsForce.resize(pointsPosition.size());
    vec2f * restrict pointsForceData = pointsForce.data();

    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t s = 0; s < springsEndpoint.size(); s += 4)
    {
        //
        // s0..3
        // s0.pA.pos.x      
        //

        auto const s0_pA_index = springsEndpointData[s].PointAIndex;
        auto const s0_pB_index = springsEndpointData[s].PointBIndex;
        auto const s1_pA_index = springsEndpointData[s + 1].PointAIndex;
        auto const s1_pB_index = springsEndpointData[s + 1].PointBIndex;
        auto const s2_pA_index = springsEndpointData[s + 2].PointAIndex;
        auto const s2_pB_index = springsEndpointData[s + 2].PointBIndex;
        auto const s3_pA_index = springsEndpointData[s + 3].PointAIndex;
        auto const s3_pB_index = springsEndpointData[s + 3].PointBIndex;

        // s*.p*.x, s*.p*.y
        __m128 s0_pA_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s0_pA_index]))));
        __m128 s0_pB_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s0_pB_index]))));
        __m128 s1_pA_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s1_pA_index]))));
        __m128 s1_pB_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s1_pB_index]))));
        __m128 s2_pA_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s2_pA_index]))));
        __m128 s2_pB_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s2_pB_index]))));
        __m128 s3_pA_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s3_pA_index]))));
        __m128 s3_pB_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsPositionData[s3_pB_index]))));

        __m128 s0s1_pA_pos = _mm_movelh_ps(s0_pA_pos, s1_pA_pos); // x0,y0,x1,y1
        __m128 s2s3_pA_pos = _mm_movelh_ps(s2_pA_pos, s3_pA_pos); // x2,y2,x3,y3
        __m128 s0s1_pB_pos = _mm_movelh_ps(s0_pB_pos, s1_pB_pos); // x0,y0,x1,y1
        __m128 s2s3_pB_pos = _mm_movelh_ps(s2_pB_pos, s3_pB_pos); // x2,y2,x3,y3
        
        simdpp::float32<4> s0s1_deltaPos = simdpp::float32<4>(s0s1_pB_pos) - simdpp::float32<4>(s0s1_pA_pos); // x0,y0,x1,y1
        simdpp::float32<4> s2s3_deltaPos = simdpp::float32<4>(s2s3_pB_pos) - simdpp::float32<4>(s2s3_pA_pos); // x2,y2,x3,y3

        simdpp::float32<4> deltaPosX = simdpp::unzip4_lo(s0s1_deltaPos, s2s3_deltaPos); // x0,x1,x2,x3
        simdpp::float32<4> deltaPosY = simdpp::unzip4_hi(s0s1_deltaPos, s2s3_deltaPos); // y0,y1,y2,y3

        // Calculate normalized spring vector
        simdpp::float32<4> const springLength = simdpp::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);
        simdpp::float32<4> springDirX = deltaPosX / springLength;
        simdpp::float32<4> springDirY = deltaPosY / springLength;
        
        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A

        simdpp::float32<4> fX =
            springDirX
            * (springLength - simdpp::load<simdpp::float32<4>>(&(springsRestLengthData[s])))
            * simdpp::load<simdpp::float32<4>>(&(springsStiffnessCoefficientData[s]));

        simdpp::float32<4> fY =
            springDirY
            * (springLength - simdpp::load<simdpp::float32<4>>(&(springsRestLengthData[s])))
            * simdpp::load<simdpp::float32<4>>(&(springsStiffnessCoefficientData[s]));

        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // TODOHERE

        // Calculate damp force on point A
        ////vec2f const relVelocity = pointsVelocity[pointBIndex] - pointsVelocity[pointAIndex];
        ////vec2f const fDampA =
        ////    springDir
        ////    * relVelocity.dot(springDir)
        ////    * springsDamperCoefficient[springIndex];


        //
        // Zero-out forces where spring length is zero
        //

        simdpp::mask_float32<4> const validMask = (springLength != 0.0f);

        fX = simdpp::bit_and(fX, validMask);
        fY = simdpp::bit_and(fY, validMask);


        //
        // Apply forces
        //

        simdpp::float32<4> s01_f = simdpp::zip4_lo(fX, fY); // x0,y0,x1,y1
        __m128 s1_f = _mm_movehl_ps(s01_f.wrapped(), s01_f.wrapped());
        simdpp::float32<4> s23_f = simdpp::zip4_hi(fX, fY); // x2,y2,x3,y3
        __m128 s3_f = _mm_movehl_ps(s23_f.wrapped(), s23_f.wrapped());

        //
        // S0
        //

        __m128 tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s0_pA_index]))));
        tmpF = _mm_add_ps(tmpF, s01_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s0_pA_index])), _mm_castps_pd(tmpF));

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s0_pB_index]))));
        tmpF = _mm_sub_ps(tmpF, s01_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s0_pB_index])), _mm_castps_pd(tmpF));

        //
        // S1
        //

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s1_pA_index]))));
        tmpF = _mm_add_ps(tmpF, s1_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s1_pA_index])), _mm_castps_pd(tmpF));

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s1_pB_index]))));
        tmpF = _mm_sub_ps(tmpF, s1_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s1_pB_index])), _mm_castps_pd(tmpF));

        //
        // S2
        //

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s2_pA_index]))));
        tmpF = _mm_add_ps(tmpF, s23_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s2_pA_index])), _mm_castps_pd(tmpF));

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s2_pB_index]))));
        tmpF = _mm_sub_ps(tmpF, s23_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s2_pB_index])), _mm_castps_pd(tmpF));

        //
        // S3
        //

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s3_pA_index]))));
        tmpF = _mm_add_ps(tmpF, s3_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s3_pA_index])), _mm_castps_pd(tmpF));

        tmpF = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsForceData[s3_pB_index]))));
        tmpF = _mm_sub_ps(tmpF, s3_f);
        _mm_store_sd(reinterpret_cast<double * restrict>(&(pointsForceData[s3_pB_index])), _mm_castps_pd(tmpF));
    }

    //
    // Results
    //

    std::vector<vec2f> expectedPointsForce;
    expectedPointsForce.resize(pointsPosition.size());

    for (size_t springIndex = 0; springIndex < springsEndpoint.size(); springIndex++)
    {
        auto const pointAIndex = springsEndpoint[springIndex].PointAIndex;
        auto const pointBIndex = springsEndpoint[springIndex].PointBIndex;

        vec2f const displacement = pointsPosition[pointBIndex] - pointsPosition[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        vec2f const fSpringA =
            springDir
            * (displacementLength - springsRestLength[springIndex])
            * springsStiffnessCoefficient[springIndex];


        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = pointsVelocity[pointBIndex] - pointsVelocity[pointAIndex];
        vec2f const fDampA =
            springDir
            * relVelocity.dot(springDir)
            * springsDamperCoefficient[springIndex];


        //
        // Apply forces
        //

        // TODOTEST
        //expectedPointsForce[pointAIndex] += fSpringA + fDampA;
        //expectedPointsForce[pointBIndex] -= fSpringA + fDampA;
        expectedPointsForce[pointAIndex] += fSpringA;
        expectedPointsForce[pointBIndex] -= fSpringA;
    }

    for (int p = 0; p < pointsForce.size(); ++p)
    {
        EXPECT_FLOAT_EQ(expectedPointsForce[p].x, pointsForce[p].x);
        EXPECT_FLOAT_EQ(expectedPointsForce[p].y, pointsForce[p].y);
    }
}