/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-07
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
 * Bomb specialization for bombs that explode on impact.
 */
class ImpactBomb final : public Bomb
{
public:

    ImpactBomb(
        ObjectId id,
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
        // We can always be removed
        return true;
    }

    virtual void OnBombRemoved() override
    {
        // Notify removal
        mGameEventHandler->OnBombRemoved(
            mId,
            BombType::ImpactBomb,
            mParentWorld.IsUnderwater(
                GetPosition()));

        // Detach ourselves, if we're attached
        DetachIfAttached();
    }

    virtual void OnNeighborhoodDisturbed() override
    {
        if (State::Idle == mState)
        {
            // Transition to trigger-explosion
            mState = State::TriggeringExplosion;
        }
    }

    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const override;

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state we are just idle
        Idle,

        // Dummy state, just transitions to Exploding
        TriggeringExplosion,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    static constexpr auto ExplosionProgressInterval = 20ms;
    static constexpr uint8_t ExplosionStepsCount = 9;

    inline void TransitionToExploding(
        GameWallClock::time_point currentWallClockTime,
        GameParameters const & gameParameters)
    {
        mState = State::Exploding;

        assert(mExplodingStepCounter < ExplosionStepsCount);

        // Check whether we're done
        if (mExplodingStepCounter == ExplosionStepsCount - 1)
        {
            // Transition to expired
            mState = State::Expired;
        }
        else
        {
            // Invoke blast handler
            mPhysicsHandler.DoBombExplosion(
                GetPosition(),
                static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount - 1),
                gameParameters);

            // Increment counter
            ++mExplodingStepCounter;

            // Schedule next transition
            mNextStateTransitionTimePoint = currentWallClockTime + ExplosionProgressInterval;
        }
    }

    State mState;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The counters for the various states. Fine to rollover!
    uint8_t mExplodingStepCounter; // Zero on first explosion state
};

}
