/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              20250-1-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

FireExtinguishingBombGadget::FireExtinguishingBombGadget(
    GlobalGadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::FireExtinguishingBomb,
        pointIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::Idle)
    , mExplosionFadeoutCounter(0u)
    , mExplosionPosition(vec2f::zero())
    , mExplosionPlaneId(NonePlaneId)
{
}

bool FireExtinguishingBombGadget::Update(
    GameWallClock::time_point /*currentWallClockTime*/,
    float currentSimulationTime,
    Storm::Parameters const & /*stormParameters*/,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::Idle:
        {
            // Check if any of the spring endpoints is burning
            if (mShipPoints.IsBurning(mPointIndex))
            {
                // Triggered!
                Detonate(currentSimulationTime, gameParameters);
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

void FireExtinguishingBombGadget::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::Idle:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::FireExtinguishingBomb, 0),
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
                TextureFrameId(Render::GenericMipMappedTextureGroups::FireExtinguishingBomb, 0),
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

void FireExtinguishingBombGadget::Detonate(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    if (State::Idle == mState)
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
            GameParameters::BaseBombBlastForce
            * 7.0f // Bomb-specific multiplier (very low, just for NPC scenics)
            * (gameParameters.IsUltraViolentMode
                ? std::min(gameParameters.BombBlastForceAdjustment * 10.0f, GameParameters::MaxBombBlastForceAdjustment * 2.0f)
                : gameParameters.BombBlastForceAdjustment);

        // Blast radius
        float const blastForceRadius =
            (
                gameParameters.IsUltraViolentMode
                ? std::min(gameParameters.BombBlastRadius * 10.0f, GameParameters::MaxBombBlastRadius * 2.0f)
                : gameParameters.BombBlastRadius
            )
            * 0.3f; // Bomb-specific multiplier

        // Blast heat
        // Note: ship's explosion state machine will change temperatures forcibly
        float const blastHeat = 0.0f;

        // Blast heat radius
        // Note: also used for extinguish radius
        float const blastHeatRadius =
            (
                gameParameters.IsUltraViolentMode
                ? std::min(gameParameters.BombBlastRadius * 10.0f, GameParameters::MaxBombBlastRadius * 2.0f)
                : gameParameters.BombBlastRadius
            )
            * 3.2f; // Bomb-specific multiplier

        // Start explosion
        mShipPhysicsHandler.StartExplosion(
            currentSimulationTime,
            mExplosionPlaneId,
            mExplosionPosition,
            blastForce,
            blastForceRadius,
            blastHeat,
            blastHeatRadius,
            blastHeatRadius - blastForceRadius + 3.0f, // Render radius to equal heat (extinguishing) radius, plus magic offset
            ExplosionType::FireExtinguishing,
            gameParameters);

        // Notify explosion
        mGameEventHandler->OnBombExplosion(
            GadgetType::FireExtinguishingBomb,
            mShipPoints.IsCachedUnderwater(mPointIndex),
            1);

        //
        // Transition to Exploding state
        //

        mState = State::Exploding;
    }
}

}