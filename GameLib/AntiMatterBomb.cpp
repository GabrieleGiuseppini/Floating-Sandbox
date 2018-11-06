/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

AntiMatterBomb::AntiMatterBomb(
    ObjectId id,
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
    , mNextStateTransitionTimePoint(GameWallClock::time_point::max())
    , mCurrentStateStartTimePoint(GameWallClock::GetInstance().Now())
    , mCurrentStateProgress(0.0f)
    , mCurrentCloudRotationAngle(0.0f)
{
    // Notify start containment
    mGameEventHandler->OnAntiMatterBombContained(mId, true);
}

bool AntiMatterBomb::Update(
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::Contained_1:
        {
            //
            // Update current revolution progress
            //

            auto const millisInRevolution = 
                std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentStateStartTimePoint).count()
                % std::chrono::duration_cast<std::chrono::milliseconds>(ContainedRevolutionInterval).count();

            mCurrentStateProgress =
                static_cast<float>(millisInRevolution)
                / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(ContainedRevolutionInterval).count());

            // Update rotation angle
            mCurrentCloudRotationAngle = 2 * Pi<float> * mCurrentStateProgress;
                            
            return true;
        }

        case State::TriggeringPreImploding_2:
        {
            //
            // Fake state, transition immediately to Pre-Imploding
            //

            mState = State::PreImploding_3;
            mCurrentStateStartTimePoint = now;

            // Invoke handler
            mPhysicsHandler.DoAntiMatterBombPreimplosion(
                GetPosition(),
                0.0f,
                gameParameters);

            // Notify        
            mGameEventHandler->OnAntiMatterBombPreImploding();
            mGameEventHandler->OnAntiMatterBombContained(mId, false);

            // Schedule next transition
            mNextStateTransitionTimePoint = now + PreImplosionInterval;
        }

        case State::PreImploding_3:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to imploding
                //

                mState = State::Imploding_4;
                mCurrentStateStartTimePoint = now;

                // Detach self (or else bomb will move along with ship performing
                // its implosion)
                DetachIfAttached();

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    0.0f,
                    gameParameters);

                // Notify
                mGameEventHandler->OnAntiMatterBombImploding();

                // Schedule next transition
                mNextStateTransitionTimePoint = now + ImplosionInterval;
            }
            else
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(PreImplosionInterval).count());

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombPreimplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }

            return true;
        }

        case State::Imploding_4:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to exploding
                //

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachIfAttached();

                // Invoke explosion handler
                mPhysicsHandler.DoAntiMatterBombExplosion(
                    GetPosition(),
                    0.0f,
                    gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    BombType::AntiMatterBomb,
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Transition state
                mState = State::Exploding_5;
                mCurrentStateStartTimePoint = now;

                // Schedule next transition
                mNextStateTransitionTimePoint = now + ExplosionInterval;
            }
            else
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentStateStartTimePoint)
                    .count();

                mCurrentStateProgress =
                    static_cast<float>(millisInCurrentState)
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(ImplosionInterval).count());

                // Invoke handler
                mPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }

            return true;
        }

        case State::Exploding_5:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to next state
                //

                mState = State::Expired_6;
            }
            else
            {
                //
                // Update current progress
                //

                auto const millisInCurrentState = std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentStateStartTimePoint)
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

            return true;
        }

        case State::Expired_6:
        default:
        {
            // Let us disappear
            return false;
        }
    }
}

void AntiMatterBomb::Upload(
    int shipId,
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
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis, 
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::PreImploding_3:
        {
            float const alpha = std::max(0.0f, 1.0f - mCurrentStateProgress);

            // Armor - disappearing away
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                alpha);

            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);

            // Cloud, disappearing away
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0,
                mCurrentCloudRotationAngle,
                alpha);

            break;
        }

        case State::Imploding_4:
        {
            // Sphere
            renderContext.UploadShipGenericTextureRenderSpecification(
                shipId,
                GetConnectedComponentId(),
                TextureFrameId(TextureGroupType::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0,
                mRotationBaseAxis,
                GetRotationOffsetAxis(),
                1.0f);
            
            break;
        }

        case State::Exploding_5:
        {
            // TODOHERE
            break;
        }

        case State::Expired_6:
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