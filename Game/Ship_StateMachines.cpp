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
    //
    // Update progress
    //

    float constexpr ExplosionDuration = 1.0f; // In "simulation seconds"

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
        // as the whole explosion duration includes also gfx effects while
        // the actual blast should last for less time
        float const blastProgress = explosionStateMachine.CurrentProgress * 4.0f;

        // Blast radius: from 1.0 to BlastRadius, linearly with progress
        float const blastRadius =
            1.0f +
            std::max(explosionStateMachine.BlastRadius - 1.0f, 0.0f) * std::min(1.0f, blastProgress);

        //
        // Blast force and heat
        //
        // Go through all points and, for each point in radius:
        //  - Apply blast force
        //  - Apply blast heat
        // - Keep non-ephemeral point that is closest to blast position; we'll Detach() it later
        //   (if this is the fist frame of the blast sequence)
        //

        float const squareBlastRadius = blastRadius * blastRadius;

        // Q = q*dt
        float const blastHeat =
            explosionStateMachine.BlastHeat * 1000.0f // KJoule->Joule
            * GameParameters::SimulationStepTimeDuration<float>;

        float closestPointSquareDistance = std::numeric_limits<float>::max();
        ElementIndex closestPointIndex = NoneElementIndex;

        // Visit all points
        for (auto const pointIndex : mPoints)
        {
            vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
            float const squarePointDistance = pointRadius.squareLength();
            if (squarePointDistance < squareBlastRadius)
            {
                float const pointRadiusLength = std::sqrt(squarePointDistance);

                //
                // Apply blast force
                //
                // (inversely proportional to square root of distance, not second power as one would expect though)
                //

                vec2f const blastDir = pointRadius.normalise_approx(pointRadiusLength);

                mPoints.AddStaticForce(
                    pointIndex,
                    blastDir * explosionStateMachine.BlastForce / std::sqrt(std::max((pointRadiusLength * 0.3f) + 0.7f, 1.0f)));

                //
                // Inject heat at this point
                //

                // Calc temperature delta
                // T = Q/HeatCapacity
                float const deltaT =
                    blastHeat
                    / std::max(pointRadiusLength, 1.0f)
                    * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

                // Increase temperature
                mPoints.SetTemperature(
                    pointIndex,
                    mPoints.GetTemperature(pointIndex) + deltaT);

                // Update water velocity
                mPoints.SetWaterVelocity(
                    pointIndex,
                    mPoints.GetWaterVelocity(pointIndex) + blastDir * 100.0f * mPoints.GetWater(pointIndex)); // Magic number

                //
                // Check whether this point is the closest point
                //

                if (squarePointDistance < closestPointSquareDistance)
                {
                    closestPointSquareDistance = squarePointDistance;
                    closestPointIndex = pointIndex;
                }
            }
        }

        //
        // Eventually detach the closest point
        //

        if (blastProgress == 0.0f // First frame
            && NoneElementIndex != closestPointIndex)
        {
            // Choose a detach velocity - using the same distribution as Debris
            vec2f const detachVelocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
                GameParameters::MinDebrisParticlesVelocity,
                GameParameters::MaxDebrisParticlesVelocity);

            // Detach point
            mPoints.Detach(
                closestPointIndex,
                detachVelocity,
                Points::DetachOptions::GenerateDebris
                | Points::DetachOptions::FireDestroyEvent,
                currentSimulationTime,
                gameParameters);
        }

        //
        // Apply side-effects
        //

        OnBlast(
            centerPosition,
            blastRadius,
            explosionStateMachine.BlastForce,
            gameParameters);

        //
        // Check if blast is over
        //

        if (blastProgress > 1.0f)
        {
            // Stop blasting
            explosionStateMachine.IsBlasting = false;
        }
    }

    return false;
}

void Ship::UploadExplosionStateMachine(
    ExplosionStateMachine const & explosionStateMachine,
    Render::RenderContext & renderContext)
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(mId);

    shipRenderContext.UploadExplosion(
        explosionStateMachine.Plane,
        explosionStateMachine.CenterPosition,
        explosionStateMachine.BlastRadius,
        explosionStateMachine.Type,
        explosionStateMachine.PersonalitySeed,
        explosionStateMachine.CurrentProgress);
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