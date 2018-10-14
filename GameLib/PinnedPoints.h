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
        , mCurrentPinnedPoints()
    {
    }

    void OnPointDestroyed(ElementIndex pointElementIndex);

    void OnSpringDestroyed(ElementIndex springElementIndex);

    bool ToggleAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters)
    {
        float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

        //
        // See first if there's a pinned point within the search radius, most recent first;
        // if so we unpin it and we're done
        //

        for (auto it = mCurrentPinnedPoints.begin(); it != mCurrentPinnedPoints.end(); ++it)
        {
            assert(!mShipPoints.IsDeleted(*it));
            assert(mShipPoints.IsPinned(*it));

            float squareDistance = (mShipPoints.GetPosition(*it) - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // Found a pinned point

                // Unpin it
                mShipPoints.Unpin(*it);

                // Remove from set of pinned points
                mCurrentPinnedPoints.erase(it);

                // Notify
                mGameEventHandler->OnPinToggled(
                    false,
                    mParentWorld.IsUnderwater(mShipPoints.GetPosition(*it)));

                // We're done
                return true;
            }
        }


        //
        // No pinned points in radius...
        // ...so find closest unpinned point within the search radius, and
        // if found, pin it
        //

        ElementIndex nearestUnpinnedPointIndex = NoneElementIndex;
        float nearestUnpinnedPointDistance = std::numeric_limits<float>::max();

        for (auto pointIndex : mShipPoints)
        {
            if (!mShipPoints.IsDeleted(pointIndex) && !mShipPoints.IsPinned(pointIndex))
            {
                float squareDistance = (mShipPoints.GetPosition(pointIndex) - targetPos).squareLength();
                if (squareDistance < squareSearchRadius)
                {
                    // This point is within the search radius

                    // Keep the nearest
                    if (squareDistance < squareSearchRadius && squareDistance < nearestUnpinnedPointDistance)
                    {
                        nearestUnpinnedPointIndex = pointIndex;
                        nearestUnpinnedPointDistance = squareDistance;
                    }
                }
            }
        }

        if (NoneElementIndex != nearestUnpinnedPointIndex)
        {
            // We have a nearest, unpinned point

            // Pin it
            mShipPoints.Pin(nearestUnpinnedPointIndex);

            // Add to set of pinned points, unpinning eventual pins that might get purged 
            mCurrentPinnedPoints.emplace(
                [this](auto purgedPinnedPointIndex)
                {
                    this->mShipPoints.Unpin(purgedPinnedPointIndex);
                },
                nearestUnpinnedPointIndex);

            // Notify
            mGameEventHandler->OnPinToggled(
                true,
                mParentWorld.IsUnderwater(mShipPoints.GetPosition(nearestUnpinnedPointIndex)));

            // We're done
            return true;
        }

        // No point found on this ship
        return false;
    }

    //
    // Render
    //

    void Upload(
        int shipId,
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
    
    // The container of all the ship's points
    Points & mShipPoints;

    // The container of all the ship's springs
    Springs & mShipSprings;

    // The current set of pinned points
    CircularList<ElementIndex, GameParameters::MaxPinnedPoints> mCurrentPinnedPoints;
};

}
