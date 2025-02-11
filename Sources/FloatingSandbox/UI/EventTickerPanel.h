/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UILib/UnFocusablePanel.h>

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>

#include <Simulation/ISimulationEventHandlers.h>

#include <wx/wx.h>

#include <memory>
#include <string>

class EventTickerPanel final
    : public UnFocusablePanel
    , public IGenericShipEventHandler
    , public IStructuralShipEventHandler
    , public IWavePhenomenaEventHandler
    , public IElectricalElementEventHandler
    , public IGameEventHandler
{
public:

    EventTickerPanel(wxWindow* parent);

    void UpdateSimulation();

public:

    //
    // Game events
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterGenericShipEventHandler(this);
        gameController.RegisterStructuralShipEventHandler(this);
        gameController.RegisterWavePhenomenaEventHandler(this);
        gameController.RegisterElectricalElementEventHandler(this);
        gameController.RegisterGameEventHandler(this);
    }

    virtual void OnGameReset() override;

    virtual void OnShipLoaded(
        unsigned int id,
        ShipMetadata const & author) override;

    virtual void OnSinkingBegin(ShipId shipId) override;

    virtual void OnSinkingEnd(ShipId shipId) override;

    virtual void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTsunami(float x) override;

    virtual void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSpringRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTriangleRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSwitchEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnSwitchToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnPowerProbeToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnGadgetPlaced(
        GlobalGadgetId gadgetId,
        GadgetType gadgetType,
        bool isUnderwater) override;

    virtual void OnGadgetRemoved(
        GlobalGadgetId gadgetId,
        GadgetType gadgetType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        GadgetType gadgetType,
        bool isUnderwater,
        unsigned int size) override;

private:

    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void AppendFutureTickerText(std::string const & text);
    void Render(wxDC& dc);

private:

    // The current text in the ticker. The text scrolls to the left.
    // This string is always full and long TickerTextSize, eventually
    // padded with spaces.
    std::string mCurrentTickerText;

    // The future text that will go into the ticker. This text also scrolls to
    // the left. This string might be empty.
    std::string mFutureTickerText;

    // The size of a character
    wxSize mCharSize;

    // The fraction of the character width that we're currently scrolled by
    int mCurrentCharWidthStep;
};
