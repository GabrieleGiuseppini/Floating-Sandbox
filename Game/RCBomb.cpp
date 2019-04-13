/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

RCBomb::RCBomb(
    BombId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    IPhysicsHandler & physicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::RCBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        physicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::IdlePingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mExplosionTimePoint(GameWallClock::time_point::min())
    , mPingOnStepCounter(0u)
    , mExplodingStepCounter(0u)
{
}

bool RCBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOn state
                //

                mState = State::IdlePingOn;

                ++mPingOnStepCounter;

                mGameEventHandler->OnRCBombPing(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + SlowPingOnInterval;
            }

            return true;
        }

        case State::IdlePingOn:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOff state
                //

                mState = State::IdlePingOff;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + SlowPingOffInterval;
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (currentWallClockTime > mExplosionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Transition
                TransitionToExploding(currentWallClockTime, gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    BombType::RCBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);
            }
            else if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to DetonationLeadIn state
                //

                TransitionToDetonationLeadIn(currentWallClockTime);
            }

            return true;
        }

        case State::Exploding:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                TransitionToExploding(currentWallClockTime, gameParameters);
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

void RCBomb::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::IdlePingOn:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::DetonationLeadIn:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
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
                GetPlaneId(),
                TextureFrameId(TextureGroupType::RcBombExplosion, mExplodingStepCounter),
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

void RCBomb::Detonate()
{
    if (State::IdlePingOff == mState
        || State::IdlePingOn == mState)
    {
        //
        // Transition to DetonationLeadIn state
        //

        auto const currentWallClockTime = GameWallClock::GetInstance().Now();

        TransitionToDetonationLeadIn(currentWallClockTime);

        // Schedule explosion
        mExplosionTimePoint = currentWallClockTime + DetonationLeadInToExplosionInterval;
    }
}

}