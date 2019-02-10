/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

ImpactBomb::ImpactBomb(
    ObjectId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    IPhysicsHandler & physicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::ImpactBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        physicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Idle)
    , mNextStateTransitionTimePoint(GameWallClock::time_point::min())
    , mExplodingStepCounter(0u)
{
}

bool ImpactBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::Idle:
        {
            return true;
        }

        case State::TriggeringExplosion:
        {
            //
            // Transition to Exploding state
            //

            TransitionToExploding(
                currentWallClockTime,
                gameParameters);

            return true;
        }

        case State::Exploding:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                TransitionToExploding(
                    currentWallClockTime,
                    gameParameters);
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
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::ImpactBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Exploding:
        {
            assert(mExplodingStepCounter >= 0);
            assert(mExplodingStepCounter < ExplosionStepsCount);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::RcBombExplosion, mExplodingStepCounter), // Squat on RC bomb explosion
                GetPosition(),
                1.0f + static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount),
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