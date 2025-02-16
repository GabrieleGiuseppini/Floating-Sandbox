/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-01-17
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
 * Gadget specialization for probes that provide physical properties of a particle.
 */
class PhysicsProbeGadget final : public Gadget
{
public:

    PhysicsProbeGadget(
        GlobalGadgetId id,
        ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual float GetMass() const override
    {
        // Physics probes are weightless!
        return 0.0f;
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
        float /*currentSimulationTime*/,
        GameParameters const & /*gameParameters*/) override
    {
        // Doe niets
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
        PingOff,
        PingOn
    };

    State mState;

    static constexpr auto PingOffInterval = 150ms;
    static constexpr auto PingOnInterval = 150ms;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;
};

}
