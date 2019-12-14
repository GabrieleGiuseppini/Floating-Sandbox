/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

ImpactBomb::ImpactBomb(
    BombId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::ImpactBomb,
        springIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Idle)
{
}

bool ImpactBomb::Update(
    GameWallClock::time_point /*currentWallClockTime*/,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::Idle:
        {
            // Check if any of the spring endpoints has reached the trigger temperature
            auto springIndex = GetAttachedSpringIndex();
            if (!!springIndex)
            {
                if (mShipPoints.GetTemperature(mShipSprings.GetEndpointAIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger
                    || mShipPoints.GetTemperature(mShipSprings.GetEndpointBIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger)
                {
                    // Triggered...
                    mState = State::TriggeringExplosion;
                }
            }

            return true;
        }

        case State::TriggeringExplosion:
        {
            //
            // Explode
            //

            // Blast strength
            float const blastStrength =
                600.0f // Magic number
                * gameParameters.BombBlastForceAdjustment
                * (gameParameters.IsUltraViolentMode ? 100.0f : 1.0f);

            // Blast heat
            float const blastHeat =
                gameParameters.BombBlastHeat * 1.2f // Just a bit more caustic
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Explode
            mShipPhysicsHandler.StartExplosion(
                currentSimulationTime,
                GetPlaneId(),
                GetPosition(),
                gameParameters.BombBlastRadius,
                blastStrength,
                blastHeat,
                ExplosionType::Deflagration,
                gameParameters);

            //
            // Transition to Expired state
            //

            mState = State::Expired;

            return true;
        }

        case State::Expired:
        default:
        {
            return false;
        }
    }
}

void ImpactBomb::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::Idle:
        case State::TriggeringExplosion:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::ImpactBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

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