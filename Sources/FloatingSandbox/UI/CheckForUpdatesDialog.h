/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-06-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../UpdateChecker.h"

#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/wx.h>

#include <memory>

class CheckForUpdatesDialog : public wxDialog
{
public:

    CheckForUpdatesDialog(
        wxWindow * parent);

    virtual ~CheckForUpdatesDialog();

    std::optional<UpdateChecker::Outcome> GetHasVersionOutcome() const
    {
        return mHasVersionOutcome;
    }

private:

    void OnCheckCompletionTimer(wxTimerEvent & event);

    void ShowNoUpdateMessage(wxString const & message);

private:

    std::unique_ptr<wxTimer> mCheckCompletionTimer;

    wxBoxSizer * mPanelSizer;

    wxPanel * mCheckingPanel;
    wxGauge * mCheckingGauge;

    wxPanel * mNoUpdatePanel;
    wxStaticText * mNoUpdateMessage;

private:

    std::unique_ptr<UpdateChecker> mUpdateChecker;

    std::optional<UpdateChecker::Outcome> mHasVersionOutcome;
};
