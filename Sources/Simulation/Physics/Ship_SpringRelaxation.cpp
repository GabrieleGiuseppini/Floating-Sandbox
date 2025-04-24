/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/Algorithms.h>
#include <Core/SysSpecifics.h>

namespace Physics {

void Ship::RecalculateSpringRelaxationParallelism(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
{
    RecalculateSpringRelaxationSpringForcesParallelism(simulationParallelism);
    RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism(simulationParallelism, simulationParameters);
}

void Ship::RecalculateSpringRelaxationSpringForcesParallelism(size_t simulationParallelism)
{
    // Clear threading state
    mSpringRelaxationSpringForcesTasks.clear();

    //
    // Given the available simulation parallelism as a constraint (max), calculate
    // the best parallelism for the spring relaxation algorithm
    //

    ElementCount const numberOfSprings = mSprings.GetElementCount();

    // Springs -> Threads:
    //    10,000 : 1t = 800  2t = 970  3t = 1000  4t = 5t = 6t = 8t =
    //    50,000 : 1t = 4000  2t = 3600  3t = 2900  4t = 2900  5t = 3500  6t = 8t =
    // 1,000,000 : 1t = 103000  2t = 66000  3t = 48000  4t = 56000  5t = 64000  6t = 7t = 8t = 122000

    size_t springRelaxationParallelism;
    if (numberOfSprings < 50000)
    {
        // Not worth it
        springRelaxationParallelism = 1;
    }
    else
    {
        // Go for 4 - more than 4 makes algorithm always worse
        springRelaxationParallelism = std::min(size_t(4), simulationParallelism);
    }

    LogMessage("Ship::RecalculateSpringRelaxationSpringForcesParallelism: springs=", numberOfSprings, " simulationParallelism=", simulationParallelism,
        " springRelaxationParallelism=", springRelaxationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(springRelaxationParallelism);

    //
    // Prepare tasks
    //
    // We want all but the last thread to work on a multiple of the vectorization word size
    //

    //assert(numberOfSprings >= static_cast<ElementCount>(springRelaxationParallelism) * vectorization_float_count<ElementCount>); // Commented out because: numberOfSprings may be < 4
    ElementCount const numberOfVecSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(springRelaxationParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex springStart = 0;
    for (size_t t = 0; t < springRelaxationParallelism; ++t)
    {
        ElementIndex const springEnd = (t < springRelaxationParallelism - 1)
            ? springStart + numberOfVecSpringsPerThread * vectorization_float_count<ElementCount>
            : numberOfSprings;

        vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(t);

        mSpringRelaxationSpringForcesTasks.emplace_back(
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

void Ship::RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
{
    // Clear threading state
    mSpringRelaxationIntegrationTasks.clear();
    mSpringRelaxationIntegrationAndSeaFloorCollisionTasks.clear();

    //
    // Given the available simulation parallelism as a constraint (max), calculate
    // the best parallelism for integration and collisions
    //

    ElementCount const numberOfPoints = mPoints.GetBufferElementCount();

    size_t const actualParallelism = std::max(
        std::min(
            numberOfPoints <= 12000 ? size_t(1) : (size_t(1) + (numberOfPoints - 12000) / 4000),
            simulationParallelism),
        size_t(1)); // Capping to 1!

    LogMessage("Ship::RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism: points=", numberOfPoints, " simulationParallelism=", simulationParallelism,
        " actualParallelism=", actualParallelism);

    //
    // Prepare tasks
    //
    // We want each thread to work on a multiple of our vectorization word size
    //

    //assert((numberOfPoints % (static_cast<ElementCount>(actualParallelism) * vectorization_float_count<ElementCount>)) == 0); // Commented out because: only applies to non-last thread
    assert(numberOfPoints >= static_cast<ElementCount>(actualParallelism) * vectorization_float_count<ElementCount>);
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(actualParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex pointStart = 0;
    for (size_t t = 0; t < actualParallelism; ++t)
    {
        ElementIndex const pointEnd = (t < actualParallelism - 1)
            ? pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>
            : numberOfPoints;

        assert(((pointEnd - pointStart) % vectorization_float_count<ElementCount>) == 0);

        // Note: we store a reference to SimulationParameters in the lambda; this is only safe
        // if SimulationParameters is never re-created

        mSpringRelaxationIntegrationTasks.emplace_back(
            [this, pointStart, pointEnd, &simulationParameters]()
            {
                IntegrateAndResetDynamicForces(
                    pointStart,
                    pointEnd,
                    simulationParameters);
            });

        mSpringRelaxationIntegrationAndSeaFloorCollisionTasks.emplace_back(
            [this, pointStart, pointEnd, &simulationParameters]()
            {
                IntegrateAndResetDynamicForces(
                    pointStart,
                    pointEnd,
                    simulationParameters);

                HandleCollisionsWithSeaFloor(
                    pointStart,
                    pointEnd,
                    simulationParameters);
            });

        pointStart = pointEnd;
    }
}

void Ship::RunSpringRelaxationAndDynamicForcesIntegration(
    SimulationParameters const & simulationParameters,
    ThreadManager & threadManager)
{
    // We run the sea floor collision detection every these many iterations of the spring relaxation loop
    int constexpr SeaFloorCollisionPeriod = 2;

    auto & threadPool = threadManager.GetSimulationThreadPool();

    int const numMechanicalDynamicsIterations = simulationParameters.NumMechanicalDynamicsIterations<int>();
    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0 | others at first iteration only

        // Apply spring forces
        threadPool.Run(mSpringRelaxationSpringForcesTasks);

        // - DynamicForces = sf | sf + others at first iteration only

        if ((iter % SeaFloorCollisionPeriod) < SeaFloorCollisionPeriod - 1)
        {
            // Integrate dynamic and static forces,
            // and reset dynamic forces

            threadPool.Run(mSpringRelaxationIntegrationTasks);
        }
        else
        {
            assert((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1);

            // Integrate dynamic and static forces,
            // and reset dynamic forces

            // Handle collisions with sea floor
            //  - Changes position and velocity

            threadPool.Run(mSpringRelaxationIntegrationAndSeaFloorCollisionTasks);
        }

        // - DynamicForces = 0
    }

#ifdef _DEBUG
    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::IntegrateAndResetDynamicForces(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, simulationParameters);

    switch (mSpringRelaxationSpringForcesTasks.size())
    {
        case 1:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 1>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

            break;
        }

        case 2:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 2>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

            break;
        }

        case 3:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 3>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

            break;
        }

        case 4:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 4>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

            break;
        }

        default:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points>(
                mPoints,
                mSpringRelaxationSpringForcesTasks.size(),
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

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
    // 3. Remaining one-by-one's
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

#else

///////////////////////////////////////////////////////////////
// Architecture-agnostic
///////////////////////////////////////////////////////////////

void Ship::ApplySpringsForces(
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    vec2f * restrict dynamicForceBuffer)
{
    static_assert(vectorization_float_count<int> >= 4);

    vec2f const * restrict const positionBuffer = mPoints.GetPositionBufferAsVec2();
    vec2f const * restrict const velocityBuffer = mPoints.GetVelocityBufferAsVec2();

    Springs::Endpoints const * restrict const endpointsBuffer = mSprings.GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = mSprings.GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSprings.GetStiffnessCoefficientBuffer();
    float const * restrict const dampingCoefficientBuffer = mSprings.GetDampingCoefficientBuffer();

    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, mSprings.GetPerfectSquareCount() * 4);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
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

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        vec2f const pointJPos = positionBuffer[pointJIndex];
        vec2f const pointKPos = positionBuffer[pointKIndex];
        vec2f const pointLPos = positionBuffer[pointLIndex];
        vec2f const pointMPos = positionBuffer[pointMIndex];

        vec2f const s0_dis = pointLPos - pointJPos;
        vec2f const s1_dis = pointKPos - pointMPos;
        vec2f const s2_dis = pointKPos - pointJPos;
        vec2f const s3_dis = pointLPos - pointMPos;

        float const s0_len = s0_dis.length();
        float const s1_len = s1_dis.length();
        float const s2_len = s2_dis.length();
        float const s3_len = s3_dis.length();

        vec2f const s0_dir = s0_dis.normalise(s0_len);
        vec2f const s1_dir = s1_dis.normalise(s1_len);
        vec2f const s2_dir = s2_dis.normalise(s2_len);
        vec2f const s3_dir = s3_dis.normalise(s3_len);

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

        float const s0_hookForceMag = (s0_len - restLengthBuffer[s]) * stiffnessCoefficientBuffer[s];
        float const s1_hookForceMag = (s1_len - restLengthBuffer[s + 1]) * stiffnessCoefficientBuffer[s + 1];
        float const s2_hookForceMag = (s2_len - restLengthBuffer[s + 2]) * stiffnessCoefficientBuffer[s + 2];
        float const s3_hookForceMag = (s3_len - restLengthBuffer[s + 3]) * stiffnessCoefficientBuffer[s + 3];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //

        vec2f const pointJVel = velocityBuffer[pointJIndex];
        vec2f const pointKVel = velocityBuffer[pointKIndex];
        vec2f const pointLVel = velocityBuffer[pointLIndex];
        vec2f const pointMVel = velocityBuffer[pointMIndex];

        vec2f const s0_relVel = pointLVel - pointJVel;
        vec2f const s1_relVel = pointKVel - pointMVel;
        vec2f const s2_relVel = pointKVel - pointJVel;
        vec2f const s3_relVel = pointLVel - pointMVel;

        float const s0_dampForceMag = s0_relVel.dot(s0_dir) * dampingCoefficientBuffer[s];
        float const s1_dampForceMag = s1_relVel.dot(s1_dir) * dampingCoefficientBuffer[s + 1];
        float const s2_dampForceMag = s2_relVel.dot(s2_dir) * dampingCoefficientBuffer[s + 2];
        float const s3_dampForceMag = s3_relVel.dot(s3_dir) * dampingCoefficientBuffer[s + 3];

        //
        // 3. Apply forces:
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //

        vec2f const s0_forceA = s0_dir * (s0_hookForceMag + s0_dampForceMag);
        vec2f const s1_forceA = s1_dir * (s1_hookForceMag + s1_dampForceMag);
        vec2f const s2_forceA = s2_dir * (s2_hookForceMag + s2_dampForceMag);
        vec2f const s3_forceA = s3_dir * (s3_hookForceMag + s3_dampForceMag);

        dynamicForceBuffer[pointJIndex] += (s0_forceA + s2_forceA);
        dynamicForceBuffer[pointLIndex] -= (s0_forceA + s3_forceA);
        dynamicForceBuffer[pointMIndex] += (s1_forceA + s3_forceA);
        dynamicForceBuffer[pointKIndex] -= (s1_forceA + s2_forceA);
    }

    //
    // 2. Remaining one-by-one's
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

#endif

float Ship::CalculateIntegrationVelocityFactor(
    float dt,
    SimulationParameters const & simulationParameters) const
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
        pow((1.0f - SimulationParameters::GlobalDamping),
        12.0f / simulationParameters.NumMechanicalDynamicsIterations<float>());

    // Incorporate adjustment
    float const globalDampingCoefficient = 1.0f -
        (
            simulationParameters.GlobalDampingAdjustment <= 1.0f
            ? globalDamping * (1.0f - (simulationParameters.GlobalDampingAdjustment - 1.0f) * (simulationParameters.GlobalDampingAdjustment - 1.0f))
            : globalDamping +
                (simulationParameters.GlobalDampingAdjustment - 1.0f) * (simulationParameters.GlobalDampingAdjustment - 1.0f)
                / ((simulationParameters.MaxGlobalDampingAdjustment - 1.0f) * (simulationParameters.MaxGlobalDampingAdjustment - 1.0f))
                * (1.0f - globalDamping)
        );

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = globalDampingCoefficient / dt;

    return velocityFactor;
}

}