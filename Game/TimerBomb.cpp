/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

TimerBomb::TimerBomb(
    ObjectId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    IPhysicsHandler & physicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::TimerBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        physicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::SlowFuseBurning)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowFuseToDetonationLeadInInterval / FuseStepCount)
    , mFuseFlameFrameIndex(0)
    , mFuseStepCounter(0)
    , mExplodingStepCounter(0)
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
                // Transition to Exploding state
                //

                mState = State::Exploding;

                assert(0 == mExplodingStepCounter);

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Invoke explosion handler
                mPhysicsHandler.DoBombExplosion(
                    GetPosition(),
                    static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount - 1),
                    gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    BombType::TimerBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + ExplosionProgressInterval;
            }
            else
            {
                // Increment frame counter
                ++mDetonationLeadInShapeFrameCounter;
            }

            return true;
        }

        case State::Exploding:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                assert(mExplodingStepCounter < ExplosionStepsCount);

                // Check whether we're done
                if (mExplodingStepCounter == ExplosionStepsCount - 1)
                {
                    // Transition to expired
                    mState = State::Expired;
                }
                else
                {
                    ++mExplodingStepCounter;

                    // Invoke explosion handler
                    mPhysicsHandler.DoBombExplosion(
                        GetPosition(),
                        static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount - 1),
                        gameParameters);

                    // Schedule next transition
                    mNextStateTransitionTimePoint = currentWallClockTime + ExplosionProgressInterval;
                }
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
        mNextStateTransitionTimePoint = GameWallClock::GetInstance().Now()
            + FastFuseToDetonationLeadInInterval / FuseStepCount;
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
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBombFuse, mFuseFlameFrameIndex),
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
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBomb, FuseLengthStepCount),
                shakenPosition,
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Exploding:
        {
            assert(mExplodingStepCounter < ExplosionStepsCount);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBombExplosion, mExplodingStepCounter),
                GetPosition(),
                1.0f + static_cast<float>(mExplodingStepCounter + 1) / static_cast<float>(ExplosionStepsCount),
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::Defusing:
        {
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBombDefuse, mDefuseStepCounter),
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
                // TODO: will be replaced with plain PlaneId
                static_cast<ConnectedComponentId>(GetPlaneId()),
                TextureFrameId(TextureGroupType::TimerBomb, mFuseStepCounter / FuseFramesPerFuseLengthCount),
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

}