/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <wx/filepicker.h>
#include <wx/wx.h>

#include <memory>

class CheckForUpdatesDialog : public wxDialog
{
public:

    CheckForUpdatesDialog(
        wxWindow * parent,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager);

    virtual ~CheckForUpdatesDialog();

    void Open();

private:

    void OnScreenshotDirPickerChanged(wxCommandEvent & event);
    void OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);

private:

    void PopulateMainPanel(wxPanel * panel);

    void ReadSettings();

private:

    // Main panel
    wxDirPickerCtrl * mScreenshotDirPickerCtrl;
    wxCheckBox * mShowTipOnStartupCheckBox;
    wxCheckBox * mShowShipDescriptionAtShipLoadCheckBox;
    wxCheckBox * mShowTsunamiNotificationsCheckBox;

    // Buttons
    wxButton * mOkButton;

private:

    wxWindow * const mParent;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
};
