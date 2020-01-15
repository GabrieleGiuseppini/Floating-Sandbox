/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <UIControls/SliderControl.h>

#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <functional>
#include <memory>

class PreferencesDialog : public wxDialog
{
public:

    PreferencesDialog(
        wxWindow * parent,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
		std::function<void()> onChangeCallback);

    virtual ~PreferencesDialog();

    void Open();

private:

    void OnScreenshotDirPickerChanged(wxCommandEvent & event);
    void OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnCheckForUpdatesAtStartupCheckBoxClicked(wxCommandEvent & event);
    void OnSaveSettingsOnExitCheckBoxClicked(wxCommandEvent & event);
    void OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnAutoShowSwitchboardCheckBoxClicked(wxCommandEvent & event);
    void OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnZoomIncrementSpinCtrl(wxSpinEvent & event);
    void OnPanIncrementSpinCtrl(wxSpinEvent & event);
	void OnShowStatusTextCheckBoxClicked(wxCommandEvent & event);
	void OnShowExtendedStatusTextCheckBoxClicked(wxCommandEvent & event);

    void OnGlobalMuteCheckBoxClicked(wxCommandEvent & event);
    void OnPlayBackgroundMusicCheckBoxClicked(wxCommandEvent & event);
    void OnPlaySinkingMusicCheckBoxClicked(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);

private:

    void PopulateGamePanel(wxPanel * panel);
    void PopulateMusicPanel(wxPanel * panel);

    void ReadSettings();

    static float ZoomIncrementSpinToZoomIncrement(int spinPosition);
    static int ZoomIncrementToZoomIncrementSpin(float zoomIncrement);

    static float PanIncrementSpinToPanIncrement(int spinPosition);
    static int PanIncrementToPanIncrementSpin(float panIncrement);

    void ReconcileSoundSettings();

private:

    // Game panel
    wxDirPickerCtrl * mScreenshotDirPickerCtrl;
    wxCheckBox * mShowTipOnStartupCheckBox;
    wxCheckBox * mCheckForUpdatesAtStartupCheckBox;
    wxCheckBox * mSaveSettingsOnExitCheckBox;
    wxCheckBox * mShowShipDescriptionAtShipLoadCheckBox;
    wxCheckBox * mAutoShowSwitchboardCheckBox;
    wxCheckBox * mShowTsunamiNotificationsCheckBox;
    wxSpinCtrl * mZoomIncrementSpinCtrl;
    wxSpinCtrl * mPanIncrementSpinCtrl;
	wxCheckBox * mShowStatusTextCheckBox;
	wxCheckBox * mShowExtendedStatusTextCheckBox;

    // Global Sound and Music
    wxCheckBox * mGlobalMuteCheckBox;
    SliderControl<float> * mBackgroundMusicVolumeSlider;
    wxCheckBox * mPlayBackgroundMusicCheckBox;
    SliderControl<float> * mSinkingMusicVolumeSlider;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    // Buttons
    wxButton * mOkButton;

private:

    wxWindow * const mParent;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
	std::function<void()> mOnChangeCallback;
};
