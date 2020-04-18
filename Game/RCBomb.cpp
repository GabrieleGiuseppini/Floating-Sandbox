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
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::RCBomb,
        springIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::IdlePingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mExplosionIgnitionTimestamp(GameWallClock::time_point::min())
    , mPingOnStepCounter(0u)
    , mExplosionFadeoutCounter(0u)
{
}

bool RCBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & /*stormParameters*/,
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

                // Blast radius
                float const blastRadius =
                    gameParameters.BombBlastRadius
                    * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

                // Blast strength
                float const blastStrength =
                    75.0f // Magic number
                    * gameParameters.BombBlastForceAdjustment;

                // Blast heat
                float const blastHeat =
                    gameParameters.BombBlastHeat * 0.8f // Just a bit less caustic
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
                    BombType::RCBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                //
                // Transition to Exploding state
                //

                mState = State::Exploding;
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

void RCBomb::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::IdlePingOn:
        {
            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::DetonationLeadIn:
        {
            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
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

            renderContext.UploadShipGenericMipMappedTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::RcBomb, 0),
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