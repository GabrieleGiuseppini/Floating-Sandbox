/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-06-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <Core/CircularList.h>
#include <Core/Vectors.h>

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
        SimulationEventDispatcher & simulationEventDispatcher,
        Points & shipPoints)
        : mParentWorld(parentWorld)
        , mSimulationEventHandler(simulationEventDispatcher)
        , mShipPoints(shipPoints)
        , mCurrentPinnedPoints()
    {
    }

    void OnEphemeralParticleDestroyed(ElementIndex pointElementIndex);

    bool ToggleAt(
        vec2f const & targetPos,
        SimulationParameters const & simulationParameters)
    {
        float const squareSearchRadius = simulationParameters.ObjectSearchRadiusWorld * simulationParameters.ObjectSearchRadiusWorld;

        //
        // See first if there's a pinned point within the search radius, most recent first;
        // if so we unpin it and we're done
        //

        for (auto it = mCurrentPinnedPoints.begin(); it != mCurrentPinnedPoints.end(); ++it)
        {
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
                mSimulationEventHandler.OnPinToggled(
                    false,
                    mParentWorld.GetOceanSurface().IsUnderwater(mShipPoints.GetPosition(*it)));

                // We're done
                return true;
            }
        }


        //
        // No pinned points in radius...
        // ...so find closest unpinned point within the search radius, and
        // if found, pin it.
        //
        // We only allow non-ephemerals and air bubble ephemerals to be pinned.
        //

        ElementIndex nearestUnpinnedPointIndex = NoneElementIndex;
        float nearestUnpinnedPointDistance = std::numeric_limits<float>::max();

        for (auto pointIndex : mShipPoints)
        {
            if (mShipPoints.IsActive(pointIndex)
                && !mShipPoints.IsPinned(pointIndex)
                && (!mShipPoints.IsEphemeral(pointIndex) || Points::EphemeralType::AirBubble == mShipPoints.GetEphemeralType(pointIndex)))
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
            mSimulationEventHandler.OnPinToggled(
                true,
                mParentWorld.GetOceanSurface().IsUnderwater(mShipPoints.GetPosition(nearestUnpinnedPointIndex)));

            // We're done
            return true;
        }

        // No point found on this ship
        return false;
    }

    void RemoveAll()
    {
        for (auto it = mCurrentPinnedPoints.begin(); it != mCurrentPinnedPoints.end(); ++it)
        {
            assert(mShipPoints.IsPinned(*it));

            // Unpin it
            mShipPoints.Unpin(*it);

            // Notify
            mSimulationEventHandler.OnPinToggled(
                false,
                mParentWorld.GetOceanSurface().IsUnderwater(mShipPoints.GetPosition(*it)));
        }

        mCurrentPinnedPoints.clear();
    }

    //
    // Render
    //

    void Upload(
        ShipId shipId,
        RenderContext & renderContext) const;

private:

    // Our parent world
    World & mParentWorld;

    // The game event handler
    SimulationEventDispatcher & mSimulationEventHandler;

    // The container of all the ship's points
    Points & mShipPoints;

    // The current set of pinned points
    CircularList<ElementIndex, SimulationParameters::MaxPinnedPoints> mCurrentPinnedPoints;
};

}
