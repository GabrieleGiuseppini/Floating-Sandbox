/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IGameEventHandlers.h"

#include <optional>
#include <vector>

/*
 * Dispatches events to multiple sinks, aggregating some events in the process.
 */
class GameEventDispatcher final
    : public IGameEventHandler
    , public IGameStatisticsEventHandler
{
public:

    GameEventDispatcher()
        : mGameSinks()
        , mGameStatisticsSinks()
    {
    }

    GameEventDispatcher(GameEventDispatcher &) = delete;
    GameEventDispatcher(GameEventDispatcher &&) = delete;
    GameEventDispatcher operator=(GameEventDispatcher &) = delete;
    GameEventDispatcher operator=(GameEventDispatcher &&) = delete;

public:

    //
    // Game
    //

    void OnGameReset() override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnGameReset();
        }
    }

    void OnShipLoaded(
        ShipId id,
        ShipMetadata const & shipMetadata) override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnShipLoaded(id, shipMetadata);
        }
    }

    void OnTsunamiNotification(float x) override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnTsunamiNotification(x);
        }
    }

    void OnAutoFocusTargetChanged(
        std::optional<AutoFocusTargetKindType> autoFocusTarget) override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnAutoFocusTargetChanged(autoFocusTarget);
        }
    }

    void OnSilenceStarted() override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnSilenceStarted();
        }
    }

    void OnSilenceLifted() override
    {
        for (auto sink : mGameSinks)
        {
            sink->OnSilenceLifted();
        }
    }

    //
    // Game Statistics
    //

    void OnFrameRateUpdated(
        float immediateFps,
        float averageFps) override
    {
        for (auto sink : mGameStatisticsSinks)
        {
            sink->OnFrameRateUpdated(
                immediateFps,
                averageFps);
        }
    }

    void OnCurrentUpdateDurationUpdated(float currentUpdateDuration) override
    {
        for (auto sink : mGameStatisticsSinks)
        {
            sink->OnCurrentUpdateDurationUpdated(currentUpdateDuration);
        }
    }

public:

    /*
     * Flushes all events aggregated so far and clears the state.
     */
    void Flush()
    {
    }

    void RegisterGameEventHandler(IGameEventHandler * sink)
    {
        mGameSinks.push_back(sink);
    }

    void RegisterGameStatisticsEventHandler(IGameStatisticsEventHandler * sink)
    {
        mGameStatisticsSinks.push_back(sink);
    }

private:

    // The registered sinks
    std::vector<IGameEventHandler *> mGameSinks;
    std::vector<IGameStatisticsEventHandler *> mGameStatisticsSinks;
};
