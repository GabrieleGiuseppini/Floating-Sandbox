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
        ObjectId id,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) override;

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
        int shipId,
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
        PreImploding_2,
        Imploding_3,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding_4,

        // This is the final state; once this state is reached, we're expired
        Expired_5
    };

    static constexpr auto ContainedRevolutionInterval = 1000ms;
    static constexpr auto PreImplosionInterval = 1000ms;
    static constexpr auto ImplosionInterval = 16000ms;
    static constexpr auto ExplosionInterval = 1000ms;

    State mState;

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
