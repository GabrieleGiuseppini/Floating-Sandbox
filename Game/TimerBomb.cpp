/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

TimerBomb::TimerBomb(
    BombId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::TimerBomb,
        springIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::SlowFuseBurning)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowFuseToDetonationLeadInInterval / FuseStepCount)
    , mFuseFlameFrameIndex(0)
    , mFuseStepCounter(0)
    , mDefuseStepCounter(0)
    , mDetonationLeadInShapeFrameCounter(0)
{
    // Notify start slow fuse
    mGameEventHandler->OnTimerBombFuse(
        mId,
        false);
}

bool TimerBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::SlowFuseBurning:
        case State::FastFuseBurning:
        {
            // Check if we're underwater
            if (mParentWorld.IsUnderwater(GetPosition()))
            {
                //
                // Transition to defusing
                //

                mState = State::Defusing;

                mGameEventHandler->OnTimerBombFuse(mId, std::nullopt);

                mGameEventHandler->OnTimerBombDefused(true, 1);

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

                    mGameEventHandler->OnTimerBombFuse(mId, std::nullopt);

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
                // Check if any of the spring endpoints has reached the trigger temperature
                auto springIndex = GetAttachedSpringIndex();
                if (!!springIndex)
                {
                    if (mShipPoints.GetTemperature(mShipSprings.GetEndpointAIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger
                        || mShipPoints.GetTemperature(mShipSprings.GetEndpointBIndex(*springIndex)) > GameParameters::BombsTemperatureTrigger)
                    {
                        // Triggered!

                        //
                        // Transition to fast fusing
                        //

                        TransitionToFastFusing(currentWallClockTime);
                    }
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

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Blast radius
                float const blastRadius =
                    gameParameters.BombBlastRadius
                    * (gameParameters.IsUltraViolentMode ? 10.0f : 1.0f);

                // Blast strength
                float const blastStrength =
                    500.0f // Magic number
                    * gameParameters.BombBlastForceAdjustment;

                // Blast heat
                float const blastHeat =
                    gameParameters.BombBlastHeat
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
                    BombType::TimerBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                //
                // Transition to Expired state
                //

                mState = State::Expired;
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

        case State::Expired:
        default:
        {
            return false;
        }
    }
}

void TimerBomb::OnNeighborhoodDisturbed()
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

void TimerBomb::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::SlowFuseBurning:
        case State::FastFuseBurning:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBombFuse, mFuseFlameFrameIndex),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
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

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBomb, FuseLengthStepCount),
                shakenPosition,
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Defusing:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBombDefuse, mDefuseStepCounter),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Defused:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(Render::GenericTextureGroups::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0f,
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

void TimerBomb::TransitionToFastFusing(GameWallClock::time_point currentWallClockTime)
{
    mState = State::FastFuseBurning;

    if (State::Defused == mState)
    {
        // Start from scratch
        mFuseStepCounter = 0;
        mDefuseStepCounter = 0;
    }

    // Notify fast fuse
    mGameEventHandler->OnTimerBombFuse(
        mId,
        true);

    // Schedule next transition
    mNextStateTransitionTimePoint = currentWallClockTime
        + FastFuseToDetonationLeadInInterval / FuseStepCount;
}

}