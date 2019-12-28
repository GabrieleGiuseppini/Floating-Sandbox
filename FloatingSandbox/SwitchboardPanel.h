/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>

#include <wx/sizer.h>
#include <wx/wx.h>

#include <memory>
#include <string>

class SwitchboardPanel
    : public wxPanel
    , public ILifecycleGameEventHandler
    , public IElectricalElementGameEventHandler
{
public:

    SwitchboardPanel(wxWindow* parent);

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

};
