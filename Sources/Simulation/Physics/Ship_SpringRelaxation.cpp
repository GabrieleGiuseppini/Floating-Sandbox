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
    // Clear threading state
    mSpringRelaxationTasks.clear();

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
#if !FS_IS_PLATFORM_MOBILE()
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
    (void)simulationParameters;
#else
    springRelaxationParallelism = std::min(
        static_cast<size_t>(numberOfSprings / simulationParameters.SpringRelaxationSpringsPerThread),
        simulationParallelism);
#endif

    LogMessage("Ship::RecalculateSpringRelaxationParallelism: springs=", numberOfSprings, " simulationParallelism=", simulationParallelism,
        " springRelaxationParallelism=", springRelaxationParallelism);

    //
    // Given the available simulation parallelism as a constraint (max), calculate
    // the best parallelism for integration and collisions
    //

    ElementCount const numberOfPoints = mPoints.GetBufferElementCount();

    size_t integrationParallelism;
#if !FS_IS_PLATFORM_MOBILE()
    integrationParallelism = std::max(
        std::min(
            numberOfPoints <= 12000 ? size_t(1) : (size_t(1) + (numberOfPoints - 12000) / 4000),
            simulationParallelism),
        size_t(1)); // Capping to 1!
#else
    integrationParallelism = std::min(
        static_cast<size_t>(numberOfPoints / simulationParameters.SpringRelaxationPointsPerThread),
        simulationParallelism);
#endif

    LogMessage("Ship::RecalculateSpringRelaxationParallelism: points=", numberOfPoints, " simulationParallelism=", simulationParallelism,
        " integrationParallelism=", integrationParallelism);

    //
    // Calculate final parallelism
    //

    // TODO: re-check algo: springs par'm and integr par'm are _min_
    // TODO: make sure numberOfSprings/Points >= final_parallelism * vec_float_count, unless we have less than 4

    size_t const commonParallelism = std::max(springRelaxationParallelism, integrationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(commonParallelism);

    //
    // Prepare tasks
    //
    // We want all but the last thread to work on a multiple of the vectorization word size
    //

    // TODO: other asserts here

    //assert(numberOfSprings >= static_cast<ElementCount>(commonParallelism) * vectorization_float_count<ElementCount>); // Commented out because: numberOfSprings may be < 4
    ElementCount const numberOfVecSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(commonParallelism) * vectorization_float_count<ElementCount>);

    //assert((numberOfPoints % (static_cast<ElementCount>(commonParallelism) * vectorization_float_count<ElementCount>)) == 0); // Commented out because: only applies to non-last thread
    assert(numberOfPoints >= static_cast<ElementCount>(commonParallelism) * vectorization_float_count<ElementCount>);
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(commonParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex springStart = 0;
    ElementIndex pointStart = 0;
    for (size_t t = 0; t < commonParallelism; ++t)
    {
        ElementIndex const springEnd = (t < commonParallelism - 1)
            ? springStart + numberOfVecSpringsPerThread * vectorization_float_count<ElementCount>
            : numberOfSprings;

        ElementIndex const pointEnd = (t < commonParallelism - 1)
            ? pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>
            : numberOfPoints;

        mSpringRelaxationTasks.emplace_back(
            [this, t, springStart, springEnd, pointStart, pointEnd, &simulationParameters]()
            {
                RunSpringRelaxation_Thread(
                    t,
                    springStart,
                    springEnd,
                    pointStart,
                    pointEnd,
                    simulationParameters);
            });

        springStart = springEnd;
        pointStart = pointEnd;
    }
}

void Ship::RunSpringRelaxation(ThreadManager & threadManager)
{
    //
    // Prepare inter-thread signals
    //

    mSpringRelaxationSpringForcesCompleted = 0;
    mSpringRelaxationIntegrationsCompleted = 0;

    //
    // Run spring relaxation
    //

    auto & threadPool = threadManager.GetSimulationThreadPool();
    threadPool.Run(mSpringRelaxationTasks);

#ifdef _DEBUG

    //
    // We have dirtied positions
    //

    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::RunSpringRelaxation_Thread(
    size_t threadIndex,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    SimulationParameters const & simulationParameters)
{
    //
    // This routine is run ONCE by each thread - in parallel, each on a different start-end point index slice;
    // threads sync among themselves via spinlocks using atomic counters
    //
    // I really think this is a work of art
    //

    // We run the sea floor collision detection every these many iterations of the spring relaxation loop
    int constexpr SeaFloorCollisionPeriod = 2;

    // Get the dynamic forces buffer dedicated to this thread
    vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(threadIndex);

    // Get the total count of threads participating
    int const numberOfThreads = static_cast<int>(mSpringRelaxationTasks.size());

    //
    // Loop for all mechanical dynamics iterations
    //

    int const numMechanicalDynamicsIterations = simulationParameters.NumMechanicalDynamicsIterations<int>();
    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0 | others at first iteration only

        //
        // Apply spring forces
        //

        Algorithms::ApplySpringsForces(
            mPoints,
            mSprings,
            startSpringIndex,
            endSpringIndex,
            dynamicForceBuffer);

        // - DynamicForces = sf | sf + others at first iteration only

        // Signal completion
        //++mSpringRelaxationSpringForcesCompleted;
        mSpringRelaxationSpringForcesCompleted.fetch_add(1, std::memory_order_acq_rel);

        //
        // Wait for all completions
        //

        // ...in a spinlock
        while (true)
        {
            if (mSpringRelaxationSpringForcesCompleted.load() == (iter + 1) * numberOfThreads)
            {
                break;
            }
        }

        //
        // Integrate dynamic and static forces,
        // and reset dynamic forces
        //

        IntegrateAndResetDynamicForces(
            startPointIndex,
            endPointIndex,
            simulationParameters);

        if ((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1)
        {
            // Handle collisions with sea floor
            //  - Changes position and velocity

            HandleCollisionsWithSeaFloor(
                startPointIndex,
                endPointIndex,
                simulationParameters);
        }

        // - DynamicForces = 0

        // Signal completion
        //++mSpringRelaxationIntegrationsCompleted;
        mSpringRelaxationIntegrationsCompleted.fetch_add(1, std::memory_order_acq_rel);

        //
        // Wait for all completions
        //

        // ...in a spinlock
        while (true)
        {
            if (mSpringRelaxationIntegrationsCompleted.load() == (iter + 1) * numberOfThreads)
            {
                break;
            }
        }
    }
}

void Ship::IntegrateAndResetDynamicForces(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, simulationParameters);

    switch (mSpringRelaxationTasks.size())
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
                mSpringRelaxationTasks.size(),
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                dt,
                velocityFactor);

            break;
        }
    }
}

float Ship::CalculateIntegrationVelocityFactor(
    float dt,
    SimulationParameters const & simulationParameters) const
{
    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    //
    // Considering that:
    //
    //  v1 = d*v0
    //  v2 = d*v1 = (d^2)*v0
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