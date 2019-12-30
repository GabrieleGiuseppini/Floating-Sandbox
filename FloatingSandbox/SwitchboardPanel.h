/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UIControls/ShipAutomaticSwitchControl.h>
#include <UIControls/ShipInteractiveSwitchControl.h>

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>
#include <Game/ResourceLoader.h>

#include <wx/bitmap.h>
#include <wx/sizer.h>
#include <wx/wx.h>

#include <memory>
#include <string>
#include <unordered_map>

class SwitchboardPanel
    : public wxPanel
    , public ILifecycleGameEventHandler
    , public IElectricalElementGameEventHandler
{
public:

    static std::unique_ptr<SwitchboardPanel> Create(
        wxWindow * parent,
        std::shared_ptr<IGameController> gameController,
        ResourceLoader & resourceLoader);

    virtual ~SwitchboardPanel();

public:

    //
    // Game event handlers
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterElectricalElementEventHandler(this);
    }

    virtual void OnGameReset() override;

    virtual void OnSwitchCreated(
        SwitchId switchId,
        std::string const & name,
        SwitchType type,
        SwitchState state) override;

    virtual void OnSwitchEnabled(
        SwitchId switchId,
        bool isEnabled) override;

    virtual void OnSwitchToggled(
        SwitchId switchId,
        SwitchState newState) override;

private:

    SwitchboardPanel(
        wxWindow * parent,
        std::shared_ptr<IGameController> gameController,
        ResourceLoader & resourceLoader);

    void MakePanel();

private:

    wxPanel * mSwitchPanel;
    wxFlexGridSizer * mSwitchSizer;

    struct SwitchInfo
    {
        SwitchType Type;
        wxPanel * SwitchControl;
        bool IsEnabled;

        SwitchInfo(
            SwitchType type,
            wxPanel * switchControl)
            : Type(type)
            , SwitchControl(switchControl)
            , IsEnabled(true)
        {}
    };

    std::unordered_map<SwitchId, SwitchInfo> mSwitchMap;

private:

    std::shared_ptr<IGameController> const mGameController;

    //
    // Bitmaps
    //

    wxBitmap mInteractiveSwitchOnEnabledBitmap;
    wxBitmap mInteractiveSwitchOffEnabledBitmap;
    wxBitmap mInteractiveSwitchOnDisabledBitmap;
    wxBitmap mInteractiveSwitchOffDisabledBitmap;
};
