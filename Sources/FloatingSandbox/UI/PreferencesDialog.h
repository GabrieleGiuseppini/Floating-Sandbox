/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../UIPreferencesManager.h"

#include <UILib/LocalizationManager.h>
#include <UILib/SliderControl.h>

#include <Game/GameAssetManager.h>

#include <wx/bitmap.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/listbox.h>
#include <wx/radiobut.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <functional>

class PreferencesDialog : public wxDialog
{
public:

    PreferencesDialog(
        wxWindow * parent,
        UIPreferencesManager & uiPreferencesManager,
        std::function<void()> onChangeCallback,
        std::function<void()> shipResetCallback,
        GameAssetManager const & gameAssetManager);

    virtual ~PreferencesDialog();

    void Open();

private:

    void OnScreenshotDirPickerChanged(wxCommandEvent & event);
    void OnStartInFullScreenCheckBoxClicked(wxCommandEvent & event);
    void OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnCheckForUpdatesAtStartupCheckBoxClicked(wxCommandEvent & event);
    void OnSaveSettingsOnExitCheckBoxClicked(wxCommandEvent & event);
    void OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnZoomIncrementSpinCtrl(wxSpinEvent & event);
    void OnPanIncrementSpinCtrl(wxSpinEvent & event);
    void OnCameraSpeedAdjustmentSpinCtrl(wxSpinEvent & event);
    void OnShowStatusTextCheckBoxClicked(wxCommandEvent & event);
    void OnShowExtendedStatusTextCheckBoxClicked(wxCommandEvent & event);
    void OnLanguagesListBoxSelected(wxCommandEvent & event);

    void OnReloadLastLoadedShipOnStartupCheckBoxClicked(wxCommandEvent & event);
    void OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnContinuousAutoFocusOnShipCheckBoxClicked(wxCommandEvent & event);
    void OnAutoFocusOnShipLoadCheckBoxClicked(wxCommandEvent & event);
    void OnAutoShowSwitchboardCheckBoxClicked(wxCommandEvent & event);
    void OnShowElectricalNotificationsCheckBoxClicked(wxCommandEvent & event);
    void OnAutoTexturizationModeRadioButtonClick(wxCommandEvent & event);
    void OnForceSharedAutoTexturizationSettingsOntoShipCheckBoxClicked(wxCommandEvent & event);

    void OnAutoFocusOnNpcPlacementCheckBoxClicked(wxCommandEvent & event);
    void OnAutoToggleToExteriorViewWhenNonNpcToolIsSelectedCheckBoxClicked(wxCommandEvent & event);
    void OnShowNpcNotificationsCheckBoxClicked(wxCommandEvent & event);

    void OnGlobalMuteCheckBoxClicked(wxCommandEvent & event);
    void OnPlayBackgroundMusicCheckBoxClicked(wxCommandEvent & event);
    void OnPlaySinkingMusicCheckBoxClicked(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);

private:

    void PopulateGamePanel(wxPanel * panel);
    void PopulateShipPanel(wxPanel * panel);
    void PopulateNpcPanel(wxPanel * panel);
    void PopulateMusicPanel(wxPanel * panel);

    void ReadSettings();

    static float ZoomIncrementSpinToZoomIncrement(int spinPosition);
    static int ZoomIncrementToZoomIncrementSpin(float zoomIncrement);

    static int PanIncrementSpinToPanIncrement(int spinPosition);
    static int PanIncrementToPanIncrementSpin(int panIncrement);

    float CameraSpeedAdjustmentSpinToCameraSpeedAdjustment(int spinPosition) const;
    int CameraSpeedAdjustmentToCameraSpeedAdjustmentSpin(float zoomIncrement) const;

    void ReconciliateShipAutoTexturizationModeSettings();
    void ReconcileSoundSettings();

    int GetLanguagesListBoxIndex(std::optional<std::string> languageIdentifier) const;

private:

    // Game panel
    wxDirPickerCtrl * mScreenshotDirPickerCtrl;
    wxCheckBox * mStartInFullScreenCheckBox;
    wxCheckBox * mShowTipOnStartupCheckBox;
    wxCheckBox * mCheckForUpdatesAtStartupCheckBox;
    wxCheckBox * mSaveSettingsOnExitCheckBox;
    wxCheckBox * mShowTsunamiNotificationsCheckBox;
    wxSpinCtrl * mZoomIncrementSpinCtrl;
    wxSpinCtrl * mPanIncrementSpinCtrl;
    wxSpinCtrl * mCameraSpeedAdjustmentSpinCtrl;
    wxCheckBox * mShowStatusTextCheckBox;
    wxCheckBox * mShowExtendedStatusTextCheckBox;
    wxListBox * mLanguagesListBox;
    wxComboBox * mDisplayUnitsSettingsComboBox;

    // Ships panel
    wxCheckBox * mReloadLastLoadedShipOnStartupCheckBox;
    wxCheckBox * mShowShipDescriptionAtShipLoadCheckBox;
    wxCheckBox * mContinuousAutoFocusOnShipCheckBox;
    wxCheckBox * mAutoFocusOnShipLoadCheckBox;
    wxCheckBox * mAutoShowSwitchboardCheckBox;
    wxCheckBox * mShowElectricalNotificationsCheckBox;
    wxRadioButton * mFlatStructureAutoTexturizationModeRadioButton;
    wxRadioButton * mMaterialTexturesAutoTexturizationModeRadioButton;
    wxCheckBox * mForceSharedAutoTexturizationSettingsOntoShipCheckBox;
    SliderControl<float> * mMaterialTextureMagnificationSlider;
    SliderControl<float> * mMaterialTextureTransparencySlider;

    // NPCs panel
    SliderControl<size_t> * mMaxNpcsSlider;
    SliderControl<size_t> * mNpcsPerGroupSlider;
    wxCheckBox * mAutoFocusOnNpcPlacementCheckBox;
    wxCheckBox * mAutoToggleToExteriorViewWhenNonNpcToolIsSelectedCheckBox;
    wxCheckBox * mShowNpcNotificationsCheckBox;

    // Global Sound and Music panel
    wxCheckBox * mGlobalMuteCheckBox;
    SliderControl<float> * mBackgroundMusicVolumeSlider;
    wxCheckBox * mPlayBackgroundMusicCheckBox;
    SliderControl<float> * mSinkingMusicVolumeSlider;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    // Buttons
    wxButton * mOkButton;

    // Icons
    std::unique_ptr<wxBitmap> mWarningIcon;

private:

    wxWindow * const mParent;
    UIPreferencesManager & mUIPreferencesManager;
    std::function<void()> mOnChangeCallback;
    std::function<void()> mShipResetCallback;

    std::vector<LocalizationManager::LanguageInfo> const mAvailableLanguages;

    bool mHasWarnedAboutLanguageSettingChanges = false;
    bool mHasDirtySettingsThatRequireRestart = false;
};
