#include "Utils.h"

#include <Core/SysSpecifics.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

static void UpdateSpringForces_Naive(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> pointsPosition;
    std::vector<vec2f> pointsVelocity;
    std::vector<vec2f> pointsForce;
    std::vector<SpringEndpoints> springsEndpoints;
    std::vector<float> springsStiffnessCoefficient;
    std::vector<float> springsDamperCoefficient;
    std::vector<float> springsRestLength;

    MakeGraph2(size, pointsPosition, pointsVelocity, pointsForce,
        springsEndpoints, springsStiffnessCoefficient, springsDamperCoefficient, springsRestLength);

    for (auto _ : state)
    {
        for (size_t springIndex = 0; springIndex < size; ++springIndex)
        {
            auto const pointAIndex = springsEndpoints[springIndex].PointAIndex;
            auto const pointBIndex = springsEndpoints[springIndex].PointBIndex;

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

            pointsForce[pointAIndex] += fSpringA + fDampA;
            pointsForce[pointBIndex] -= fSpringA + fDampA;
        }
    }

    benchmark::DoNotOptimize(pointsForce);
}
BENCHMARK(UpdateSpringForces_Naive);

/* LibSimDpp has been purged
static void UpdateSpringForces_LibSimdPpAndIntrinsics(benchmark::State& state)
{
    auto const size = MakeSize(SampleSize);

    std::vector<vec2f> pointsPosition;
    std::vector<vec2f> pointsVelocity;
    std::vector<vec2f> pointsForce;
    std::vector<SpringEndpoints> springsEndpoints;
    std::vector<float> springsStiffnessCoefficient;
    std::vector<float> springsDamperCoefficient;
    std::vector<float> springsRestLength;

    MakeGraph2(size, pointsPosition, pointsVelocity, pointsForce,
        springsEndpoints, springsStiffnessCoefficient, springsDamperCoefficient, springsRestLength);

    vec2f const * const restrict pointsPositionData = pointsPosition.data();
    vec2f const * const restrict pointsVelocityData = pointsVelocity.data();
    vec2f * const restrict pointsForceData = pointsForce.data();
    SpringEndpoints const * const restrict springsEndpointsData = springsEndpoints.data();
    float const * const restrict springsStiffnessCoefficientData = springsStiffnessCoefficient.data();
    float const * const restrict springsDamperCoefficientData = springsDamperCoefficient.data();
    float const * const restrict springsRestLengthData = springsRestLength.data();


    __m128 const Zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);

    for (auto _ : state)
    {
        for (size_t s = 0; s < springsEndpoints.size(); s += 4)
        {
            //
            // s0..3
            // s0.pA.pos.x
            //

            auto const s0_pA_index = springsEndpointsData[s].PointAIndex;
            auto const s0_pB_index = springsEndpointsData[s].PointBIndex;
            auto const s1_pA_index = springsEndpointsData[s + 1].PointAIndex;
            auto const s1_pB_index = springsEndpointsData[s + 1].PointBIndex;
            auto const s2_pA_index = springsEndpointsData[s + 2].PointAIndex;
            auto const s2_pB_index = springsEndpointsData[s + 2].PointBIndex;
            auto const s3_pA_index = springsEndpointsData[s + 3].PointAIndex;
            auto const s3_pB_index = springsEndpointsData[s + 3].PointBIndex;

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

            // Calculate normalized spring length
            simdpp::float32<4> const springLength = simdpp::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);
            simdpp::mask_float32<4> const validMask = (springLength != 0.0f);
            simdpp::float32<4> const springDirX = simdpp::bit_and(deltaPosX / springLength, validMask);
            simdpp::float32<4> const springDirY = simdpp::bit_and(deltaPosY / springLength, validMask);

            //
            // 1. Hooke's law
            //

            // Scalar force on point A along each spring
            simdpp::float32<4> fS =
                (springLength - simdpp::load<simdpp::float32<4>>(&(springsRestLengthData[s])))
                * simdpp::load<simdpp::float32<4>>(&(springsStiffnessCoefficientData[s]));



            //
            // 2. Damper forces
            //
            // Damp the velocities of the two points, as if the points were also connected by a damper
            // along the same direction as the spring
            //

            // s*.p*.vx, s*.p*.vy
            __m128 s0_pA_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s0_pA_index]))));
            __m128 s0_pB_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s0_pB_index]))));
            __m128 s1_pA_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s1_pA_index]))));
            __m128 s1_pB_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s1_pB_index]))));
            __m128 s2_pA_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s2_pA_index]))));
            __m128 s2_pB_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s2_pB_index]))));
            __m128 s3_pA_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s3_pA_index]))));
            __m128 s3_pB_vel = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(&(pointsVelocityData[s3_pB_index]))));

            __m128 s0s1_pA_vel = _mm_movelh_ps(s0_pA_vel, s1_pA_vel); // x0,y0,x1,y1
            __m128 s2s3_pA_vel = _mm_movelh_ps(s2_pA_vel, s3_pA_vel); // x2,y2,x3,y3
            __m128 s0s1_pB_vel = _mm_movelh_ps(s0_pB_vel, s1_pB_vel); // x0,y0,x1,y1
            __m128 s2s3_pB_vel = _mm_movelh_ps(s2_pB_vel, s3_pB_vel); // x2,y2,x3,y3

            simdpp::float32<4> s0s1_deltaVel = simdpp::float32<4>(s0s1_pB_vel) - simdpp::float32<4>(s0s1_pA_vel); // x0,y0,x1,y1
            simdpp::float32<4> s2s3_deltaVel = simdpp::float32<4>(s2s3_pB_vel) - simdpp::float32<4>(s2s3_pA_vel); // x2,y2,x3,y3

            simdpp::float32<4> deltaVelX = simdpp::unzip4_lo(s0s1_deltaVel, s2s3_deltaVel); // x0,x1,x2,x3
            simdpp::float32<4> deltaVelY = simdpp::unzip4_hi(s0s1_deltaVel, s2s3_deltaVel); // y0,y1,y2,y3

            // deltaVelProjection = deltaVel.dot(springDir)
            simdpp::float32<4> deltaVelProjection = deltaVelX * springDirX + deltaVelY * springDirY;

            // Scalar force on point A along each spring
            fS = fS + (
                deltaVelProjection
                * simdpp::load<simdpp::float32<4>>(&(springsDamperCoefficientData[s])));

            // Make force vectors by multiplying scalar forces with spring normalized vector
            simdpp::float32<4> fX = springDirX * fS;
            simdpp::float32<4> fY = springDirY * fS;


            //
            // Apply forces - must do on each point alone as we might be adding to the same point
            //

            simdpp::float32<4> s01_f = simdpp::zip4_lo(fX, fY);             // f0.x,f0.y (, f1.x, f1.y)
            __m128 s1_f = _mm_movehl_ps(s01_f.wrapped(), s01_f.wrapped());  // f1.x, f1.y
            simdpp::float32<4> s23_f = simdpp::zip4_hi(fX, fY);             // f2.x, f2.y (, f3.x, f4.y)
            __m128 s3_f = _mm_movehl_ps(s23_f.wrapped(), s23_f.wrapped());  // f3.x, f4.y

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
    }

    benchmark::DoNotOptimize(pointsForce);
}
BENCHMARK(UpdateSpringForces_LibSimdPpAndIntrinsics);
*/