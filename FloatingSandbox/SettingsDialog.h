/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SliderControl.h"
#include "SoundController.h"

#include <GameLib/GameController.h>
#include <GameLib/ResourceLoader.h>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/checkbox.h>
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

    void OnSeeShipThroughSeaWaterCheckBoxClick(wxCommandEvent & event);
    void OnShipRenderModeRadioBox(wxCommandEvent & event);
    void OnVectorFieldRenderModeRadioBox(wxCommandEvent & event);
    void OnShowStressCheckBoxClick(wxCommandEvent & event);
    void OnWireframeModeCheckBoxClick(wxCommandEvent & event);

    void OnPlayBreakSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlayStressSoundsCheckBoxClick(wxCommandEvent & event);
    void OnPlaySinkingMusicCheckBoxClick(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);
    void OnApplyButton(wxCommandEvent & event);

private:

    // Controls

    // Mechanics
    std::unique_ptr<SliderControl> mMechanicalQualitySlider;
    std::unique_ptr<SliderControl> mStiffnessSlider;
    std::unique_ptr<SliderControl> mStrengthSlider;

    // Fluids
    std::unique_ptr<SliderControl> mWaterDensitySlider;
    std::unique_ptr<SliderControl> mWaterDragSlider;
    std::unique_ptr<SliderControl> mWaterIntakeSlider;
    std::unique_ptr<SliderControl> mWaterCrazynessSlider;
    std::unique_ptr<SliderControl> mWaterDiffusionSpeedAdjustmentSlider;
    std::unique_ptr<SliderControl> mWaterLevelOfDetailSlider;

    // Sky
    std::unique_ptr<SliderControl> mNumberOfStarsSlider;
    std::unique_ptr<SliderControl> mNumberOfCloudsSlider;
    std::unique_ptr<SliderControl> mWindSpeedSlider;

    // World
    std::unique_ptr<SliderControl> mWaveHeightSlider;
    std::unique_ptr<SliderControl> mSeaDepthSlider;
    std::unique_ptr<SliderControl> mOceanFloorBumpinessSlider;
    std::unique_ptr<SliderControl> mOceanFloorDetailAmplificationSlider;
    std::unique_ptr<SliderControl> mLightDiffusionSlider;

    // Interactions
    std::unique_ptr<SliderControl> mDestroyRadiusSlider;
    std::unique_ptr<SliderControl> mBombBlastRadiusSlider;
    std::unique_ptr<SliderControl> mAntiMatterBombImplosionStrengthSlider;
    wxCheckBox * mUltraViolentCheckBox;
    wxCheckBox * mGenerateDebrisCheckBox;
    wxCheckBox * mGenerateSparklesCheckBox;

    // Rendering
    std::unique_ptr<SliderControl> mSeaWaterTransparencySlider;
    std::unique_ptr<SliderControl> mWaterContrastSlider;
    wxCheckBox * mSeeShipThroughSeaWaterCheckBox;
    wxRadioBox * mShipRenderModeRadioBox;
    wxRadioBox * mVectorFieldRenderModeRadioBox;
    wxCheckBox* mShowStressCheckBox;
    wxCheckBox* mWireframeModeCheckBox;

    // Sound
    std::unique_ptr<SliderControl> mEffectsVolumeSlider;
    std::unique_ptr<SliderControl> mToolsVolumeSlider;
    std::unique_ptr<SliderControl> mMusicVolumeSlider;
    wxCheckBox * mPlayBreakSoundsCheckBox;
    wxCheckBox * mPlayStressSoundsCheckBox;
    wxCheckBox * mPlaySinkingMusicCheckBox;

    wxButton * mOkButton;
    wxButton * mCancelButton;
    wxButton * mApplyButton;

    // Icons
    std::unique_ptr<wxBitmap> mWarningIcon;

private:

    void PopulateMechanicsPanel(wxPanel * panel);
    void PopulateFluidsPanel(wxPanel * panel);
    void PopulateSkyPanel(wxPanel * panel);
    void PopulateWorldPanel(wxPanel * panel);
    void PopulateInteractionsPanel(wxPanel * panel);
    void PopulateRenderingPanel(wxPanel * panel);
    void PopulateSoundPanel(wxPanel * panel);

    void ReadSettings();

    void ApplySettings();

private:

    wxWindow * const mParent;
    std::shared_ptr<GameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
};
