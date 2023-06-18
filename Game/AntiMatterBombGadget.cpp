/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

AntiMatterBombGadget::AntiMatterBombGadget(
    GadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::AntiMatterBomb,
        pointIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Contained_1)
    , mLastUpdateTimePoint(GameWallClock::GetInstance().Now())
    , mNextStateTransitionTimePoint(GameWallClock::time_point::max())
    , mCurrentStateStartTimePoint(mLastUpdateTimePoint)
    , mCurrentStateProgress(0.0f)
    , mCurrentCloudRotationAngle(0.0f)
    , mExplosionPosition(vec2f::zero())
{
    // Notify start containment
    mGameEventHandler->OnAntiMatterBombContained(mId, true);
}

bool AntiMatterBombGadget::Update(
    GameWallClock::time_point currentWallClockTime,
    float /*currentSimulationTime*/,
    Storm::Parameters const & /*stormParameters*/,
    GameParameters const & gameParameters)
{
    auto const wallClockElapsedInFrame = std::chrono::duration<float>(currentWallClockTime - mLastUpdateTimePoint);
    mLastUpdateTimePoint = currentWallClockTime;

    switch (mState)
    {
        case State::Contained_1:
        {
            // Check if our particle has reached the trigger temperature
            if (mShipPoints.GetTemperature(mPointIndex) > GameParameters::BombsTemperatureTrigger + 1000.0f)
            {
                // Triggered!
                Detonate();
            }

            // Update cloud rotation angle
            mCurrentCloudRotationAngle += ContainedCloudRevolutionSpeed * wallClockElapsedInFrame.count();

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
            mShipPhysicsHandler.DoAntiMatterBombPreimplosion(
                GetPosition(),
                0.0f,
                CalculatePreImplosionRadius(0.0f),
                gameParameters);

            // Notify
            mGameEventHandler->OnAntiMatterBombPreImploding();
            mGameEventHandler->OnAntiMatterBombContained(mId, false);

            // Schedule next transition
            mNextStateTransitionTimePoint = currentWallClockTime + PreImplosionInterval;

            return true;
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
                mCurrentCloudRotationAngle += ContainedCloudRevolutionSpeed * (1.0f - mCurrentStateProgress) * wallClockElapsedInFrame.count();

                // Invoke handler
                mShipPhysicsHandler.DoAntiMatterBombPreimplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    CalculatePreImplosionRadius(mCurrentStateProgress),
                    gameParameters);
            }
            else
            {
                //
                // Transition to pre_imploding <-> imploding pause
                //

                mState = State::PreImplodingToImplodingPause_4;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + PreImplosionToImplosionPauseInterval;
            }

            return true;
        }

        case State::PreImplodingToImplodingPause_4:
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
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(PreImplosionToImplosionPauseInterval).count());
            }
            else
            {
                //
                // Transition to imploding
                //

                mState = State::Imploding_5;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Invoke handler
                mShipPhysicsHandler.DoAntiMatterBombImplosion(
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

        case State::Imploding_5:
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
                mCurrentCloudRotationAngle += ImplosionCloudRevolutionSpeed * mCurrentStateProgress * wallClockElapsedInFrame.count();

                // Invoke handler
                mShipPhysicsHandler.DoAntiMatterBombImplosion(
                    GetPosition(),
                    mCurrentStateProgress,
                    gameParameters);
            }
            else
            {
                //
                // Transition to pre-exploding
                //

                mState = State::PreExploding_6;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Freeze current position (or else explosion will move
                // along with ship performing its blast)
                mExplosionPosition = GetPosition();

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + PreExplosionInterval;
            }

            return true;
        }

        case State::PreExploding_6:
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
                mShipPhysicsHandler.DoAntiMatterBombImplosion(
                    mExplosionPosition,
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
                    GadgetType::AntiMatterBomb,
                    mShipPoints.IsCachedUnderwater(mPointIndex),
                    1);

                // Invoke explosion handler
                mShipPhysicsHandler.DoAntiMatterBombExplosion(
                    mExplosionPosition,
                    0.0f,
                    gameParameters);

                // Transition state
                mState = State::Exploding_7;
                mCurrentStateStartTimePoint = currentWallClockTime;
                mCurrentStateProgress = 0.0f;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + ExplosionInterval;
            }

            return true;
        }

        case State::Exploding_7:
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
                mShipPhysicsHandler.DoAntiMatterBombExplosion(
                    mExplosionPosition,
                    mCurrentStateProgress,
                    gameParameters);
            }
            else
            {
                //
                // Transition to next state
                //

                mState = State::Expired_8;
            }

            return true;
        }

        case State::Expired_8:
        default:
        {
            // Detach ourselves
            assert(mShipPoints.IsGadgetAttached(mPointIndex));
            mShipPoints.DetachGadget(
                mPointIndex,
                mShipSprings);

            // Let us disappear
            return false;
        }
    }
}

void AntiMatterBombGadget::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::Contained_1:
        case State::TriggeringPreImploding_2:
        {
            // Armor
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::PreImploding_3:
        {
            // Armor
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            // Pre-implosion
            renderContext.UploadAMBombPreImplosion(
                GetPosition(),
                mCurrentStateProgress,
                CalculatePreImplosionRadius(mCurrentStateProgress));

            break;
        }

        case State::PreImplodingToImplodingPause_4:
        case State::Imploding_5:
        {
            // Armor
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombArmor, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Sphere
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphere, 0),
                GetPosition(),
                1.0f,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            // Rotating cloud
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::AntiMatterBombSphereCloud, 0),
                GetPosition(),
                1.0f,
                mCurrentCloudRotationAngle,
                1.0f);

            break;
        }

        case State::PreExploding_6:
        {
            // Cross-of-light
            renderContext.UploadCrossOfLight(
                mExplosionPosition,
                mCurrentStateProgress);

            break;
        }

        case State::Exploding_7:
        case State::Expired_8:
        default:
        {
            // No drawing
            break;
        }
    }
}

void AntiMatterBombGadget::Detonate()
{
    if (State::Contained_1 == mState)
    {
        // Transition to fake Trigger-PreImploding state
        mState = State::TriggeringPreImploding_2;
    }
}

}