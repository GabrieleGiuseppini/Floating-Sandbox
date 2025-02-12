/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Render/GameTextureDatabases.h>

#include <Core/GameRandomEngine.h>

namespace Physics {

ImpactBombGadget::ImpactBombGadget(
    GlobalGadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    SimulationEventDispatcher & simulationEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::ImpactBomb,
        pointIndex,
        parentWorld,
        simulationEventDispatcher,
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Idle)
    , mExplosionFadeoutCounter(0u)
    , mExplosionPosition(vec2f::zero())
    , mExplosionPlaneId(NonePlaneId)
{
}

bool ImpactBombGadget::Update(
    GameWallClock::time_point /*currentWallClockTime*/,
    float currentSimulationTime,
    Storm::Parameters const & /*stormParameters*/,
    SimulationParameters const & simulationParameters)
{
    switch (mState)
    {
        case State::Idle:
        {
            // Check if our particle has reached the trigger temperature
            if (mShipPoints.GetTemperature(mPointIndex) > SimulationParameters::BombsTemperatureTrigger)
            {
                // Triggered...
                mState = State::TriggeringExplosion;
            }

            return true;
        }

        case State::TriggeringExplosion:
        {
            //
            // Explode
            //

            // Freeze explosion position and plane (or else explosion will move
            // along with ship performing its blast)
            mExplosionPosition = GetPosition();
            mExplosionPlaneId = GetPlaneId();

            // Blast force
            float const blastForce =
                SimulationParameters::BaseBombBlastForce
                * 40.0f // Bomb-specific multiplier
                * (simulationParameters.IsUltraViolentMode
                    ? std::min(simulationParameters.BombBlastForceAdjustment * 10.0f, SimulationParameters::MaxBombBlastForceAdjustment * 2.0f)
                    : simulationParameters.BombBlastForceAdjustment);

            // Blast radius
            float const blastRadius = simulationParameters.IsUltraViolentMode
                ? std::min(simulationParameters.BombBlastRadius * 10.0f, SimulationParameters::MaxBombBlastRadius * 2.0f)
                : simulationParameters.BombBlastRadius;

            // Blast heat
            float const blastHeat =
                simulationParameters.BombBlastHeat
                * 1.2f // Bomb-specific multiplier
                * (simulationParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Start explosion
            mShipPhysicsHandler.StartExplosion(
                currentSimulationTime,
                mExplosionPlaneId,
                mExplosionPosition,
                blastForce,
                blastRadius,
                blastHeat,
                blastRadius,
                7.0f, // Radius offset spectacularization
                ExplosionType::Deflagration,
                simulationParameters);

            // Notify explosion
            mSimulationEventHandler.OnBombExplosion(
                GadgetType::ImpactBomb,
                mShipPoints.IsCachedUnderwater(mPointIndex),
                1);

            //
            // Transition to Exploding state
            //

            mState = State::Exploding;

            return true;
        }

        case State::Exploding:
        {
            ++mExplosionFadeoutCounter;
            if (mExplosionFadeoutCounter >= ExplosionFadeoutStepsCount)
            {
                // Transition to expired
                mState = State::Expired;
            }

            return true;
        }

        case State::Expired:
        default:
        {
            // Detach ourselves
            assert(mShipPoints.IsGadgetAttached(mPointIndex));
            mShipPoints.DetachGadget(
                mPointIndex,
                mShipSprings);

            // Disappear
            return false;
        }
    }
}

void ImpactBombGadget::Upload(
    ShipId shipId,
    RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::Idle:
        case State::TriggeringExplosion:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::ImpactBomb, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Exploding:
        {
            // Calculate current progress
            float const progress =
                static_cast<float>(mExplosionFadeoutCounter + 1)
                / static_cast<float>(ExplosionFadeoutStepsCount);

            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                mExplosionPlaneId,
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::ImpactBomb, 0),
                mExplosionPosition,
                1.0f, // Scale
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f - progress);  // Alpha

            break;
        }

        case State::Expired:
        {
            // No drawing
            break;
        }
    }
}

}