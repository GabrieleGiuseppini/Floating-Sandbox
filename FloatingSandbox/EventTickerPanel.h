/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameController.h>
#include <Game/GameEventHandlers.h>

#include <wx/wx.h>

#include <memory>
#include <string>

class EventTickerPanel
    : public wxPanel
    , public ILifecycleGameEventHandler
    , public IStructuralGameEventHandler
    , public IWavePhenomenaGameEventHandler
    , public IGenericGameEventHandler
{
public:

    EventTickerPanel(wxWindow* parent);

	virtual ~EventTickerPanel();

    void Update();

public:

    //
    // Game events
    //

    void RegisterEventHandler(GameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterStructuralEventHandler(this);
        gameController.RegisterWavePhenomenaEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    virtual void OnGameReset() override;

    virtual void OnShipLoaded(
        unsigned int id,
        std::string const & name,
        std::optional<std::string> const & author) override;

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

    virtual void OnBombPlaced(
        BombId bombId,
        BombType bombType,
        bool isUnderwater) override;

    virtual void OnBombRemoved(
        BombId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        BombType bombType,
        bool isUnderwater,
        unsigned int size) override;

private:

    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void AppendFutureTickerText(std::string const & text);
    void Render(wxDC& dc);

private:

    wxFont mFont;
    std::unique_ptr<wxBitmap> mBufferedDCBitmap;

private:

    static constexpr size_t TickerTextSize = 1024u;
    static constexpr unsigned int TickerFontSize = 12;
    static constexpr unsigned int TickerCharStep = 1;

    // The current text in the ticker. The text scrolls to the left.
    // This string is always full and long TickerTextSize, eventually
    // padded with spaces.
    std::string mCurrentTickerText;

    // The future text that will go into the ticker. This text also scrolls to
    // the left. This string might be empty.
    std::string mFutureTickerText;

    // The fraction of a character we're currently scrolled by
    unsigned int mCurrentCharStep;
};
