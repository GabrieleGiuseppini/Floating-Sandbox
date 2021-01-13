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
#include <GameCore/Vectors.h>

#include <functional>
#include <memory>

namespace Physics
{

/*
 * This class manages a set of gadgets, i.e. "thinghies" that the user may attach
 * to a ship and which perform various things.
 *
 * All game events are taken care of by this class.
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
        , mNextLocalGadgetId(0)
    {
    }

    void Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void OnPointDetached(ElementIndex pointElementIndex);

    void OnSpringDestroyed(ElementIndex springElementIndex);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<AntiMatterBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<ImpactBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<RCBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleGadgetAt<TimerBomb>(
            targetPos,
            gameParameters);
    }

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

    //
    // Render
    //

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

private:

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
                    // Tell it we're removing it
                    (*it)->OnRemoved();

                    // Remove from set of gadgets - forget about it
                    mCurrentGadgets.erase(it);
                }

                // We're done
                return true;
            }
        }


        //
        // No gadgetss in radius...
        // ...so find closest spring with no attached gadget within the search radius, and
        // if found, attach gadget to it it
        //

        ElementIndex nearestCandidateSpringIndex = NoneElementIndex;
        float nearestCandidateSpringDistance = std::numeric_limits<float>::max();

        for (auto springIndex : mShipSprings)
        {
            if (!mShipSprings.IsDeleted(springIndex) && !mShipSprings.IsGadgetAttached(springIndex))
            {
                float squareDistance = (mShipSprings.GetMidpointPosition(springIndex, mShipPoints) - targetPos).squareLength();
                if (squareDistance < squareSearchRadius)
                {
                    // This spring is within the search radius

                    // Keep the nearest
                    if (squareDistance < squareSearchRadius && squareDistance < nearestCandidateSpringDistance)
                    {
                        nearestCandidateSpringIndex = springIndex;
                        nearestCandidateSpringDistance = squareDistance;
                    }
                }
            }
        }

        if (NoneElementIndex != nearestCandidateSpringIndex)
        {
            // We have a nearest candidate spring

            // Create gadget
            std::unique_ptr<Gadget> gadget(
                new TGadget(
                    GadgetId(mShipId, mNextLocalGadgetId++),
                    nearestCandidateSpringIndex,
                    mParentWorld,
                    mGameEventHandler,
                    mShipPhysicsHandler,
                    mShipPoints,
                    mShipSprings));

            // Attach gadget to the spring
            mShipSprings.AttachGadget(
                nearestCandidateSpringIndex,
                gadget->GetMass(),
                mShipPoints);

            // Notify
            mGameEventHandler->OnGadgetPlaced(
                gadget->GetId(),
                gadget->GetType(),
                mParentWorld.IsUnderwater(
                    gadget->GetPosition()));

            // Add new gadget to set of gadgetss, removing eventual gadgets that might get purged
            mCurrentGadgets.emplace(
                [](std::unique_ptr<Gadget> const & purgedGadget)
                {
                    // Tell it we're removing it
                    purgedGadget->OnRemoved();
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

    // The current set of gadgets
    CircularList<std::unique_ptr<Gadget>, GameParameters::MaxGadgets> mCurrentGadgets;

    // The next gadget ID value
    LocalGadgetId mNextLocalGadgetId;
};

}
