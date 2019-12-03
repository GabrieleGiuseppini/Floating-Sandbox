/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-12-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "Ship_StateMachines.h"

#include <GameCore/GameRandomEngine.h>

#include <cassert>

namespace Physics {

bool Ship::UpdateExplosionStateMachine(
    ExplosionStateMachine & explosionStateMachine,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float constexpr ExplosionDuration = 1.5f;

    //
    // Update progress
    //

    explosionStateMachine.CurrentProgress =
        (currentSimulationTime - explosionStateMachine.StartSimulationTime)
        / ExplosionDuration;

    if (explosionStateMachine.CurrentProgress > 1.0f)
    {
        // We're done
        return true;
    }

    if (explosionStateMachine.IsBlasting)
    {
        //
        // Update explosion
        //

        vec2f const centerPosition = explosionStateMachine.CenterPosition;

        // Blast progress: reaches max at a fraction of the blast duration,
        // as the whole duration includes gfx effects
        float const blastProgress = explosionStateMachine.CurrentProgress * 9.0f;

        // Blast radius: from 0.6 to BlastRadius, linearly
        float const blastRadius =
            0.6f
            + (std::max(explosionStateMachine.BlastRadius - 0.6f, 0.0f)) * std::min(1.0f, blastProgress);

        //
        // Blast force
        //

        float const blastStrength =
            750.0f // Magic number
            * (gameParameters.IsUltraViolentMode ? 100.0f : 1.0f);

        // Store the force field
        AddForceField<BlastForceField>(
            centerPosition,
            blastRadius,
            blastStrength,
            explosionStateMachine.IsFirstFrame);

        //
        // Blast heat
        //

        // Q = q*dt
        float const blastHeat =
            explosionStateMachine.BlastHeat * 1000.0f // KJoule->Joule
            * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f)
            * GameParameters::SimulationStepTimeDuration<float>;

        float const blastHeatSquareRadius =
            blastRadius * blastRadius
            * 1.5f; // Larger radius, so to heat parts that are not swept by the blast and stay behind

        // Search all non-ephemeral points within the radius
        for (auto pointIndex : mPoints.RawShipPoints())
        {
            float const pointSquareDistance = (mPoints.GetPosition(pointIndex) - centerPosition).squareLength();
            if (pointSquareDistance < blastHeatSquareRadius)
            {
                //
                // Inject heat at this point
                //

                // Calc temperature delta
                // T = Q/HeatCapacity
                float const deltaT =
                    blastHeat
                    / mPoints.GetMaterialHeatCapacity(pointIndex);

                // Increase temperature
                mPoints.SetTemperature(
                    pointIndex,
                    mPoints.GetTemperature(pointIndex) + deltaT);
            }
        }

        if (blastProgress > 1.0f)
        {
            // Stop blasting
            explosionStateMachine.IsBlasting = false;
        }
    }

    explosionStateMachine.IsFirstFrame = false;

    return false;
}

void Ship::UploadExplosionStateMachine(
    ExplosionStateMachine const & explosionStateMachine,
    Render::RenderContext & renderContext)
{
    //TODOTEST
    ////renderContext.UploadShipExplosion(
    ////    mId,
    ////    explosionStateMachine.Plane,
    ////    explosionStateMachine.CenterPosition,
    ////    explosionStateMachine.BlastRadius,
    ////    explosionStateMachine.PersonalitySeed,
    ////    explosionStateMachine.CurrentProgress);
}

////////////////////////////////////////////////////////////////////

void Ship::UpdateStateMachines(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    for (auto smIt = mStateMachines.begin(); smIt != mStateMachines.end(); /* incremented in loop */)
    {
        bool isExpired = false;

        switch ((*smIt)->Type)
        {
            case StateMachineType::Explosion:
            {
                isExpired = UpdateExplosionStateMachine(
                    dynamic_cast<ExplosionStateMachine &>(*(*smIt)),
                    currentSimulationTime,
                    gameParameters);

                break;
            }
        }

        if (isExpired)
            smIt = mStateMachines.erase(smIt);
        else
            ++smIt;
    }
}

void Ship::UploadStateMachines(Render::RenderContext & renderContext)
{
    for (auto const & sm : mStateMachines)
    {
        switch (sm->Type)
        {
            case StateMachineType::Explosion:
            {
                UploadExplosionStateMachine(
                    dynamic_cast<ExplosionStateMachine const &>(*sm),
                    renderContext);

                break;
            }
        }
    }
}

}