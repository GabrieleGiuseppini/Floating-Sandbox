/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Ship::RecalculateSpringRelaxationParallelism(size_t simulationParallelism)
{
    //
    // Given the available simulation parallelism as a constraint, calculate 
    // the best parallelism for the spring relaxation algorithm
    // 

    // TODOHERE
    (void)simulationParallelism;
}

void Ship::RunSpringRelaxationAndDynamicForcesIntegration(
    GameParameters const & gameParameters,
    ThreadManager & threadManager)
{
    // TODOHERE
    (void)threadManager;

    int const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<int>();

    // We run ocean floor collision handling every so often
    int constexpr SeaFloorCollisionPeriod = 2;
    float const seaFloorCollisionDt = gameParameters.MechanicalSimulationStepTimeDuration<float>() * static_cast<float>(SeaFloorCollisionPeriod);

    for (int iter = 0; iter < numMechanicalDynamicsIterations; ++iter)
    {
        // - DynamicForces = 0 | others at first iteration only

        // Apply spring forces
        ApplySpringsForces_BySprings(gameParameters);

        // - DynamicForces = fs | fs + others at first iteration only

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
}

}