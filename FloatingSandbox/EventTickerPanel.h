/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>

#include <wx/wx.h>

#include <memory>
#include <string>

class EventTickerPanel
    : public wxPanel
    , public ILifecycleGameEventHandler
    , public IStructuralGameEventHandler
    , public IWavePhenomenaGameEventHandler
    , public IElectricalElementGameEventHandler
    , public IGenericGameEventHandler
{
public:

    EventTickerPanel(wxWindow* parent);

	virtual ~EventTickerPanel();

    void UpdateSimulation();

public:

    //
    // Game events
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterStructuralEventHandler(this);
        gameController.RegisterWavePhenomenaEventHandler(this);
        gameController.RegisterElectricalElementEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
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
        ElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnSwitchToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnPowerProbeToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnGadgetPlaced(
        GadgetId gadgetId,
        GadgetType gadgetType,
        bool isUnderwater) override;

    virtual void OnGadgetRemoved(
        GadgetId gadgetId,
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

    // The width of a character
    unsigned int mCharWidth;

    // The fraction of the character width that we're currently scrolled by
    unsigned int mCurrentCharWidthStep;
};
