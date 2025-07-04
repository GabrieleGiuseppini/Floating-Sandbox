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
    switch (simulationParameters.SpringRelaxationParallelComputationMode)
    {
        case SpringRelaxationParallelComputationModeType::FullSpeed:
        {
            RecalculateSpringRelaxationParallelism_FullSpeed(simulationParallelism, simulationParameters);
            break;
        }

        case SpringRelaxationParallelComputationModeType::StepByStep:
        {
            RecalculateSpringRelaxationParallelism_StepByStep(simulationParallelism, simulationParameters);
            break;
        }

        case SpringRelaxationParallelComputationModeType::Hybrid:
        {
            RecalculateSpringRelaxationParallelism_Hybrid(simulationParallelism, simulationParameters);
            break;
        }
    }
}

void Ship::RecalculateSpringRelaxationParallelism_FullSpeed(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
{
    LogMessage("Ship::RecalculateSpringRelaxationParallelism_FullSpeed: simulationParallelism=", simulationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //
    // We want threads to work on a multiple of the vectorization word size - unless there aren't enough elements
    //

    mSpringRelaxation_FullSpeed_Tasks.clear();

    ElementCount const numberOfSprings = mSprings.GetElementCount();
    ElementCount const numberOfVecSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementCount const numberOfPoints = mPoints.GetBufferElementCount();
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex springStart = 0;
    ElementIndex pointStart = 0;
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = (t < simulationParallelism - 1)
            ? std::min(
                springStart + numberOfVecSpringsPerThread * vectorization_float_count<ElementCount>,
                numberOfSprings)
            : numberOfSprings;

        ElementIndex const pointEnd = (t < simulationParallelism - 1)
            ? std::min(
                pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>,
                numberOfPoints)
            : numberOfPoints;

        mSpringRelaxation_FullSpeed_Tasks.emplace_back(
            [this, t, springStart, springEnd, pointStart, pointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_FullSpeed_Thread(
                    t,
                    springStart,
                    springEnd,
                    pointStart,
                    pointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        springStart = springEnd;
        pointStart = pointEnd;
    }
}

void Ship::RecalculateSpringRelaxationParallelism_StepByStep(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
{
    LogMessage("Ship::RecalculateSpringRelaxationParallelism_StepByStep: simulationParallelism=", simulationParallelism);


    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //
    // We want all but the last thread to work on a multiple of the vectorization word size
    //

    mSpringRelaxation_StepByStep_SpringForcesTasks.clear();
    mSpringRelaxation_StepByStep_IntegrationTasks.clear();
    mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks.clear();

    ElementCount const numberOfSprings = mSprings.GetElementCount();
    ElementCount const numberOfVecSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementCount const numberOfPoints = mPoints.GetBufferElementCount();
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex springStart = 0;
    ElementIndex pointStart = 0;
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = (t < simulationParallelism - 1)
            ? springStart + numberOfVecSpringsPerThread * vectorization_float_count<ElementCount>
            : numberOfSprings;

        vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(t);

        mSpringRelaxation_StepByStep_SpringForcesTasks.emplace_back(
            [this, springStart, springEnd, dynamicForceBuffer]()
            {
                Algorithms::ApplySpringsForces(
                    mPoints,
                    mSprings,
                    springStart,
                    springEnd,
                    dynamicForceBuffer);
            });

        springStart = springEnd;

        ElementIndex const pointEnd = (t < simulationParallelism - 1)
            ? pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>
            : numberOfPoints;

        assert(((pointEnd - pointStart) % vectorization_float_count<ElementCount>) == 0);

        // Note: we store a reference to SimulationParameters in the lambda; this is only safe
        // if SimulationParameters is never re-created

        mSpringRelaxation_StepByStep_IntegrationTasks.emplace_back(
            [this, pointStart, pointEnd, simulationParallelism, &simulationParameters]()
            {
                IntegrateAndResetDynamicForces(
                    pointStart,
                    pointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks.emplace_back(
            [this, pointStart, pointEnd, simulationParallelism, &simulationParameters]()
            {
                IntegrateAndResetDynamicForces(
                    pointStart,
                    pointEnd,
                    simulationParallelism,
                    simulationParameters);

                HandleCollisionsWithSeaFloor(
                    pointStart,
                    pointEnd,
                    simulationParameters);
            });

        pointStart = pointEnd;
    }
}

void Ship::RecalculateSpringRelaxationParallelism_Hybrid(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
{
    LogMessage("Ship::RecalculateSpringRelaxationParallelism_Hybrid: simulationParallelism=", simulationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //
    // We want threads to work on a multiple of the vectorization word size - unless there aren't enough elements
    //

    mSpringRelaxation_Hybrid_1_Tasks.clear();
    mSpringRelaxation_Hybrid_2_Tasks.clear();

    ElementCount const numberOfSprings = mSprings.GetElementCount();
    ElementCount const numberOfVecSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementCount const numberOfPoints = mPoints.GetBufferElementCount();
    ElementCount const numberOfVecPointsPerThread = numberOfPoints / (static_cast<ElementCount>(simulationParallelism) * vectorization_float_count<ElementCount>);

    ElementIndex springStart = 0;
    ElementIndex pointStart = 0;
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = (t < simulationParallelism - 1)
            ? std::min(
                springStart + numberOfVecSpringsPerThread * vectorization_float_count<ElementCount>,
                numberOfSprings)
            : numberOfSprings;

        ElementIndex const pointEnd = (t < simulationParallelism - 1)
            ? std::min(
                pointStart + numberOfVecPointsPerThread * vectorization_float_count<ElementCount>,
                numberOfPoints)
            : numberOfPoints;

        mSpringRelaxation_Hybrid_1_Tasks.emplace_back(
            [this, t, springStart, springEnd, pointStart, pointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_Hybrid_Thread_1(
                    t,
                    springStart,
                    springEnd,
                    pointStart,
                    pointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        mSpringRelaxation_Hybrid_2_Tasks.emplace_back(
            [this, t, springStart, springEnd, pointStart, pointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_Hybrid_Thread_2(
                    t,
                    springStart,
                    springEnd,
                    pointStart,
                    pointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        springStart = springEnd;
        pointStart = pointEnd;
    }
}

void Ship::RunSpringRelaxation(
    ThreadManager & threadManager,
    SimulationParameters const & simulationParameters)
{
    switch (simulationParameters.SpringRelaxationParallelComputationMode)
    {
        case SpringRelaxationParallelComputationModeType::FullSpeed:
        {
            RunSpringRelaxation_FullSpeed(threadManager);
            break;
        }

        case SpringRelaxationParallelComputationModeType::StepByStep:
        {
            RunSpringRelaxation_StepByStep(threadManager, simulationParameters);
            break;
        }

        case SpringRelaxationParallelComputationModeType::Hybrid:
        {
            RunSpringRelaxation_Hybrid(threadManager, simulationParameters);
            break;
        }
    }
}

void Ship::RunSpringRelaxation_FullSpeed(ThreadManager & threadManager)
{
    //
    // Prepare inter-thread signals
    //

    mSpringRelaxation_FullSpeed_SpringForcesCompleted = 0;
    mSpringRelaxation_FullSpeed_IntegrationsCompleted = 0;

    //
    // Run spring relaxation
    //

    auto & threadPool = threadManager.GetSimulationThreadPool();
    threadPool.Run(mSpringRelaxation_FullSpeed_Tasks);

#ifdef _DEBUG
    //
    // We have dirtied positions
    //

    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::RunSpringRelaxation_FullSpeed_Thread(
    size_t threadIndex,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t parallelism,
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
    int const numberOfThreads = static_cast<int>(mSpringRelaxation_FullSpeed_Tasks.size());

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
        mSpringRelaxation_FullSpeed_SpringForcesCompleted.fetch_add(1, std::memory_order_acq_rel);

        //
        // Wait for all completions
        //

        // ...in a spinlock
        while (true)
        {
            if (mSpringRelaxation_FullSpeed_SpringForcesCompleted.load() == (iter + 1) * numberOfThreads)
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
            parallelism,
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
        mSpringRelaxation_FullSpeed_IntegrationsCompleted.fetch_add(1, std::memory_order_acq_rel);

        //
        // Wait for all completions
        //

        // ...in a spinlock
        while (true)
        {
            if (mSpringRelaxation_FullSpeed_IntegrationsCompleted.load() == (iter + 1) * numberOfThreads)
            {
                break;
            }
        }
    }
}

void Ship::RunSpringRelaxation_StepByStep(
    ThreadManager & threadManager,
    SimulationParameters const & simulationParameters)
{
    // We run the sea floor collision detection every these many iterations of the spring relaxation loop
    int constexpr SeaFloorCollisionPeriod = 2;

    auto & threadPool = threadManager.GetSimulationThreadPool();

    int const numMechanicalDynamicsIterations = simulationParameters.NumMechanicalDynamicsIterations<int>();
    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0 | others at first iteration only

        // Apply spring forces
        threadPool.Run(mSpringRelaxation_StepByStep_SpringForcesTasks);

        // - DynamicForces = sf | sf + others at first iteration only

        if ((iter % SeaFloorCollisionPeriod) < SeaFloorCollisionPeriod - 1)
        {
            // Integrate dynamic and static forces,
            // and reset dynamic forces

            threadPool.Run(mSpringRelaxation_StepByStep_IntegrationTasks);
        }
        else
        {
            assert((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1);

            // Integrate dynamic and static forces,
            // and reset dynamic forces

            // Handle collisions with sea floor
            //  - Changes position and velocity

            threadPool.Run(mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks);
        }

        // - DynamicForces = 0
    }

#ifdef _DEBUG
    //
    // We have dirtied positions
    //

    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::RunSpringRelaxation_Hybrid(
    ThreadManager & threadManager,
    SimulationParameters const & simulationParameters)
{
    // We run the sea floor collision detection every these many iterations of the spring relaxation loop
    int constexpr SeaFloorCollisionPeriod = 2;

    auto & threadPool = threadManager.GetSimulationThreadPool();

    int const numMechanicalDynamicsIterations = simulationParameters.NumMechanicalDynamicsIterations<int>();
    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        mSpringRelaxation_Hybrid_IterationCompleted = 0;

        if ((iter % SeaFloorCollisionPeriod) < SeaFloorCollisionPeriod - 1)
        {
            // Integrate dynamic and static forces,
            // and reset dynamic forces

            threadPool.Run(mSpringRelaxation_Hybrid_1_Tasks);
        }
        else
        {
            assert((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1);

            // Integrate dynamic and static forces,
            // and reset dynamic forces

            // Handle collisions with sea floor
            //  - Changes position and velocity

            threadPool.Run(mSpringRelaxation_Hybrid_2_Tasks);
        }
    }

#ifdef _DEBUG
    //
    // We have dirtied positions
    //

    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

void Ship::RunSpringRelaxation_Hybrid_Thread_1(
    size_t threadIndex,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t parallelism,
    SimulationParameters const & simulationParameters)
{
    // Get the dynamic forces buffer dedicated to this thread
    vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(threadIndex);

    // Get the total count of threads participating
    int const numberOfThreads = static_cast<int>(mSpringRelaxation_Hybrid_1_Tasks.size());

    //
    //
    //

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
    mSpringRelaxation_Hybrid_IterationCompleted.fetch_add(1, std::memory_order_acq_rel);

    //
    // Wait for all completions
    //

    // ...in a spinlock
    while (true)
    {
        if (mSpringRelaxation_Hybrid_IterationCompleted.load() == numberOfThreads)
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
        parallelism,
        simulationParameters);
}

void Ship::RunSpringRelaxation_Hybrid_Thread_2(
    size_t threadIndex,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t parallelism,
    SimulationParameters const & simulationParameters)
{
    // Get the dynamic forces buffer dedicated to this thread
    vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(threadIndex);

    // Get the total count of threads participating
    int const numberOfThreads = static_cast<int>(mSpringRelaxation_Hybrid_2_Tasks.size());

    //
    //
    //

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
    mSpringRelaxation_Hybrid_IterationCompleted.fetch_add(1, std::memory_order_acq_rel);

    //
    // Wait for all completions
    //

    // ...in a spinlock
    while (true)
    {
        if (mSpringRelaxation_Hybrid_IterationCompleted.load() == numberOfThreads)
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
        parallelism,
        simulationParameters);

    // Handle collisions with sea floor
    //  - Changes position and velocity

    HandleCollisionsWithSeaFloor(
        startPointIndex,
        endPointIndex,
        simulationParameters);
}

void Ship::IntegrateAndResetDynamicForces(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t parallelism,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.MechanicalSimulationStepTimeDuration<float>();
    float const velocityFactor = CalculateIntegrationVelocityFactor(dt, simulationParameters);

    switch (parallelism)
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
                parallelism,
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