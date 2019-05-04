/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SliderControl.h"
#include "SoundController.h"

#include <Game/GameController.h>
#include <Game/ResourceLoader.h>

#include <wx/bitmap.h>
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
        std::shared_ptr<GameController> gameController,
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
    void OnDepthOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnDepthOceanColorStartChanged(wxColourPickerEvent & event);
    void OnDepthOceanColorEndChanged(wxColourPickerEvent & event);
    void OnFlatOceanRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnFlatOceanColorChanged(wxColourPickerEvent & event);
    void OnSeeShipThroughOceanCheckBoxClick(wxCommandEvent & event);

    void OnTextureLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnFlatLandRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnFlatLandColorChanged(wxColourPickerEvent & event);

    void OnFlatSkyColorChanged(wxColourPickerEvent & event);

    void OnTextureShipRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnStructureShipRenderModeRadioButtonClick(wxCommandEvent & event);
    void OnShowStressCheckBoxClick(wxCommandEvent & event);

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

    // Mechanics
    std::unique_ptr<SliderControl> mMechanicalQualitySlider;
    std::unique_ptr<SliderControl> mStrengthSlider;

    // Fluids
    std::unique_ptr<SliderControl> mWaterDensitySlider;
    std::unique_ptr<SliderControl> mWaterDragSlider;
    std::unique_ptr<SliderControl> mWaterIntakeSlider;
    std::unique_ptr<SliderControl> mWaterCrazynessSlider;
    std::unique_ptr<SliderControl> mWaterDiffusionSpeedSlider;
    std::unique_ptr<SliderControl> mWaterLevelOfDetailSlider;

    // Sky
    std::unique_ptr<SliderControl> mNumberOfStarsSlider;
    std::unique_ptr<SliderControl> mNumberOfCloudsSlider;

    // Wind and Waves
    std::unique_ptr<SliderControl> mWindSpeedBaseSlider;
    wxCheckBox* mModulateWindCheckBox;
    std::unique_ptr<SliderControl> mWindGustAmplitudeSlider;
    std::unique_ptr<SliderControl> mBasalWaveHeightAdjustmentSlider;
    std::unique_ptr<SliderControl> mBasalWaveLengthAdjustmentSlider;
    std::unique_ptr<SliderControl> mBasalWaveSpeedAdjustmentSlider;

    // World
    std::unique_ptr<SliderControl> mSeaDepthSlider;
    std::unique_ptr<SliderControl> mOceanFloorBumpinessSlider;
    std::unique_ptr<SliderControl> mOceanFloorDetailAmplificationSlider;
    std::unique_ptr<SliderControl> mLuminiscenceSlider;
    std::unique_ptr<SliderControl> mLightSpreadSlider;
    std::unique_ptr<SliderControl> mRotAcceler8rSlider;

    // Interactions
    std::unique_ptr<SliderControl> mDestroyRadiusSlider;
    std::unique_ptr<SliderControl> mBombBlastRadiusSlider;
    std::unique_ptr<SliderControl> mAntiMatterBombImplosionStrengthSlider;
    std::unique_ptr<SliderControl> mFloodRadiusSlider;
    std::unique_ptr<SliderControl> mFloodQuantitySlider;
    wxCheckBox * mUltraViolentCheckBox;
    wxCheckBox * mGenerateDebrisCheckBox;
    wxCheckBox * mGenerateSparklesCheckBox;
    wxCheckBox * mGenerateAirBubblesCheckBox;

    // Rendering
    wxRadioButton * mTextureOceanRenderModeRadioButton;
    wxRadioButton * mDepthOceanRenderModeRadioButton;
    wxColourPickerCtrl * mDepthOceanColorStartPicker;
    wxColourPickerCtrl * mDepthOceanColorEndPicker;
    wxRadioButton * mFlatOceanRenderModeRadioButton;
    wxColourPickerCtrl * mFlatOceanColorPicker;
    wxCheckBox * mSeeShipThroughOceanCheckBox;
    std::unique_ptr<SliderControl> mOceanTransparencySlider;
    wxRadioButton * mTextureLandRenderModeRadioButton;
    wxRadioButton * mFlatLandRenderModeRadioButton;
    wxColourPickerCtrl * mFlatLandColorPicker;
    wxColourPickerCtrl * mFlatSkyColorPicker;
    wxRadioButton * mTextureShipRenderModeRadioButton;
    wxRadioButton * mStructureShipRenderModeRadioButton;
    wxCheckBox* mShowStressCheckBox;
    std::unique_ptr<SliderControl> mWaterContrastSlider;

    // Sound
    std::unique_ptr<SliderControl> mEffectsVolumeSlider;
    std::unique_ptr<SliderControl> mToolsVolumeSlider;
    std::unique_ptr<SliderControl> mMusicVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlayWindSoundCheckBox;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    // Advanced
    std::unique_ptr<SliderControl> mSpringStiffnessSlider;
    std::unique_ptr<SliderControl> mSpringDampingSlider;
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

    void PopulateMechanicsPanel(wxPanel * panel);
    void PopulateFluidsPanel(wxPanel * panel);
    void PopulateSkyPanel(wxPanel * panel);
    void PopulateWindAndWavesPanel(wxPanel * panel);
    void PopulateWorldPanel(wxPanel * panel);
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
    std::shared_ptr<GameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
};
