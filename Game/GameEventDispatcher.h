/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventHandlers.h"

#include <GameCore/TupleKeys.h>

#include <algorithm>
#include <optional>
#include <vector>

class GameEventDispatcher
    : public ILifecycleGameEventHandler
    , public IStructuralGameEventHandler
    , public IWavePhenomenaGameEventHandler
    , public IStatisticsGameEventHandler
    , public IGenericGameEventHandler
{
public:

    GameEventDispatcher()
        : mSpringRepairedEvents()
        , mTriangleRepairedEvents()
        , mStressEvents()
        , mBreakEvents()
        , mLightFlickerEvents()
        , mBombExplosionEvents()
        , mRCBombPingEvents()
        , mTimerBombDefusedEvents()
        // Sinks
        , mLifecycleSinks()
        , mStructuralSinks()
        , mWavePhenomenaSinks()
        , mStatisticsSinks()
        , mGenericSinks()
    {
    }

public:

    //
    // Lifecycle
    //

    virtual void OnGameReset() override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnGameReset();
        }
    }

    virtual void OnShipLoaded(
        unsigned int id,
        std::string const & name,
        std::optional<std::string> const & author) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnShipLoaded(id, name, author);
        }
    }

    virtual void OnSinkingBegin(ShipId shipId) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnSinkingBegin(shipId);
        }
    }

    virtual void OnSinkingEnd(ShipId shipId) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnSinkingEnd(shipId);
        }
    }

    //
    // Structural
    //

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

    //
    // Wave phenomena
    //

    virtual void OnTsunami(float x) override
    {
        for (auto sink : mWavePhenomenaSinks)
        {
            sink->OnTsunami(x);
        }
    }

    virtual void OnTsunamiNotification(float x) override
    {
        for (auto sink : mWavePhenomenaSinks)
        {
            sink->OnTsunamiNotification(x);
        }
    }

    //
    // Statistics
    //

    virtual void OnFrameRateUpdated(
        float immediateFps,
        float averageFps) override
    {
        for (auto sink : mStatisticsSinks)
        {
            sink->OnFrameRateUpdated(
                immediateFps,
                averageFps);
        }
    }

    virtual void OnUpdateToRenderRatioUpdated(
        float immediateURRatio)
    {
        for (auto sink : mStatisticsSinks)
        {
            sink->OnUpdateToRenderRatioUpdated(
                immediateURRatio);
        }
    }

    //
    // Generic
    //

    virtual void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
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
        for (auto sink : mGenericSinks)
        {
            sink->OnSawed(isMetal, size);
        }
    }

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
        {
            sink->OnPinToggled(isPinned, isUnderwater);
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
        for (auto sink : mGenericSinks)
        {
            sink->OnWaterTaken(waterTaken);
        }
    }

    virtual void OnWaterSplashed(float waterSplashed) override
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
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
        for (auto sink : mGenericSinks)
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
        for (auto sink : mGenericSinks)
        {
            sink->OnCustomProbe(
                name,
                value);
        }
    }

    //
    // Bombs
    //

    virtual void OnBombPlaced(
        BombId bombId,
        BombType bombType,
        bool isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
        {
            sink->OnBombPlaced(
                bombId,
                bombType,
                isUnderwater);
        }
    }

    virtual void OnBombRemoved(
        BombId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
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
        BombId bombId,
        std::optional<bool> isFast)
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
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
        BombId bombId,
        bool isContained)
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
        {
            sink->OnAntiMatterBombContained(
                bombId,
                isContained);
        }
    }

    virtual void OnAntiMatterBombPreImploding()
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
        {
            sink->OnAntiMatterBombPreImploding();
        }
    }

    virtual void OnAntiMatterBombImploding()
    {
        // No need to aggregate this one
        for (auto sink : mGenericSinks)
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
        //
        // Publish aggregations
        //

        for (auto * sink : mStructuralSinks)
        {
            for (auto const & entry : mStressEvents)
            {
                sink->OnStress(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mBreakEvents)
            {
                sink->OnBreak(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }
        }

        mStressEvents.clear();
        mBreakEvents.clear();

        for (auto * sink : mGenericSinks)
        {
            for (auto const & entry : mSpringRepairedEvents)
            {
                sink->OnSpringRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mTriangleRepairedEvents)
            {
                sink->OnTriangleRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
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

        mSpringRepairedEvents.clear();
        mTriangleRepairedEvents.clear();
        mLightFlickerEvents.clear();
        mBombExplosionEvents.clear();
        mRCBombPingEvents.clear();
        mTimerBombDefusedEvents.clear();
        mTimerBombDefusedEvents.clear();
    }

    void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * sink)
    {
        mLifecycleSinks.push_back(sink);
    }

    void RegisterStructuralEventHandler(IStructuralGameEventHandler * sink)
    {
        mStructuralSinks.push_back(sink);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * sink)
    {
        mWavePhenomenaSinks.push_back(sink);
    }

    void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * sink)
    {
        mStatisticsSinks.push_back(sink);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * sink)
    {
        mGenericSinks.push_back(sink);
    }

private:

    // The current events being aggregated
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mSpringRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mTriangleRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mStressEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mBreakEvents;
    unordered_tuple_map<std::tuple<DurationShortLongType, bool>, unsigned int> mLightFlickerEvents;
    unordered_tuple_map<std::tuple<BombType, bool>, unsigned int> mBombExplosionEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mRCBombPingEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mTimerBombDefusedEvents;

    // The registered sinks
    std::vector<ILifecycleGameEventHandler *> mLifecycleSinks;
    std::vector<IStructuralGameEventHandler *> mStructuralSinks;
    std::vector<IWavePhenomenaGameEventHandler *> mWavePhenomenaSinks;
    std::vector<IStatisticsGameEventHandler *> mStatisticsSinks;
    std::vector<IGenericGameEventHandler *> mGenericSinks;
};
