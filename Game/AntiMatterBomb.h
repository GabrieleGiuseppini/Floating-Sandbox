/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace Physics
{

/*
 * Bomb specialization for spectacular anti-matter bombs.
 */
class AntiMatterBomb final : public Bomb
{
public:

    AntiMatterBomb(
        BombId id,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        IPhysicsHandler & physicsHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        GameParameters const & gameParameters) override;

    virtual bool MayBeRemoved() const override
    {
        // We may only be removed if we're in the Contained state
        return (State::Contained_1 == mState);
    }

    virtual void OnBombRemoved() override
    {
        // Stop containment if it's in containment
        if (State::Contained_1 == mState)
        {
            mGameEventHandler->OnAntiMatterBombContained(mId, false);
        }

        // Notify removal
        mGameEventHandler->OnBombRemoved(
            mId,
            BombType::AntiMatterBomb,
            mParentWorld.IsUnderwater(
                GetPosition()));

        // Detach ourselves, if we're attached
        DetachIfAttached();
    }

    virtual void OnNeighborhoodDisturbed() override
    {
        Detonate();
    }

    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const override;

    void Detonate();

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state the bomb is contained and awaiting for detonation
        Contained_1,

        // Once detonated, the bomb goes through these states in sequence
        // before exploding
        TriggeringPreImploding_2,
        PreImploding_3,
        Imploding_4,

        // Short pause before exploding, to show cross of light
        PreExploding_5,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding_6,

        // This is the final state; once this state is reached, we're expired
        Expired_7
    };

    static constexpr auto ContainedCloudRevolutionSpeed = -2.0f * Pi<float> / std::chrono::duration<float>(2.0f).count();
    static constexpr auto PreImplosionInterval = 1000ms;
    static constexpr auto ImplosionInterval = 16000ms;
    static constexpr auto ImplosionCloudRevolutionSpeed = 2.0f * Pi<float> / std::chrono::duration<float>(0.5f).count();
    static constexpr auto PreExplosionInterval = 1000ms;
    static constexpr auto ExplosionInterval = 1000ms;

    State mState;

    // The timestamp of the last update
    GameWallClock::time_point mLastUpdateTimePoint;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The tracking of how long we've been at the current state; exact meaning
    // depends on the state
    GameWallClock::time_point mCurrentStateStartTimePoint;
    float mCurrentStateProgress;

    // The current cloud rotation angle
    float mCurrentCloudRotationAngle;
};

}
