/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElectricalPanel.h"
#include "Materials.h"

#include <Core/GameTypes.h>

#include <optional>

/*
 * These interfaces define the methods that game event handlers must implement.
 *
 * The methods are default-implemented to facilitate implementation of handlers that
 * only care about a subset of the events.
 */

struct IStructuralShipEventHandler
{
    virtual void OnStress(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnImpact(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        float /*kineticEnergy*/) // Dissipated; J
    {
        // Default-implemented
    }

    virtual void OnBreak(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnDestroy(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnSpringRepaired(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnTriangleRepaired(
        StructuralMaterial const & /*structuralMaterial*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnSawed(
        bool /*isMetal*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnLaserCut(
        unsigned int /*size*/)
    {
        // Default-implemented
    }
};

struct IGenericShipEventHandler
{
    virtual void OnSinkingBegin(ShipId /*shipId*/)
    {
        // Default-implemented
    }

    virtual void OnSinkingEnd(ShipId /*shipId*/)
    {
        // Default-implemented
    }

    virtual void OnShipRepaired(ShipId /*shipId*/)
    {
        // Default-implemented
    }

    virtual void OnPinToggled(
        bool /*isPinned*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnWaterTaken(float /*waterTaken*/)
    {
        // Default-implemented
    }

    virtual void OnWaterSplashed(float /*waterSplashed*/)
    {
        // Default-implemented
    }

    virtual void OnWaterDisplaced(float /*waterDisplacedMagnitude*/)
    {
        // Default-implemented
    }

    virtual void OnAirBubbleSurfaced(unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnWaterReaction(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnWaterReactionExplosion(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnPhysicsProbeReading(
        vec2f const & /*velocity*/,
        float /*temperature*/,
        float /*depth*/,
        float /*pressure*/)
    {
        // Default-implemented
    }

    virtual void OnCustomProbe(
        std::string const & /*name*/,
        float /*value*/)
    {
        // Default-implemented
    }

    //
    // Gadgets
    //

    virtual void OnGadgetPlaced(
        GlobalGadgetId /*gadgetId*/,
        GadgetType /*gadgetType*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnGadgetRemoved(
        GlobalGadgetId /*gadgetId*/,
        GadgetType /*gadgetType*/,
        std::optional<bool> /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnBombExplosion(
        GadgetType /*gadgetType*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnRCBombPing(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombFuse(
        GlobalGadgetId /*gadgetId*/,
        std::optional<bool> /*isFast*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombDefused(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnAntiMatterBombContained(
        GlobalGadgetId /*gadgetId*/,
        bool /*isContained*/)
    {
        // Default-implemented
    }

    virtual void OnAntiMatterBombPreImploding()
    {
        // Default-implemented
    }

    virtual void OnAntiMatterBombImploding()
    {
        // Default-implemented
    }

    // Misc

    virtual void OnWatertightDoorOpened(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnWatertightDoorClosed(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnFishCountUpdated(size_t /*count*/)
    {
        // Default-implemented
    }

    virtual void OnPhysicsProbePanelOpened()
    {
        // Default-implemented
    }

    virtual void OnPhysicsProbePanelClosed()
    {
        // Default-implemented
    }
};

struct IWavePhenomenaEventHandler
{
    virtual void OnTsunami(float /*x*/)
    {
        // Default-implemented
    }
};

struct ICombustionEventHandler
{
    virtual void OnPointCombustionBegin()
    {
        // Default-implemented
    }

    virtual void OnPointCombustionEnd()
    {
        // Default-implemented
    }

    virtual void OnCombustionSmothered()
    {
        // Default-implemented
    }

    virtual void OnCombustionExplosion(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }
};

struct ISimulationStatisticsEventHandler
{
    virtual void OnStaticPressureUpdated(
        float /*netForce*/,
        float /*complexity*/)
    {
        // Default-implemented
    }
};

struct IAtmosphereEventHandler
{
    virtual void OnStormBegin()
    {
        // Default-implemented
    }

    virtual void OnStormEnd()
    {
        // Default-implemented
    }

    virtual void OnWindSpeedUpdated(
        float const /*zeroSpeedMagnitude*/,
        float const /*baseSpeedMagnitude*/,
        float const /*baseAndStormSpeedMagnitude*/,
        float const /*preMaxSpeedMagnitude*/,
        float const /*maxSpeedMagnitude*/,
        vec2f const & /*windSpeed*/)
    {
        // Default-implemented
    }

    virtual void OnRainUpdated(float const /*density*/)
    {
        // Default-implemented
    }

    virtual void OnThunder()
    {
        // Default-implemented
    }

    virtual void OnLightning()
    {
        // Default-implemented
    }

    virtual void OnLightningHit(StructuralMaterial const & /*structuralMaterial*/)
    {
        // Default-implemented
    }
};

struct IElectricalElementEventHandler
{
    virtual void OnLampBroken(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnLampExploded(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnLampImploded(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnLightFlicker(
        DurationShortLongType /*duration*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    //
    // Announcements
    //

    virtual void OnElectricalElementAnnouncementsBegin()
    {
        // Default-implemented
    }

    virtual void OnSwitchCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        SwitchType /*type*/,
        ElectricalState /*state*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnPowerProbeCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        PowerProbeType /*type*/,
        ElectricalState /*state*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnEngineControllerCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnEngineMonitorCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        float /*thrustMagnitude*/,
        float /*rpm*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnWaterPumpCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        float /*normalizedForce*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnWatertightDoorCreated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalElementInstanceIndex /*instanceIndex*/,
        bool /*isOpen*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        std::optional<ElectricalPanel::ElementMetadata> const & /*panelElementMetadata*/)
    {
        // Default-implemented
    }

    virtual void OnElectricalElementAnnouncementsEnd()
    {
        // Default-implemented
    }

    //
    // State changes
    //

    virtual void OnSwitchEnabled(
        GlobalElectricalElementId /*electricalElementId*/,
        bool /*isEnabled*/)
    {
        // Default-implemented
    }

    virtual void OnSwitchToggled(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalState /*newState*/)
    {
        // Default-implemented
    }

    virtual void OnPowerProbeToggled(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalState /*newState*/)
    {
        // Default-implemented
    }

    virtual void OnEngineControllerEnabled(
        GlobalElectricalElementId /*electricalElementId*/,
        bool /*isEnabled*/)
    {
        // Default-implemented
    }

    virtual void OnEngineControllerUpdated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        float /*oldControllerValue*/,
        float /*newControllerValue*/)
    {
        // Default-implemented
    }

    virtual void OnEngineMonitorUpdated(
        GlobalElectricalElementId /*electricalElementId*/,
        float /*thrustMagnitude*/,
        float /*rpm*/)
    {
        // Default-implemented
    }

    virtual void OnShipSoundUpdated(
        GlobalElectricalElementId /*electricalElementId*/,
        ElectricalMaterial const & /*electricalMaterial*/,
        bool /*isPlaying*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnWaterPumpEnabled(
        GlobalElectricalElementId /*electricalElementId*/,
        bool /*isEnabled*/)
    {
        // Default-implemented
    }

    virtual void OnWaterPumpUpdated(
        GlobalElectricalElementId /*electricalElementId*/,
        float /*normalizedForce*/)
    {
        // Default-implemented
    }

    virtual void OnWatertightDoorEnabled(
        GlobalElectricalElementId /*electricalElementId*/,
        bool /*isEnabled*/)
    {
        // Default-implemented
    }

    virtual void OnWatertightDoorUpdated(
        GlobalElectricalElementId /*electricalElementId*/,
        bool /*isOpen*/)
    {
        // Default-implemented
    }
};

struct INpcEventHandler
{
    virtual void OnNpcSelectionChanged(
        std::optional<NpcId> /*selectedNpc*/)
    {
        // Default-implemented
    }

    virtual void OnNpcCountsUpdated(
        size_t /*totalNpcCount*/)
    {
        // Default-implemented
    }

    virtual void OnHumanNpcCountsUpdated(
        size_t /*insideShipCount*/,
        size_t /*outsideShipCount*/)
    {
        // Default-implemented
    }
};
