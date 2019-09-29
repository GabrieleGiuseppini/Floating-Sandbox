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
    SliderControl<float> * mMechanicalQualitySlider;
    SliderControl<float> * mStrengthSlider;
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

    // Ocean and Sky
    SliderControl<float> * mOceanDepthSlider;
    SliderControl<float> * mOceanFloorBumpinessSlider;
    SliderControl<float> * mOceanFloorDetailAmplificationSlider;
    SliderControl<float> * mNumberOfStarsSlider;
    SliderControl<float> * mNumberOfCloudsSlider;

    // Wind and Waves
    SliderControl<float> * mWindSpeedBaseSlider;
    wxCheckBox* mModulateWindCheckBox;
    SliderControl<float> * mWindGustAmplitudeSlider;
    SliderControl<float> * mBasalWaveHeightAdjustmentSlider;
    SliderControl<float> * mBasalWaveLengthAdjustmentSlider;
    SliderControl<float> * mBasalWaveSpeedAdjustmentSlider;
    SliderControl<float> * mTsunamiRateSlider;
    SliderControl<float> * mRogueWaveRateSlider;

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
    wxCheckBox * mGenerateSparklesCheckBox;
    wxCheckBox * mGenerateAirBubblesCheckBox;
    SliderControl<float> * mAirBubbleDensitySlider;

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
    SliderControl<float> * mWaterContrastSlider;
    SliderControl<float> * mWaterLevelOfDetailSlider;
    wxCheckBox * mDrawHeatOverlayCheckBox;
    SliderControl<float> * mHeatOverlayTransparencySlider;
    wxRadioButton * mMode1ShipFlameRenderModeRadioButton;
    wxRadioButton * mMode2ShipFlameRenderModeRadioButton;
    wxRadioButton * mNoDrawShipFlameRenderModeRadioButton;
    wxCheckBox * mDrawHeatBlasterFlameCheckBox;
    SliderControl<float> * mShipFlameSizeAdjustmentSlider;

    // Sound
    SliderControl<float> * mEffectsVolumeSlider;
    SliderControl<float> * mToolsVolumeSlider;
    SliderControl<float> * mMusicVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlayWindSoundCheckBox;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    // Advanced
    SliderControl<float> * mSpringStiffnessSlider;
    SliderControl<float> * mSpringDampingSlider;
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
