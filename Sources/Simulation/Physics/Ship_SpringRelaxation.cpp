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
    RecalculateSpringRelaxationSpringForcesParallelism(simulationParallelism, simulationParameters);
    RecalculateSpringRelaxationIntegrationAndSeaFloorCollisionParallelism(simulationParallelism, simulationParameters);
}

void Ship::RecalculateSpringRelaxationSpringForcesParallelism(
    size_t simulationParallelism,
    SimulationParameters const & simulationParameters)
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
                Algorithms::ApplySpringsForces(
                    mPoints,
                    mSprings,
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

    size_t actualParallelism;
#if !FS_IS_PLATFORM_MOBILE()
    actualParallelism = std::max(
        std::min(
            numberOfPoints <= 12000 ? size_t(1) : (size_t(1) + (numberOfPoints - 12000) / 4000),
            simulationParallelism),
        size_t(1)); // Capping to 1!
#else
    actualParallelism = std::min(
        static_cast<size_t>(numberOfPoints / simulationParameters.SpringRelaxationPointsPerThread),
        simulationParallelism);
#endif

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