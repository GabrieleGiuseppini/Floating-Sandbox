/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/SysSpecifics.h>

namespace Physics {

void Ship::RecalculateSpringRelaxationParallelism(size_t simulationParallelism)
{
    // Clear threading state
    mSpringRelaxationTasks.clear();

    //
    // Given the available simulation parallelism as a constraint (max), calculate 
    // the best parallelism for the spring relaxation algorithm
    // 

    // Number of 4-spring blocks per thread, assuming we use all parallelism
    ElementCount const numberOfSprings = static_cast<ElementCount>(mSprings.GetElementCount());

    // Springs -> Threads:
    //     1,000 : 1t = 113  2t = 124  3t = 140  4t = 5t = 6t = 8t =
    //    10,000 : 1t = 1070  2t = 1100  3t = 1200  4t = 5t = 6t = 8t =
    //    50,000 : 1t = 5200  2t = 4400  3t = 4060  4t = 3600  5t = 3700  6t = 8t =
    //   100,000 : 1t = 10600  2t = 8100  3t = 6800  4t = 6100  5t = 6300  6t = 8t =
    //   500,000 : 1t = 57000  2t = 38000  3t = 31000  4t = 28000  5t = 31000  6t = 29000  8t =
    // 1,000,000 : 1t = 140000  2t = 89000  3t = 75000  4t = 68000  5t = 76000  6t = 7t = 8t = 128000

    size_t springRelaxationParallelism;
    if (numberOfSprings < 50000)
    {
        // Not worth it
        springRelaxationParallelism = 1;
    }
    else
    {
        springRelaxationParallelism = std::min(size_t(4), simulationParallelism);
    }

    // TODOTEST
    springRelaxationParallelism = 5;

    LogMessage("Ship::RecalculateSpringRelaxationParallelism: springs=", mSprings.GetElementCount(), " simulationParallelism=", simulationParallelism,
        " springRelaxationParallelism=", springRelaxationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(springRelaxationParallelism);

    //
    // Prepare tasks
    //

    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(springRelaxationParallelism) * 4);

    ElementIndex springStart = 0;
    for (size_t t = 0; t < springRelaxationParallelism; ++t)
    {
        ElementIndex const springEnd = (t < springRelaxationParallelism - 1)
            ? springStart + numberOfFourSpringsPerThread * 4
            : numberOfSprings;

        vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(t);

        mSpringRelaxationTasks.emplace_back(
            [this, springStart, springEnd, dynamicForceBuffer]()
            {
                ApplySpringsForces(
                    springStart,
                    springEnd,
                    dynamicForceBuffer);
            });

        springStart = springEnd;
    }
}

void Ship::RunSpringRelaxationAndDynamicForcesIntegration(
    GameParameters const & gameParameters,
    ThreadManager & threadManager)
{    
    int const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<int>();

    // We run ocean floor collision handling every so often
    int constexpr SeaFloorCollisionPeriod = 2;
    float const seaFloorCollisionDt = gameParameters.MechanicalSimulationStepTimeDuration<float>() * static_cast<float>(SeaFloorCollisionPeriod);

    auto & threadPool = threadManager.GetSimulationThreadPool();

    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0 | others at first iteration only

        // Apply spring forces
        threadPool.Run(mSpringRelaxationTasks);

        // - DynamicForces = sf | sf + others at first iteration only

        // Integrate dynamic and static forces,
        // and reset dynamic forces
        IntegrateAndResetDynamicForces(gameParameters);

        // - DynamicForces = 0

        if ((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1)
        {
            // Handle collisions with sea floor
            //  - Changes position and velocity
            HandleCollisionsWithSeaFloor(
                seaFloorCollisionDt,
                gameParameters);
        }
    }

#ifdef _DEBUG
    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::IntegrateAndResetDynamicForces(GameParameters const & gameParameters)
{
    switch (mSpringRelaxationTasks.size())
    {
        case 1:
        {
            IntegrateAndResetDynamicForces_1(gameParameters);
            break;
        }

        case 2:
        {
            IntegrateAndResetDynamicForces_2(gameParameters);
            break;
        }

        case 3:
        {
            IntegrateAndResetDynamicForces_3(gameParameters);
            break;
        }

        case 4:
        {
            IntegrateAndResetDynamicForces_4(gameParameters);
            break;
        }

        default:
        {
            IntegrateAndResetDynamicForces_N(mSpringRelaxationTasks.size(), gameParameters);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////
// SSE
///////////////////////////////////////////////////////////////

#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()

void Ship::ApplySpringsForces(
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex, // Excluded
    vec2f * restrict dynamicForceBuffer)
{
    // This implementation is for 4-float SSE
    static_assert(vectorization_float_count<int> >= 4);

    vec2f const * restrict const positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f const * restrict const velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    Springs::Endpoints const * restrict const endpointsBuffer = mSprings.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = mSprings.GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSprings.GetStiffnessCoefficientBuffer();
    float const * restrict const dampingCoefficientBuffer = mSprings.GetDampingCoefficientBuffer();

    __m128 const Zero = _mm_setzero_ps();
    aligned_to_vword vec2f tmpSpringForces[4];

    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, mSprings.GetPerfectSquareCount() * 4);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
        // XMM register notation:
        //   low (left, or top) -> height (right, or bottom)

        //
        //    J          M   ---  a
        //    |\        /|
        //    | \s0  s1/ |
        //    |  \    /  |
        //  s2|   \  /   |s3
        //    |    \/    |
        //    |    /\    |
        //    |   /  \   |
        //    |  /    \  |
        //    | /      \ |
        //    |/        \|
        //    K          L  ---  b
        //

        //
        // Calculate displacements, string lengths, and spring directions
        //
        // Steps:
        // 
        // l_pos_x   -   j_pos_x   =  s0_dis_x
        // l_pos_y   -   j_pos_y   =  s0_dis_y
        // k_pos_x   -   m_pos_x   =  s1_dis_x
        // k_pos_y   -   m_pos_y   =  s1_dis_y
        // 
        // Swap 2H with 2L in first register, then:
        // 
        // k_pos_x   -   j_pos_x   =  s2_dis_x
        // k_pos_y   -   j_pos_y   =  s2_dis_y
        // l_pos_x   -   m_pos_x   =  s3_dis_x
        // l_pos_y   -   m_pos_y   =  s3_dis_y
        // 

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;
        
        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        // ?_pos_x
        // ?_pos_y
        // *
        // *
        __m128 const j_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointJIndex)));
        __m128 const k_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointKIndex)));
        __m128 const l_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointLIndex)));
        __m128 const m_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + pointMIndex)));

        __m128 const jm_pos_xy = _mm_movelh_ps(j_pos_xy, m_pos_xy); // First argument goes low
        __m128 lk_pos_xy = _mm_movelh_ps(l_pos_xy, k_pos_xy); // First argument goes low
        __m128 const s0s1_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);
        lk_pos_xy = _mm_shuffle_ps(lk_pos_xy, lk_pos_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);

        // Shuffle:
        //
        // s0_dis_x     s0_dis_y
        // s1_dis_x     s1_dis_y
        // s2_dis_x     s2_dis_y
        // s3_dis_x     s3_dis_y
        __m128 const s0s1s2s3_dis_x = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0x88);
        __m128 const s0s1s2s3_dis_y = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0xDD);

        // Calculate spring lengths: sqrt( x*x + y*y )
        //
        // Note: the kung-fu below (reciprocal square, then reciprocal, etc.) should be faster:
        //
        //  Standard: sqrt 12, (div 11, and 1), (div 11, and 1) = 5instrs/36cycles
        //  This one: rsqrt 4, and 1, (mul 4), (mul 4), rec 4, and 1 = 6instrs/18cycles

        __m128 const sq_len =
            _mm_add_ps(
                _mm_mul_ps(s0s1s2s3_dis_x, s0s1s2s3_dis_x),
                _mm_mul_ps(s0s1s2s3_dis_y, s0s1s2s3_dis_y));

        __m128 const validMask = _mm_cmpneq_ps(sq_len, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f        

        __m128 const s0s1s2s3_springLength_inv =
            _mm_and_ps(
                _mm_rsqrt_ps(sq_len),
                validMask);

        __m128 const s0s1s2s3_springLength =
            _mm_and_ps(
                _mm_rcp_ps(s0s1s2s3_springLength_inv),
                validMask);

        // Calculate spring directions        
        __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_dis_x, s0s1s2s3_springLength_inv);
        __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_dis_y, s0s1s2s3_springLength_inv);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        __m128 const s0s1s2s3_hooke_forceModuli =
            _mm_mul_ps(
                _mm_sub_ps(
                    s0s1s2s3_springLength,
                    _mm_load_ps(restLengthBuffer + s)),
                _mm_load_ps(stiffnessCoefficientBuffer + s));

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy: 
        // 
        // (s0_relv_x * s0_sdir_x  +  s0_relv_y * s0_sdir_y) * dampCoeff[s0]
        // (s1_relv_x * s1_sdir_x  +  s1_relv_y * s1_sdir_y) * dampCoeff[s1]
        // (s2_relv_x * s2_sdir_x  +  s2_relv_y * s2_sdir_y) * dampCoeff[s2]
        // (s3_relv_x * s3_sdir_x  +  s3_relv_y * s3_sdir_y) * dampCoeff[s3]
        //

        // ?_vel_x
        // ?_vel_y
        // *
        // *
        __m128 const j_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointJIndex)));
        __m128 const k_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointKIndex)));
        __m128 const l_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointLIndex)));
        __m128 const m_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + pointMIndex)));

        __m128 const jm_vel_xy = _mm_movelh_ps(j_vel_xy, m_vel_xy); // First argument goes low
        __m128 lk_vel_xy = _mm_movelh_ps(l_vel_xy, k_vel_xy); // First argument goes low
        __m128 const s0s1_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);
        lk_vel_xy = _mm_shuffle_ps(lk_vel_xy, lk_vel_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);

        __m128 s0s1s2s3_rvel_x = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0x88);
        __m128 s0s1s2s3_rvel_y = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0xDD);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_rvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_rvel_y, s0s1s2s3_sdir_y)),
                _mm_load_ps(dampingCoefficientBuffer + s));

        //
        // 3. Apply forces: 
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  s0_tforce_a_x  =   s0_sdir_x  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  s1_tforce_a_x  =   s1_sdir_x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_x  =   s2_sdir_x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_x  =   s3_sdir_x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  s0_tforce_a_y  =   s0_sdir_y  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  s1_tforce_a_y  =   s1_sdir_y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_y  =   s2_sdir_y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_y  =   s3_sdir_y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        __m128 const tForceModuli = _mm_add_ps(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        __m128 const s0s1s2s3_tforceA_x =
            _mm_mul_ps(
                s0s1s2s3_sdir_x,
                tForceModuli);

        __m128 const s0s1s2s3_tforceA_y =
            _mm_mul_ps(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      dynamicForceBuffer[pointAIndex] += total_forceA;
        //      dynamicForceBuffer[pointBIndex] -= total_forceA;
        //
        // j_sforce += s0_a_tforce + s2_a_tforce
        // m_sforce += s1_a_tforce + s3_a_tforce
        // 
        // l_sforce -= s0_a_tforce + s3_a_tforce
        // k_sforce -= s1_a_tforce + s2_a_tforce


        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        __m128 const jm_sforce_xy = _mm_add_ps(s0s1_tforceA_xy, s2s3_tforceA_xy);
        s2s3_tforceA_xy = _mm_shuffle_ps(s2s3_tforceA_xy, s2s3_tforceA_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const lk_sforce_xy = _mm_add_ps(s0s1_tforceA_xy, s2s3_tforceA_xy);

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), jm_sforce_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), lk_sforce_xy);

        dynamicForceBuffer[pointJIndex] += tmpSpringForces[0];
        dynamicForceBuffer[pointMIndex] += tmpSpringForces[1];
        dynamicForceBuffer[pointLIndex] -= tmpSpringForces[2];
        dynamicForceBuffer[pointKIndex] -= tmpSpringForces[3];
    }

    //
    // 2. Remaining four-by-four's
    //

    ElementCount const endSpringIndexVectorized = endSpringIndex - (endSpringIndex % 4);

    for (; s < endSpringIndexVectorized; s += 4)
    {
        // Spring 0 displacement (s0_position.x, s0_position.y, *, *)
        __m128 const s0pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_displacement.x, s0_displacement.y, *, *
        __m128 const s0_displacement_xy = _mm_sub_ps(s0pb_pos_xy, s0pa_pos_xy);

        // Spring 1 displacement (s1_position.x, s1_position.y, *, *)
        __m128 const s1pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_displacement.x, s1_displacement.y
        __m128 const s1_displacement_xy = _mm_sub_ps(s1pb_pos_xy, s1pa_pos_xy);

        // s0_displacement.x, s0_displacement.y, s1_displacement.x, s1_displacement.y
        __m128 const s0s1_displacement_xy = _mm_movelh_ps(s0_displacement_xy, s1_displacement_xy); // First argument goes low

        // Spring 2 displacement (s2_position.x, s2_position.y, *, *)
        __m128 const s2pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_displacement.x, s2_displacement.y
        __m128 const s2_displacement_xy = _mm_sub_ps(s2pb_pos_xy, s2pa_pos_xy);

        // Spring 3 displacement (s3_position.x, s3_position.y, *, *)
        __m128 const s3pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_displacement.x, s3_displacement.y
        __m128 const s3_displacement_xy = _mm_sub_ps(s3pb_pos_xy, s3pa_pos_xy);

        // s2_displacement.x, s2_displacement.y, s3_displacement.x, s3_displacement.y
        __m128 const s2s3_displacement_xy = _mm_movelh_ps(s2_displacement_xy, s3_displacement_xy); // First argument goes low

        // Shuffle displacements:
        // s0_displacement.x, s1_displacement.x, s2_displacement.x, s3_displacement.x
        __m128 s0s1s2s3_displacement_x = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0x88);
        // s0_displacement.y, s1_displacement.y, s2_displacement.y, s3_displacement.y
        __m128 s0s1s2s3_displacement_y = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0xDD);

        // Calculate spring lengths

        // s0_displacement.x^2, s1_displacement.x^2, s2_displacement.x^2, s3_displacement.x^2
        __m128 const s0s1s2s3_displacement_x2 = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_displacement_x);
        // s0_displacement.y^2, s1_displacement.y^2, s2_displacement.y^2, s3_displacement.y^2
        __m128 const s0s1s2s3_displacement_y2 = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_displacement_y);

        // s0_displacement.x^2 + s0_displacement.y^2, s1_displacement.x^2 + s1_displacement.y^2, s2_displacement..., s3_displacement...
        __m128 const s0s1s2s3_displacement_x2_p_y2 = _mm_add_ps(s0s1s2s3_displacement_x2, s0s1s2s3_displacement_y2);

        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_displacement_x2_p_y2, Zero);

        __m128 const s0s1s2s3_springLength_inv =
            _mm_and_ps(
                _mm_rsqrt_ps(s0s1s2s3_displacement_x2_p_y2),
                validMask);

        __m128 const s0s1s2s3_springLength =
            _mm_and_ps(
                _mm_rcp_ps(s0s1s2s3_springLength_inv),
                validMask);

        // Calculate spring directions
        __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength_inv);
        __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength_inv);
        
        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        __m128 const s0s1s2s3_restLength = _mm_load_ps(restLengthBuffer + s);
        __m128 const s0s1s2s3_stiffness = _mm_load_ps(stiffnessCoefficientBuffer + s);

        __m128 const s0s1s2s3_hooke_forceModuli = _mm_mul_ps(
            _mm_sub_ps(s0s1s2s3_springLength, s0s1s2s3_restLength),
            s0s1s2s3_stiffness);

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy: 
        //
        // ( relV[s0].x * sprDir[s0].x  +  relV[s0].y * sprDir[s0].y )  *  dampCoeff[s0]
        // ( relV[s1].x * sprDir[s1].x  +  relV[s1].y * sprDir[s1].y )  *  dampCoeff[s1]
        // ( relV[s2].x * sprDir[s2].x  +  relV[s2].y * sprDir[s2].y )  *  dampCoeff[s2]
        // ( relV[s3].x * sprDir[s3].x  +  relV[s3].y * sprDir[s3].y )  *  dampCoeff[s3]
        //

        // Spring 0 rel vel (s0_vel.x, s0_vel.y, *, *)
        __m128 const s0pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_relvel_x, s0_relvel_y, *, *
        __m128 const s0_relvel_xy = _mm_sub_ps(s0pb_vel_xy, s0pa_vel_xy);

        // Spring 1 rel vel (s1_vel.x, s1_vel.y, *, *)
        __m128 const s1pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_relvel_x, s1_relvel_y, *, *
        __m128 const s1_relvel_xy = _mm_sub_ps(s1pb_vel_xy, s1pa_vel_xy);

        // s0_relvel.x, s0_relvel.y, s1_relvel.x, s1_relvel.y
        __m128 const s0s1_relvel_xy = _mm_movelh_ps(s0_relvel_xy, s1_relvel_xy); // First argument goes low

        // Spring 2 rel vel (s2_vel.x, s2_vel.y, *, *)
        __m128 const s2pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_relvel_x, s2_relvel_y, *, *
        __m128 const s2_relvel_xy = _mm_sub_ps(s2pb_vel_xy, s2pa_vel_xy);

        // Spring 3 rel vel (s3_vel.x, s3_vel.y, *, *)
        __m128 const s3pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_relvel_x, s3_relvel_y, *, *
        __m128 const s3_relvel_xy = _mm_sub_ps(s3pb_vel_xy, s3pa_vel_xy);

        // s2_relvel.x, s2_relvel.y, s3_relvel.x, s3_relvel.y
        __m128 const s2s3_relvel_xy = _mm_movelh_ps(s2_relvel_xy, s3_relvel_xy); // First argument goes low

        // Shuffle rel vals:
        // s0_relvel.x, s1_relvel.x, s2_relvel.x, s3_relvel.x
        __m128 s0s1s2s3_relvel_x = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0x88);
        // s0_relvel.y, s1_relvel.y, s2_relvel.y, s3_relvel.y
        __m128 s0s1s2s3_relvel_y = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0xDD);

        // Damping coeffs
        __m128 const s0s1s2s3_dampingCoeff = _mm_load_ps(dampingCoefficientBuffer + s);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_relvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_relvel_y, s0s1s2s3_sdir_y)),
                s0s1s2s3_dampingCoeff);

        //
        // 3. Apply forces: 
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  total_forceA[s0].x  =   springDir[s0].x  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  total_forceA[s1].x  =   springDir[s1].x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].x  =   springDir[s2].x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].x  =   springDir[s3].x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  total_forceA[s0].y  =   springDir[s0].y  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  total_forceA[s1].y  =   springDir[s1].y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].y  =   springDir[s2].y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].y  =   springDir[s3].y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        __m128 const tForceModuli = _mm_add_ps(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        __m128 const s0s1s2s3_tforceA_x =
            _mm_mul_ps(
                s0s1s2s3_sdir_x,
                tForceModuli);

        __m128 const s0s1s2s3_tforceA_y =
            _mm_mul_ps(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      pointSpringForceBuffer[pointAIndex] += total_forceA;
        //      pointSpringForceBuffer[pointBIndex] -= total_forceA;
        //

        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), s0s1_tforceA_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), s2s3_tforceA_xy);

        dynamicForceBuffer[endpointsBuffer[s + 0].PointAIndex] += tmpSpringForces[0];
        dynamicForceBuffer[endpointsBuffer[s + 0].PointBIndex] -= tmpSpringForces[0];
        dynamicForceBuffer[endpointsBuffer[s + 1].PointAIndex] += tmpSpringForces[1];
        dynamicForceBuffer[endpointsBuffer[s + 1].PointBIndex] -= tmpSpringForces[1];
        dynamicForceBuffer[endpointsBuffer[s + 2].PointAIndex] += tmpSpringForces[2];
        dynamicForceBuffer[endpointsBuffer[s + 2].PointBIndex] -= tmpSpringForces[2];
        dynamicForceBuffer[endpointsBuffer[s + 3].PointAIndex] += tmpSpringForces[3];
        dynamicForceBuffer[endpointsBuffer[s + 3].PointBIndex] -= tmpSpringForces[3];
    }

    //
    // 3. One-by-one
    //

    for (; s < endSpringIndex; ++s)
    {
        auto const pointAIndex = endpointsBuffer[s].PointAIndex;
        auto const pointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = positionBuffer[pointBIndex] - positionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[s])
            * stiffnessCoefficientBuffer[s];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = velocityBuffer[pointBIndex] - velocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * dampingCoefficientBuffer[s];

        //
        // 3. Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        dynamicForceBuffer[pointAIndex] += forceA;
        dynamicForceBuffer[pointBIndex] -= forceA;
    }
}

void Ship::IntegrateAndResetDynamicForces_N(
    size_t parallelism,
    GameParameters const & gameParameters)
{
    // This implementation is for 4-float SSE
    static_assert(vectorization_float_count<int> >= 4);

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, gameParameters);

    float * restrict const positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float * const restrict * restrict const dynamicForceBufferOfBuffers = mPoints.GetDynamicForceBuffersAsFloat();

    __m128 const zero_4 = _mm_setzero_ps();
    __m128 const dt_4 = _mm_load1_ps(&dt);
    __m128 const velocityFactor_4 = _mm_load1_ps(&velocityFactor);

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; i += 4)
    {
        __m128 springForce_2 = zero_4;
        for (size_t b = 0; b < parallelism; ++b)
        {
            springForce_2 =
                _mm_add_ps(
                    springForce_2,
                    _mm_load_ps(dynamicForceBufferOfBuffers[b] + i));
        }

        // vec2f const deltaPos =
        //    velocityBuffer[i] * dt
        //    + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];
        __m128 const deltaPos_2 =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_load_ps(velocityBuffer + i),
                    dt_4),
                _mm_mul_ps(
                    _mm_add_ps(
                        springForce_2,
                        _mm_load_ps(staticForceBuffer + i)),
                    _mm_load_ps(integrationFactorBuffer + i)));

        // positionBuffer[i] += deltaPos;
        __m128 pos_2 = _mm_load_ps(positionBuffer + i);
        pos_2 = _mm_add_ps(pos_2, deltaPos_2);
        _mm_store_ps(positionBuffer + i, pos_2);

        // velocityBuffer[i] = deltaPos * velocityFactor;
        __m128 const vel_2 =
            _mm_mul_ps(
                deltaPos_2,
                velocityFactor_4);
        _mm_store_ps(velocityBuffer + i, vel_2);

        // Zero out spring forces now that we've integrated them
        for (size_t b = 0; b < parallelism; ++b)
        {
            _mm_store_ps(dynamicForceBufferOfBuffers[b] + i, zero_4);
        }
    }
}

#else

///////////////////////////////////////////////////////////////
// Architecture-agnostic
///////////////////////////////////////////////////////////////

void Ship::ApplySpringsForces(
    ElementIndex startSpringIndex,
    ElementIndex stopSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
    // TODOHERE
}

void Ship::IntegrateAndResetDynamicForces_N(
    size_t parallelism,
    GameParameters const & gameParameters)
{
    // TODOHERE: from SpringLab's Pseudo-vectorized n
}

#endif

float Ship::CalculateIntegrationVelocityFactor(
    float dt, 
    GameParameters const & gameParameters) const
{
    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    //
    // Considering that:
    //
    //  v1 = d*v0
    //  v2 = d*v1 =(d^2)*v0
    //  ...
    //  vN = (d^N)*v0
    //
    // ...the more the number of iterations, the more damped the initial velocity would be.
    // We want damping to be independent from the number of iterations though, so we need to find the value
    // d such that after N iterations the damping is the same as our reference value, which is based on
    // 12 (basis) iterations. For example, double the number of iterations requires square root (1/2) of
    // this value.
    //

    float const globalDamping = 1.0f -
        pow((1.0f - GameParameters::GlobalDamping),
        12.0f / gameParameters.NumMechanicalDynamicsIterations<float>());

    // Incorporate adjustment
    float const globalDampingCoefficient = 1.0f -
        (
            gameParameters.GlobalDampingAdjustment <= 1.0f
            ? globalDamping * (1.0f - (gameParameters.GlobalDampingAdjustment - 1.0f) * (gameParameters.GlobalDampingAdjustment - 1.0f))
            : globalDamping +
                (gameParameters.GlobalDampingAdjustment - 1.0f) * (gameParameters.GlobalDampingAdjustment - 1.0f)
                / ((gameParameters.MaxGlobalDampingAdjustment - 1.0f) * (gameParameters.MaxGlobalDampingAdjustment - 1.0f))
                * (1.0f - globalDamping)
        );

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = globalDampingCoefficient / dt;

    return velocityFactor;
}

void Ship::IntegrateAndResetDynamicForces_1(GameParameters const & gameParameters)
{
    assert(mSpringRelaxationTasks.size() == 1);

    //
    // This loop is compiled with packed SSE instructions on MSVC 2022,
    // integrating two points at each iteration
    //

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, gameParameters);

    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible

    float * restrict const positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float * const restrict dynamicForceBuffer = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(0));

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const totalDynamicForce = dynamicForceBuffer[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (totalDynamicForce + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        dynamicForceBuffer[i] = 0.0f;
    }
}

void Ship::IntegrateAndResetDynamicForces_2(GameParameters const & gameParameters)
{
    assert(mSpringRelaxationTasks.size() == 2);

    //
    // This loop is compiled with packed SSE instructions on MSVC 2022,
    // integrating two points at each iteration
    //

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, gameParameters);

    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible

    float * restrict const positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float * const restrict dynamicForceBuffer1 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(0));
    float * const restrict dynamicForceBuffer2 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(1));

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const totalDynamicForce = dynamicForceBuffer1[i] + dynamicForceBuffer2[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (totalDynamicForce + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        dynamicForceBuffer1[i] = 0.0f;
        dynamicForceBuffer2[i] = 0.0f;
    }
}

void Ship::IntegrateAndResetDynamicForces_3(GameParameters const & gameParameters)
{
    assert(mSpringRelaxationTasks.size() == 3);

    //
    // This loop is compiled with packed SSE instructions on MSVC 2022,
    // integrating two points at each iteration
    //

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, gameParameters);

    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible

    float * restrict const positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float * const restrict dynamicForceBuffer1 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(0));
    float * const restrict dynamicForceBuffer2 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(1));
    float * const restrict dynamicForceBuffer3 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(2));

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const totalDynamicForce = dynamicForceBuffer1[i] + dynamicForceBuffer2[i] + dynamicForceBuffer3[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (totalDynamicForce + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        dynamicForceBuffer1[i] = 0.0f;
        dynamicForceBuffer2[i] = 0.0f;
        dynamicForceBuffer3[i] = 0.0f;
    }
}

void Ship::IntegrateAndResetDynamicForces_4(GameParameters const & gameParameters)
{
    assert(mSpringRelaxationTasks.size() == 4);

    //
    // This loop is compiled with packed SSE instructions on MSVC 2022,
    // integrating two points at each iteration
    //

    float const dt = gameParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, gameParameters);

    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible

    float * restrict const positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict const velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float const * const restrict staticForceBuffer = mPoints.GetStaticForceBufferAsFloat();
    float const * const restrict integrationFactorBuffer = mPoints.GetIntegrationFactorBufferAsFloat();

    float * const restrict dynamicForceBuffer1 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(0));
    float * const restrict dynamicForceBuffer2 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(1));
    float * const restrict dynamicForceBuffer3 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(2));
    float * const restrict dynamicForceBuffer4 = reinterpret_cast<float *>(mPoints.GetParallelDynamicForceBuffer(3));

    size_t const count = mPoints.GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const totalDynamicForce = dynamicForceBuffer1[i] + dynamicForceBuffer2[i] + dynamicForceBuffer3[i] + dynamicForceBuffer4[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (totalDynamicForce + staticForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        dynamicForceBuffer1[i] = 0.0f;
        dynamicForceBuffer2[i] = 0.0f;
        dynamicForceBuffer3[i] = 0.0f;
        dynamicForceBuffer4[i] = 0.0f;
    }
}

}