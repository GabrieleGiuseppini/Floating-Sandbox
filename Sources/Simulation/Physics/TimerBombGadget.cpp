/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Render/GameTextureDatabases.h>

#include <Core/GameRandomEngine.h>

namespace Physics {

TimerBombGadget::TimerBombGadget(
    GlobalGadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<SimulationEventDispatcher> simulationEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::TimerBomb,
        pointIndex,
        parentWorld,
        std::move(simulationEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::SlowFuseBurning)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowFuseToDetonationLeadInInterval / FuseStepCount)
    , mFuseFlameFrameIndex(0)
    , mFuseStepCounter(0)
    , mDefuseStepCounter(0)
    , mDetonationLeadInShapeFrameCounter(0)
    , mExplosionFadeoutCounter(0u)
    , mExplosionPosition(vec2f::zero())
    , mExplosionPlaneId(NonePlaneId)
{
    // Notify start slow fuse
    mSimulationEventHandler->OnTimerBombFuse(
        mId,
        false);
}

bool TimerBombGadget::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters)
{
    switch (mState)
    {
        case State::SlowFuseBurning:
        case State::FastFuseBurning:
        {
            float constexpr FuseVerticalOffset = 5.0f; // Fuse position wrt center of bomb

            // Check if we're underwater
            if (float const bombDepth = mShipPoints.GetCachedDepth(mPointIndex);
                bombDepth >= 0.0)
            {
                //
                // Defuse
                //

                // Emit smoke
                mShipPoints.CreateEphemeralParticleHeavySmoke(
                    GetPosition() + vec2f(0.0f, FuseVerticalOffset),
                    bombDepth - FuseVerticalOffset,
                    simulationParameters.AirTemperature + stormParameters.AirTemperatureDelta + 300.0f,
                    currentSimulationTime,
                    GetPlaneId(),
                    simulationParameters);

                // Transition to defusing
                mState = State::Defusing;

                // Notify
                mSimulationEventHandler->OnTimerBombFuse(mId, std::nullopt);
                mSimulationEventHandler->OnTimerBombDefused(true, 1);

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + DefusingInterval / DefuseStepsCount;
            }
            else if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                // Check if we're done
                if (mFuseStepCounter == FuseStepCount - 1)
                {
                    //
                    // Transition to DetonationLeadIn state
                    //

                    mState = State::DetonationLeadIn;

                    mSimulationEventHandler->OnTimerBombFuse(mId, std::nullopt);

                    // Schedule next transition
                    mNextStateTransitionTimePoint = currentWallClockTime + DetonationLeadInToExplosionInterval;
                }
                else
                {
                    // Go to next step
                    ++mFuseStepCounter;

                    // Schedule next transition
                    if (State::SlowFuseBurning == mState)
                        mNextStateTransitionTimePoint = currentWallClockTime + SlowFuseToDetonationLeadInInterval / FuseStepCount;
                    else
                        mNextStateTransitionTimePoint = currentWallClockTime + FastFuseToDetonationLeadInInterval / FuseStepCount;
                }
            }
            else if (mState == State::SlowFuseBurning)
            {
                // Check if our particle has reached the trigger temperature
                if (mShipPoints.GetTemperature(mPointIndex) > SimulationParameters::BombsTemperatureTrigger)
                {
                    // Triggered!

                    //
                    // Transition to fast fusing
                    //

                    TransitionToFastFusing(currentWallClockTime);
                }
            }

            // Alternate sparkle frame
            if (mFuseFlameFrameIndex == mFuseStepCounter)
                mFuseFlameFrameIndex = mFuseStepCounter + 1;
            else
                mFuseFlameFrameIndex = mFuseStepCounter;

            return true;
        }

        case State::DetonationLeadIn:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
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
                    * 80.0f // Bomb-specific multiplier
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
                    * 1.0f // Bomb-specific multiplier
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
                    10.0f, // Radius offset spectacularization
                    ExplosionType::Deflagration,
                    simulationParameters);

                // Notify explosion
                mSimulationEventHandler->OnBombExplosion(
                    GadgetType::TimerBomb,
                    mShipPoints.IsCachedUnderwater(mPointIndex),
                    1);

                //
                // Transition to Exploding state
                //

                mState = State::Exploding;
            }
            else
            {
                // Increment frame counter
                ++mDetonationLeadInShapeFrameCounter;
            }

            return true;
        }

        case State::Defusing:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                assert(mDefuseStepCounter < DefuseStepsCount);

                // Check whether we're done
                if (mDefuseStepCounter == DefuseStepsCount - 1)
                {
                    // Transition to defused
                    mState = State::Defused;
                }
                else
                {
                    ++mDefuseStepCounter;
                }

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + DefusingInterval / DefuseStepsCount;
            }

            return true;
        }

        case State::Defused:
        {
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

void TimerBombGadget::OnNeighborhoodDisturbed(
    float /*currentSimulationTime*/,
    SimulationParameters const & /*simulationParameters*/)
{
    if (State::SlowFuseBurning == mState
        || State::Defused == mState)
    {
        //
        // Transition (again, if we're defused) to fast fuse burning
        //

        TransitionToFastFusing(GameWallClock::GetInstance().Now());
    }
}

void TimerBombGadget::Upload(
    ShipId shipId,
    RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::SlowFuseBurning:
        case State::FastFuseBurning:
        {
            // Render bomb
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Render fuse
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBombFuse, mFuseFlameFrameIndex),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::DetonationLeadIn:
        {
            static constexpr float ShakeOffset = 0.3f;
            vec2f shakenPosition =
                GetPosition()
                + (0 == (mDetonationLeadInShapeFrameCounter % 2)
                    ? vec2f(-ShakeOffset, 0.0f)
                    : vec2f(ShakeOffset, 0.0f));

            // Render bomb
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBomb, FuseLengthStepCount),
                shakenPosition,
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Defusing:
        {
            // Render bomb
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Defused:
        {
            // Render inert bomb
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0f,
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

            // Render disappearing bomb
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                mExplosionPlaneId,
                TextureFrameId(GameTextureDatabases::GenericMipMappedTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
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

void TimerBombGadget::TransitionToFastFusing(GameWallClock::time_point currentWallClockTime)
{
    mState = State::FastFuseBurning;

    if (State::Defused == mState)
    {
        // Start from scratch
        mFuseStepCounter = 0;
        mDefuseStepCounter = 0;
    }

    // Notify fast fuse
    mSimulationEventHandler->OnTimerBombFuse(
        mId,
        true);

    // Schedule next transition
    mNextStateTransitionTimePoint = currentWallClockTime
        + FastFuseToDetonationLeadInInterval / FuseStepCount;
}

}