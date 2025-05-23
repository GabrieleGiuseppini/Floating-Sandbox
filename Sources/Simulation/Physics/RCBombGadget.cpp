/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Render/GameTextureDatabases.h>

#include <Core/GameRandomEngine.h>

namespace Physics {

RCBombGadget::RCBombGadget(
    GlobalGadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    SimulationEventDispatcher & simulationEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::RCBomb,
        pointIndex,
        parentWorld,
        simulationEventDispatcher,
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::IdlePingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mExplosionIgnitionTimestamp(GameWallClock::time_point::min())
    , mPingOnStepCounter(0u)
    , mExplosionFadeoutCounter(0u)
    , mExplosionPosition(vec2f::zero())
    , mExplosionPlaneId(NonePlaneId)
{
}

bool RCBombGadget::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & /*stormParameters*/,
    SimulationParameters const & simulationParameters)
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

                    mSimulationEventHandler.OnRCBombPing(
                        mShipPoints.IsCachedUnderwater(mPointIndex),
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
                if (mShipPoints.GetTemperature(mPointIndex) > SimulationParameters::BombsTemperatureTrigger)
                {
                    // Triggered!
                    Detonate(currentSimulationTime, simulationParameters);
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

                // Freeze explosion position and plane (or else explosion will move
                // along with ship performing its blast)
                mExplosionPosition = GetPosition();
                mExplosionPlaneId = GetPlaneId();

                // Blast force
                float const blastForce =
                    SimulationParameters::BaseBombBlastForce
                    * 55.0f // Bomb-specific multiplier
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
                    * 0.8f // Bomb-specific multiplier
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
                    8.0f, // Radius offset spectacularization
                    ExplosionType::Deflagration,
                    simulationParameters);

                // Notify explosion
                mSimulationEventHandler.OnBombExplosion(
                    GadgetType::RCBomb,
                    mShipPoints.IsCachedUnderwater(mPointIndex),
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

void RCBombGadget::Upload(
    ShipId shipId,
    RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::IdlePingOff:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::IdlePingOn:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::DetonationLeadIn:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBomb, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBombPing, (mPingOnStepCounter - 1) % PingFramesCount),
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
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::RcBomb, 0),
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

void RCBombGadget::Detonate(
    float /*currentSimulationTime*/,
    SimulationParameters const & /*simulationParameters*/)
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