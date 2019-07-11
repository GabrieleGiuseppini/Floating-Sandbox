/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SliderControl.h"
#include "SoundController.h"

#include <Game/IGameController.h>
#include <Game/ResourceLoader.h>

#include <wx/bitmap.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/clrpicker.h>
#include <wx/dialog.h>
#include <wx/radiobox.h>

#include <memory>

class SettingsDialog : public wxDialog
{
public:

    SettingsDialog(
        wxWindow * parent,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        ResourceLoader const & resourceLoader);

    virtual ~SettingsDialog();

    void Open();

private:

    void OnUltraViolentCheckBoxClick(wxCommandEvent & event);
    void OnGenerateDebrisCheckBoxClick(wxCommandEvent & event);
    void OnGenerateSparklesCheckBoxClick(wxCommandEvent & event);
    void OnGenerateAirBubblesCheckBoxClick(wxCommandEvent & event);

    void OnModulateWindCheckBoxClick(wxCommandEvent & event);

    void OnTextureOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnTextureOceanChanged(wxCommandEvent & event);
    void OnDepthOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnDepthOceanColorStartChanged(wxColourPickerEvent & event);
    void OnDepthOceanColorEndChanged(wxColourPickerEvent & event);
    void OnFlatOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnFlatOceanColorChanged(wxColourPickerEvent & event);
    void OnSeeShipThroughOceanCheckBoxClick(wxCommandEvent & event);
    void OnDrawHeatOverlayCheckBoxClick(wxCommandEvent & event);
    void OnDrawHeatBlasterFlameCheckBoxClick(wxCommandEvent & event);

    void OnTextureLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnTextureLandChanged(wxCommandEvent & event);
    void OnFlatLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnFlatLandColorChanged(wxColourPickerEvent & event);

    void OnFlatSkyColorChanged(wxColourPickerEvent & event);

    void OnTextureShipRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnStructureShipRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnShowStressCheckBoxClick(wxCommandEvent & event);

    void OnMode1ShipFlameRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnMode2ShipFlameRenderModeRadioButtonClick(wxCommandEvent & event);

    void OnDebugShipRenderModeRadioBox(wxCommandEvent & event);
    void OnVectorFieldRenderModeRadioBox(wxCommandEvent & event);

    void OnPlayBreakSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlayStressSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlayWindSoundCheckBoxClick(wxCommandEvent & event);
    void OnPlaySinkingMusicCheckBoxClick(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);
    void OnApplyButton(wxCommandEvent & event);

private:

    //////////////////////////////////////////////////////
    // Control tabs
    //////////////////////////////////////////////////////

    // Mechanics, Fluids, and Light
    SliderControl * mMechanicalQualitySlider;
    SliderControl * mStrengthSlider;
    SliderControl * mRotAcceler8rSlider;
    SliderControl * mWaterDensitySlider;
    SliderControl * mWaterDragSlider;
    SliderControl * mWaterIntakeSlider;
    SliderControl * mWaterCrazynessSlider;
    SliderControl * mWaterDiffusionSpeedSlider;
    SliderControl * mLuminiscenceSlider;
    SliderControl * mLightSpreadSlider;

    // Heat and Combustion
    SliderControl * mThermalConductivityAdjustmentSlider;
    SliderControl * mHeatDissipationAdjustmentSlider;
    SliderControl * mIgnitionTemperatureAdjustmentSlider;
    SliderControl * mMeltingTemperatureAdjustmentSlider;
    SliderControl * mCombustionSpeedAdjustmentSlider;
    SliderControl * mCombustionHeatAdjustmentSlider;
    SliderControl * mFlameThrowerRadiusSlider;
    SliderControl * mFlameThrowerHeatFlowSlider;
    SliderControl * mAirTemperatureSlider;
    SliderControl * mWaterTemperatureSlider;

    // Ocean and Sky
    SliderControl * mOceanDepthSlider;
    SliderControl * mOceanFloorBumpinessSlider;
    SliderControl * mOceanFloorDetailAmplificationSlider;
    SliderControl * mNumberOfStarsSlider;
    SliderControl * mNumberOfCloudsSlider;

    // Wind and Waves
    SliderControl * mWindSpeedBaseSlider;
    wxCheckBox* mModulateWindCheckBox;
    SliderControl * mWindGustAmplitudeSlider;
    SliderControl * mBasalWaveHeightAdjustmentSlider;
    SliderControl * mBasalWaveLengthAdjustmentSlider;
    SliderControl * mBasalWaveSpeedAdjustmentSlider;
    SliderControl * mTsunamiRateSlider;
    SliderControl * mRogueWaveRateSlider;

    // Interactions
    SliderControl * mDestroyRadiusSlider;
    SliderControl * mBombBlastRadiusSlider;
    SliderControl * mAntiMatterBombImplosionStrengthSlider;
    SliderControl * mFloodRadiusSlider;
    SliderControl * mFloodQuantitySlider;
    SliderControl * mRepairRadiusSlider;
    SliderControl * mRepairSpeedAdjustmentSlider;
    wxCheckBox * mUltraViolentCheckBox;
    wxCheckBox * mGenerateDebrisCheckBox;
    wxCheckBox * mGenerateSparklesCheckBox;
    wxCheckBox * mGenerateAirBubblesCheckBox;
    SliderControl * mAirBubbleDensitySlider;

    // Rendering
    wxRadioButton * mTextureOceanRenderModeRadioButton;
    wxRadioButton * mDepthOceanRenderModeRadioButton;
    wxBitmapComboBox * mTextureOceanComboBox;
    wxColourPickerCtrl * mDepthOceanColorStartPicker;
    wxColourPickerCtrl * mDepthOceanColorEndPicker;
    wxRadioButton * mFlatOceanRenderModeRadioButton;
    wxColourPickerCtrl * mFlatOceanColorPicker;
    wxCheckBox * mSeeShipThroughOceanCheckBox;
    SliderControl * mOceanTransparencySlider;
    SliderControl * mOceanDarkeningRateSlider;
    wxRadioButton * mTextureLandRenderModeRadioButton;
    wxBitmapComboBox * mTextureLandComboBox;
    wxRadioButton * mFlatLandRenderModeRadioButton;
    wxColourPickerCtrl * mFlatLandColorPicker;
    wxColourPickerCtrl * mFlatSkyColorPicker;
    wxRadioButton * mTextureShipRenderModeRadioButton;
    wxRadioButton * mStructureShipRenderModeRadioButton;
    wxCheckBox* mShowStressCheckBox;
    SliderControl * mWaterContrastSlider;
    SliderControl * mWaterLevelOfDetailSlider;
    wxCheckBox * mDrawHeatOverlayCheckBox;
    SliderControl * mHeatOverlayTransparencySlider;
    wxRadioButton * mMode1ShipFlameRenderModeRadioButton;
    wxRadioButton * mMode2ShipFlameRenderModeRadioButton;
    wxRadioButton * mNoDrawShipFlameRenderModeRadioButton;
    wxCheckBox * mDrawHeatBlasterFlameCheckBox;
    SliderControl * mShipFlameSizeAdjustmentSlider;

    // Sound
    SliderControl * mEffectsVolumeSlider;
    SliderControl * mToolsVolumeSlider;
    SliderControl * mMusicVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlayWindSoundCheckBox;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    // Advanced
    SliderControl * mSpringStiffnessSlider;
    SliderControl * mSpringDampingSlider;
    wxRadioBox * mDebugShipRenderModeRadioBox;
    wxRadioBox * mVectorFieldRenderModeRadioBox;

    //////////////////////////////////////////////////////

    // Buttons
    wxButton * mOkButton;
    wxButton * mCancelButton;
    wxButton * mApplyButton;

    // Icons
    std::unique_ptr<wxBitmap> mWarningIcon;

private:

    void PopulateMechanicsFluidsLightsPanel(wxPanel * panel);
    void PopulateHeatPanel(wxPanel * panel);
    void PopulateOceanAndSkyPanel(wxPanel * panel);
    void PopulateWindAndWavesPanel(wxPanel * panel);
    void PopulateInteractionsPanel(wxPanel * panel);
    void PopulateRenderingPanel(wxPanel * panel);
    void PopulateSoundPanel(wxPanel * panel);
    void PopulateAdvancedPanel(wxPanel * panel);

    void ReadSettings();
    void ReconciliateOceanRenderModeSettings();
    void ReconciliateLandRenderModeSettings();

    void ApplySettings();

private:

    wxWindow * const mParent;
    std::shared_ptr<IGameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
};
