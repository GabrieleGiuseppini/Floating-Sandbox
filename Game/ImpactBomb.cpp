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
    , mExplosionFadeoutCounter(0u)
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

            // Detach self (or else explosion will move along with ship performing
            // its blast)
            DetachIfAttached();

            // Blast radius
            float const blastRadius =
                gameParameters.BombBlastRadius
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Blast strength
            float const blastStrength =
                600.0f // Magic number
                * gameParameters.BombBlastForceAdjustment;

            // Blast heat
            float const blastHeat =
                gameParameters.BombBlastHeat * 1.2f // Just a bit more caustic
                * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

            // Start explosion
            mShipPhysicsHandler.StartExplosion(
                currentSimulationTime,
                GetPlaneId(),
                GetPosition(),
                blastRadius,
                blastStrength,
                blastHeat,
                ExplosionType::Deflagration,
                gameParameters);

            // Notify explosion
            mGameEventHandler->OnBombExplosion(
                BombType::ImpactBomb,
                mParentWorld.IsUnderwater(GetPosition()),
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

        case State::Exploding:
        {
            // Calculate current progress
            float const progress =
                static_cast<float>(mExplosionFadeoutCounter + 1)
                / static_cast<float>(ExplosionFadeoutStepsCount);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::ImpactBomb, 0),
                GetPosition(),
                1.0f, // Scale
                mRotationBaseAxis,
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