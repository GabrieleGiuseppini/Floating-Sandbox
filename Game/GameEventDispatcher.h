/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IGameEventHandler.h"

#include <GameCore/TupleKeys.h>

#include <algorithm>
#include <optional>
#include <vector>

class GameEventDispatcher : public IGameEventHandler
{
public:

    GameEventDispatcher()
        : mSpringRepairedEvents()
        , mTriangleRepairedEvents()
        , mStressEvents()
        , mBreakEvents()
        , mSinkingBeginEvents()
        , mSinkingEndEvents()
        , mLightFlickerEvents()
        , mBombExplosionEvents()
        , mRCBombPingEvents()
        , mTimerBombDefusedEvents()
        , mSinks()
    {
    }

public:

    virtual void OnGameReset() override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnGameReset();
        }
    }

    virtual void OnShipLoaded(
        unsigned int id,
        std::string const & name,
        std::optional<std::string> const & author) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnShipLoaded(id, name, author);
        }
    }

    virtual void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnDestroy(structuralMaterial, isUnderwater, size);
        }
    }

    virtual void OnSpringRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mSpringRepairedEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    virtual void OnTriangleRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mTriangleRepairedEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    virtual void OnSawed(
        bool isMetal,
        unsigned int size) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnSawed(isMetal, size);
        }
    }

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnPinToggled(isPinned, isUnderwater);
        }
    }

    virtual void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mStressEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    virtual void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mBreakEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    virtual void OnSinkingBegin(ShipId shipId) override
    {
        if (mSinkingBeginEvents.end() == std::find(mSinkingBeginEvents.begin(), mSinkingBeginEvents.end(), shipId))
        {
            mSinkingBeginEvents.push_back(shipId);
        }
    }

    virtual void OnSinkingEnd(ShipId shipId) override
    {
        if (mSinkingEndEvents.end() == std::find(mSinkingEndEvents.begin(), mSinkingEndEvents.end(), shipId))
        {
            mSinkingEndEvents.push_back(shipId);
        }
    }

    virtual void OnLightFlicker(
        DurationShortLongType duration,
        bool isUnderwater,
        unsigned int size) override
    {
        mLightFlickerEvents[std::make_tuple(duration, isUnderwater)] += size;
    }

    virtual void OnWaterTaken(float waterTaken) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnWaterTaken(waterTaken);
        }
    }

    virtual void OnWaterSplashed(float waterSplashed) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnWaterSplashed(waterSplashed);
        }
    }

    virtual void OnWindSpeedUpdated(
        float const zeroSpeedMagnitude,
        float const baseSpeedMagnitude,
        float const preMaxSpeedMagnitude,
        float const maxSpeedMagnitude,
        vec2f const & windSpeed) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnWindSpeedUpdated(
                zeroSpeedMagnitude,
                baseSpeedMagnitude,
                preMaxSpeedMagnitude,
                maxSpeedMagnitude,
                windSpeed);
        }
    }

    virtual void OnCustomProbe(
        std::string const & name,
        float value) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnCustomProbe(
                name,
                value);
        }
    }

    virtual void OnFrameRateUpdated(
        float immediateFps,
        float averageFps) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnFrameRateUpdated(
                immediateFps,
                averageFps);
        }
    }

    virtual void OnUpdateToRenderRatioUpdated(
        float immediateURRatio)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnUpdateToRenderRatioUpdated(
                immediateURRatio);
        }
    }

    //
    // Bombs
    //

    virtual void OnBombPlaced(
        ObjectId bombId,
        BombType bombType,
        bool isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnBombPlaced(
                bombId,
                bombType,
                isUnderwater);
        }
    }

    virtual void OnBombRemoved(
        ObjectId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnBombRemoved(
                bombId,
                bombType,
                isUnderwater);
        }
    }

    virtual void OnBombExplosion(
        BombType bombType,
        bool isUnderwater,
        unsigned int size) override
    {
        mBombExplosionEvents[std::make_tuple(bombType, isUnderwater)] += size;
    }

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override
    {
        mRCBombPingEvents[std::make_tuple(isUnderwater)] += size;
    }

    virtual void OnTimerBombFuse(
        ObjectId bombId,
        std::optional<bool> isFast)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnTimerBombFuse(
                bombId,
                isFast);
        }
    }

    virtual void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override
    {
        mTimerBombDefusedEvents[std::make_tuple(isUnderwater)] += size;
    }

    virtual void OnAntiMatterBombContained(
        ObjectId bombId,
        bool isContained)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnAntiMatterBombContained(
                bombId,
                isContained);
        }
    }

    virtual void OnAntiMatterBombPreImploding()
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnAntiMatterBombPreImploding();
        }
    }

    virtual void OnAntiMatterBombImploding()
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnAntiMatterBombImploding();
        }
    }

public:

    /*
     * Flushes all events aggregated so far and clears the state.
     */
    void Flush()
    {
        // Publish aggregations
        for (IGameEventHandler * sink : mSinks)
        {
            for (auto const & entry : mSpringRepairedEvents)
            {
                sink->OnSpringRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mTriangleRepairedEvents)
            {
                sink->OnTriangleRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mStressEvents)
            {
                sink->OnStress(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mBreakEvents)
            {
                sink->OnBreak(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & shipId : mSinkingBeginEvents)
            {
                sink->OnSinkingBegin(shipId);
            }

            for (auto const & shipId : mSinkingEndEvents)
            {
                sink->OnSinkingEnd(shipId);
            }

            for (auto const & entry : mLightFlickerEvents)
            {
                sink->OnLightFlicker(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mBombExplosionEvents)
            {
                sink->OnBombExplosion(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mRCBombPingEvents)
            {
                sink->OnRCBombPing(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mTimerBombDefusedEvents)
            {
                sink->OnTimerBombDefused(std::get<0>(entry.first), entry.second);
            }
        }

        // Clear collections
        mSpringRepairedEvents.clear();
        mTriangleRepairedEvents.clear();
        mStressEvents.clear();
        mBreakEvents.clear();
        mSinkingBeginEvents.clear();
        mSinkingEndEvents.clear();
        mLightFlickerEvents.clear();
        mBombExplosionEvents.clear();
        mRCBombPingEvents.clear();
        mTimerBombDefusedEvents.clear();
        mTimerBombDefusedEvents.clear();
    }

    void RegisterSink(IGameEventHandler * sink)
    {
        mSinks.push_back(sink);
    }

private:

    // The current events being aggregated
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mSpringRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mTriangleRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mStressEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mBreakEvents;
    std::vector<ShipId> mSinkingBeginEvents;
    std::vector<ShipId> mSinkingEndEvents;
    unordered_tuple_map<std::tuple<DurationShortLongType, bool>, unsigned int> mLightFlickerEvents;
    unordered_tuple_map<std::tuple<BombType, bool>, unsigned int> mBombExplosionEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mRCBombPingEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mTimerBombDefusedEvents;

    // The registered sinks
    std::vector<IGameEventHandler *> mSinks;
};
