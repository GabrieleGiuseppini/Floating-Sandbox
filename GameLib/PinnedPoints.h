/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-06-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "CircularList.h"
#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "Physics.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <memory>

namespace Physics
{	

/*
 * This class manages the set of points that has been pinned.
 *
 * All game events are taken care of by this class.
 */
class PinnedPoints final
{
public:

    PinnedPoints(
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
    {
    }

    void OnPointDestroyed(ElementIndex pointElementIndex);

    void OnSpringDestroyed(ElementIndex springElementIndex);

    bool ToggleTimerBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<TimerBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleRCBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<RCBomb>(
            targetPos,
            gameParameters);
    }

    void DetonateRCBombs();

    void Upload(
        int shipId,
        RenderContext & renderContext) const;

private:

    template <typename TBomb>
    bool ToggleBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

        //
        // See first if there's a bomb within the search radius, most recent first;
        // if so we remove it and we're done
        //

        for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); ++it)
        {
            float squareDistance = ((*it)->GetPosition() - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // Found a bomb

                // Tell it we're removing it
                (*it)->OnBombRemoved();

                // Remove from set of bombs - forget about it
                mCurrentBombs.erase(it);

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
                    ObjectIdGenerator::GetInstance().Generate(),
                    nearestUnarmedSpringIndex,
                    mParentWorld,
                    mGameEventHandler,
                    mBlastHandler,
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
                [this, &gameParameters](std::unique_ptr<Bomb> const & purgedBomb)
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

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // The handler to invoke for each explosion
    Bomb::BlastHandler const mBlastHandler;
    
    // The container of all the ship's points
    Points & mShipPoints;

    // The container of all the ship's springs
    Springs & mShipSprings;

    // The current set of bombs
    CircularList<std::unique_ptr<Bomb>, GameParameters::MaxBombs> mCurrentBombs;
};

}
