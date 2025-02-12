/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-05
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
 * Gadget specialization for fire-extinguishing bombs.
 *
 * These bombs detonate with fire, but are also remote-controlled and sensitive
 * to disturbances.
 */
class FireExtinguishingBombGadget final : public Gadget
{
public:

    FireExtinguishingBombGadget(
        GlobalGadgetId id,
        ElementIndex pointIndex,
        World & parentWorld,
        SimulationEventDispatcher & simulationEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual float GetMass() const override
    {
        return SimulationParameters::BombMass;
    }

    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters) override;

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
        SimulationParameters const & simulationParameters) override
    {
        Detonate(currentSimulationTime, simulationParameters);
    }

    virtual void Upload(
        ShipId shipId,
        RenderContext & renderContext) const override;

    void Detonate(
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state we wait for remote detonation or disturbance
        Idle,

        // We are exploding (only used for rendering purposes)
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    State mState;

    static constexpr int ExplosionFadeoutStepsCount = 8;

    // The counters for the various states. Fine to rollover!
    uint8_t mExplosionFadeoutCounter; // Betewen 0 and ExplosionFadeoutStepsCount (excluded)

    // The position and plane at which the explosion has started
    vec2f mExplosionPosition;
    PlaneId mExplosionPlaneId;
};

}
