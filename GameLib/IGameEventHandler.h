/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "Material.h"

#include <optional>

/*
 * This interface defines the methods that game event handlers must implement.
 *
 * The methods are default-implemented to facilitate implementation of handlers that
 * only care about a subset of the events.
 */
class IGameEventHandler
{
public:

    virtual ~IGameEventHandler()
    {
    }

    virtual void OnGameReset()
    {
        // Default-implemented
    }

    virtual void OnShipLoaded(
        unsigned int /*id*/,
        std::string const & /*name*/,
        std::optional<std::string> const & /*author*/)
    {
        // Default-implemented
    }

    virtual void OnDestroy(
        Material const * /*material*/,
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

    virtual void OnPinToggled(
        bool /*isPinned*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnStress(
        Material const * /*material*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnBreak(
        Material const * /*material*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnSinkingBegin(ShipId /*shipId*/)
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

    virtual void OnWaterTaken(float /*waterTaken*/)
    {
        // Default-implemented
    }

    virtual void OnWaterSplashed(float /*waterSplashed*/)
    {
        // Default-implemented
    }

    virtual void OnCustomProbe(
        std::string const & /*name*/,
        float /*value*/)
    {
        // Default-implemented
    }

    virtual void OnFrameRateUpdated(
        float /*immediateFps*/,
        float /*averageFps*/)
    {
        // Default-implemented
    }

    virtual void OnUpdateToRenderRatioUpdated(
        float /*immediateURRatio*/)
    {
        // Default-implemented
    }

    //
    // Bombs
    //

    virtual void OnBombPlaced(
        ObjectId /*bombId*/,
        BombType /*bombType*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnBombRemoved(
        ObjectId /*bombId*/,
        BombType /*bombType*/,
        std::optional<bool> /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnBombExplosion(
        BombType /*bombType*/,
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
        ObjectId /*bombId*/,
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
        ObjectId /*bombId*/,
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
};
