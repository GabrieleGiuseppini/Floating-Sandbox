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
 * This class manages a set of bombs.
 *
 * All game events are taken care of by this class.
 *
 * The explosion handler can be used to modify the world due to the explosion.
 */
class Bombs final
{
public:

    Bombs(
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
        , mCurrentBombs()
        , mNextLocalBombId(0)
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
        return ToggleBombAt<AntiMatterBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<ImpactBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<RCBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<TimerBomb>(
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

    template <typename TBomb>
    bool ToggleBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

        //
        // See first if there's a bomb within the search radius, most recent first;
        // if so - and it allows us to remove it - then we remove it and we're done
        //

        for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); ++it)
        {
            float squareDistance = ((*it)->GetPosition() - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // Found a bomb

                // Check whether it's ok with being removed
                if ((*it)->MayBeRemoved())
                {
                    // Tell it we're removing it
                    (*it)->OnBombRemoved();

                    // Remove from set of bombs - forget about it
                    mCurrentBombs.erase(it);
                }

                // We're done
                return true;
            }
        }


        //
        // No bombs in radius...
        // ...so find closest spring with no attached bomb within the search radius, and
        // if found, attach bomb to it it
        //

        ElementIndex nearestUnarmedSpringIndex = NoneElementIndex;
        float nearestUnarmedSpringDistance = std::numeric_limits<float>::max();

        for (auto springIndex : mShipSprings)
        {
            if (!mShipSprings.IsDeleted(springIndex) && !mShipSprings.IsBombAttached(springIndex))
            {
                float squareDistance = (mShipSprings.GetMidpointPosition(springIndex, mShipPoints) - targetPos).squareLength();
                if (squareDistance < squareSearchRadius)
                {
                    // This spring is within the search radius

                    // Keep the nearest
                    if (squareDistance < squareSearchRadius && squareDistance < nearestUnarmedSpringDistance)
                    {
                        nearestUnarmedSpringIndex = springIndex;
                        nearestUnarmedSpringDistance = squareDistance;
                    }
                }
            }
        }

        if (NoneElementIndex != nearestUnarmedSpringIndex)
        {
            // We have a nearest, unarmed spring

            // Create bomb
            std::unique_ptr<Bomb> bomb(
                new TBomb(
                    BombId(mShipId, mNextLocalBombId++),
                    nearestUnarmedSpringIndex,
                    mParentWorld,
                    mGameEventHandler,
                    mShipPhysicsHandler,
                    mShipPoints,
                    mShipSprings));

            // Attach bomb to the spring
            mShipSprings.AttachBomb(
                nearestUnarmedSpringIndex,
                mShipPoints,
                gameParameters);

            // Notify
            mGameEventHandler->OnBombPlaced(
                bomb->GetId(),
                bomb->GetType(),
                mParentWorld.IsUnderwater(
                    bomb->GetPosition()));

            // Add new bomb to set of bombs, removing eventual bombs that might get purged
            mCurrentBombs.emplace(
                [](std::unique_ptr<Bomb> const & purgedBomb)
                {
                    // Tell it we're removing it
                    purgedBomb->OnBombRemoved();
                },
                std::move(bomb));

            // We're done
            return true;
        }

        // No spring found on this ship
        return false;
    }

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

    // The current set of bombs
    CircularList<std::unique_ptr<Bomb>, GameParameters::MaxBombs> mCurrentBombs;

    // The next bomb ID value
    LocalBombId mNextLocalBombId;
};

}
