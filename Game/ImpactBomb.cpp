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
    IShipStructureHandler & shipStructureHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::ImpactBomb,
        springIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipStructureHandler,
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

            mShipStructureHandler.StartExplosion(
                currentSimulationTime,
                GetPlaneId(),
                GetPosition(),
                gameParameters.BombBlastRadius,
                gameParameters.BombBlastHeat * 1.2f, // Just a bit more caustic
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
                TextureFrameId(TextureGroupType::ImpactBomb, 0),
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