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
        // as the whole duration includes also gfx effects while the blast should
        // last for less time
        float const blastProgress = explosionStateMachine.CurrentProgress * 4.0f;

        // Blast radius: from 0.0 to BlastRadius, linearly with progress
        float const blastRadius =
            explosionStateMachine.BlastRadius * std::min(1.0f, blastProgress);

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
        for (auto pointIndex : mPoints)
        {
            vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
            float const squarePointDistance = pointRadius.squareLength();
            if (squarePointDistance < squareBlastRadius)
            {
                float const pointRadiusLength = std::sqrt(squarePointDistance);

                //
                // Apply blast force
                //
                // (inversely proportional to distance, not second power as one would expect though)
                //

                mPoints.AddStaticForce(
                    pointIndex,
                    pointRadius.normalise(pointRadiusLength) / std::max(pointRadiusLength, 1.0f) * explosionStateMachine.BlastForce);

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

        if (explosionStateMachine.IsFirstFrame
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
        // Blast ocean surface displacement
        //

        if (gameParameters.DoDisplaceWater)
        {
            // Explosion depth (positive when underwater)
            float const explosionDepth = mParentWorld.GetDepth(centerPosition);
            float const absExplosionDepth = std::abs(explosionDepth);

            // No effect when abs depth greater than this
            float constexpr MaxDepth = 20.0f;

            // Calculate (lateral) radius: depends on depth (abs)
            //  radius(depth) = ax + b
            //  radius(0) = maxRadius
            //  radius(maxDepth) = MinRadius;
            float constexpr MinRadius = 1.0f;
            float const maxRadius = 20.0f * blastRadius; // Spectacular, spectacular
            float const radius = maxRadius + (absExplosionDepth / MaxDepth * (MinRadius - maxRadius));

            // Calculate displacement: depends on depth
            //  displacement(depth) =  ax^2 + bx + c
            //  f(MaxDepth) = 0
            //  f(0) = MaxDisplacement
            //  f'(MaxDepth) = 0
            float constexpr MaxDisplacement = 6.0f; // Max displacement
            float constexpr a = -MaxDisplacement / (MaxDepth * MaxDepth);
            float constexpr b = 2.0f * MaxDisplacement / MaxDepth;
            float constexpr c = -MaxDisplacement;
            float const displacement =
                (a * absExplosionDepth * absExplosionDepth + b * absExplosionDepth + c)
                * (absExplosionDepth > MaxDepth ? 0.0f : 1.0f) // Turn off at far-away depths
                * (explosionDepth <= 0.0f ? 1.0f : -1.0f); // Follow depth sign

            // Displace
            for (float r = 0.0f; r <= radius; r += 0.5f)
            {
                float const d = displacement * (1.0f - r / radius);
                mParentWorld.DisplaceOceanSurfaceAt(centerPosition.x - r, d);
                mParentWorld.DisplaceOceanSurfaceAt(centerPosition.x + r, d);
            }
        }

        //
        // Scare fishes
        //

        mParentWorld.DisturbOceanAt(
            centerPosition,
            blastRadius * 125.0f,
            std::chrono::milliseconds(0));

        //
        // Check if blast is over
        //

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