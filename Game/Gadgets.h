/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-05-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "Physics.h"
#include "RenderContext.h"

#include <GameCore/CircularList.h>
#include <GameCore/GameTypes.h>
#include <GameCore/StrongTypeDef.h>
#include <GameCore/Vectors.h>

#include <functional>
#include <memory>

namespace Physics
{

/*
 * Container of gadgets, i.e. "thinghies" that the user may attach
 * to particles of a ship and which perform various actions.
 *
 * The physics handler can be used to feed-back actions to the world.
 */
class Gadgets final
{
public:

    Gadgets(
        World & parentWorld,
        ShipId shipId,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mParentWorld(parentWorld)
        , mShipId(shipId)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(shipPhysicsHandler)
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
        , mCurrentGadgets()
        , mCurrentPhysicsProbeGadget()
        , mNextLocalGadgetId(0)
    {
    }

    bool AreBombsInProximity(vec2f const & position) const;

    void Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void OnPointDetached(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void OnSpringDestroyed(
        ElementIndex springElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void OnElectricSpark(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<AntiMatterBombGadget>(
            targetPos,
            gameParameters);
    }

    bool ToggleFireExtinguishingBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<FireExtinguishingBombGadget>(
            targetPos,
            gameParameters);
    }

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<ImpactBombGadget>(
            targetPos,
            gameParameters);
    }

    std::optional<bool> TogglePhysicsProbeAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void RemovePhysicsProbe();

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<RCBombGadget>(
            targetPos,
            gameParameters);
    }

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<TimerBombGadget>(
            targetPos,
            gameParameters);
    }

    void DetonateRCBombs(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void DetonateAntiMatterBombs();

    //
    // Render
    //

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

private:

    template <typename TGadget>
    std::unique_ptr<Gadget> InternalCreateGadget(
        ElementIndex pointIndex,
        StrongTypedBool<struct DoNotify> doNotify)
    {
        // Create gadget
        std::unique_ptr<Gadget> gadget(
            new TGadget(
                GlobalGadgetId(mShipId, mNextLocalGadgetId++),
                pointIndex,
                mParentWorld,
                mGameEventHandler,
                mShipPhysicsHandler,
                mShipPoints,
                mShipSprings));

        // Attach gadget to the particle
        assert(!mShipPoints.IsGadgetAttached(pointIndex));
        mShipPoints.AttachGadget(
            pointIndex,
            gadget->GetMass(),
            mShipSprings);

        if (doNotify)
        {
            // Notify
            mGameEventHandler->OnGadgetPlaced(
                gadget->GetId(),
                gadget->GetType(),
                mParentWorld.GetOceanSurface().IsUnderwater(
                    gadget->GetPosition()));
        }

        return gadget;
    }

    void InternalPreGadgetRemoval(
        Gadget & gadget,
        StrongTypedBool<struct DoNotify> doNotify)
    {
        // Tell gadget we're removing it
        gadget.OnExternallyRemoved();

        // Detach gadget from its particle
        assert(mShipPoints.IsGadgetAttached(gadget.GetPointIndex()));
        mShipPoints.DetachGadget(
            gadget.GetPointIndex(),
            mShipSprings);

        if (doNotify)
        {
            // Notify removal
            mGameEventHandler->OnGadgetRemoved(
                gadget.GetId(),
                gadget.GetType(),
                mParentWorld.GetOceanSurface().IsUnderwater(gadget.GetPosition()));
        }
    }

    template <typename TGadget>
    bool ToggleGadgetAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

        //
        // See first if there's a gadget within the search radius, most recent first;
        // if so - and it allows us to remove it - then we remove it and we're done
        //

        for (auto it = mCurrentGadgets.begin(); it != mCurrentGadgets.end(); ++it)
        {
            float squareDistance = ((*it)->GetPosition() - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // Found a gadget

                // Check whether it's ok with being removed
                if ((*it)->MayBeRemoved())
                {
                    //
                    // Remove gadget
                    //

                    InternalPreGadgetRemoval(
                        *(*it),
                        StrongTypedTrue<DoNotify>);

                    mCurrentGadgets.erase(it); // Safe to invalidate iterators, we're leaving anyway
                }

                // We're done
                return true;
            }
        }

        //
        // No gadget in radius...
        // ...so find closest particle with at least one spring and no attached gadget within the search radius, and
        // if found, attach gadget to it
        //

        ElementIndex nearestCandidatePointIndex = NoneElementIndex;
        float nearestCandidatePointDistance = std::numeric_limits<float>::max();

        for (auto pointIndex : mShipPoints.RawShipPoints())
        {
            if (!mShipPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty()
                && !mShipPoints.IsGadgetAttached(pointIndex))
            {
                float squareDistance = (mShipPoints.GetPosition(pointIndex) - targetPos).squareLength();
                if (squareDistance < squareSearchRadius)
                {
                    // This particle is within the search radius

                    // Keep the nearest
                    if (squareDistance < squareSearchRadius && squareDistance < nearestCandidatePointDistance)
                    {
                        nearestCandidatePointIndex = pointIndex;
                        nearestCandidatePointDistance = squareDistance;
                    }
                }
            }
        }

        if (NoneElementIndex != nearestCandidatePointIndex)
        {
            // We have a nearest candidate particle

            // Create gadget
            auto gadget = InternalCreateGadget<TGadget>(
                nearestCandidatePointIndex,
                StrongTypedTrue<DoNotify>);

            // Add new gadget to set of gadgets, removing eventual gadgets that might get purged
            mCurrentGadgets.emplace(
                [this](std::unique_ptr<Gadget> const & purgedGadget)
                {
                    InternalPreGadgetRemoval(
                        *purgedGadget,
                        StrongTypedTrue<DoNotify>);
                },
                std::move(gadget));

            // We're done
            return true;
        }

        // No spring found on this ship
        return false;
    }

private:

    static float constexpr NeighborhoodRadius = 3.5f; // Magic number

private:

    // Our parent world
    World & mParentWorld;

    // The ID of the ship we belong to
    ShipId const mShipId;

    // The game event handler
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // The handler to invoke for acting on the ship
    IShipPhysicsHandler & mShipPhysicsHandler;

    // The container of all the ship's points
    Points & mShipPoints;

    // The container of all the ship's springs
    Springs & mShipSprings;

    // The current set of gadgets, excluding physics probe gadget
    CircularList<std::unique_ptr<Gadget>, GameParameters::MaxGadgets> mCurrentGadgets;

    // The current physics probe gadget
    std::unique_ptr<Gadget> mCurrentPhysicsProbeGadget;

    // The next gadget ID value
    GadgetId mNextLocalGadgetId;
};

}
