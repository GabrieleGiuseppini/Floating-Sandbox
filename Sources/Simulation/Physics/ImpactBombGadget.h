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
 * Gadget specialization for bombs that explode on impact.
 */
class ImpactBombGadget final : public Gadget
{
public:

    ImpactBombGadget(
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
        float /*currentSimulationTime*/,
        SimulationParameters const & /*simulationParameters*/) override
    {
        if (State::Idle == mState)
        {
            // Transition to trigger-explosion
            mState = State::TriggeringExplosion;
        }
    }

    virtual void Upload(
        ShipId shipId,
        RenderContext & renderContext) const override;

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state we are just idle
        Idle,

        // Dummy state, just starts explosion
        TriggeringExplosion,

        // We are exploding (only used for rendering purposes)
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    State mState;

    static constexpr int ExplosionFadeoutStepsCount = 8;

    uint8_t mExplosionFadeoutCounter; // Betewen 0 and ExplosionFadeoutStepsCount (excluded)

    // The position and plane ID at which the explosion has started
    vec2f mExplosionPosition;
    PlaneId mExplosionPlaneId;
};

}
