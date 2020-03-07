/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SettingsManager.h"

#include <UIControls/SliderControl.h>

#include <Game/IGameControllerSettingsOptions.h>
#include <Game/ResourceLoader.h>

#include <GameCore/Settings.h>

#include <wx/bitmap.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/clrpicker.h>
#include <wx/listctrl.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>

#include <memory>
#include <vector>

class SettingsDialog : public wxFrame
{
public:

    SettingsDialog(
        wxWindow * parent,
        std::shared_ptr<SettingsManager> settingsManager,
		std::shared_ptr<IGameControllerSettingsOptions> gameControllerSettingsOptions,
        ResourceLoader const & resourceLoader);

    virtual ~SettingsDialog();

    void Open();

private:

    void OnUltraViolentCheckBoxClick(wxCommandEvent & event);
    void OnGenerateDebrisCheckBoxClick(wxCommandEvent & event);
    void OnGenerateSparklesForCutsCheckBoxClick(wxCommandEvent & event);
    void OnGenerateAirBubblesCheckBoxClick(wxCommandEvent & event);
    void OnGenerateEngineWakeCheckBoxClick(wxCommandEvent & event);

    void OnModulateWindCheckBoxClick(wxCommandEvent & event);

	void OnRestoreDefaultTerrainButton(wxCommandEvent & event);
	void OnDoRainWithStormCheckBoxClick(wxCommandEvent & event);

    void OnOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnTextureOceanChanged(wxCommandEvent & event);
    void OnDepthOceanColorStartChanged(wxColourPickerEvent & event);
    void OnDepthOceanColorEndChanged(wxColourPickerEvent & event);
    void OnDefaultWaterColorChanged(wxColourPickerEvent & event);
    void OnSeeShipThroughOceanCheckBoxClick(wxCommandEvent & event);
    void OnDrawHeatOverlayCheckBoxClick(wxCommandEvent & event);
    void OnDrawHeatBlasterFlameCheckBoxClick(wxCommandEvent & event);

    void OnLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnTextureLandChanged(wxCommandEvent & event);
    void OnFlatLandColorChanged(wxColourPickerEvent & event);
    void OnFlatSkyColorChanged(wxColourPickerEvent & event);

    void OnFlatOceanColorChanged(wxColourPickerEvent & event);
    void OnFlatLampLightColorChanged(wxColourPickerEvent & event);

    void OnShipRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnShowStressCheckBoxClick(wxCommandEvent & event);
    void OnShipFlameRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnDebugShipRenderModeRadioBox(wxCommandEvent & event);
    void OnVectorFieldRenderModeRadioBox(wxCommandEvent & event);

    void OnPlayBreakSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlayStressSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlayWindSoundCheckBoxClick(wxCommandEvent & event);

	void OnPersistedSettingsListCtrlSelected(wxListEvent & event);
	void OnPersistedSettingsListCtrlActivated(wxListEvent & event);
	void OnApplyPersistedSettingsButton(wxCommandEvent & event);
	void OnRevertToPersistedSettingsButton(wxCommandEvent & event);
	void OnReplacePersistedSettingsButton(wxCommandEvent & event);
	void OnDeletePersistedSettingsButton(wxCommandEvent & event);
	void OnSaveSettingsTextEdited(wxCommandEvent & event);
	void OnSaveSettingsButton(wxCommandEvent & event);

	void OnRevertToDefaultsButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnUndoButton(wxCommandEvent & event);

    void OnCloseButton(wxCloseEvent & event);

private:

    //////////////////////////////////////////////////////
    // Control tabs
    //////////////////////////////////////////////////////

    // Mechanics, Fluids, and Light
    SliderControl<float> * mMechanicalQualitySlider;
    SliderControl<float> * mStrengthSlider;
    SliderControl<float> * mGlobalDampingAdjustmentSlider;
    SliderControl<float> * mRotAcceler8rSlider;
    SliderControl<float> * mWaterDensitySlider;
    SliderControl<float> * mWaterDragSlider;
    SliderControl<float> * mWaterIntakeSlider;
    SliderControl<float> * mWaterCrazynessSlider;
    SliderControl<float> * mWaterDiffusionSpeedSlider;
    SliderControl<float> * mLuminiscenceSlider;
    SliderControl<float> * mLightSpreadSlider;

    // Heat and Combustion
    SliderControl<float> * mThermalConductivityAdjustmentSlider;
    SliderControl<float> * mHeatDissipationAdjustmentSlider;
    SliderControl<float> * mIgnitionTemperatureAdjustmentSlider;
    SliderControl<float> * mMeltingTemperatureAdjustmentSlider;
    SliderControl<float> * mCombustionSpeedAdjustmentSlider;
    SliderControl<float> * mCombustionHeatAdjustmentSlider;
    SliderControl<float> * mAirTemperatureSlider;
    SliderControl<float> * mWaterTemperatureSlider;
    SliderControl<float> * mElectricalElementHeatProducedAdjustmentSlider;
    SliderControl<float> * mHeatBlasterRadiusSlider;
    SliderControl<float> * mHeatBlasterHeatFlowSlider;
    SliderControl<unsigned int> * mMaxBurningParticlesSlider;

    // Ocean, Smoke, and Sky
    SliderControl<float> * mOceanDepthSlider;
    SliderControl<float> * mOceanFloorBumpinessSlider;
    SliderControl<float> * mOceanFloorDetailAmplificationSlider;
    SliderControl<float> * mOceanFloorElasticitySlider;
    SliderControl<float> * mOceanFloorFrictionSlider;
    SliderControl<float> * mSmokeEmissionDensityAdjustmentSlider;
    SliderControl<float> * mSmokeParticleLifetimeAdjustmentSlider;
	SliderControl<float> * mStormStrengthAdjustmentSlider;
	wxCheckBox* mDoRainWithStormCheckBox;
    SliderControl<float> * mRainFloodAdjustmentSlider;
	SliderControl<std::chrono::seconds::rep> * mStormDurationSlider;
	SliderControl<std::chrono::minutes::rep> * mStormRateSlider;
    SliderControl<unsigned int> * mNumberOfStarsSlider;
    SliderControl<unsigned int> * mNumberOfCloudsSlider;

    // Wind and Waves
    SliderControl<float> * mWindSpeedBaseSlider;
    wxCheckBox* mModulateWindCheckBox;
    SliderControl<float> * mWindGustAmplitudeSlider;
    SliderControl<float> * mBasalWaveHeightAdjustmentSlider;
    SliderControl<float> * mBasalWaveLengthAdjustmentSlider;
    SliderControl<float> * mBasalWaveSpeedAdjustmentSlider;
    SliderControl<std::chrono::minutes::rep> * mTsunamiRateSlider;
    SliderControl<std::chrono::minutes::rep> * mRogueWaveRateSlider;

    // Interactions
    SliderControl<float> * mDestroyRadiusSlider;
    SliderControl<float> * mBombBlastRadiusSlider;
    SliderControl<float> * mBombBlastHeatSlider;
    SliderControl<float> * mAntiMatterBombImplosionStrengthSlider;
    SliderControl<float> * mFloodRadiusSlider;
    SliderControl<float> * mFloodQuantitySlider;
    SliderControl<float> * mRepairRadiusSlider;
    SliderControl<float> * mRepairSpeedAdjustmentSlider;
    wxCheckBox * mUltraViolentCheckBox;
    wxCheckBox * mGenerateDebrisCheckBox;
    wxCheckBox * mGenerateSparklesForCutsCheckBox;
    wxCheckBox * mGenerateAirBubblesCheckBox;
    wxCheckBox * mGenerateEngineWakeCheckBox;
    SliderControl<float> * mAirBubbleDensitySlider;
    SliderControl<float> * mEngineThrustAdjustmentSlider;

    // Rendering
    wxRadioButton * mTextureOceanRenderModeRadioButton;
    wxRadioButton * mDepthOceanRenderModeRadioButton;
    wxBitmapComboBox * mTextureOceanComboBox;
    wxColourPickerCtrl * mDepthOceanColorStartPicker;
    wxColourPickerCtrl * mDepthOceanColorEndPicker;
    wxRadioButton * mFlatOceanRenderModeRadioButton;
    wxColourPickerCtrl * mFlatOceanColorPicker;
    wxCheckBox * mSeeShipThroughOceanCheckBox;
    SliderControl<float> * mOceanTransparencySlider;
    SliderControl<float> * mOceanDarkeningRateSlider;
    wxRadioButton * mTextureLandRenderModeRadioButton;
    wxBitmapComboBox * mTextureLandComboBox;
    wxRadioButton * mFlatLandRenderModeRadioButton;
    wxColourPickerCtrl * mFlatLandColorPicker;
    wxColourPickerCtrl * mFlatSkyColorPicker;
    wxRadioButton * mTextureShipRenderModeRadioButton;
    wxRadioButton * mStructureShipRenderModeRadioButton;
    wxCheckBox* mShowStressCheckBox;
    wxColourPickerCtrl * mFlatLampLightColorPicker;
    wxColourPickerCtrl * mDefaultWaterColorPicker;
    SliderControl<float> * mWaterContrastSlider;
    SliderControl<float> * mWaterLevelOfDetailSlider;
    wxCheckBox * mDrawHeatOverlayCheckBox;
    SliderControl<float> * mHeatOverlayTransparencySlider;
    wxRadioButton * mMode1ShipFlameRenderModeRadioButton;
    wxRadioButton * mMode2ShipFlameRenderModeRadioButton;
    wxRadioButton * mMode3ShipFlameRenderModeRadioButton;
    wxRadioButton * mNoDrawShipFlameRenderModeRadioButton;
    wxCheckBox * mDrawHeatBlasterFlameCheckBox;
    SliderControl<float> * mShipFlameSizeAdjustmentSlider;

    // Sound and Advanced
    SliderControl<float> * mEffectsVolumeSlider;
    SliderControl<float> * mToolsVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlayWindSoundCheckBox;
    SliderControl<float> * mSpringStiffnessSlider;
    SliderControl<float> * mSpringDampingSlider;
    wxRadioBox * mDebugShipRenderModeRadioBox;
    wxRadioBox * mVectorFieldRenderModeRadioBox;

	// Settings Management
	wxListCtrl * mPersistedSettingsListCtrl;
	wxTextCtrl * mPersistedSettingsDescriptionTextCtrl;
	wxButton * mApplyPersistedSettingsButton;
	wxButton * mRevertToPersistedSettingsButton;
	wxButton * mReplacePersistedSettingsButton;
	wxButton * mDeletePersistedSettingsButton;
	wxTextCtrl * mSaveSettingsNameTextCtrl;
	wxTextCtrl * mSaveSettingsDescriptionTextCtrl;
	wxButton * mSaveSettingsButton;

    //////////////////////////////////////////////////////

    // Buttons
	wxButton * mRevertToDefaultsButton;
    wxButton * mOkButton;
    wxButton * mCancelButton;
    wxButton * mUndoButton;

    // Icons
    std::unique_ptr<wxBitmap> mWarningIcon;

private:

    void DoCancel();
    void DoClose();

    void PopulateMechanicsFluidsLightsPanel(wxPanel * panel);
    void PopulateHeatPanel(wxPanel * panel);
    void PopulateOceanSmokeSkyPanel(wxPanel * panel);
    void PopulateWindAndWavesPanel(wxPanel * panel);
    void PopulateInteractionsPanel(wxPanel * panel);
    void PopulateRenderingPanel(wxPanel * panel);
    void PopulateSoundAndAdvancedPanel(wxPanel * panel);
	void PopulateSettingsManagementPanel(wxPanel * panel);

    void SyncControlsWithSettings(Settings<GameSettings> const & settings);

    void ReconciliateOceanRenderModeSettings();
    void ReconciliateLandRenderModeSettings();

    void OnLiveSettingsChanged();
    void ReconcileDirtyState();

	long GetSelectedPersistedSettingIndexFromCtrl() const;
	void InsertPersistedSettingInCtrl(int index, PersistedSettingsKey const & psKey);
	void LoadPersistedSettings(int index, bool withDefaults);
	void ReconciliateLoadPersistedSettings();
	void SavePersistedSettings(PersistedSettingsMetadata const & metadata);
	void ReconciliateSavePersistedSettings();

	void OnPersistenceError(std::string const & errorMessage) const;

private:

    wxWindow * const mParent;
    std::shared_ptr<SettingsManager> mSettingsManager;
	std::shared_ptr<IGameControllerSettingsOptions> mGameControllerSettingsOptions;

    //
    // State
    //

    // The current settings, always enforced
    Settings<GameSettings> mLiveSettings;

    // The settings when the dialog was last opened
    Settings<GameSettings> mCheckpointSettings;

    // Tracks whether the user has changed any settings since the dialog
    // was last opened. When false there's a guarantee that the current live
    // settings have not been modified.
    bool mHasBeenDirtyInCurrentSession;

	// Tracks whether the current settings are (possibly) dirty wrt the defaults.
	// Best effort, we assume all changes deviate from the default.
	bool mAreSettingsDirtyWrtDefaults;

	// The persisted settings currently displayed in the LoadSettings list;
	// maintained in-sync with the SettingsManager's official list of
	// persisted settings, and used to hold metadata for the list.
	std::vector<PersistedSettingsMetadata> mPersistedSettings;
};
