/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
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
 * Gadget specialization for bombs that explode when a remote control is triggered.
 */
class RCBombGadget final : public Gadget
{
public:

    RCBombGadget(
        GlobalGadgetId id,
        ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual float GetMass() const override
    {
        return GameParameters::BombMass;
    }

    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters) override;

    virtual bool MayBeRemoved() const override
    {
        // We can always be removed
        return true;
    }

    virtual void OnExternallyRemoved() override
    {
    }

    virtual void OnNeighborhoodDisturbed(
        float currentSimulationTime,
        GameParameters const & gameParameters) override
    {
        Detonate(currentSimulationTime, gameParameters);
    }

    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const override;

    void Detonate(
        float currentSimulationTime,
        GameParameters const & gameParameters);

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In these states we wait for remote detonation or disturbance,
        // and ping regularly at long intervals, transitioning between
        // on and off
        IdlePingOff,
        IdlePingOn,

        // In this state we are about to explode; we wait a little time
        // before exploding, and ping regularly at short intervals
        DetonationLeadIn,

        // We are exploding (only used for rendering purposes)
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    State mState;

    static constexpr auto SlowPingOffInterval = 750ms;
    static constexpr auto SlowPingOnInterval = 250ms;
    static constexpr auto FastPingInterval = 100ms;
    static constexpr auto DetonationLeadInToExplosionInterval = 1500ms;
    static constexpr int PingFramesCount = 4;
    static constexpr int ExplosionFadeoutStepsCount = 8;

    inline void TransitionToDetonationLeadIn(GameWallClock::time_point currentWallClockTime)
    {
        mState = State::DetonationLeadIn;

        ++mPingOnStepCounter;

        mGameEventHandler->OnRCBombPing(
            mParentWorld.GetOceanSurface().IsUnderwater(GetPosition()),
            1);

        // Schedule next transition
        mNextStateTransitionTimePoint = currentWallClockTime + FastPingInterval;
    }

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The timestamp at which we'll explode while in detonation lead-in
    GameWallClock::time_point mExplosionIgnitionTimestamp;

    // The counters for the various states. Fine to rollover!
    uint8_t mPingOnStepCounter; // Set to one upon entering
    uint8_t mExplosionFadeoutCounter; // Betewen 0 and ExplosionFadeoutStepsCount (excluded)

    // The position and plane at which the explosion has started
    vec2f mExplosionPosition;
    PlaneId mExplosionPlaneId;
};

}
