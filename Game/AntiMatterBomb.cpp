/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

AntiMatterBomb::AntiMatterBomb(
    BombId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    IPhysicsHandler & physicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::AntiMatterBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        physicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Contained_1)
    , mLastUpdateTimePoint(GameWallClock::GetInstance().Now())
    , mNextStateTransitionTimePoint(GameWallClock::time_point::max())
    , mCurrentStateStartTimePoint(mLastUpdateTimePoint)
    , mCurrentStateProgress(0.0f)
    , mCurrentCloudRotationAngle(0.0f)
{
    // Notify start containment
    mGameEventHandler->OnAntiMatterBombContained(mId, true);
}

bool AntiMatterBomb::Update(
    GameWallClock::time_point currentWallClockTime,
    GameParameters const & gameParameters)
{
    auto const elapsed = std::chrono::duration<float>(currentWallClockTime - mLastUpdateTimePoint);
    mLastUpdateTimePoint = currentWallClockTime;

    switch (mState)
    {
        case State::Contained_1:
        {
            // Update cloud rotation angle
            mCurrentCloudRotationAngle += ContainedCloudRevolutionSpeed * elapsed.count();

            return true;
        }

        case State::TriggeringPreImploding_2:
        {
            //
            // Fake state, transition immediately to Pre-Imploding
            //

            mState = State::PreImploding_3;
            mCurrentStateStartTimePoint = currentWallClockTime;
            mCurrentStateProgress = 0.0f;

            // Invoke handler
            mPhysicsHandler.DoAntiMatterBombPreimplosion(
                GetPosition(),
                0.0f,
                gameParameters);

            // Notify
            mGameEventHandler->OnAntiMatterBombPreImploding();
            mGameEventHandler->OnAntiMatterBombContained(mId, false);

            // Schedule next transition
            mNextStateTransitionTimePoint = currentWallClockTime + PreImplosionInterval;
        }

        case State::PreImploding_3:
        {
            if (currentWallClockTime <= mNextStateTransitionTimePoint)
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(currentWallClockTime - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(PreImplosionInterval).count());

                // Update cloud rotation angle: going to zero with progress
                mCurrentCloudRotationAngle += ContainedCloudRevolutionSpeed * (1.0f - mCurrentStateProgress) * elapsed.count();

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombPreimplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }
            else
            {
                //
                // Transition to imploding
                //

                mState = State::Imploding_4;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    0.0f,
                    gameParameters);

                // Notify
                mGameEventHandler->OnAntiMatterBombImploding();

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + ImplosionInterval;
            }

            return true;
        }

        case State::Imploding_4:
        {
            if (currentWallClockTime <= mNextStateTransitionTimePoint)
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(currentWallClockTime - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(ImplosionInterval).count());

                // Update cloud rotation angle: going to max with progress
                mCurrentCloudRotationAngle += ImplosionCloudRevolutionSpeed * mCurrentStateProgress * elapsed.count();

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }
            else
            {
                //
                // Transition to pre-exploding
                //

                mState = State::PreExploding_5;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + PreExplosionInterval;
            }

            return true;
        }

        case State::PreExploding_5:
        {
            if (currentWallClockTime <= mNextStateTransitionTimePoint)
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(currentWallClockTime - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(PreExplosionInterval).count());

                // Invoke handler at max of implosion strength
                mPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    1.0f,
                    gameParameters);
            }
            else
            {
                //
                // Transition to exploding
                //

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    BombType::AntiMatterBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Invoke explosion handler
                mPhysicsHandler.DoAntiMatterBombExplosion(
                    GetPosition(),
                    0.0f,
                    gameParameters);

                // Transition state
                mState = State::Exploding_6;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + ExplosionInterval;
            }

            return true;
        }

        case State::Exploding_6:
        {
            if (currentWallClockTime <= mNextStateTransitionTimePoint)
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(currentWallClockTime - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(ExplosionInterval).count());

                //
                // Invoke explosion handler
                //

                // Invoke explosion handler
                mPhysicsHandler.DoAntiMatterBombExplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }
            else
            {
                //
                // Transition to next state
                //

                mState = State::Expired_7;
            }

            return true;
        }

        case State::Expired_7:
        default:
        {
            // Let us disappear
            return false;
        }
    }
}

void AntiMatterBomb::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::Contained_1:
        case State::TriggeringPreImploding_2:
        {
            // Armor
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::PreImploding_3:
        {
            // Armor
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::Imploding_4:
        {
            // Armor
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetPlaneId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::PreExploding_5:
        {
            // Cross-of-light
            renderContext.UploadCrossOfLight(
                GetPosition(),
                mCurrentStateProgress);

            break;
        }

        case State::Exploding_6:
        case State::Expired_7:
        default:
        {
            // No drawing
            break;
        }
    }
}

void AntiMatterBomb::Detonate()
{
    if (State::Contained_1 == mState)
    {
        // Transition to fake Trigger-PreImploding state
        mState = State::TriggeringPreImploding_2;
    }
}

}