/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

RCBomb::RCBomb(
    ObjectId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    BlastHandler blastHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::RCBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        blastHandler,
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
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            if (now > mNextStateTransitionTimePoint)
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
                mNextStateTransitionTimePoint = now + SlowPingOnInterval;
            }

            return true;
        }

        case State::IdlePingOn:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOff state
                //

                mState = State::IdlePingOff;

                // Schedule next transition
                mNextStateTransitionTimePoint = now + SlowPingOffInterval;
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (now > mExplosionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Transition
                TransitionToExploding(now, gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);
            }
            else if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to DetonationLeadIn state
                //

                TransitionToDetonationLeadIn(now);
            }

            return true;
        }

        case State::Exploding:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                TransitionToExploding(now, gameParameters);
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
    int shipId,
    RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

            break;
        }

        case State::IdlePingOn:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
                GetPosition(),
                1.0,
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

            break;
        }

        case State::DetonationLeadIn:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBomb, 0),
                GetPosition(),
                1.0,
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
                GetPosition(),
                1.0,
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

            break;
        }

        case State::Exploding:
        {
            assert(mExplodingStepCounter >= 1);
            assert(mExplodingStepCounter <= ExplosionStepsCount);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::RcBombExplosion, mExplodingStepCounter - 1),
                GetPosition(),
                1.0f + static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount),
                std::make_pair(mRotationBaseAxis, GetRotationOffsetAxis()));

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

        auto now = GameWallClock::GetInstance().Now();

        TransitionToDetonationLeadIn(now);

        // Schedule explosion
        mExplosionTimePoint = now + DetonationLeadInToExplosionInterval;
    }
}


}