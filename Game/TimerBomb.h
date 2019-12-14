/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace Physics
{

/*
 * Bomb specialization for bombs that explode after a time interval.
 */
class TimerBomb final : public Bomb
{
public:

    TimerBomb(
        BombId id,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        GameParameters const & gameParameters) override;

    virtual bool MayBeRemoved() const override
    {
        // We can always be removed
        return true;
    }

    virtual void OnBombRemoved() override
    {
        // Stop fuse if it's burning
        if (State::SlowFuseBurning == mState
            || State::FastFuseBurning == mState)
        {
            mGameEventHandler->OnTimerBombFuse(mId, std::nullopt);
        }

        // Notify removal
        mGameEventHandler->OnBombRemoved(
            mId,
            BombType::TimerBomb,
            mParentWorld.IsUnderwater(
                GetPosition()));

        // Detach ourselves, if we're attached
        DetachIfAttached();
    }

    virtual void OnNeighborhoodDisturbed() override;

    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const override;

private:

    void TransitionToFastFusing(GameWallClock::time_point currentWallClockTime);

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state the fuse burns slowly, and after a while the bomb moves
        // to detonation lead-in
        SlowFuseBurning,

        // In this state the fuse burns fast, and then the bomb moves to exploding
        FastFuseBurning,

        // In this state we are about to explode; we wait a little time and then
        // move to exploding
        DetonationLeadIn,

        // We enter this state once the bomb gets underwater; we play a short
        // smoke animation and then we transition to defuse
        Defusing,

        // Final state of defusing; we just stick around
        Defused,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    static constexpr auto SlowFuseToDetonationLeadInInterval = 8000ms;
    static constexpr auto FastFuseToDetonationLeadInInterval = 2000ms;
    static constexpr int FuseStepCount = 16;
    static constexpr int FuseLengthStepCount = 4;
    static constexpr int FuseFramesPerFuseLengthCount = FuseStepCount / FuseLengthStepCount;

    static constexpr auto DetonationLeadInToExplosionInterval = 1500ms;

    static constexpr auto DefusingInterval = 500ms;
    static constexpr uint8_t DefuseStepsCount = 3;

    State mState;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The fuse flame frame index, which is calculated at state transitions
    TextureFrameIndex mFuseFlameFrameIndex;

    // The counters for the various states; set to zero upon
    // entering the state for the first time. Fine to rollover!
    uint8_t mFuseStepCounter;
    uint8_t mDefuseStepCounter;
    uint8_t mDetonationLeadInShapeFrameCounter;
};

}
