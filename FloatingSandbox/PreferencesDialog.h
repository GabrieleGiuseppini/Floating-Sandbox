/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <memory>

class PreferencesDialog : public wxDialog
{
public:

    PreferencesDialog(
        wxWindow * parent,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager);

    virtual ~PreferencesDialog();

    void Open();

private:

    void OnScreenshotDirPickerChanged(wxCommandEvent & event);
    void OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnCheckForUpdatesAtStartupCheckBoxClicked(wxCommandEvent & event);
    void OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnZoomIncrementSpinCtrl(wxSpinEvent & event);

    void OnOkButton(wxCommandEvent & event);

private:

    void PopulateMainPanel(wxPanel * panel);

    void ReadSettings();

    static float ZoomIncrementSpinToZoomIncrement(int spinPosition);
    static int ZoomIncrementToZoomIncrementSpin(float zoomIncrement);

private:

    // Main panel
    wxDirPickerCtrl * mScreenshotDirPickerCtrl;
    wxCheckBox * mShowTipOnStartupCheckBox;
    wxCheckBox * mCheckForUpdatesAtStartupCheckBox;
    wxCheckBox * mShowShipDescriptionAtShipLoadCheckBox;
    wxCheckBox * mShowTsunamiNotificationsCheckBox;
    wxSpinCtrl * mZoomIncrementSpinCtrl;

    // Buttons
    wxButton * mOkButton;

private:

    wxWindow * const mParent;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
};
