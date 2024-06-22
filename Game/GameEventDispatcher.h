/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IGameEventHandlers.h"

#include <GameCore/Log.h>
#include <GameCore/TupleKeys.h>

#include <algorithm>
#include <optional>
#include <vector>

/*
 * Dispatches events to multiple sinks, aggregating some events in the process.
 */
class GameEventDispatcher final
    : public ILifecycleGameEventHandler
    , public IStructuralGameEventHandler
    , public IWavePhenomenaGameEventHandler
    , public ICombustionGameEventHandler
    , public IStatisticsGameEventHandler
    , public IAtmosphereGameEventHandler
    , public IElectricalElementGameEventHandler
    , public INpcGameEventHandler
    , public IGenericGameEventHandler
{
public:

    GameEventDispatcher()
        : mStressEvents()
        , mBreakEvents()
        , mLampBrokenEvents()
        , mLampExplodedEvents()
        , mLampImplodedEvents()
        , mCombustionExplosionEvents()
        , mLightningHitEvents()
        , mLightFlickerEvents()
        , mSpringRepairedEvents()
        , mTriangleRepairedEvents()
        , mPinToggledEvents()
        , mWaterDisplacedEvents(0.0f)
        , mAirBubbleSurfacedEvents(0u)
        , mBombExplosionEvents()
        , mRCBombPingEvents()
        , mTimerBombDefusedEvents()
        , mWatertightDoorOpenedEvents()
        , mWatertightDoorClosedEvents()
        // Sinks
        , mLifecycleSinks()
        , mStructuralSinks()
        , mWavePhenomenaSinks()
        , mCombustionSinks()
        , mStatisticsSinks()
        , mAtmosphereSinks()
        , mElectricalElementSinks()
        , mNpcSinks()
        , mGenericSinks()
    {
    }

public:

    //
    // Lifecycle
    //

    void OnGameReset() override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnGameReset();
        }
    }

    void OnShipLoaded(
        ShipId id,
        ShipMetadata const & shipMetadata) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnShipLoaded(id, shipMetadata);
        }
    }

    void OnSinkingBegin(ShipId shipId) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnSinkingBegin(shipId);
        }
    }

    void OnSinkingEnd(ShipId shipId) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnSinkingEnd(shipId);
        }
    }

    void OnShipRepaired(ShipId shipId) override
    {
        for (auto sink : mLifecycleSinks)
        {
            sink->OnShipRepaired(shipId);
        }
    }

    //
    // Structural
    //

    void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mStressEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mBreakEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    void OnLampBroken(
        bool isUnderwater,
        unsigned int size) override
    {
        mLampBrokenEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnLampExploded(
        bool isUnderwater,
        unsigned int size) override
    {
        mLampExplodedEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnLampImploded(
        bool isUnderwater,
        unsigned int size) override
    {
        mLampImplodedEvents[std::make_tuple(isUnderwater)] += size;
    }

    //
    // Wave phenomena
    //

    void OnTsunami(float x) override
    {
        for (auto sink : mWavePhenomenaSinks)
        {
            sink->OnTsunami(x);
        }
    }

    void OnTsunamiNotification(float x) override
    {
        for (auto sink : mWavePhenomenaSinks)
        {
            sink->OnTsunamiNotification(x);
        }
    }

    //
    // Combustion
    //

    void OnPointCombustionBegin() override
    {
        for (auto sink : mCombustionSinks)
        {
            sink->OnPointCombustionBegin();
        }
    }

    void OnPointCombustionEnd() override
    {
        for (auto sink : mCombustionSinks)
        {
            sink->OnPointCombustionEnd();
        }
    }

    void OnCombustionSmothered() override
    {
        for (auto sink : mCombustionSinks)
        {
            sink->OnCombustionSmothered();
        }
    }

    void OnCombustionExplosion(
        bool isUnderwater,
        unsigned int size) override
    {
        mCombustionExplosionEvents[std::make_tuple(isUnderwater)] += size;
    }

    //
    // Statistics
    //

    void OnFrameRateUpdated(
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

    void OnCurrentUpdateDurationUpdated(float currentUpdateDuration) override
    {
        for (auto sink : mStatisticsSinks)
        {
            sink->OnCurrentUpdateDurationUpdated(currentUpdateDuration);
        }
    }

    void OnStaticPressureUpdated(
        float netForce,
        float complexity) override
    {
        for (auto sink : mStatisticsSinks)
        {
            sink->OnStaticPressureUpdated(
                netForce,
                complexity);
        }
    }

    //
    // Atmosphere
    //

    void OnStormBegin() override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnStormBegin();
        }
    }

    void OnStormEnd() override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnStormEnd();
        }
    }

    void OnWindSpeedUpdated(
        float const zeroSpeedMagnitude,
        float const baseSpeedMagnitude,
        float const baseAndStormSpeedMagnitude,
        float const preMaxSpeedMagnitude,
        float const maxSpeedMagnitude,
        vec2f const & windSpeed) override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnWindSpeedUpdated(
                zeroSpeedMagnitude,
                baseSpeedMagnitude,
                baseAndStormSpeedMagnitude,
                preMaxSpeedMagnitude,
                maxSpeedMagnitude,
                windSpeed);
        }
    }

    void OnRainUpdated(float const density) override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnRainUpdated(density);
        }
    }

    void OnThunder() override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnThunder();
        }
    }

    void OnLightning() override
    {
        for (auto sink : mAtmosphereSinks)
        {
            sink->OnLightning();
        }
    }

    void OnLightningHit(StructuralMaterial const & structuralMaterial) override
    {
        mLightningHitEvents[std::make_tuple(&structuralMaterial)] += 1;
    }

    //
    // Electrical elements
    //

    void OnLightFlicker(
        DurationShortLongType duration,
        bool isUnderwater,
        unsigned int size) override
    {
        mLightFlickerEvents[std::make_tuple(duration, isUnderwater)] += size;
    }

    void OnElectricalElementAnnouncementsBegin() override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnElectricalElementAnnouncementsBegin();
        }
    }

    void OnSwitchCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        SwitchType type,
        ElectricalState state,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnSwitchCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), "): State=", static_cast<bool>(state));

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnSwitchCreated(electricalElementId, instanceIndex, type, state, electricalMaterial, panelElementMetadata);
        }
    }

    void OnPowerProbeCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        PowerProbeType type,
        ElectricalState state,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnPowerProbeCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), "): State=", static_cast<bool>(state));

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnPowerProbeCreated(electricalElementId, instanceIndex, type, state, electricalMaterial, panelElementMetadata);
        }
    }

    void OnEngineControllerCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnEngineControllerCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), ")");

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineControllerCreated(electricalElementId, instanceIndex, electricalMaterial, panelElementMetadata);
        }
    }

    void OnEngineMonitorCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        float thrustMagnitude,
        float rpm,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnEngineMonitorCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), "): Thrust=", thrustMagnitude, " RPM=", rpm);

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineMonitorCreated(electricalElementId, instanceIndex, thrustMagnitude, rpm, electricalMaterial, panelElementMetadata);
        }
    }

    void OnWaterPumpCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        float normalizedForce,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnWaterPumpCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), ")");

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWaterPumpCreated(electricalElementId, instanceIndex, normalizedForce, electricalMaterial, panelElementMetadata);
        }
    }

    void OnWatertightDoorCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        bool isOpen,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
    {
        LogMessage("OnWatertightDoorCreated(EEID=", electricalElementId, " IID=", int(instanceIndex), ")");

        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWatertightDoorCreated(electricalElementId, instanceIndex, isOpen, electricalMaterial, panelElementMetadata);
        }
    }

    void OnElectricalElementAnnouncementsEnd() override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnElectricalElementAnnouncementsEnd();
        }
    }

    void OnSwitchEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnSwitchEnabled(electricalElementId, isEnabled);
        }
    }

    void OnSwitchToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnSwitchToggled(electricalElementId, newState);
        }
    }

    void OnPowerProbeToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnPowerProbeToggled(electricalElementId, newState);
        }
    }

    void OnEngineControllerEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineControllerEnabled(electricalElementId, isEnabled);
        }
    }

    void OnEngineControllerUpdated(
        ElectricalElementId electricalElementId,
        ElectricalMaterial const & electricalMaterial,
        float oldControllerValue,
        float newControllerValue) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineControllerUpdated(electricalElementId, electricalMaterial, oldControllerValue, newControllerValue);
        }
    }

    void OnEngineMonitorUpdated(
        ElectricalElementId electricalElementId,
        float thrustMagnitude,
        float rpm) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineMonitorUpdated(electricalElementId, thrustMagnitude, rpm);
        }
    }

    void OnShipSoundUpdated(
        ElectricalElementId electricalElementId,
        ElectricalMaterial const & electricalMaterial,
        bool isPlaying,
        bool isUnderwater) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnShipSoundUpdated(electricalElementId, electricalMaterial, isPlaying, isUnderwater);
        }
    }

    void OnWaterPumpEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWaterPumpEnabled(electricalElementId, isEnabled);
        }
    }

    void OnWaterPumpUpdated(
        ElectricalElementId electricalElementId,
        float normalizedForce) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWaterPumpUpdated(electricalElementId, normalizedForce);
        }
    }

    void OnWatertightDoorEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWatertightDoorEnabled(electricalElementId, isEnabled);
        }
    }

    void OnWatertightDoorUpdated(
        ElectricalElementId electricalElementId,
        bool isOpen) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWatertightDoorUpdated(electricalElementId, isOpen);
        }
    }

    //
    // NPC
    //

    void OnHumanNpcCountsUpdated(
        unsigned int insideShipCount,
        unsigned int outsideShipCount) override
    {
        for (auto sink : mNpcSinks)
        {
            sink->OnHumanNpcCountsUpdated(insideShipCount, outsideShipCount);
        }
    }

    //
    // Generic
    //

    void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnDestroy(structuralMaterial, isUnderwater, size);
        }
    }

    void OnSpringRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mSpringRepairedEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    void OnTriangleRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mTriangleRepairedEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    void OnSawed(
        bool isMetal,
        unsigned int size) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnSawed(isMetal, size);
        }
    }

    virtual void OnLaserCut(unsigned int size) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnLaserCut(size);
        }
    }

    void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override
    {
        mPinToggledEvents.emplace(isPinned, isUnderwater);
    }

    void OnWaterTaken(float waterTaken) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnWaterTaken(waterTaken);
        }
    }

    void OnWaterSplashed(float waterSplashed) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnWaterSplashed(waterSplashed);
        }
    }

    void OnWaterDisplaced(float waterDisplacedMagnitude) override
    {
        mWaterDisplacedEvents += waterDisplacedMagnitude;
    }

    void OnAirBubbleSurfaced(unsigned int size) override
    {
        mAirBubbleSurfacedEvents += size;
    }

    void OnWaterReaction(
        bool isUnderwater,
        unsigned int size) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnWaterReaction(isUnderwater, size);
        }
    }

    void OnWaterReactionExplosion(
        bool isUnderwater,
        unsigned int size) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnWaterReactionExplosion(isUnderwater, size);
        }
    }

    void OnSilenceStarted() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnSilenceStarted();
        }
    }

    void OnSilenceLifted() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnSilenceLifted();
        }
    }

    void OnPhysicsProbeReading(
        vec2f const & velocity,
        float temperature,
        float depth,
        float pressure) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnPhysicsProbeReading(
                velocity,
                temperature,
                depth,
                pressure);
        }
    }

    void OnCustomProbe(
        std::string const & name,
        float value) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnCustomProbe(
                name,
                value);
        }
    }

    void OnGadgetPlaced(
        GadgetId gadgetId,
        GadgetType gadgetType,
        bool isUnderwater) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnGadgetPlaced(
                gadgetId,
                gadgetType,
                isUnderwater);
        }
    }

    void OnGadgetRemoved(
        GadgetId gadgetId,
        GadgetType gadgetType,
        std::optional<bool> isUnderwater) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnGadgetRemoved(
                gadgetId,
                gadgetType,
                isUnderwater);
        }
    }

    void OnBombExplosion(
        GadgetType gadgetType,
        bool isUnderwater,
        unsigned int size) override
    {
        mBombExplosionEvents[std::make_tuple(gadgetType, isUnderwater)] += size;
    }

    void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override
    {
        mRCBombPingEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnTimerBombFuse(
        GadgetId gadgetId,
        std::optional<bool> isFast) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnTimerBombFuse(
                gadgetId,
                isFast);
        }
    }

    void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override
    {
        mTimerBombDefusedEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnAntiMatterBombContained(
        GadgetId gadgetId,
        bool isContained) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnAntiMatterBombContained(
                gadgetId,
                isContained);
        }
    }

    void OnAntiMatterBombPreImploding() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnAntiMatterBombPreImploding();
        }
    }

    void OnAntiMatterBombImploding() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnAntiMatterBombImploding();
        }
    }

    void OnWatertightDoorOpened(
        bool isUnderwater,
        unsigned int size) override
    {
        mWatertightDoorOpenedEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnWatertightDoorClosed(
        bool isUnderwater,
        unsigned int size) override
    {
        mWatertightDoorClosedEvents[std::make_tuple(isUnderwater)] += size;
    }

    void OnFishCountUpdated(size_t count) override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnFishCountUpdated(count);
        }
    }

    void OnPhysicsProbePanelOpened() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnPhysicsProbePanelOpened();
        }
    }

    void OnPhysicsProbePanelClosed() override
    {
        for (auto sink : mGenericSinks)
        {
            sink->OnPhysicsProbePanelClosed();
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

            for (auto const & entry : mLampBrokenEvents)
            {
                sink->OnLampBroken(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mLampExplodedEvents)
            {
                sink->OnLampExploded(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mLampImplodedEvents)
            {
                sink->OnLampImploded(std::get<0>(entry.first), entry.second);
            }
        }

        mStressEvents.clear();
        mBreakEvents.clear();
        mLampBrokenEvents.clear();
        mLampExplodedEvents.clear();
        mLampImplodedEvents.clear();

        for (auto * sink : mCombustionSinks)
        {
            for (auto const & entry : mCombustionExplosionEvents)
            {
                sink->OnCombustionExplosion(std::get<0>(entry.first), entry.second);
            }
        }

        mCombustionExplosionEvents.clear();

        for (auto * sink : mAtmosphereSinks)
        {
            for (auto const & entry : mLightningHitEvents)
            {
                sink->OnLightningHit(*(std::get<0>(entry.first)));
            }
        }

        mLightningHitEvents.clear();

        for (auto * sink : mElectricalElementSinks)
        {
            for (auto const & entry : mLightFlickerEvents)
            {
                sink->OnLightFlicker(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }
        }

        mLightFlickerEvents.clear();

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

            for (auto const & entry : mPinToggledEvents)
            {
                sink->OnPinToggled(std::get<0>(entry), std::get<1>(entry));
            }

            if (mWaterDisplacedEvents != 0.0f)
            {
                sink->OnWaterDisplaced(mWaterDisplacedEvents);
            }

            if (mAirBubbleSurfacedEvents > 0)
            {
                sink->OnAirBubbleSurfaced(mAirBubbleSurfacedEvents);
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

            for (auto const & entry : mWatertightDoorOpenedEvents)
            {
                sink->OnWatertightDoorOpened(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mWatertightDoorClosedEvents)
            {
                sink->OnWatertightDoorClosed(std::get<0>(entry.first), entry.second);
            }
        }

        mSpringRepairedEvents.clear();
        mTriangleRepairedEvents.clear();
        mPinToggledEvents.clear();
        mWaterDisplacedEvents = 0.0f;
        mAirBubbleSurfacedEvents = 0u;
        mBombExplosionEvents.clear();
        mRCBombPingEvents.clear();
        mTimerBombDefusedEvents.clear();
        mWatertightDoorOpenedEvents.clear();
        mWatertightDoorClosedEvents.clear();
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

    void RegisterCombustionEventHandler(ICombustionGameEventHandler * sink)
    {
        mCombustionSinks.push_back(sink);
    }

    void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * sink)
    {
        mStatisticsSinks.push_back(sink);
    }

    void RegisterAtmosphereEventHandler(IAtmosphereGameEventHandler * sink)
    {
        mAtmosphereSinks.push_back(sink);
    }

    void RegisterElectricalElementEventHandler(IElectricalElementGameEventHandler * sink)
    {
        mElectricalElementSinks.push_back(sink);
    }

    void RegisterNpcEventHandler(INpcGameEventHandler * sink)
    {
        mNpcSinks.push_back(sink);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * sink)
    {
        mGenericSinks.push_back(sink);
    }

private:

    // The current events being aggregated
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mStressEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mBreakEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampBrokenEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampExplodedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampImplodedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mCombustionExplosionEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *>, unsigned int> mLightningHitEvents;
    unordered_tuple_map<std::tuple<DurationShortLongType, bool>, unsigned int> mLightFlickerEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mSpringRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mTriangleRepairedEvents;
    unordered_tuple_set<std::tuple<bool, bool>> mPinToggledEvents;
    float mWaterDisplacedEvents;
    unsigned int mAirBubbleSurfacedEvents;
    unordered_tuple_map<std::tuple<GadgetType, bool>, unsigned int> mBombExplosionEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mRCBombPingEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mTimerBombDefusedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mWatertightDoorOpenedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mWatertightDoorClosedEvents;

    // The registered sinks
    std::vector<ILifecycleGameEventHandler *> mLifecycleSinks;
    std::vector<IStructuralGameEventHandler *> mStructuralSinks;
    std::vector<IWavePhenomenaGameEventHandler *> mWavePhenomenaSinks;
    std::vector<ICombustionGameEventHandler *> mCombustionSinks;
    std::vector<IStatisticsGameEventHandler *> mStatisticsSinks;
    std::vector<IAtmosphereGameEventHandler *> mAtmosphereSinks;
    std::vector<IElectricalElementGameEventHandler *> mElectricalElementSinks;
    std::vector<INpcGameEventHandler *> mNpcSinks;
    std::vector<IGenericGameEventHandler *> mGenericSinks;
};
