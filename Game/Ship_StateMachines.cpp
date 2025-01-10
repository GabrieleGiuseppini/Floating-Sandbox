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

    float const elapsed = currentSimulationTime - explosionStateMachine.StartSimulationTime;

    float const explosionBlastForceProgress = elapsed / GameParameters::ExplosionBlastForceDuration;
    explosionStateMachine.CurrentRenderProgress = elapsed / GameParameters::ExplosionRenderDuration;

    if (explosionBlastForceProgress > 1.0f && explosionStateMachine.CurrentRenderProgress > 1.0f)
    {
        // We're done
        return true;
    }

    if (explosionBlastForceProgress <= 1.0f)
    {
        // Continue updating

        if (explosionStateMachine.Type != ExplosionType::FireExtinguishing)
        {
            InternalUpdateExplosionStateMachine<false, true>(
                explosionStateMachine,
                explosionBlastForceProgress,
                currentSimulationTime,
                gameParameters);
        }
        else
        {
            InternalUpdateExplosionStateMachine<true, false>(
                explosionStateMachine,
                explosionBlastForceProgress,
                currentSimulationTime,
                gameParameters);
        }
    }

    explosionStateMachine.IsFirstIteration = false;

    return false;
}

template<bool DoExtinguishFire, bool DoDetachNearestPoint>
void Ship::InternalUpdateExplosionStateMachine(
    ExplosionStateMachine & explosionStateMachine,
    float explosionBlastForceProgress,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // Update explosion
    //

    vec2f const centerPosition = explosionStateMachine.CenterPosition;

    // Blast force radius: from 1.0 to BlastForceRadius, linearly with progress
    float const blastForceRadius =
        1.0f +
        std::max(explosionStateMachine.BlastForceRadius - 1.0f, 0.0f) * std::min(1.0f, explosionBlastForceProgress); // Clamp to 1.0 max

    // Blast heat radius: from 0.0 to BlastHeatRadius, linearly with progress
    float const blastHeatRadius =
        explosionStateMachine.BlastHeatRadius
        * explosionBlastForceProgress;

    //
    // Blast force and heat
    //
    // Go through all points and, for each point in radius:
    //  - Apply blast force
    //  - Apply blast heat (note: it's supposed to be negative for fire=extionguishing explosions)
    //  - Keep non-ephemeral point that is closest to blast position; we'll Detach() it later
    //    (if this is the fist frame of the blast sequence)
    //

    float const squareHeatRadius = blastHeatRadius * blastHeatRadius;
    float const squareForceRadius = blastForceRadius * blastForceRadius;

    // Q = q*dt
    float const blastHeat =
        explosionStateMachine.BlastHeat * 1000.0f // KJoule->Joule
        * GameParameters::SimulationStepTimeDuration<float>;

    float nearestStructuralPointSquareDistance = std::numeric_limits<float>::max();
    ElementIndex nearestStructuralPointIndex = NoneElementIndex;

    // Visit all points
    for (auto const pointIndex : mPoints)
    {
        vec2f const pointRadius = mPoints.GetPosition(pointIndex) - centerPosition;
        float const squarePointDistance = pointRadius.squareLength();

        if (squarePointDistance < squareHeatRadius)
        {
            float const scalingFactor = (1.0f - squarePointDistance / squareHeatRadius);

            //
            // Inject heat at this point
            //

            mPoints.AddHeat(
                pointIndex,
                blastHeat * scalingFactor);

            if constexpr (DoExtinguishFire)
            {
                //
                // Extinguish it if burning
                //

                if (mPoints.IsBurningForExtinguisherHeatSubtraction(pointIndex))
                {
                    mPoints.SmotherCombustion(pointIndex, true); // Fake it's water
                }

                //
                // Also send temperature below combustion point
                //

                float const oldTemperature = mPoints.GetTemperature(pointIndex);
                float const deltaTemperature = mPoints.GetMaterialIgnitionTemperature(pointIndex) / 2.0f - oldTemperature;

                mPoints.SetTemperature(
                    pointIndex,
                    oldTemperature + std::min(deltaTemperature * scalingFactor, 0.0f));
            }
        }

        if (squarePointDistance < squareForceRadius)
        {
            //
            // Apply blast force
            //
            // (inversely proportional to square root of distance, not second power as one would expect though)
            //

            float const pointRadiusLength = std::sqrt(squarePointDistance);

            vec2f const blastDir = pointRadius.normalise_approx(pointRadiusLength);

            mPoints.AddStaticForce(
                pointIndex,
                blastDir * explosionStateMachine.BlastForceMagnitude / std::sqrt(std::max((pointRadiusLength * 0.3f) + 0.7f, 1.0f)));

            // Update water velocity
            mPoints.SetWaterVelocity(
                pointIndex,
                mPoints.GetWaterVelocity(pointIndex) + blastDir * 100.0f * mPoints.GetWater(pointIndex)); // Magic number

            if constexpr (DoDetachNearestPoint)
            {
                //
                // Check whether this point is the closest point, if it's structural
                //

                if (squarePointDistance < nearestStructuralPointSquareDistance
                    && pointIndex < mPoints.GetRawShipPointCount()
                    && !mPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty())
                {
                    nearestStructuralPointSquareDistance = squarePointDistance;
                    nearestStructuralPointIndex = pointIndex;
                }
            }
        }
    }

    //
    // Eventually detach the nearest point
    //

    if (explosionStateMachine.IsFirstIteration
        && NoneElementIndex != nearestStructuralPointIndex)
    {
        assert(DoDetachNearestPoint);

        // Choose a detach velocity - using the same distribution as Debris
        vec2f const detachVelocity = GameRandomEngine::GetInstance().GenerateUniformRadialVector(
            GameParameters::MinDebrisParticlesVelocity,
            GameParameters::MaxDebrisParticlesVelocity);

        // Detach point
        mPoints.Detach(
            nearestStructuralPointIndex,
            detachVelocity,
            Points::DetachOptions::GenerateDebris
            | Points::DetachOptions::FireDestroyEvent,
            currentSimulationTime,
            gameParameters);
    }

    //
    // Apply side-effects
    //

    mParentWorld.OnBlast(
        mId,
        centerPosition,
        explosionStateMachine.BlastForceMagnitude,
        blastForceRadius,
        explosionStateMachine.BlastHeat,
        blastHeatRadius,
        explosionStateMachine.Type,
        gameParameters);
}

void Ship::UploadExplosionStateMachine(
    ExplosionStateMachine const & explosionStateMachine,
    Render::RenderContext & renderContext)
{
    if (explosionStateMachine.CurrentRenderProgress <= 1.0f)
    {
        auto & shipRenderContext = renderContext.GetShipRenderContext(mId);

        shipRenderContext.UploadExplosion(
            explosionStateMachine.Plane,
            explosionStateMachine.CenterPosition,
            explosionStateMachine.BlastForceRadius + explosionStateMachine.RenderRadiusOffset,
            explosionStateMachine.Type,
            explosionStateMachine.PersonalitySeed,
            explosionStateMachine.CurrentRenderProgress);
    }
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