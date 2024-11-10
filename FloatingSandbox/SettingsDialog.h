/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SettingsManager.h"

#include <UILib/BitmapToggleButton.h>
#include <UILib/SliderControl.h>

#include <Game/IGameControllerSettingsOptions.h>
#include <Game/ResourceLocator.h>

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
        SettingsManager & settingsManager,
        IGameControllerSettingsOptions & gameControllerSettingsOptions,
        ResourceLocator const & resourceLocator);

    virtual ~SettingsDialog();

    void Open();

private:

    void OnOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnSkyRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnNpcRenderModeRadioButtonClick(wxCommandEvent & event);

    void OnRevertToDefaultsButton(wxCommandEvent & event);
    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnUndoButton(wxCommandEvent & event);

    void OnCloseButton(wxCloseEvent & event);

private:

    //////////////////////////////////////////////////////
    // Control tabs
    //////////////////////////////////////////////////////

    // Mechanics and Thermodynamics
    SliderControl<float> * mStrengthSlider;
    SliderControl<float> * mGlobalDampingAdjustmentSlider;
    SliderControl<float> * mElasticityAdjustmentSlider;
    SliderControl<float> * mStaticFrictionAdjustmentSlider;
    SliderControl<float> * mKineticFrictionAdjustmentSlider;
    SliderControl<float> * mStaticPressureForceAdjustmentSlider;
    SliderControl<float> * mThermalConductivityAdjustmentSlider;
    SliderControl<float> * mHeatDissipationAdjustmentSlider;
    SliderControl<float> * mIgnitionTemperatureAdjustmentSlider;
    SliderControl<float> * mMeltingTemperatureAdjustmentSlider;
    SliderControl<float> * mCombustionSpeedAdjustmentSlider;
    SliderControl<float> * mCombustionHeatAdjustmentSlider;
    SliderControl<unsigned int> * mMaxBurningParticlesPerShipSlider;
    BitmapToggleButton * mUltraViolentToggleButton;

    // Ocean and Water
    SliderControl<float> * mWaterDensityAdjustmentSlider;
    SliderControl<float> * mWaterFrictionDragSlider;
    SliderControl<float> * mWaterPressureDragSlider;
    SliderControl<float> * mWaterImpactForceAdjustmentSlider;
    SliderControl<float> * mHydrostaticPressureCounterbalanceAdjustmentSlider;
    SliderControl<float> * mWaterIntakeSlider;
    SliderControl<float> * mWaterCrazynessSlider;
    SliderControl<float> * mWaterDiffusionSpeedSlider;
    SliderControl<float> * mWaterTemperatureSlider;
    SliderControl<float> * mOceanDepthSlider;
    SliderControl<float> * mOceanFloorBumpinessSlider;
    SliderControl<float> * mOceanFloorDetailAmplificationSlider;
    SliderControl<float> * mOceanFloorElasticityCoefficientSlider;
    SliderControl<float> * mOceanFloorFrictionCoefficientSlider;
    SliderControl<float> * mOceanFloorSiltHardnessSlider;
    SliderControl<float> * mRotAcceler8rSlider;

    // Wind and Waves
    SliderControl<float> * mWindSpeedBaseSlider;
    wxCheckBox * mModulateWindCheckBox;
    SliderControl<float> * mWindGustAmplitudeSlider;
    SliderControl<float> * mBasalWaveHeightAdjustmentSlider;
    SliderControl<float> * mBasalWaveLengthAdjustmentSlider;
    SliderControl<float> * mBasalWaveSpeedAdjustmentSlider;
    wxCheckBox * mDoDisplaceWaterCheckBox;
    SliderControl<float> * mWaterDisplacementWaveHeightAdjustmentSlider;
    SliderControl<float> * mWaveSmoothnessAdjustmentSlider;
    SliderControl<std::chrono::minutes::rep> * mTsunamiRateSlider;
    SliderControl<std::chrono::seconds::rep> * mRogueWaveRateSlider;
    SliderControl<float> * mStormStrengthAdjustmentSlider;
    wxCheckBox * mDoRainWithStormCheckBox;
    SliderControl<float> * mRainFloodAdjustmentSlider;
    SliderControl<float> * mLightningBlastProbabilitySlider;
    SliderControl<std::chrono::seconds::rep> * mStormDurationSlider;
    SliderControl<std::chrono::minutes::rep> * mStormRateSlider;

    // Air and Sky
    SliderControl<float> * mAirDensityAdjustmentSlider;
    SliderControl<float> * mAirFrictionDragSlider;
    SliderControl<float> * mAirPressureDragSlider;
    SliderControl<float> * mAirTemperatureSlider;
    SliderControl<float> * mAirBubbleDensitySlider;
    SliderControl<float> * mSmokeEmissionDensityAdjustmentSlider;
    SliderControl<float> * mSmokeParticleLifetimeAdjustmentSlider;
    SliderControl<unsigned int> * mNumberOfStarsSlider;
    SliderControl<unsigned int> * mNumberOfCloudsSlider;
    wxCheckBox * mDoDayLightCycleCheckBox;
    SliderControl<std::chrono::minutes::rep> * mDayLightCycleDurationSlider;

    // Lights, Electricals, Fishes, NPCs
    SliderControl<float> * mLuminiscenceSlider;
    SliderControl<float> * mLightSpreadSlider;
    wxCheckBox * mGenerateEngineWakeCheckBox;
    SliderControl<float> * mEngineThrustAdjustmentSlider;
    SliderControl<float> * mWaterPumpPowerAdjustmentSlider;
    SliderControl<float> * mElectricalElementHeatProducedAdjustmentSlider;
    SliderControl<unsigned int> * mNumberOfFishesSlider;
    SliderControl<float> * mFishSizeMultiplierSlider;
    SliderControl<float> * mFishSpeedAdjustmentSlider;
    wxCheckBox * mDoFishShoalingCheckBox;
    SliderControl<float> * mFishShoalRadiusAdjustmentSlider;
    SliderControl<float> * mNpcFrictionAdjustmentSlider;
    SliderControl<float> * mNpcSizeMultiplierSlider;
    wxCheckBox * mDoApplyPhysicsToolsToNpcsCheckBox;

    // Destructive Tools
    SliderControl<float> * mDestroyRadiusSlider;
    SliderControl<float> * mBombBlastRadiusSlider;
    SliderControl<float> * mBombBlastForceAdjustmentSlider;
    SliderControl<float> * mBombBlastHeatSlider;
    SliderControl<float> * mAntiMatterBombImplosionStrengthSlider;
    SliderControl<float> * mBlastToolRadiusSlider;
    SliderControl<float> * mBlastToolForceAdjustmentSlider;
    SliderControl<float> * mLaserRayHeatFlowSlider;

    // Other Tools
    SliderControl<float> * mFloodRadiusSlider;
    SliderControl<float> * mFloodQuantitySlider;
    SliderControl<float> * mHeatBlasterRadiusSlider;
    SliderControl<float> * mHeatBlasterHeatFlowSlider;
    SliderControl<float> * mInjectPressureQuantitySlider;
    SliderControl<float> * mRepairRadiusSlider;
    SliderControl<float> * mRepairSpeedAdjustmentSlider;
    SliderControl<float> * mScrubRotRadiusSlider;
    SliderControl<float> * mWindMakerWindSpeedSlider;

    // Rendering
    wxRadioButton * mTextureOceanRenderModeRadioButton;
    wxRadioButton * mDepthOceanRenderModeRadioButton;
    wxBitmapComboBox * mTextureOceanComboBox;
    wxColourPickerCtrl * mDepthOceanColorStartPicker;
    wxColourPickerCtrl * mDepthOceanColorEndPicker;
    wxRadioButton * mFlatOceanRenderModeRadioButton;
    wxColourPickerCtrl * mFlatOceanColorPicker;
    wxCheckBox * mOceanRenderDetailModeDetailedCheckBox;
    wxCheckBox * mSeeShipThroughOceanCheckBox;
    SliderControl<float> * mOceanTransparencySlider;
    SliderControl<float> * mOceanDepthDarkeningRateSlider;
    wxRadioButton * mFlatSkyRenderModeRadioButton;
    wxColourPickerCtrl * mFlatSkyColorPicker;
    wxRadioButton * mCrepuscularSkyRenderModeRadioButton;
    wxColourPickerCtrl * mCrepuscularColorPicker;
    wxCheckBox * mDoMoonlightCheckBox;
    wxColourPickerCtrl * mMoonlightColorPicker;
    wxCheckBox * mCloudRenderDetailModeDetailedCheckBox;
    wxRadioButton * mTextureLandRenderModeRadioButton;
    wxBitmapComboBox * mTextureLandComboBox;
    wxRadioButton * mFlatLandRenderModeRadioButton;
    wxColourPickerCtrl * mFlatLandColorPicker;
    wxCheckBox * mLandRenderDetailModeDetailedCheckBox;
    wxColourPickerCtrl * mFlatLampLightColorPicker;
    wxRadioBox * mHeatRenderModeRadioBox;
    SliderControl<float> * mHeatSensitivitySlider;
    wxRadioBox * mStressRenderModeRadioBox;
    SliderControl<float> * mShipFlameSizeAdjustmentSlider;
    SliderControl<float> * mShipFlameKaosAdjustmentSlider;
    SliderControl<float> * mShipAmbientLightSensitivitySlider;
    SliderControl<float> * mShipDepthDarkeningSensitivitySlider;
    wxColourPickerCtrl * mDefaultWaterColorPicker;
    SliderControl<float> * mWaterContrastSlider;
    SliderControl<float> * mWaterLevelOfDetailSlider;
    wxRadioButton * mTextureNpcRenderModeRadioButton;
    wxRadioButton * mQuadWithRolesNpcRenderModeRadioButton;
    wxRadioButton * mQuadFlatNpcRenderModeRadioButton;
    wxColourPickerCtrl * mQuadFlatNpcColorPicker;

    // Sound and Advanced Settings
    SliderControl<float> * mEffectsVolumeSlider;
    SliderControl<float> * mToolsVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlayWindSoundCheckBox;
    wxCheckBox * mPlayAirBubbleSurfaceSoundCheckBox;
    SliderControl<float> * mStrengthRandomizationDensityAdjustmentSlider;
    SliderControl<float> * mStrengthRandomizationExtentSlider;
    SliderControl<float> * mSpringStiffnessSlider;
    SliderControl<float> * mSpringDampingSlider;
    wxRadioBox * mDebugShipRenderModeRadioBox;
    wxCheckBox * mDrawExplosionsCheckBox;
    wxCheckBox * mDrawFlamesCheckBox;
    wxCheckBox * mShowFrontiersCheckBox;
    wxCheckBox * mShowAABBsCheckBox;
    wxCheckBox * mShowStressCheckBox;
    wxCheckBox * mDrawHeatBlasterFlameCheckBox;
    wxRadioBox * mVectorFieldRenderModeRadioBox;
    wxCheckBox * mGenerateDebrisCheckBox;
    wxCheckBox * mGenerateSparklesForCutsCheckBox;
    SliderControl<unsigned int> * mMaxNumSimulationThreadsSlider;
    SliderControl<float> * mNumMechanicalIterationsAdjustmentSlider;

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

    void PopulateMechanicsAndThermodynamicsPanel(wxPanel * panel, ResourceLocator const & resourceLocator);
    void PopulateWaterAndOceanPanel(wxPanel * panel);
    void PopulateWindAndWavesPanel(wxPanel * panel);
    void PopulateAirAndSkyPanel(wxPanel * panel);
    void PopulateLightsElectricalFishesNpcsPanel(wxPanel * panel);
    void PopulateDestructiveToolsPanel(wxPanel * panel, ResourceLocator const & resourceLocator);
    void PopulateOtherToolsPanel(wxPanel * panel, ResourceLocator const & resourceLocator);
    void PopulateRenderingPanel(wxPanel * panel);
    void PopulateSoundAndAdvancedSettingsPanel(wxPanel * panel);
    void PopulateSettingsManagementPanel(wxPanel * panel);

    void SyncControlsWithSettings(Settings<GameSettings> const & settings);
    void ReconciliateOceanRenderModeSettings();
    void ReconciliateLandRenderModeSettings();
    void ReconciliateSkyRenderModeSettings();
    void ReconciliateMoonlightSettings();
    void ReconciliateNpcRenderModeSettings();
    void OnLiveSettingsChanged();
    void ReconcileDirtyState();

    long GetSelectedPersistedSettingIndexFromCtrl() const;
    void InsertPersistedSettingInCtrl(int index, PersistedSettingsKey const & psKey);
    void LoadPersistedSettings(size_t index, bool withDefaults);
    void ReconciliateLoadPersistedSettings();
    void SavePersistedSettings(PersistedSettingsMetadata const & metadata);
    void ReconciliateSavePersistedSettings();
    void OnPersistenceError(std::string const & errorMessage) const;

private:

    wxWindow * const mParent;
    SettingsManager & mSettingsManager;
    IGameControllerSettingsOptions & mGameControllerSettingsOptions;

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
