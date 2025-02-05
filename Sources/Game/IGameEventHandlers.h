/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Simulation/ShipMetadata.h>

#include <Core/GameTypes.h>

#include <optional>

/*
 * These interfaces define the methods that game event handlers must implement.
 *
 * The methods are default-implemented to facilitate implementation of handlers that
 * only care about a subset of the events.
 */

struct IGameEventHandler
{
    virtual void OnGameReset()
    {
        // Default-implemented
    }

    virtual void OnShipLoaded(
        ShipId /*id*/,
        ShipMetadata const & /*shipMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnTsunamiNotification(float /*x*/)
    {
        // Default-implemented
    }

    // Published at each change of auto-focus target
    virtual void OnAutoFocusTargetChanged(
        std::optional<AutoFocusTargetKindType> /*autoFocusTarget*/)
    {
        // Default-implemented
    }

    virtual void OnSilenceStarted()
    {
        // Default-implemented
    }

    virtual void OnSilenceLifted()
    {
        // Default-implemented
    }
};

struct IGameStatisticsEventHandler
{
    virtual void OnFrameRateUpdated(
        float /*immediateFps*/,
        float /*averageFps*/)
    {
        // Default-implemented
    }

    virtual void OnCurrentUpdateDurationUpdated(float /*currentUpdateDuration*/)
    {
        // Default-implemented
    }
};
