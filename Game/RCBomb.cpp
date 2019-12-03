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
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipStructureHandler & shipStructureHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::RCBomb,
        springIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipStructureHandler,
        shipPoints,
        shipSprings)
    , mState(State::IdlePingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mExplosionIgnitionTimestamp(GameWallClock::time_point::min())
    , mPingOnStepCounter(0u)
{
}

bool RCBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::IdlePingOff:
        case State::IdlePingOn:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                if (mState == State::IdlePingOff)
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
                else
                {
                    assert(mState == State::IdlePingOn);

                    //
                    // Transition to PingOff state
                    //

                    mState = State::IdlePingOff;

                    // Schedule next transition
                    mNextStateTransitionTimePoint = currentWallClockTime + SlowPingOffInterval;
                }
            }
            else
            {
                // Check if any of the spring endpoints has reached the trigger temperature
                auto springIndex = GetAttachedSpringIndex();
                if (!!springIndex)
                {
                    if (mShipPoints.GetTemperature(mShipSprings.GetEndpointAIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger
                        || mShipPoints.GetTemperature(mShipSprings.GetEndpointBIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger)
                    {
                        // Triggered!

                        Detonate();
                    }
                }
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (currentWallClockTime > mExplosionIgnitionTimestamp)
            {
                //
                // Explode
                //

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Start explosion
                mShipStructureHandler.StartExplosion(
                    currentSimulationTime,
                    GetPlaneId(),
                    GetPosition(),
                    0.5f, // Strength
                    gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    BombType::RCBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                //
                // Transition to Expired state
                //

                mState = State::Expired;
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
        mExplosionIgnitionTimestamp = currentWallClockTime + DetonationLeadInToExplosionInterval;
    }
}

}