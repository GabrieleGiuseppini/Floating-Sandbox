/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISimulationEventHandlers.h"

#include <Core/Log.h>
#include <Core/TupleKeys.h>

#include <algorithm>
#include <optional>
#include <vector>

/*
 * Dispatches events to multiple sinks, aggregating some events in the process.
 */
class SimulationEventDispatcher final
    : public IStructuralShipEventHandler
    , public IGenericShipEventHandler
    , public IWavePhenomenaEventHandler
    , public ICombustionEventHandler
    , public ISimulationStatisticsEventHandler
    , public IAtmosphereEventHandler
    , public IElectricalElementEventHandler
    , public INpcEventHandler
{
public:

    SimulationEventDispatcher()
        : mStressEvents()
        , mImpactEvents()
        , mBreakEvents()
        , mLampBrokenEvents()
        , mLampExplodedEvents()
        , mLampImplodedEvents()
        , mCombustionExplosionEvents()
        , mLightningHitEvents()
        , mLightFlickerEvents()
        , mSpringRepairedEvents()
        , mTriangleRepairedEvents()
        , mLaserCutEvents(0)
        , mWaterDisplacedEvents(0.0f)
        , mAirBubbleSurfacedEvents(0u)
        , mBombExplosionEvents()
        , mRCBombPingEvents()
        , mTimerBombDefusedEvents()
        , mWatertightDoorOpenedEvents()
        , mWatertightDoorClosedEvents()
        , mLastNpcCountsUpdated()
        , mLastHumanNpcCountsUpdated()
        // Sinks
        , mStructuralShipSinks()
        , mGenericShipSinks()
        , mWavePhenomenaSinks()
        , mCombustionSinks()
        , mSimulationStatisticsSinks()
        , mAtmosphereSinks()
        , mElectricalElementSinks()
        , mNpcSinks()
    {
    }

    SimulationEventDispatcher(SimulationEventDispatcher &) = delete;
    SimulationEventDispatcher(SimulationEventDispatcher &&) = delete;
    SimulationEventDispatcher operator=(SimulationEventDispatcher &) = delete;
    SimulationEventDispatcher operator=(SimulationEventDispatcher &&) = delete;

public:

    //
    // Structural Ship
    //

    void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mStressEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    virtual void OnImpact(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        float kineticEnergy)
    {
        mImpactEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += kineticEnergy;
    }

    void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        mBreakEvents[std::make_tuple(&structuralMaterial, isUnderwater)] += size;
    }

    void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override
    {
        for (auto sink : mStructuralShipSinks)
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
        for (auto sink : mStructuralShipSinks)
        {
            sink->OnSawed(isMetal, size);
        }
    }

    virtual void OnLaserCut(unsigned int size) override
    {
        mLaserCutEvents += size;
    }

    //
    // Generic Ship
    //

    void OnSinkingBegin(ShipId shipId) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnSinkingBegin(shipId);
        }
    }

    void OnSinkingEnd(ShipId shipId) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnSinkingEnd(shipId);
        }
    }

    void OnShipRepaired(ShipId shipId) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnShipRepaired(shipId);
        }
    }

    void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnPinToggled(isPinned, isUnderwater);
        }
    }

    void OnWaterTaken(float waterTaken) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnWaterTaken(waterTaken);
        }
    }

    void OnWaterSplashed(float waterSplashed) override
    {
        for (auto sink : mGenericShipSinks)
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
        for (auto sink : mGenericShipSinks)
        {
            sink->OnWaterReaction(isUnderwater, size);
        }
    }

    void OnWaterReactionExplosion(
        bool isUnderwater,
        unsigned int size) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnWaterReactionExplosion(isUnderwater, size);
        }
    }

    void OnPhysicsProbeReading(
        vec2f const & velocity,
        float temperature,
        float depth,
        float pressure) override
    {
        for (auto sink : mGenericShipSinks)
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
        for (auto sink : mGenericShipSinks)
        {
            sink->OnCustomProbe(
                name,
                value);
        }
    }

    void OnGadgetPlaced(
        GlobalGadgetId gadgetId,
        GadgetType gadgetType,
        bool isUnderwater) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnGadgetPlaced(
                gadgetId,
                gadgetType,
                isUnderwater);
        }
    }

    void OnGadgetRemoved(
        GlobalGadgetId gadgetId,
        GadgetType gadgetType,
        std::optional<bool> isUnderwater) override
    {
        for (auto sink : mGenericShipSinks)
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
        GlobalGadgetId gadgetId,
        std::optional<bool> isFast) override
    {
        for (auto sink : mGenericShipSinks)
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
        GlobalGadgetId gadgetId,
        bool isContained) override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnAntiMatterBombContained(
                gadgetId,
                isContained);
        }
    }

    void OnAntiMatterBombPreImploding() override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnAntiMatterBombPreImploding();
        }
    }

    void OnAntiMatterBombImploding() override
    {
        for (auto sink : mGenericShipSinks)
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
        for (auto sink : mGenericShipSinks)
        {
            sink->OnFishCountUpdated(count);
        }
    }

    void OnPhysicsProbePanelOpened() override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnPhysicsProbePanelOpened();
        }
    }

    void OnPhysicsProbePanelClosed() override
    {
        for (auto sink : mGenericShipSinks)
        {
            sink->OnPhysicsProbePanelClosed();
        }
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
    // Simulation Statistics
    //

    void OnStaticPressureUpdated(
        float netForce,
        float complexity) override
    {
        for (auto sink : mSimulationStatisticsSinks)
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
    // Electrical Element
    //

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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnSwitchEnabled(electricalElementId, isEnabled);
        }
    }

    void OnSwitchToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnSwitchToggled(electricalElementId, newState);
        }
    }

    void OnPowerProbeToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnPowerProbeToggled(electricalElementId, newState);
        }
    }

    void OnEngineControllerEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineControllerEnabled(electricalElementId, isEnabled);
        }
    }

    void OnEngineControllerUpdated(
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
        float thrustMagnitude,
        float rpm) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnEngineMonitorUpdated(electricalElementId, thrustMagnitude, rpm);
        }
    }

    void OnShipSoundUpdated(
        GlobalElectricalElementId electricalElementId,
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
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWaterPumpEnabled(electricalElementId, isEnabled);
        }
    }

    void OnWaterPumpUpdated(
        GlobalElectricalElementId electricalElementId,
        float normalizedForce) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWaterPumpUpdated(electricalElementId, normalizedForce);
        }
    }

    void OnWatertightDoorEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override
    {
        for (auto sink : mElectricalElementSinks)
        {
            sink->OnWatertightDoorEnabled(electricalElementId, isEnabled);
        }
    }

    void OnWatertightDoorUpdated(
        GlobalElectricalElementId electricalElementId,
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

    void OnNpcSelectionChanged(
        std::optional<NpcId> selectedNpc) override
    {
        for (auto sink : mNpcSinks)
        {
            sink->OnNpcSelectionChanged(selectedNpc);
        }
    }

    void OnNpcCountsUpdated(
        size_t totalNpcCount) override
    {
        mLastNpcCountsUpdated = totalNpcCount;
    }

    void OnHumanNpcCountsUpdated(
        size_t insideShipCount,
        size_t outsideShipCount) override
    {
        mLastHumanNpcCountsUpdated = { insideShipCount, outsideShipCount };
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

        for (auto * sink : mStructuralShipSinks)
        {
            for (auto const & entry : mStressEvents)
            {
                sink->OnStress(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mImpactEvents)
            {
                sink->OnImpact(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mBreakEvents)
            {
                sink->OnBreak(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mSpringRepairedEvents)
            {
                sink->OnSpringRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mTriangleRepairedEvents)
            {
                sink->OnTriangleRepaired(*(std::get<0>(entry.first)), std::get<1>(entry.first), entry.second);
            }

            if (mLaserCutEvents > 0)
            {
                sink->OnLaserCut(mLaserCutEvents);
            }
        }

        mStressEvents.clear();
        mImpactEvents.clear();
        mBreakEvents.clear();
        mSpringRepairedEvents.clear();
        mTriangleRepairedEvents.clear();
        mLaserCutEvents = 0;

        for (auto * sink : mGenericShipSinks)
        {
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

        mWaterDisplacedEvents = 0.0f;
        mAirBubbleSurfacedEvents = 0u;
        mBombExplosionEvents.clear();
        mRCBombPingEvents.clear();
        mTimerBombDefusedEvents.clear();
        mWatertightDoorOpenedEvents.clear();
        mWatertightDoorClosedEvents.clear();

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

            for (auto const & entry : mLightFlickerEvents)
            {
                sink->OnLightFlicker(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }
        }

        mLampBrokenEvents.clear();
        mLampExplodedEvents.clear();
        mLampImplodedEvents.clear();
        mLightFlickerEvents.clear();

        for (auto * sink : mNpcSinks)
        {
            if (mLastNpcCountsUpdated.has_value())
            {
                sink->OnNpcCountsUpdated(*mLastNpcCountsUpdated);
            }

            if (mLastHumanNpcCountsUpdated.has_value())
            {
                sink->OnHumanNpcCountsUpdated(
                    std::get<0>(*mLastHumanNpcCountsUpdated),
                    std::get<1>(*mLastHumanNpcCountsUpdated));
            }
        }

        mLastNpcCountsUpdated.reset();
        mLastHumanNpcCountsUpdated.reset();
    }

    void RegisterStructuralShipEventHandler(IStructuralShipEventHandler * sink)
    {
        mStructuralShipSinks.push_back(sink);
    }

    void RegisterGenericShipEventHandler(IGenericShipEventHandler * sink)
    {
        mGenericShipSinks.push_back(sink);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaEventHandler * sink)
    {
        mWavePhenomenaSinks.push_back(sink);
    }

    void RegisterCombustionEventHandler(ICombustionEventHandler * sink)
    {
        mCombustionSinks.push_back(sink);
    }

    void RegisterSimulationStatisticsEventHandler(ISimulationStatisticsEventHandler * sink)
    {
        mSimulationStatisticsSinks.push_back(sink);
    }

    void RegisterAtmosphereEventHandler(IAtmosphereEventHandler * sink)
    {
        mAtmosphereSinks.push_back(sink);
    }

    void RegisterElectricalElementEventHandler(IElectricalElementEventHandler * sink)
    {
        mElectricalElementSinks.push_back(sink);
    }

    void RegisterNpcEventHandler(INpcEventHandler * sink)
    {
        mNpcSinks.push_back(sink);
    }

private:

    // The current events being aggregated
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mStressEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, float> mImpactEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mBreakEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampBrokenEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampExplodedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mLampImplodedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mCombustionExplosionEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *>, unsigned int> mLightningHitEvents;
    unordered_tuple_map<std::tuple<DurationShortLongType, bool>, unsigned int> mLightFlickerEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mSpringRepairedEvents;
    unordered_tuple_map<std::tuple<StructuralMaterial const *, bool>, unsigned int> mTriangleRepairedEvents;
    unsigned int mLaserCutEvents;
    float mWaterDisplacedEvents;
    unsigned int mAirBubbleSurfacedEvents;
    unordered_tuple_map<std::tuple<GadgetType, bool>, unsigned int> mBombExplosionEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mRCBombPingEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mTimerBombDefusedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mWatertightDoorOpenedEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mWatertightDoorClosedEvents;
    std::optional<size_t> mLastNpcCountsUpdated;
    std::optional<std::tuple<size_t, size_t>> mLastHumanNpcCountsUpdated;

    // The registered sinks

    std::vector<IStructuralShipEventHandler *> mStructuralShipSinks;
    std::vector<IGenericShipEventHandler *> mGenericShipSinks;
    std::vector<IWavePhenomenaEventHandler *> mWavePhenomenaSinks;
    std::vector<ICombustionEventHandler *> mCombustionSinks;
    std::vector<ISimulationStatisticsEventHandler *> mSimulationStatisticsSinks;
    std::vector<IAtmosphereEventHandler *> mAtmosphereSinks;
    std::vector<IElectricalElementEventHandler *> mElectricalElementSinks;
    std::vector<INpcEventHandler *> mNpcSinks;
};
