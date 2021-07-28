/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "LocalizationManager.h"
#include "UIPreferencesManager.h"

#include <UIControls/SliderControl.h>

#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/listbox.h>
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
    void OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnZoomIncrementSpinCtrl(wxSpinEvent & event);
    void OnPanIncrementSpinCtrl(wxSpinEvent & event);
    void OnShowStatusTextCheckBoxClicked(wxCommandEvent & event);
    void OnShowExtendedStatusTextCheckBoxClicked(wxCommandEvent & event);
    void OnLanguagesListBoxSelected(wxCommandEvent & event);

    void OnReloadLastLoadedShipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnAutoZoomAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnAutoShowSwitchboardCheckBoxClicked(wxCommandEvent & event);
    void OnShowElectricalNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnAutoTexturizationModeRadioButtonClick(wxCommandEvent & event);
    void OnForceSharedAutoTexturizationSettingsOntoShipCheckBoxClicked(wxCommandEvent & event);

    void OnGlobalMuteCheckBoxClicked(wxCommandEvent & event);
    void OnPlayBackgroundMusicCheckBoxClicked(wxCommandEvent & event);
    void OnPlaySinkingMusicCheckBoxClicked(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);

private:

    void PopulateGamePanel(wxPanel * panel);
    void PopulateShipPanel(wxPanel * panel);
    void PopulateMusicPanel(wxPanel * panel);

    void ReadSettings();

    static float ZoomIncrementSpinToZoomIncrement(int spinPosition);
    static int ZoomIncrementToZoomIncrementSpin(float zoomIncrement);

    static int PanIncrementSpinToPanIncrement(int spinPosition);
    static int PanIncrementToPanIncrementSpin(int panIncrement);

    void ReconciliateShipAutoTexturizationModeSettings();
    void ReconcileSoundSettings();

    int GetLanguagesListBoxIndex(std::optional<std::string> languageIdentifier) const;

private:

    // Game panel
    wxDirPickerCtrl * mScreenshotDirPickerCtrl;
    wxCheckBox * mShowTipOnStartupCheckBox;
    wxCheckBox * mCheckForUpdatesAtStartupCheckBox;
    wxCheckBox * mSaveSettingsOnExitCheckBox;
    wxCheckBox * mShowTsunamiNotificationsCheckBox;
    wxSpinCtrl * mZoomIncrementSpinCtrl;
    wxSpinCtrl * mPanIncrementSpinCtrl;
    wxCheckBox * mShowStatusTextCheckBox;
    wxCheckBox * mShowExtendedStatusTextCheckBox;
    wxListBox * mLanguagesListBox;
    wxComboBox * mDisplayUnitsSettingsComboBox;

    // Ships panel
    wxCheckBox * mReloadLastLoadedShipOnStartupCheckBox;
    wxCheckBox * mShowShipDescriptionAtShipLoadCheckBox;
    wxCheckBox * mAutoZoomAtShipLoadCheckBox;
    wxCheckBox * mAutoShowSwitchboardCheckBox;
    wxCheckBox * mShowElectricalNotificationsCheckBox;
    wxRadioButton * mFlatStructureAutoTexturizationModeRadioButton;
    wxRadioButton * mMaterialTexturesAutoTexturizationModeRadioButton;
    wxCheckBox * mForceSharedAutoTexturizationSettingsOntoShipCheckBox;
    SliderControl<float> * mMaterialTextureMagnificationSlider;
    SliderControl<float> * mMaterialTextureTransparencySlider;
    SliderControl<float> * mStrengthRandomizationDensityAdjustmentSlider;
    SliderControl<float> * mStrengthRandomizationExtentSlider;

    // Global Sound and Music panel
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

    std::vector<LocalizationManager::LanguageInfo> const mAvailableLanguages;

    bool mHasWarnedAboutLanguageSettingChanges = false;
};
