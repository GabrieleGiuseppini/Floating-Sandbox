/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/Algorithms.h>
#include <Core/SysSpecifics.h>

namespace Physics {

namespace /* anonymous */ {

    // For energetic silt cloud impacts, we do not use mass
    // as large masses would always cause clouds, even at low velocities.
    // We pretend a fixed mass just for sanity of the simulation.
    float constexpr SiltImpactKineticEnergyPretendMass = 700.0f;

}

void Ship::RecalculateSpringRelaxationParallelism(
    ThreadPool const & simulationThreadPool,
    SimulationParameters const & simulationParameters)
{
    switch (simulationParameters.SpringRelaxationParallelComputationMode)
    {
        case SpringRelaxationParallelComputationModeType::FullSpeed:
        {
            RecalculateSpringRelaxationParallelism_FullSpeed(simulationThreadPool, simulationParameters);
            break;
        }

        case SpringRelaxationParallelComputationModeType::StepByStep:
        {
            RecalculateSpringRelaxationParallelism_StepByStep(simulationThreadPool, simulationParameters);
            break;
        }

        case SpringRelaxationParallelComputationModeType::Hybrid:
        {
            RecalculateSpringRelaxationParallelism_Hybrid(simulationThreadPool, simulationParameters);
            break;
        }
    }

    // Resize storage for per-thread silt impacts
    mPerThreadSiltImpacts.resize(simulationThreadPool.GetParallelism());
}

void Ship::RecalculateSpringRelaxationParallelism_FullSpeed(
    ThreadPool const & simulationThreadPool,
    SimulationParameters const & simulationParameters)
{
    auto const simulationParallelism = simulationThreadPool.GetParallelism();

    LogMessage("Ship::RecalculateSpringRelaxationParallelism_FullSpeed: simulationParallelism=", simulationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //

    mSpringRelaxation_FullSpeed_Tasks.clear();

    auto const springShards = CalculateSpringRelaxationSpringShards(
        mSprings.GetElementCount(),
        mSprings.GetPerfectSquareCount(),
        simulationThreadPool);

    auto const shipPointShards = CalculatePointShards(
        mPoints.GetAlignedShipPointCount(),
        simulationThreadPool);

    auto const ephemeralPointShards = CalculatePointShards(
        mPoints.GetMaxEphemeralParticleCount(),
        simulationThreadPool);

    ElementIndex springStart = 0;
    ElementIndex shipPointStart = 0;
    ElementIndex ephemeralPointStart = mPoints.GetAlignedShipPointCount();
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = springStart + static_cast<ElementCount>(springShards[t]);
        assert(springEnd <= mSprings.GetElementCount());

        ElementIndex const shipPointEnd = shipPointStart + static_cast<ElementCount>(shipPointShards[t]);
        assert(shipPointEnd <= mPoints.GetAlignedShipPointCount());

        ElementIndex const ephemeralPointEnd = ephemeralPointStart + static_cast<ElementCount>(ephemeralPointShards[t]);
        assert(ephemeralPointEnd <= mPoints.GetBufferElementCount());

        mSpringRelaxation_FullSpeed_Tasks.emplace_back(
            [this, t, springStart, springEnd, shipPointStart, shipPointEnd, ephemeralPointStart, ephemeralPointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_FullSpeed_Thread(
                    t,
                    springStart,
                    springEnd,
                    shipPointStart,
                    shipPointEnd,
                    ephemeralPointStart,
                    ephemeralPointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        springStart = springEnd;
        shipPointStart = shipPointEnd;
        ephemeralPointStart = ephemeralPointEnd;
    }
}

void Ship::RecalculateSpringRelaxationParallelism_StepByStep(
    ThreadPool const & simulationThreadPool,
    SimulationParameters const & simulationParameters)
{
    auto const simulationParallelism = simulationThreadPool.GetParallelism();

    LogMessage("Ship::RecalculateSpringRelaxationParallelism_StepByStep: simulationParallelism=", simulationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //

    mSpringRelaxation_StepByStep_SpringForcesTasks.clear();
    mSpringRelaxation_StepByStep_IntegrationTasks.clear();
    mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks.clear();
    mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionAndEphemeralTasks.clear();

    auto const springShards = CalculateSpringRelaxationSpringShards(
        mSprings.GetElementCount(),
        mSprings.GetPerfectSquareCount(),
        simulationThreadPool);

    auto const shipPointShards = CalculatePointShards(
        mPoints.GetAlignedShipPointCount(),
        simulationThreadPool);

    auto const ephemeralPointShards = CalculatePointShards(
        mPoints.GetMaxEphemeralParticleCount(),
        simulationThreadPool);

    ElementIndex springStart = 0;
    ElementIndex shipPointStart = 0;
    ElementIndex ephemeralPointStart = mPoints.GetAlignedShipPointCount();
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = springStart + static_cast<ElementCount>(springShards[t]);
        assert(springEnd <= mSprings.GetElementCount());

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

        ElementIndex const shipPointEnd = shipPointStart + static_cast<ElementCount>(shipPointShards[t]);
        assert(shipPointEnd <= mPoints.GetAlignedShipPointCount());

        assert(((shipPointEnd - shipPointStart) % vectorization_float_count<ElementCount>) == 0);

        // Note: we store a reference to SimulationParameters in the lambda; this is only safe
        // if SimulationParameters is never re-created

        mSpringRelaxation_StepByStep_IntegrationTasks.emplace_back(
            [this, shipPointStart, shipPointEnd, simulationParallelism]()
            {
                IntegrateAndResetDynamicForces(
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    mSpringRelaxationCoefficients);
            });

        mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks.emplace_back(
            [this, shipPointStart, shipPointEnd, t, simulationParallelism, &simulationParameters]()
            {
                IntegrateAndResetDynamicForces(
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    mSpringRelaxationCoefficients);

                HandleCollisionsWithSeaFloor(
                    shipPointStart,
                    shipPointEnd,
                    t,
                    mSpringRelaxationCoefficients,
                    simulationParameters);
            });

        ElementIndex const ephemeralPointEnd = ephemeralPointStart + static_cast<ElementCount>(ephemeralPointShards[t]);
        assert(ephemeralPointEnd <= mPoints.GetBufferElementCount());

        mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionAndEphemeralTasks.emplace_back(
            [this, shipPointStart, shipPointEnd, ephemeralPointStart, ephemeralPointEnd, t, simulationParallelism, &simulationParameters]()
            {
                // Ship

                IntegrateAndResetDynamicForces(
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    mSpringRelaxationCoefficients);

                HandleCollisionsWithSeaFloor(
                    shipPointStart,
                    shipPointEnd,
                    t,
                    mSpringRelaxationCoefficients,
                    simulationParameters);

                // Ephemeral

                IntegrateAndResetDynamicForces(
                    ephemeralPointStart,
                    ephemeralPointEnd,
                    simulationParallelism,
                    mSpringRelaxationCoefficients_EphemeralParticles);

                HandleCollisionsWithSeaFloor(
                    ephemeralPointStart,
                    ephemeralPointEnd,
                    t,
                    mSpringRelaxationCoefficients_EphemeralParticles,
                    simulationParameters);
            });

        shipPointStart = shipPointEnd;
        ephemeralPointStart = ephemeralPointEnd;
    }
}

void Ship::RecalculateSpringRelaxationParallelism_Hybrid(
    ThreadPool const & simulationThreadPool,
    SimulationParameters const & simulationParameters)
{
    auto const simulationParallelism = simulationThreadPool.GetParallelism();

    LogMessage("Ship::RecalculateSpringRelaxationParallelism_Hybrid: simulationParallelism=", simulationParallelism);

    //
    // Prepare dynamic force buffers
    //

    mPoints.SetDynamicForceParallelism(simulationParallelism);

    //
    // Prepare tasks
    //

    mSpringRelaxation_Hybrid_1_Tasks.clear();
    mSpringRelaxation_Hybrid_2_Tasks.clear();
    mSpringRelaxation_Hybrid_Last_Tasks.clear();

    auto const springShards = CalculateSpringRelaxationSpringShards(
        mSprings.GetElementCount(),
        mSprings.GetPerfectSquareCount(),
        simulationThreadPool);

    auto const shipPointShards = CalculatePointShards(
        mPoints.GetAlignedShipPointCount(),
        simulationThreadPool);

    auto const ephemeralPointShards = CalculatePointShards(
        mPoints.GetMaxEphemeralParticleCount(),
        simulationThreadPool);

    ElementIndex springStart = 0;
    ElementIndex shipPointStart = 0;
    ElementIndex ephemeralPointStart = mPoints.GetAlignedShipPointCount();
    for (size_t t = 0; t < simulationParallelism; ++t)
    {
        ElementIndex const springEnd = springStart + static_cast<ElementCount>(springShards[t]);
        assert(springEnd <= mSprings.GetElementCount());

        ElementIndex const shipPointEnd = shipPointStart + static_cast<ElementCount>(shipPointShards[t]);
        assert(shipPointEnd <= mPoints.GetAlignedShipPointCount());

        mSpringRelaxation_Hybrid_1_Tasks.emplace_back(
            [this, t, springStart, springEnd, shipPointStart, shipPointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_Hybrid_Thread<false>(
                    t,
                    springStart,
                    springEnd,
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        mSpringRelaxation_Hybrid_2_Tasks.emplace_back(
            [this, t, springStart, springEnd, shipPointStart, shipPointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_Hybrid_Thread<true>(
                    t,
                    springStart,
                    springEnd,
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        ElementIndex const ephemeralPointEnd = ephemeralPointStart + static_cast<ElementCount>(ephemeralPointShards[t]);
        assert(ephemeralPointEnd <= mPoints.GetBufferElementCount());

        mSpringRelaxation_Hybrid_Last_Tasks.emplace_back(
            [this, t, springStart, springEnd, shipPointStart, shipPointEnd, ephemeralPointStart, ephemeralPointEnd, simulationParallelism, &simulationParameters]()
            {
                RunSpringRelaxation_Hybrid_Thread<true>(
                    t,
                    springStart,
                    springEnd,
                    shipPointStart,
                    shipPointEnd,
                    simulationParallelism,
                    simulationParameters);

                RunSpringRelaxation_Hybrid_EphemeralParticle_Thread(
                    t,
                    ephemeralPointStart,
                    ephemeralPointEnd,
                    simulationParallelism,
                    simulationParameters);
            });

        springStart = springEnd;
        shipPointStart = shipPointEnd;
        ephemeralPointStart = ephemeralPointEnd;
    }
}

Ship::SpringRelaxationCoefficients Ship::CalculateSpringRelaxationCoefficients(
    float numMechanicalDynamicsIterations,
    SimulationParameters const & simulationParameters)
{
    SpringRelaxationCoefficients coeffs;

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

    coeffs.Dt = SimulationParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    float const globalDamping = 1.0f -
        pow((1.0f - SimulationParameters::GlobalDamping),
            12.0f / numMechanicalDynamicsIterations);

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
    coeffs.IntegrationVelocityFactor  = globalDampingCoefficient / coeffs.Dt;

    //
    // Silt hardness
    //

    float constexpr MinDepthHardnessReference = 0.05f; // Dictates magnitude of discontinuity when entering silt for the first time; reference at 40 iterations/frame
    coeffs.MinSiltDepthHardness = 1.0f - std::powf(1.0f - MinDepthHardnessReference, 40.0f / numMechanicalDynamicsIterations);
    float constexpr MaxDepthHardnessReference = 0.2f; // At max depth; reference at 40 iterations/frame
    coeffs.MaxSiltDepthHardness = 1.0f - std::powf(1.0f - MaxDepthHardnessReference, 40.0f / numMechanicalDynamicsIterations);

    return coeffs;
}

void Ship::RunSpringRelaxation(
    ThreadManager & threadManager,
    SimulationParameters const & simulationParameters)
{
    //
    // Recalculate all coefficients, and prepare silt impacts
    //

    mSpringRelaxationCoefficients = CalculateSpringRelaxationCoefficients(simulationParameters.NumMechanicalDynamicsIterations<float>(), simulationParameters);
    mSpringRelaxationCoefficients_EphemeralParticles = CalculateSpringRelaxationCoefficients(1.0f, simulationParameters);

    for (auto & threadSiltImpact : mPerThreadSiltImpacts)
    {
        threadSiltImpact.value.KineticEnergy = 0.0f;
    }

    //
    // Run
    //

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

    //
    // Coalesce silt impacts
    //

    mSiltImpacts.clear();

    EnergeticSiltImpact maxImpact;
    for (auto const & threadSiltImpact : mPerThreadSiltImpacts)
    {
        if (threadSiltImpact.value.KineticEnergy > maxImpact.KineticEnergy)
        {
            maxImpact = threadSiltImpact.value;
        }
    }

    if (maxImpact.KineticEnergy > 0.0f)
    {
        // Now transform the placeholder via a pretend mass
        maxImpact.KineticEnergy *= SiltImpactKineticEnergyPretendMass;

        mSiltImpacts.emplace_back(maxImpact);
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
    ElementIndex startShipPointIndex,
    ElementIndex endShipPointIndex,
    ElementIndex startEphemeralPointIndex,
    ElementIndex endEphemeralPointIndex,
    size_t parallelism,
    SimulationParameters const & simulationParameters)
{
    //
    // This routine is run ONCE by each thread - in parallel, each on a different start-end point index slice;
    // threads sync among themselves via spinlocks using atomic counters
    //
    // I really think this is a work of art
    //

    // Get the dynamic forces buffer dedicated to this thread
    vec2f * restrict const dynamicForceBuffer = mPoints.GetParallelDynamicForceBuffer(threadIndex);

    // Get the total count of threads participating
    int const numberOfThreads = static_cast<int>(mSpringRelaxation_FullSpeed_Tasks.size());

    //
    // Loop for all mechanical dynamics iterations
    //

    int const numMechanicalDynamicsIterations = GetSafeNumMechanicalDynamicsIterations(simulationParameters);
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
            startShipPointIndex,
            endShipPointIndex,
            parallelism,
            mSpringRelaxationCoefficients);

        if ((iter % SeaFloorCollisionPeriod) == SeaFloorCollisionPeriod - 1)
        {
            // Handle collisions with sea floor
            //  - Changes position and velocity

            HandleCollisionsWithSeaFloor(
                startShipPointIndex,
                endShipPointIndex,
                threadIndex,
                mSpringRelaxationCoefficients,
                simulationParameters);
        }

        // - DynamicForces = 0

        if (iter == numMechanicalDynamicsIterations - 1)
        {
            //
            // Last: ephemeral particles
            //

            //
            // Integrate dynamic and static forces,
            // and reset dynamic forces
            //

            IntegrateAndResetDynamicForces(
                startEphemeralPointIndex,
                endEphemeralPointIndex,
                parallelism,
                mSpringRelaxationCoefficients_EphemeralParticles);

            // Handle collisions with sea floor
            //  - Changes position and velocity

            HandleCollisionsWithSeaFloor(
                startEphemeralPointIndex,
                endEphemeralPointIndex,
                threadIndex,
                mSpringRelaxationCoefficients_EphemeralParticles,
                simulationParameters);

            // No need to wait
        }
        else
        {
            //
            // Sync for next iteration
            //

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
}

void Ship::RunSpringRelaxation_StepByStep(
    ThreadManager & threadManager,
    SimulationParameters const & simulationParameters)
{
    auto & threadPool = threadManager.GetSimulationThreadPool();

    int const numMechanicalDynamicsIterations = GetSafeNumMechanicalDynamicsIterations(simulationParameters);
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

            if (iter < numMechanicalDynamicsIterations - 1)
            {
                // Integrate dynamic and static forces,
                // and reset dynamic forces

                // Handle collisions with sea floor
                //  - Changes position and velocity

                threadPool.Run(mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionTasks);
            }
            else
            {
                // Integrate dynamic and static forces,
                // and reset dynamic forces

                // Handle collisions with sea floor
                //  - Changes position and velocity

                // Run ephemeral particles

                threadPool.Run(mSpringRelaxation_StepByStep_IntegrationAndSeaFloorCollisionAndEphemeralTasks);
            }
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
    auto & threadPool = threadManager.GetSimulationThreadPool();

    int const numMechanicalDynamicsIterations = GetSafeNumMechanicalDynamicsIterations(simulationParameters);

    // First N-1 steps
    for (int iter = 0; iter < numMechanicalDynamicsIterations - 1; ++iter)
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

    // Last step

    mSpringRelaxation_Hybrid_IterationCompleted = 0;

    // Integrate dynamic and static forces,
    // and reset dynamic forces

    // Handle collisions with sea floor
    //  - Changes position and velocity

    // Run for ephemeral particles

    threadPool.Run(mSpringRelaxation_Hybrid_Last_Tasks);

#ifdef _DEBUG
    //
    // We have dirtied positions
    //

    mPoints.Diagnostic_MarkPositionsAsDirty();
#endif
}

template<bool DoHandleCollisionsWithSeaFloor>
void Ship::RunSpringRelaxation_Hybrid_Thread(
    size_t threadIndex,
    ElementIndex startSpringIndex,
    ElementIndex endSpringIndex,
    ElementIndex startShipPointIndex,
    ElementIndex endShipPointIndex,
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
        startShipPointIndex,
        endShipPointIndex,
        parallelism,
        mSpringRelaxationCoefficients);

    if constexpr (DoHandleCollisionsWithSeaFloor)
    {
        // Handle collisions with sea floor
        //  - Changes position and velocity

        HandleCollisionsWithSeaFloor(
            startShipPointIndex,
            endShipPointIndex,
            threadIndex,
            mSpringRelaxationCoefficients,
            simulationParameters);
    }
}

void Ship::RunSpringRelaxation_Hybrid_EphemeralParticle_Thread(
    size_t threadIndex,
    ElementIndex startEphemeralPointIndex,
    ElementIndex endEphemeralPointIndex,
    size_t parallelism,
    SimulationParameters const & simulationParameters)
{
    //
    // Integrate dynamic and static forces,
    // and reset dynamic forces
    //

    IntegrateAndResetDynamicForces(
        startEphemeralPointIndex,
        endEphemeralPointIndex,
        parallelism,
        mSpringRelaxationCoefficients_EphemeralParticles);

    // Handle collisions with sea floor
    //  - Changes position and velocity

    HandleCollisionsWithSeaFloor(
        startEphemeralPointIndex,
        endEphemeralPointIndex,
        threadIndex,
        mSpringRelaxationCoefficients_EphemeralParticles,
        simulationParameters);
}

void Ship::IntegrateAndResetDynamicForces(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t parallelism,
    SpringRelaxationCoefficients const & coefficients)
{
    switch (parallelism)
    {
        case 1:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 1>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                coefficients.Dt,
                coefficients.IntegrationVelocityFactor);

            break;
        }

        case 2:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 2>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                coefficients.Dt,
                coefficients.IntegrationVelocityFactor);

            break;
        }

        case 3:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 3>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                coefficients.Dt,
                coefficients.IntegrationVelocityFactor);

            break;
        }

        case 4:
        {
            Algorithms::IntegrateAndResetDynamicForces<Points, 4>(
                mPoints,
                startPointIndex,
                endPointIndex,
                mPoints.GetDynamicForceBuffersAsFloat(),
                coefficients.Dt,
                coefficients.IntegrationVelocityFactor);

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
                coefficients.Dt,
                coefficients.IntegrationVelocityFactor);

            break;
        }
    }
}

void Ship::HandleCollisionsWithSeaFloor(
    ElementIndex startPointIndex,
    ElementIndex endPointIndex,
    size_t threadIndex,
    SpringRelaxationCoefficients const & coefficients,
    SimulationParameters const & simulationParameters)
{
    float const siltDepthHardnessCoeff1 = coefficients.MinSiltDepthHardness;
    float const siltDepthHardnessCoeff2 = coefficients.MaxSiltDepthHardness - coefficients.MinSiltDepthHardness;
    float const siltDustCloudEnergyThreshold =
        simulationParameters.SiltDustCloudEnergyThreshold
        * 1.0f / std::max(simulationParameters.SiltDustCloudSensitivity, 0.000001f)
        / SiltImpactKineticEnergyPretendMass; // For perf, to avoid multiplication

    OceanFloor const & oceanFloor = mParentWorld.GetOceanFloor();

    auto & maxSiltImpact = mPerThreadSiltImpacts[threadIndex];

    for (ElementIndex pointIndex = startPointIndex; pointIndex < endPointIndex; ++pointIndex)
    {
        auto const & position = mPoints.GetPosition(pointIndex);

        // Check if point is below the sea floor

        // At this moment the point might be outside of world boundaries,
        // so better clamp its x before sampling ocean floor height
        float const clampedX = Clamp(position.x, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth);
        auto const [siltY, bedrockY, integralIndex] = oceanFloor.GetHeightsIfUnderneathAt(clampedX, position.y);
        if (position.y < siltY)
        {
            // We are underneath silt - check if also underneath bedrock

            vec2f const pointVelocity = mPoints.GetVelocity(pointIndex);

            if (position.y < bedrockY)
            {
                //
                // Collision with bedrock!
                //
                // Calculate post-bounce velocity
                //
                // Note: this implementation of friction imparts directly displacement and velocity,
                // rather than imparting forces, and is an approximation of real friction in that it's
                // independent from the force against the surface
                //

                // Calculate sea floor anti-normal
                // (positive points down)
                vec2f const seaFloorAntiNormal = -oceanFloor.GetBedrockNormalAt(integralIndex);

                // Calculate the component of the point's velocity along the anti-normal,
                // i.e. towards the interior of the floor...
                float const pointVelocityAlongAntiNormal = pointVelocity.dot(seaFloorAntiNormal);

                // ...if negative, it's already pointing outside the floor, hence we leave it as-is
                if (pointVelocityAlongAntiNormal > 0.0f)
                {
                    // Decompose point velocity into normal and tangential
                    vec2f const normalVelocity = seaFloorAntiNormal * pointVelocityAlongAntiNormal;
                    vec2f const tangentialVelocity = pointVelocity - normalVelocity;

                    // Calculate normal reponse: Vn' = -e*Vn (e = elasticity, [0.0 - 1.0])
                    float const elasticityFactor = mPoints.GetOceanFloorBedrockCollisionFactors(pointIndex).ElasticityFactor;
                    vec2f const normalResponse =
                        normalVelocity
                        * elasticityFactor; // Already negative

                    // Calculate tangential response: Vt' = a*Vt (a = (1.0-friction), [0.0 - 1.0])
                    float constexpr KineticThreshold = 2.0f;
                    float const frictionFactor = (std::abs(tangentialVelocity.x) > KineticThreshold || std::abs(tangentialVelocity.y) > KineticThreshold)
                        ? mPoints.GetOceanFloorBedrockCollisionFactors(pointIndex).KineticFrictionFactor
                        : mPoints.GetOceanFloorBedrockCollisionFactors(pointIndex).StaticFrictionFactor;
                    vec2f const tangentialResponse =
                        tangentialVelocity
                        * frictionFactor;

                    //
                    // Impart final position and velocity
                    //

                    // Move point back along its velocity direction (i.e. towards where it was in the previous step,
                    // which is guaranteed to be more towards the outside), but not too much - or else springs
                    // might start oscillating between the point burrowing down and then bouncing up
                    vec2f deltaPosition = pointVelocity * coefficients.Dt;
                    float const deltaPositionLength = deltaPosition.length();
                    deltaPosition = deltaPosition.normalise_approx(deltaPositionLength) * std::min(deltaPositionLength, 0.01f); // Magic number, empirical
                    mPoints.SetPosition(
                        pointIndex,
                        position - deltaPosition);

                    // Set velocity to resultant collision velocity
                    mPoints.SetVelocity(
                        pointIndex,
                        normalResponse + tangentialResponse);
                }
            }
            else
            {
                //
                // Apply silt physics
                //

                // Velocity-based hardness
                float kineticEnergyVelocityComponent = pointVelocity.squareLength() * 0.5f;
                float const kineticEnergy = mPoints.GetMass(pointIndex) * kineticEnergyVelocityComponent;
                float const kineticEnergyHardness = LinearStep(0.0f, 30000.0f, kineticEnergy);

                // Depth-based hardness
                //
                // Notes:
                //  - Depth-based hardness cannot be too much early on - with 0.05 -> 0.2, kinetic energy hardness doesn't break structures,
                //    but with 0.05-0.05, it does; there's two possible readings:
                //      - With no depth-based hardness, kinetic energy hardness lasts longer; while with depth-based hardness, velocities get smothered early on;
                //      - With depth-based hardness, there's less discontinuity in ke hardness (bs?)
                //    This is why we add an initial tiny depth-hardness-free layer

                float constexpr DepthHardnessDepthThreshold = 0.5f; // Start depth hardness at 0.5m; very important: affects effect of kinetic energy hardness
                float constexpr MaxBuryDepth = 20.0f; // Magic, intrinsic to sand and independent from angle
                // Naive version: float const depthHardness = minDepthHardness + (maxDepthHardness - minDepthHardness) * LinearStep(DepthHardnessDepthThreshold, MaxBuryDepth, siltY - position.y);
                float const depthHardness = siltDepthHardnessCoeff1 + siltDepthHardnessCoeff2 * LinearStep(DepthHardnessDepthThreshold, MaxBuryDepth, siltY - position.y);

                // Resultant
                float const dampingFactor = 1.0f - std::max(kineticEnergyHardness, depthHardness) * simulationParameters.OceanFloorSiltHardness;
                assert(dampingFactor >= 0.0f);
                mPoints.SetVelocity(
                    pointIndex,
                    pointVelocity * dampingFactor);

                //
                // Silt clouds
                //

                // Now, a sad hack.
                // Due to the constant *relative* precision of floats, at higher distances from zero (e.g. 10K metres away)
                // spring relaxation incurs more error, resulting in wider vibrations of the meshes, which in turn would
                // trigger silt clouds more easily. We thus scale down the kinetic energy component with the (logarithm)
                // of y. We should technically do it with both x and y, but we expect large depths to be occurring more frequently
                // than large horizontal distances
                if (position.y <= -4096.0f)
                {
                    static_assert(SimulationParameters::HalfMaxWorldHeight < 16384.0f); // Or else need another reduction step

                    // Factors have been determined empirically
                    if (position.y > -8192.0f)
                        kineticEnergyVelocityComponent *= 0.938f;
                    else
                        kineticEnergyVelocityComponent *= 0.375f;
                }

                // We won't allow huge masses to generate a ton of clouds, so we use just velocity
                // and a pretend mass (later) to keep values reasonably real
                if (kineticEnergyVelocityComponent > siltDustCloudEnergyThreshold
                    && mPoints.GetConnectedTriangles(pointIndex).ConnectedTriangles.size() > 0 // Prevent debris from generating clouds
                    && kineticEnergyVelocityComponent > maxSiltImpact.value.KineticEnergy)
                {
                    maxSiltImpact.value = EnergeticSiltImpact(
                        kineticEnergyVelocityComponent,
                        vec2f(clampedX, siltY),
                        pointIndex);
                }
            }
        }
    }
}

std::vector<size_t> Ship::CalculateSpringRelaxationSpringShards(
    size_t totalSprings,
    size_t perfectSquareCount,
    ThreadPool const & simulationThreadPool)
{
    //
    // Calculates number of springs for each shard, taking into
    // account processors' speeds and task costs
    //

    float constexpr PerfectSquareSpringCost = 1.0f;
    float constexpr ImperfectSquareSpringCost = 1.2f; // Magic: extra cost per-spring when springs are not perfect squares

    size_t const perfectSquareSpringCount = perfectSquareCount * 4;
    assert(totalSprings >= perfectSquareSpringCount);

    // Calculate cpu speed normalization factor (denominator)
    float cpuSpeedNormalizationFactor = 0.0f;
    for (size_t s = 0; s < simulationThreadPool.GetParallelism(); ++s)
    {
        auto const cpuInfo = simulationThreadPool.GetThreadCpuInfo(s);
        assert(cpuInfo.has_value()); // It's a simulation thread

        cpuSpeedNormalizationFactor += cpuInfo->Speed;
    }

    LogMessage("Ship::CalculateSpringRelaxationSpringShards(totalSprings=", totalSprings, ", perfectSquareSpringCount=", perfectSquareSpringCount, ")");

    //
    // Calculate shards
    //

    std::vector<size_t> springShards(simulationThreadPool.GetParallelism(), 0u);

    size_t springsAssignedCount = 0;
    for (size_t s = 0; s < simulationThreadPool.GetParallelism(); ++s)
    {
        assert(springsAssignedCount <= totalSprings);

        // Total springs remaining
        size_t const remainingPerfectSquareSpringCount = (perfectSquareSpringCount >= springsAssignedCount)
            ? perfectSquareSpringCount - springsAssignedCount
            : 0;
        size_t const remainingImperfectSquareSpringCount = totalSprings - std::max(perfectSquareSpringCount, springsAssignedCount);
        assert(remainingPerfectSquareSpringCount <= perfectSquareSpringCount);
        assert(remainingImperfectSquareSpringCount <= totalSprings - perfectSquareSpringCount);
        assert(remainingPerfectSquareSpringCount + remainingImperfectSquareSpringCount == totalSprings - springsAssignedCount);

        // Total cost remaining
        float const remainingPerfectSquareSpringCost = static_cast<float>(remainingPerfectSquareSpringCount) * PerfectSquareSpringCost;
        float const remainingImperfectSquareSpringCost = static_cast<float>(remainingImperfectSquareSpringCount) * ImperfectSquareSpringCost;
        float const remainingSpringCost = remainingPerfectSquareSpringCost + remainingImperfectSquareSpringCost;

        // Total budget for this shard
        auto const cpuInfo = simulationThreadPool.GetThreadCpuInfo(s);
        assert(cpuInfo.has_value()); // It's a simulation thread
        assert(cpuSpeedNormalizationFactor > 0.0f);
        float const totalShardBudget = cpuInfo->Speed / cpuSpeedNormalizationFactor * remainingSpringCost;
        float remainingShardBudget = totalShardBudget;

        // Calculate springs for this shard
        size_t shardSpringCount = 0u;
        if (s == simulationThreadPool.GetParallelism() - 1)
        {
            //
            // Last shard, all springs
            //

            shardSpringCount = totalSprings - springsAssignedCount;

            // Checks and balances
            springsAssignedCount += shardSpringCount;

            LogMessage("  Shard ", s, ": ", shardSpringCount, " (cpu_speed=", cpuInfo->Speed, " budget=", remainingShardBudget, " cost=", remainingSpringCost, ")");

            assert(springsAssignedCount == totalSprings);
        }
        else
        {
            //
            // Perfect square springs first
            //

            float const shardPerfectSquareSpringCost = std::min(remainingPerfectSquareSpringCost, remainingShardBudget);

            size_t shardPerfectSquareSpringCount = static_cast<size_t>(std::floor(shardPerfectSquareSpringCost / PerfectSquareSpringCost));
            // Make sure we do full vectorization sizes
            shardPerfectSquareSpringCount = (shardPerfectSquareSpringCount / vectorization_float_count<size_t>) * vectorization_float_count<size_t>;
            // Make sure we don't overrun perfect squares
            shardPerfectSquareSpringCount = std::min(shardPerfectSquareSpringCount, remainingPerfectSquareSpringCount);

            shardSpringCount += shardPerfectSquareSpringCount;
            assert(springsAssignedCount + shardPerfectSquareSpringCount <= totalSprings);
            springsAssignedCount += shardPerfectSquareSpringCount;

            // Update remaining budget
            float shardCost = static_cast<float>(shardPerfectSquareSpringCount) * PerfectSquareSpringCost;
            remainingShardBudget = std::max(remainingShardBudget - shardCost, 0.0f);

            //
            // Imperfect squares now
            //

            if (springsAssignedCount >= perfectSquareSpringCount)
            {
                float const shardImperfectSquareSpringCost = std::min(remainingImperfectSquareSpringCost, remainingShardBudget);

                size_t shardImperfectSquareSpringCount = static_cast<size_t>(std::floor(shardImperfectSquareSpringCost / ImperfectSquareSpringCost));
                // Make sure we do full vectorization sizes
                shardImperfectSquareSpringCount = (shardImperfectSquareSpringCount / vectorization_float_count<size_t>) * vectorization_float_count<size_t>;
                // Make sure we don't overrun all springs
                shardImperfectSquareSpringCount = std::min(shardImperfectSquareSpringCount, remainingImperfectSquareSpringCount);

                shardSpringCount += shardImperfectSquareSpringCount;
                assert(springsAssignedCount + shardImperfectSquareSpringCount <= totalSprings);
                springsAssignedCount += shardImperfectSquareSpringCount;

                shardCost += static_cast<float>(shardImperfectSquareSpringCost) * PerfectSquareSpringCost;
            }

            LogMessage("  Shard ", s, ": ", shardSpringCount, " (cpu_speed=", cpuInfo->Speed, " budget=", totalShardBudget, " cost=", shardCost, ")");
        }

        springShards[s] = shardSpringCount;

        // Reduce normalization factor to account for remaining threads
        cpuSpeedNormalizationFactor -= cpuInfo->Speed;
    }

    return springShards;
}

std::vector<size_t> Ship::CalculatePointShards(
    size_t totalPoints,
    ThreadPool const & simulationThreadPool)
{
    //
    // Calculates number of points for each shard, taking into
    // account processors' speeds
    //

    // Calculate cpu speed normalization factor (denominator)
    float cpuSpeedNormalizationFactor = 0.0f;
    for (size_t s = 0; s < simulationThreadPool.GetParallelism(); ++s)
    {
        auto const cpuInfo = simulationThreadPool.GetThreadCpuInfo(s);
        assert(cpuInfo.has_value()); // It's a simulation thread

        cpuSpeedNormalizationFactor += cpuInfo->Speed;
    }

    LogMessage("Ship::CalculatePointShards(totalPoints=", totalPoints, ")");

    //
    // Calculate shards
    //

    std::vector<size_t> pointShards(simulationThreadPool.GetParallelism(), 0u);

    size_t pointsAssignedCount = 0;
    for (size_t s = 0; s < simulationThreadPool.GetParallelism(); ++s)
    {
        assert(pointsAssignedCount <= totalPoints);

        // Total points remaining
        size_t const remainingPointCount = totalPoints - pointsAssignedCount;

        // Total cost remaining
        float const remainingPointCost = static_cast<float>(remainingPointCount);

        // Total budget for this shard
        auto const cpuInfo = simulationThreadPool.GetThreadCpuInfo(s);
        assert(cpuInfo.has_value()); // It's a simulation thread
        assert(cpuSpeedNormalizationFactor > 0.0f);
        float const totalShardBudget = cpuInfo->Speed / cpuSpeedNormalizationFactor * remainingPointCost;

        // Calculate points for this shard
        size_t shardPointCount;
        if (s == simulationThreadPool.GetParallelism() - 1)
        {
            //
            // Last shard, all points
            //

            shardPointCount = remainingPointCount;

            // Checks and balances
            pointsAssignedCount += shardPointCount;

            LogMessage("  Shard ", s, ": ", shardPointCount, " (cpu_speed=", cpuInfo->Speed, " budget=", totalShardBudget, " cost=", remainingPointCost, ")");

            assert(pointsAssignedCount == totalPoints);
        }
        else
        {
            float const shardPointCost = std::min(remainingPointCost, totalShardBudget);

            shardPointCount = static_cast<size_t>(std::floor(shardPointCost));
            // Make sure we do full vectorization sizes
            shardPointCount = (shardPointCount / vectorization_float_count<size_t>) * vectorization_float_count<size_t>;
            // Make sure we don't overrun all points
            shardPointCount = std::min(shardPointCount, remainingPointCount);

            assert(pointsAssignedCount + shardPointCount <= totalPoints);
            pointsAssignedCount += shardPointCount;

            float const shardCost = static_cast<float>(shardPointCount);

            LogMessage("  Shard ", s, ": ", shardPointCount, " (cpu_speed=", cpuInfo->Speed, " budget=", totalShardBudget, " cost=", shardCost, ")");
        }

        pointShards[s] = shardPointCount;

        // Reduce normalization factor to account for remaining threads
        cpuSpeedNormalizationFactor -= cpuInfo->Speed;
    }

    return pointShards;
}

int Ship::GetSafeNumMechanicalDynamicsIterations(SimulationParameters const & simulationParameters)
{
    int const numMechanicalDynamicsIterations = simulationParameters.NumMechanicalDynamicsIterations<int>();
    return std::max(
        numMechanicalDynamicsIterations + SeaFloorCollisionPeriod - 1 - (numMechanicalDynamicsIterations + SeaFloorCollisionPeriod - 1) % SeaFloorCollisionPeriod,
        SeaFloorCollisionPeriod);
}

}
