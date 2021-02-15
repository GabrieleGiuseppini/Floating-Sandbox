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
        float const blastProgress = explosionStateMachine.CurrentProgress * 3.0f;

        // Blast radius: from 0.0 to BlastRadius, linearly with progress
        float const blastRadius =
            explosionStateMachine.BlastRadius * std::min(1.0f, blastProgress);

        //
        // Blast force
        //

        // Apply the force field
        ApplyBlastForceField(
            centerPosition,
            blastRadius,
            explosionStateMachine.BlastStrength,
            explosionStateMachine.IsFirstFrame, // DoDetachPoint
            currentSimulationTime,
            gameParameters);

        //
        // Blast heat
        //

        // Q = q*dt
        float const blastHeat =
            explosionStateMachine.BlastHeat * 1000.0f // KJoule->Joule
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
                    * mPoints.GetMaterialHeatCapacityReciprocal(pointIndex);

                // Increase temperature
                mPoints.SetTemperature(
                    pointIndex,
                    mPoints.GetTemperature(pointIndex) + deltaT);
            }
        }

        //
        // Blast ocean surface displacement
        //

        {

            float constexpr MaxDepth = 15.0f; // No effect when abs depth greater than this
            float constexpr MaxDisplacement = 6.0f; // Max displacement

            // Explostion depth (positive when underwater)
            float const explosionDepth = mParentWorld.GetOceanSurfaceHeightAt(centerPosition.x) - centerPosition.y;
            float const absExplosionDepth = std::abs(explosionDepth);

            // Calculate width: depends on depth (abs)
            float constexpr MinWidth = 1.0f;
            float const maxWidth = 2.0f * blastRadius;
            float const width = maxWidth + (absExplosionDepth / MaxDepth * (MinWidth - maxWidth));

            // Calculate displacement: depends on depth
            //  displacement =  - (1 / (x + a)) + b
            //  f(MaxDepth) = 0
            //  f(0) = MaxDisplacement
            float constexpr a = (-MaxDepth + CompileTimeSqrt(MaxDepth * MaxDepth + 4 * MaxDepth / MaxDisplacement)) / 2.0f;
            float constexpr b = 1.0f / (MaxDisplacement + a);
            float const displacement =
                -1.0f / (absExplosionDepth + a) + b
                * (absExplosionDepth > MaxDepth ? 0.0f : 1.0f) // Turn off at far-away depths
                * (explosionDepth <= 0.0f ? 1.0f : -1.0f); // Follow depth sign

            // TODOTEST
            LogMessage("|D|=", absExplosionDepth, " W=", width, "D=", displacement);

            // Displace
            // TODO: loop for half, taper down at extremes
            for (float x = centerPosition.x - width / 2.0f; x < centerPosition.x + width / 2.0f; x += 0.5f)
            {
                mParentWorld.DisplaceOceanSurfaceAt(x, displacement);
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