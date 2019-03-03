/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SettingsDialog.h"

#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/FixedTickSliderCore.h>
#include <GameCore/LinearSliderCore.h>
#include <GameCore/Log.h>

#include <wx/gbsizer.h>
#include <wx/intl.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

static constexpr int SliderWidth = 40;
static constexpr int SliderHeight = 140;
static constexpr int SliderBorder = 10;

const long ID_ULTRA_VIOLENT_CHECKBOX = wxNewId();
const long ID_GENERATE_DEBRIS_CHECKBOX = wxNewId();
const long ID_GENERATE_SPARKLES_CHECKBOX = wxNewId();
const long ID_GENERATE_AIR_BUBBLES_CHECKBOX = wxNewId();
const long ID_SCREENSHOT_DIR_PICKER = wxNewId();
const long ID_MODULATE_WIND_CHECKBOX = wxNewId();
const long ID_SEE_SHIP_THROUGH_SEA_WATER_CHECKBOX = wxNewId();
const long ID_SHOW_STRESS_CHECKBOX = wxNewId();
const long ID_PLAY_BREAK_SOUNDS_CHECKBOX = wxNewId();
const long ID_PLAY_STRESS_SOUNDS_CHECKBOX = wxNewId();
const long ID_PLAY_WIND_SOUND_CHECKBOX = wxNewId();
const long ID_PLAY_SINKING_MUSIC_CHECKBOX = wxNewId();

SettingsDialog::SettingsDialog(
    wxWindow* parent,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::shared_ptr< UISettings> uiSettings,
    ResourceLoader const & resourceLoader)
    : mParent(parent)
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
    , mUISettings(std::move(uiSettings))
{
    Create(
        mParent,
        wxID_ANY,
        _("Settings"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED,
        _T("Settings Window"));


    //
    // Load icons
    //

    mWarningIcon = std::make_unique<wxBitmap>(
        resourceLoader.GetIconFilepath("warning_icon").string(),
        wxBITMAP_TYPE_PNG);



    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);


    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);


    //
    // Mechanics
    //

    wxPanel * mechanicsPanel = new wxPanel(notebook);

    mechanicsPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateMechanicsPanel(mechanicsPanel);

    notebook->AddPage(mechanicsPanel, "Mechanics");


    //
    // Fluids
    //

    wxPanel * fluidsPanel = new wxPanel(notebook);

    fluidsPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateFluidsPanel(fluidsPanel);

    notebook->AddPage(fluidsPanel, "Fluids");


    //
    // Sky
    //

    wxPanel * skyPanel = new wxPanel(notebook);

    skyPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateSkyPanel(skyPanel);

    notebook->AddPage(skyPanel, "Sky");


    //
    // World
    //

    wxPanel * worldPanel = new wxPanel(notebook);

    worldPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateWorldPanel(worldPanel);

    notebook->AddPage(worldPanel, "World");


    //
    // Interactions
    //

    wxPanel * interactionsPanel = new wxPanel(notebook);

    interactionsPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateInteractionsPanel(interactionsPanel);

    notebook->AddPage(interactionsPanel, "Interactions");


    //
    // Rendering
    //

    wxPanel * renderingPanel = new wxPanel(notebook);

    renderingPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateRenderingPanel(renderingPanel);

    notebook->AddPage(renderingPanel, "Rendering");


    //
    // Sound
    //

    wxPanel * soundPanel = new wxPanel(notebook);

    soundPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateSoundPanel(soundPanel);

    notebook->AddPage(soundPanel, "Sound");



    //
    // Advanced
    //

    wxPanel * advancedPanel = new wxPanel(notebook);

    advancedPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateAdvancedPanel(advancedPanel);

    notebook->AddPage(advancedPanel, "Advanced");



    dialogVSizer->Add(notebook, 0, wxEXPAND);

    dialogVSizer->AddSpacer(20);


    // Buttons

    wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonsSizer->AddSpacer(20);

    mOkButton = new wxButton(this, wxID_OK);
    Connect(wxID_OK, wxEVT_BUTTON, (wxObjectEventFunction)&SettingsDialog::OnOkButton);
    buttonsSizer->Add(mOkButton, 0);

    buttonsSizer->AddSpacer(20);

    mCancelButton = new wxButton(this, wxID_CANCEL);
    buttonsSizer->Add(mCancelButton, 0);

    buttonsSizer->AddSpacer(20);

    mApplyButton = new wxButton(this, wxID_APPLY);
    mApplyButton->Enable(false);
    Connect(wxID_APPLY, wxEVT_BUTTON, (wxObjectEventFunction)&SettingsDialog::OnApplyButton);
    buttonsSizer->Add(mApplyButton, 0);

    buttonsSizer->AddSpacer(20);

    dialogVSizer->Add(buttonsSizer, 0, wxALIGN_RIGHT);

    dialogVSizer->AddSpacer(20);



    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::Open()
{
    assert(!!mGameController);

    ReadSettings();

    // We're not dirty
    mApplyButton->Enable(false);

    this->Show();
}

void SettingsDialog::OnSeeShipThroughSeaWaterCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnUltraViolentCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnGenerateDebrisCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnGenerateSparklesCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnGenerateAirBubblesCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnScreenshotDirPickerChanged(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnModulateWindCheckBoxClick(wxCommandEvent & /*event*/)
{
    mWindGustAmplitudeSlider->Enable(mModulateWindCheckBox->IsChecked());

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnShipRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnDebugShipRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnVectorFieldRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnShowStressCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnPlayBreakSoundsCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnPlayStressSoundsCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnPlayWindSoundCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnPlaySinkingMusicCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    // Write settings back to controller
    ApplySettings();

    // Close ourselves
    this->Close();
}

void SettingsDialog::OnApplyButton(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    // Write settings back to controller
    ApplySettings();

    // We're not dirty anymore
    mApplyButton->Enable(false);
}

void SettingsDialog::ApplySettings()
{
    assert(!!mGameController);


    mGameController->SetNumMechanicalDynamicsIterationsAdjustment(
        mMechanicalQualitySlider->GetValue());

    mGameController->SetStrengthAdjustment(
        mStrengthSlider->GetValue());



    mGameController->SetWaterDensityAdjustment(
        mWaterDensitySlider->GetValue());

    mGameController->SetWaterDragAdjustment(
        mWaterDragSlider->GetValue());

    mGameController->SetWaterIntakeAdjustment(
        mWaterIntakeSlider->GetValue());

    mGameController->SetWaterCrazyness(
        mWaterCrazynessSlider->GetValue());

    mGameController->SetWaterDiffusionSpeedAdjustment(
        mWaterDiffusionSpeedSlider->GetValue());

    mGameController->SetWaterLevelOfDetail(
        mWaterLevelOfDetailSlider->GetValue());



    mGameController->SetNumberOfStars(
        static_cast<size_t>(mNumberOfStarsSlider->GetValue()));

    mGameController->SetNumberOfClouds(
        static_cast<size_t>(mNumberOfCloudsSlider->GetValue()));

    mGameController->SetWindSpeedBase(
        mWindSpeedBaseSlider->GetValue());

    mGameController->SetDoModulateWind(mModulateWindCheckBox->IsChecked());

    mGameController->SetWindSpeedMaxFactor(
        mWindGustAmplitudeSlider->GetValue());



    mGameController->SetWaveHeight(
        mWaveHeightSlider->GetValue());

    mGameController->SetSeaDepth(
        mSeaDepthSlider->GetValue());

    mGameController->SetOceanFloorBumpiness(
        mOceanFloorBumpinessSlider->GetValue());

    mGameController->SetOceanFloorDetailAmplification(
        mOceanFloorDetailAmplificationSlider->GetValue());

    mGameController->SetLuminiscenceAdjustment(
        mLuminiscenceSlider->GetValue());

    mGameController->SetLightSpreadAdjustment(
        mLightSpreadSlider->GetValue());



    mGameController->SetDestroyRadius(
        mDestroyRadiusSlider->GetValue());

    mGameController->SetBombBlastRadius(
        mBombBlastRadiusSlider->GetValue());

    mGameController->SetAntiMatterBombImplosionStrength(
        mAntiMatterBombImplosionStrengthSlider->GetValue());

    mGameController->SetFloodRadius(
        mFloodRadiusSlider->GetValue());

    mGameController->SetFloodQuantity(
        mFloodQuantitySlider->GetValue());

    mGameController->SetUltraViolentMode(mUltraViolentCheckBox->IsChecked());

    mGameController->SetDoGenerateDebris(mGenerateDebrisCheckBox->IsChecked());

    mGameController->SetDoGenerateSparkles(mGenerateSparklesCheckBox->IsChecked());

    mGameController->SetDoGenerateAirBubbles(mGenerateAirBubblesCheckBox->IsChecked());

    mUISettings->SetScreenshotsFolderPath(mScreenshotDirPickerCtrl->GetPath().ToStdString());



    mGameController->SetWaterContrast(
        mWaterContrastSlider->GetValue());

    mGameController->SetSeaWaterTransparency(
        mSeaWaterTransparencySlider->GetValue());

    mGameController->SetShowShipThroughSeaWater(mSeeShipThroughSeaWaterCheckBox->IsChecked());

    auto selectedShipRenderMode = mShipRenderModeRadioBox->GetSelection();
    if (0 == selectedShipRenderMode)
    {
        mGameController->SetShipRenderMode(ShipRenderMode::Structure);
    }
    else
    {
        assert(1 == selectedShipRenderMode);
        mGameController->SetShipRenderMode(ShipRenderMode::Texture);
    }

    mGameController->SetShowShipStress(mShowStressCheckBox->IsChecked());




    mSoundController->SetMasterEffectsVolume(
        mEffectsVolumeSlider->GetValue());

    mSoundController->SetMasterToolsVolume(
        mToolsVolumeSlider->GetValue());

    mSoundController->SetMasterMusicVolume(
        mMusicVolumeSlider->GetValue());

    mSoundController->SetPlayBreakSounds(mPlayBreakSoundsCheckBox->IsChecked());

    mSoundController->SetPlayStressSounds(mPlayStressSoundsCheckBox->IsChecked());

    mSoundController->SetPlayWindSound(mPlayWindSoundCheckBox->IsChecked());

    mSoundController->SetPlaySinkingMusic(mPlaySinkingMusicCheckBox->IsChecked());



    mGameController->SetStiffnessAdjustment(
        mStiffnessSlider->GetValue());

    auto selectedDebugShipRenderMode = mDebugShipRenderModeRadioBox->GetSelection();
    if (0 == selectedDebugShipRenderMode)
    {
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::None);
    }
    else if (1 == selectedDebugShipRenderMode)
    {
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::Wireframe);
    }
    else if (2 == selectedDebugShipRenderMode)
    {
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::Points);
    }
    else if (3 == selectedDebugShipRenderMode)
    {
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::Springs);
    }
    else
    {
        assert(4 == selectedDebugShipRenderMode);
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::EdgeSprings);
    }

    auto selectedVectorFieldRenderMode = mVectorFieldRenderModeRadioBox->GetSelection();
    switch (selectedVectorFieldRenderMode)
    {
        case 0:
        {
            mGameController->SetVectorFieldRenderMode(VectorFieldRenderMode::None);
            break;
        }

        case 1:
        {
            mGameController->SetVectorFieldRenderMode(VectorFieldRenderMode::PointVelocity);
            break;
        }

        case 2:
        {
            mGameController->SetVectorFieldRenderMode(VectorFieldRenderMode::PointForce);
            break;
        }

        case 3:
        {
            mGameController->SetVectorFieldRenderMode(VectorFieldRenderMode::PointWaterVelocity);
            break;
        }

        default:
        {
            assert(4 == selectedVectorFieldRenderMode);
            mGameController->SetVectorFieldRenderMode(VectorFieldRenderMode::PointWaterMomentum);
            break;
        }
    }
}

void SettingsDialog::PopulateMechanicsPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);


    // Simulation quality

    mMechanicalQualitySlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Simulation Quality",
        "Higher values improve the rigidity of simulated structures, at the expense of longer computation times.",
        mGameController->GetNumMechanicalDynamicsIterationsAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<FixedTickSliderCore>(
            0.5f,
            mGameController->GetMinNumMechanicalDynamicsIterationsAdjustment(),
            mGameController->GetMaxNumMechanicalDynamicsIterationsAdjustment()),
        mWarningIcon.get());

    controlsSizer->Add(mMechanicalQualitySlider.get(), 1, wxALL, SliderBorder);



    // Strength

    mStrengthSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Strength Adjust",
        "Adjusts the strength of springs.",
        mGameController->GetStrengthAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinStrengthAdjustment(),
            1.0f,
            mGameController->GetMaxStrengthAdjustment()));

    controlsSizer->Add(mStrengthSlider.get(), 1, wxALL, SliderBorder);



    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::PopulateFluidsPanel(wxPanel * panel)
{
    wxGridSizer* gridSizer = new wxGridSizer(2, 4, 0, 0);

    //
    // Row 1
    //

    // Water Density

    mWaterDensitySlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Density Adjust",
        "Adjusts the density of sea water.",
        mGameController->GetWaterDensityAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterDensityAdjustment(),
            mGameController->GetMaxWaterDensityAdjustment()));

    gridSizer->Add(mWaterDensitySlider.get(), 1, wxALL, SliderBorder);


    // Water Drag

    mWaterDragSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Drag Adjust",
        "Adjusts the drag force exerted by sea water on physical bodies.",
        mGameController->GetWaterDragAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinWaterDragAdjustment(),
            1.0f,
            mGameController->GetMaxWaterDragAdjustment()));

    gridSizer->Add(mWaterDragSlider.get(), 1, wxALL, SliderBorder);


    // Water Intake

    mWaterIntakeSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Intake Adjust",
        "Adjusts the speed with which sea water enters or leaves a physical body.",
        mGameController->GetWaterIntakeAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterIntakeAdjustment(),
            mGameController->GetMaxWaterIntakeAdjustment()));

    gridSizer->Add(mWaterIntakeSlider.get(), 1, wxALL, SliderBorder);


    // Water Crazyness

    mWaterCrazynessSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Crazyness",
        "Adjusts how \"splashy\" water flows inside a physical body.",
        mGameController->GetWaterCrazyness(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterCrazyness(),
            mGameController->GetMaxWaterCrazyness()));

    gridSizer->Add(mWaterCrazynessSlider.get(), 1, wxALL, SliderBorder);



    //
    // Row 2
    //

    // Water Diffusion Speed

    mWaterDiffusionSpeedSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Diffusion Speed",
        "Adjusts the speed with which water propagates within a physical body.",
        mGameController->GetWaterDiffusionSpeedAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterDiffusionSpeedAdjustment(),
            mGameController->GetMaxWaterDiffusionSpeedAdjustment()));

    gridSizer->Add(mWaterDiffusionSpeedSlider.get(), 1, wxALL, SliderBorder);


    // Water Level of Detail

    mWaterLevelOfDetailSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Level of Detail",
        "Adjusts how detailed water inside a physical body looks.",
        mGameController->GetWaterLevelOfDetail(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterLevelOfDetail(),
            mGameController->GetMaxWaterLevelOfDetail()));

    gridSizer->Add(mWaterLevelOfDetailSlider.get(), 1, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateSkyPanel(wxPanel * panel)
{
    wxGridSizer* gridSizer = new wxGridSizer(2, 4, 0, 0);

    //
    // Row 1
    //


    // Number of Stars

    mNumberOfStarsSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Number of Stars",
        "The number of stars in the sky.",
        static_cast<float>(mGameController->GetNumberOfStars()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            static_cast<float>(mGameController->GetMinNumberOfStars()),
            static_cast<float>(mGameController->GetMaxNumberOfStars())));

    gridSizer->Add(mNumberOfStarsSlider.get(), 1, wxALL, SliderBorder);


    // Number of Clouds

    mNumberOfCloudsSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Number of Clouds",
        "The number of clouds in the sky.",
        static_cast<float>(mGameController->GetNumberOfClouds()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            static_cast<float>(mGameController->GetMinNumberOfClouds()),
            static_cast<float>(mGameController->GetMaxNumberOfClouds())));

    gridSizer->Add(mNumberOfCloudsSlider.get(), 1, wxALL, SliderBorder);


    // Wind Speed Base

    mWindSpeedBaseSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Wind Speed Base",
        "The base speed of wind (Km/h), before modulation takes place.",
        mGameController->GetWindSpeedBase(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWindSpeedBase(),
            mGameController->GetMaxWindSpeedBase()));

    gridSizer->Add(mWindSpeedBaseSlider.get(), 1, wxALL, SliderBorder);


    // Wind Gust Amplitude

    mWindGustAmplitudeSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Wind Gust Amplitude",
        "The amplitude of wind gusts, as a multiplier of the base wind speed.",
        mGameController->GetWindSpeedMaxFactor(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWindSpeedMaxFactor(),
            mGameController->GetMaxWindSpeedMaxFactor()));

    gridSizer->Add(mWindGustAmplitudeSlider.get(), 1, wxALL, SliderBorder);


    //
    // Row 2
    //

    gridSizer->AddSpacer(0);
    gridSizer->AddSpacer(0);
    gridSizer->AddSpacer(0);


    mModulateWindCheckBox = new wxCheckBox(panel, ID_MODULATE_WIND_CHECKBOX, _("Modulate Wind"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Modulate Wind Checkbox"));
    mModulateWindCheckBox->SetToolTip("Enables or disables simulation of wind variations, alternating between dead calm and high-speed gusts.");

    Connect(ID_MODULATE_WIND_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnModulateWindCheckBoxClick);

    gridSizer->Add(mModulateWindCheckBox, 0, wxALL | wxALIGN_TOP, 0);



    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateWorldPanel(wxPanel * panel)
{
    wxGridSizer* gridSizer = new wxGridSizer(2, 4, 0, 0);

    //
    // Row 1
    //

    // Wave Height

    mWaveHeightSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Wave Height",
        "The height of sea water waves (m).",
        mGameController->GetWaveHeight(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaveHeight(),
            mGameController->GetMaxWaveHeight()));

    gridSizer->Add(mWaveHeightSlider.get(), 1, wxALL, SliderBorder);


    // Sea Depth

    mSeaDepthSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Ocean Depth",
        "The ocean depth (m).",
        mGameController->GetSeaDepth(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinSeaDepth(),
            300.0f,
            mGameController->GetMaxSeaDepth()));

    gridSizer->Add(mSeaDepthSlider.get(), 1, wxALL, SliderBorder);


    // Ocean Floor Bumpiness

    mOceanFloorBumpinessSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Ocean Floor Bumpiness",
        "Adjusts how much the ocean floor rolls up and down.",
        mGameController->GetOceanFloorBumpiness(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinOceanFloorBumpiness(),
            mGameController->GetMaxOceanFloorBumpiness()));

    gridSizer->Add(mOceanFloorBumpinessSlider.get(), 1, wxALL, SliderBorder);


    // Ocean Floor Detail Amplification

    mOceanFloorDetailAmplificationSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Ocean Floor Detail",
        "Adjusts the jaggedness of the ocean floor irregularities.",
        mGameController->GetOceanFloorDetailAmplification(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinOceanFloorDetailAmplification(),
            10.0f,
            mGameController->GetMaxOceanFloorDetailAmplification()));

    gridSizer->Add(mOceanFloorDetailAmplificationSlider.get(), 1, wxALL, SliderBorder);


    //
    // Row 2
    //

    // Luminiscence

    mLuminiscenceSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Luminiscence Adjust",
        "Adjusts how much light is emitted by luminiscent materials.",
        mGameController->GetLuminiscenceAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinLuminiscenceAdjustment(),
            mGameController->GetMaxLuminiscenceAdjustment()));

    gridSizer->Add(mLuminiscenceSlider.get(), 1, wxALL, SliderBorder);

    // Light Spread

    mLightSpreadSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Light Spread Adjust",
        "Adjusts how wide light emitted by luminiscent materials spreads out.",
        mGameController->GetLightSpreadAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinLightSpreadAdjustment(),
            mGameController->GetMaxLightSpreadAdjustment()));

    gridSizer->Add(mLightSpreadSlider.get(), 1, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateInteractionsPanel(wxPanel * panel)
{
    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);


    //
    // Row 1
    //

    // Destroy Radius

    mDestroyRadiusSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Destroy Radius",
        "The starting radius of the damage caused by destructive tools (m).",
        mGameController->GetDestroyRadius(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinDestroyRadius(),
            mGameController->GetMaxDestroyRadius()));

    gridSizer->Add(
        mDestroyRadiusSlider.get(),
        wxGBPosition(0, 0),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);


    // Bomb Blast Radius

    mBombBlastRadiusSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Bomb Blast Radius",
        "The radius of bomb explosions (m).",
        mGameController->GetBombBlastRadius(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinBombBlastRadius(),
            mGameController->GetMaxBombBlastRadius()));

    gridSizer->Add(
        mBombBlastRadiusSlider.get(),
        wxGBPosition(0, 1),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);


    // Anti-matter Bomb Implosion Strength

    mAntiMatterBombImplosionStrengthSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "AM Bomb Implosion Strength",
        "Adjusts the strength of the initial anti-matter bomb implosion.",
        mGameController->GetAntiMatterBombImplosionStrength(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinAntiMatterBombImplosionStrength(),
            mGameController->GetMaxAntiMatterBombImplosionStrength()));

    gridSizer->Add(
        mAntiMatterBombImplosionStrengthSlider.get(),
        wxGBPosition(0, 2),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);

    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    mUltraViolentCheckBox = new wxCheckBox(panel, ID_ULTRA_VIOLENT_CHECKBOX, _("Ultra-Violent Mode"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Ultra-Violent Mode Checkbox"));
    mUltraViolentCheckBox->SetToolTip("Enables or disables amplification of tool forces and inflicted damages.");
    Connect(ID_ULTRA_VIOLENT_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnUltraViolentCheckBoxClick);
    checkboxesSizer->Add(mUltraViolentCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mGenerateDebrisCheckBox = new wxCheckBox(panel, ID_GENERATE_DEBRIS_CHECKBOX, _("Generate Debris"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Generate Debris Checkbox"));
    mGenerateDebrisCheckBox->SetToolTip("Enables or disables generation of debris when using destructive tools.");
    Connect(ID_GENERATE_DEBRIS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnGenerateDebrisCheckBoxClick);
    checkboxesSizer->Add(mGenerateDebrisCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mGenerateSparklesCheckBox = new wxCheckBox(panel, ID_GENERATE_SPARKLES_CHECKBOX, _("Generate Sparkles"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Generate Sparkles Checkbox"));
    mGenerateSparklesCheckBox->SetToolTip("Enables or disables generation of sparkles when using the saw tool on metal.");
    Connect(ID_GENERATE_SPARKLES_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnGenerateSparklesCheckBoxClick);
    checkboxesSizer->Add(mGenerateSparklesCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mGenerateAirBubblesCheckBox = new wxCheckBox(panel, ID_GENERATE_AIR_BUBBLES_CHECKBOX, _("Generate Air Bubbles"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Generate Air Bubbles Checkbox"));
    mGenerateAirBubblesCheckBox->SetToolTip("Enables or disables generation of air bubbles when water enters a physical body.");
    Connect(ID_GENERATE_AIR_BUBBLES_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnGenerateAirBubblesCheckBoxClick);
    checkboxesSizer->Add(mGenerateAirBubblesCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    gridSizer->Add(
        checkboxesSizer,
        wxGBPosition(0, 3),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);

    //
    // Row 2
    //

    // Flood Radius

    mFloodRadiusSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Flood Radius",
        "How wide an area is flooded by the flood tool (m).",
        mGameController->GetFloodRadius(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinFloodRadius(),
            mGameController->GetMaxFloodRadius()));

    gridSizer->Add(
        mFloodRadiusSlider.get(),
        wxGBPosition(1, 0),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);

    // Flood Quantity

    mFloodQuantitySlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Flood Quantity",
        "How much water is injected by the flood tool (m3).",
        mGameController->GetFloodQuantity(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinFloodQuantity(),
            mGameController->GetMaxFloodQuantity()));

    gridSizer->Add(
        mFloodQuantitySlider.get(),
        wxGBPosition(1, 1),
        wxGBSpan(1, 1),
        wxALL,
        SliderBorder);

    //
    // Row 3
    //

    wxBoxSizer* screenshotDirSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText * screenshotDirStaticText = new wxStaticText(panel, wxID_ANY, "Screenshot directory:", wxDefaultPosition, wxDefaultSize, 0);
    screenshotDirSizer->Add(screenshotDirStaticText, 1, wxALIGN_LEFT | wxEXPAND, 0);

    mScreenshotDirPickerCtrl = new wxDirPickerCtrl(
        panel,
        ID_SCREENSHOT_DIR_PICKER,
        _T(""),
        _("Select directory that screenshots will be saved to:"),
        wxDefaultPosition,
        wxSize(-1, -1),
        wxDIRP_DIR_MUST_EXIST | wxDIRP_USE_TEXTCTRL);
    Connect(ID_SCREENSHOT_DIR_PICKER, wxEVT_DIRPICKER_CHANGED, (wxObjectEventFunction)&SettingsDialog::OnScreenshotDirPickerChanged);
    screenshotDirSizer->Add(mScreenshotDirPickerCtrl, 1, wxALIGN_LEFT | wxEXPAND, 0);

    gridSizer->Add(
        screenshotDirSizer,
        wxGBPosition(2, 0),
        wxGBSpan(1, 4), // Take entire row
        wxALL | wxEXPAND,
        SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);


    // Water Contrast

    mWaterContrastSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Water Contrast",
        "Adjusts the contrast of water inside physical bodies.",
        mGameController->GetWaterContrast(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            1.0f));

    controlsSizer->Add(mWaterContrastSlider.get(), 1, wxALL, SliderBorder);


    // Sea Water Transparency

    mSeaWaterTransparencySlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Sea Water Transparency",
        "Adjusts the transparency of sea water.",
        mGameController->GetSeaWaterTransparency(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            1.0f));

    controlsSizer->Add(mSeaWaterTransparencySlider.get(), 1, wxALL, SliderBorder);


    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);


    mSeeShipThroughSeaWaterCheckBox = new wxCheckBox(panel, ID_SEE_SHIP_THROUGH_SEA_WATER_CHECKBOX, _("See Ship Through Water"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T(""));
    Connect(ID_SEE_SHIP_THROUGH_SEA_WATER_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnSeeShipThroughSeaWaterCheckBoxClick);

    checkboxesSizer->Add(mSeeShipThroughSeaWaterCheckBox, 0, wxALL | wxALIGN_LEFT, 5);


    wxString shipRenderModeChoices[] =
    {
        _("Draw Structure"),
        _("Draw Image")
    };

    mShipRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Ship Draw Options"), wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(shipRenderModeChoices), shipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    Connect(mShipRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnShipRenderModeRadioBox);

    checkboxesSizer->Add(mShipRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);



    mShowStressCheckBox = new wxCheckBox(panel, ID_SHOW_STRESS_CHECKBOX, _("Show Stress"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Show Stress Checkbox"));
    mShowStressCheckBox->SetToolTip("Enables or disables highlighting of the springs that are under heavy stress and close to rupture.");
    Connect(ID_SHOW_STRESS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnShowStressCheckBoxClick);

    checkboxesSizer->Add(mShowStressCheckBox, 0, wxALL | wxALIGN_LEFT, 5);


    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::PopulateSoundPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

    // Effects volume

    mEffectsVolumeSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Effects Volume",
        "Adjusts the volume of sounds generated by the simulation.",
        mSoundController->GetMasterEffectsVolume(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mEffectsVolumeSlider.get(), 1, wxALL, SliderBorder);


    // Tools volume

    mToolsVolumeSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Tools Volume",
        "Adjusts the volume of sounds generated by interactive tools.",
        mSoundController->GetMasterToolsVolume(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mToolsVolumeSlider.get(), 1, wxALL, SliderBorder);


    // Music volume

    mMusicVolumeSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Music Volume",
        "Adjusts the volume of music.",
        mSoundController->GetMasterMusicVolume(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mMusicVolumeSlider.get(), 1, wxALL, SliderBorder);


    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    mPlayBreakSoundsCheckBox = new wxCheckBox(panel, ID_PLAY_BREAK_SOUNDS_CHECKBOX, _("Play Break Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Break Sounds Checkbox"));
    mPlayBreakSoundsCheckBox->SetToolTip("Enables or disables the generation of sounds when materials break.");
    Connect(ID_PLAY_BREAK_SOUNDS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlayBreakSoundsCheckBoxClick);
    checkboxesSizer->Add(mPlayBreakSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlayStressSoundsCheckBox = new wxCheckBox(panel, ID_PLAY_STRESS_SOUNDS_CHECKBOX, _("Play Stress Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Stress Sounds Checkbox"));
    mPlayStressSoundsCheckBox->SetToolTip("Enables or disables the generation of sounds when materials are under stress.");
    Connect(ID_PLAY_STRESS_SOUNDS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlayStressSoundsCheckBoxClick);
    checkboxesSizer->Add(mPlayStressSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlayWindSoundCheckBox = new wxCheckBox(panel, ID_PLAY_WIND_SOUND_CHECKBOX, _("Play Wind Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Wind Sound Checkbox"));
    mPlayWindSoundCheckBox->SetToolTip("Enables or disables the generation of wind sounds.");
    Connect(ID_PLAY_WIND_SOUND_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlayWindSoundCheckBoxClick);
    checkboxesSizer->Add(mPlayWindSoundCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlaySinkingMusicCheckBox = new wxCheckBox(panel, ID_PLAY_SINKING_MUSIC_CHECKBOX, _("Play Farewell Music"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Sinking Music Checkbox"));
    mPlaySinkingMusicCheckBox->SetToolTip("Enables or disables playing \"Nearer My God to Thee\" when a ship starts sinking.");
    Connect(ID_PLAY_SINKING_MUSIC_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlaySinkingMusicCheckBoxClick);
    checkboxesSizer->Add(mPlaySinkingMusicCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::PopulateAdvancedPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

    // Stiffness

    mStiffnessSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Spring Stiffness Adjust",
        "This setting is for testing physical instability of the mass-spring network with high stiffness values;"
        " it is not meant for improving the rigidity of physical bodies.",
        mGameController->GetStiffnessAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinStiffnessAdjustment(),
            mGameController->GetMaxStiffnessAdjustment()),
        mWarningIcon.get());

    controlsSizer->Add(mStiffnessSlider.get(), 1, wxALL, SliderBorder);


    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    wxString debugShipRenderModeChoices[] =
    {
        _("No Debug"),
        _("Draw in Wireframe Mode"),
        _("Draw Only Points"),
        _("Draw Only Springs"),
        _("Draw Only Edge Springs")
    };

    mDebugShipRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Ship Debug Draw Options"), wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(debugShipRenderModeChoices), debugShipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    Connect(mDebugShipRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnDebugShipRenderModeRadioBox);

    checkboxesSizer->Add(mDebugShipRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);


    wxString vectorFieldRenderModeChoices[] =
    {
        _("None"),
        _("Point Velocities"),
        _("Point Forces"),
        _("Point Water Velocities"),
        _("Point Water Momenta")
    };

    mVectorFieldRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Vector Field Draw Options"), wxDefaultPosition, wxSize(-1, -1),
        WXSIZEOF(vectorFieldRenderModeChoices), vectorFieldRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    mVectorFieldRenderModeRadioBox->SetToolTip("Enables or disables rendering of vector fields.");
    Connect(mVectorFieldRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnVectorFieldRenderModeRadioBox);

    checkboxesSizer->Add(mVectorFieldRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);


    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::ReadSettings()
{
    assert(!!mGameController);


    mMechanicalQualitySlider->SetValue(mGameController->GetNumMechanicalDynamicsIterationsAdjustment());

    mStrengthSlider->SetValue(mGameController->GetStrengthAdjustment());



    mWaterDensitySlider->SetValue(mGameController->GetWaterDensityAdjustment());

    mWaterDragSlider->SetValue(mGameController->GetWaterDragAdjustment());

    mWaterIntakeSlider->SetValue(mGameController->GetWaterIntakeAdjustment());

    mWaterCrazynessSlider->SetValue(mGameController->GetWaterCrazyness());

    mWaterDiffusionSpeedSlider->SetValue(mGameController->GetWaterDiffusionSpeedAdjustment());

    mWaterLevelOfDetailSlider->SetValue(mGameController->GetWaterLevelOfDetail());



    mNumberOfStarsSlider->SetValue(static_cast<float>(mGameController->GetNumberOfStars()));

    mNumberOfCloudsSlider->SetValue(static_cast<float>(mGameController->GetNumberOfClouds()));

    mWindSpeedBaseSlider->SetValue(mGameController->GetWindSpeedBase());

    mModulateWindCheckBox->SetValue(mGameController->GetDoModulateWind());

    mWindGustAmplitudeSlider->SetValue(mGameController->GetWindSpeedMaxFactor());
    mWindGustAmplitudeSlider->Enable(mGameController->GetDoModulateWind());



    mWaveHeightSlider->SetValue(mGameController->GetWaveHeight());

    mLuminiscenceSlider->SetValue(mGameController->GetLuminiscenceAdjustment());

    mLightSpreadSlider->SetValue(mGameController->GetLightSpreadAdjustment());

    mSeaDepthSlider->SetValue(mGameController->GetSeaDepth());

    mOceanFloorBumpinessSlider->SetValue(mGameController->GetOceanFloorBumpiness());

    mOceanFloorDetailAmplificationSlider->SetValue(mGameController->GetOceanFloorDetailAmplification());



    mDestroyRadiusSlider->SetValue(mGameController->GetDestroyRadius());

    mBombBlastRadiusSlider->SetValue(mGameController->GetBombBlastRadius());

    mAntiMatterBombImplosionStrengthSlider->SetValue(mGameController->GetAntiMatterBombImplosionStrength());

    mFloodRadiusSlider->SetValue(mGameController->GetFloodRadius());

    mFloodQuantitySlider->SetValue(mGameController->GetFloodQuantity());

    mUltraViolentCheckBox->SetValue(mGameController->GetUltraViolentMode());

    mGenerateDebrisCheckBox->SetValue(mGameController->GetDoGenerateDebris());

    mGenerateSparklesCheckBox->SetValue(mGameController->GetDoGenerateSparkles());

    mGenerateAirBubblesCheckBox->SetValue(mGameController->GetDoGenerateAirBubbles());

    mScreenshotDirPickerCtrl->SetPath(mUISettings->GetScreenshotsFolderPath().string());



    mWaterContrastSlider->SetValue(mGameController->GetWaterContrast());

    mSeaWaterTransparencySlider->SetValue(mGameController->GetSeaWaterTransparency());

    mSeeShipThroughSeaWaterCheckBox->SetValue(mGameController->GetShowShipThroughSeaWater());

    auto shipRenderMode = mGameController->GetShipRenderMode();
    switch (shipRenderMode)
    {
        case ShipRenderMode::Structure:
        {
            mShipRenderModeRadioBox->SetSelection(0);
            break;
        }

        case ShipRenderMode::Texture:
        {
            mShipRenderModeRadioBox->SetSelection(1);
            break;
        }
    }

    mShowStressCheckBox->SetValue(mGameController->GetShowShipStress());



    mEffectsVolumeSlider->SetValue(mSoundController->GetMasterEffectsVolume());

    mToolsVolumeSlider->SetValue(mSoundController->GetMasterToolsVolume());

    mMusicVolumeSlider->SetValue(mSoundController->GetMasterMusicVolume());

    mPlayBreakSoundsCheckBox->SetValue(mSoundController->GetPlayBreakSounds());

    mPlayStressSoundsCheckBox->SetValue(mSoundController->GetPlayStressSounds());

    mPlayWindSoundCheckBox->SetValue(mSoundController->GetPlayWindSound());

    mPlaySinkingMusicCheckBox->SetValue(mSoundController->GetPlaySinkingMusic());




    mStiffnessSlider->SetValue(mGameController->GetStiffnessAdjustment());

    auto debugShipRenderMode = mGameController->GetDebugShipRenderMode();
    switch (debugShipRenderMode)
    {
        case DebugShipRenderMode::None:
        {
            mDebugShipRenderModeRadioBox->SetSelection(0);
            break;
        }

        case DebugShipRenderMode::Wireframe:
        {
            mDebugShipRenderModeRadioBox->SetSelection(1);
            break;
        }

        case DebugShipRenderMode::Points:
        {
            mDebugShipRenderModeRadioBox->SetSelection(2);
            break;
        }

        case DebugShipRenderMode::Springs:
        {
            mDebugShipRenderModeRadioBox->SetSelection(3);
            break;
        }

        case DebugShipRenderMode::EdgeSprings:
        {
            mDebugShipRenderModeRadioBox->SetSelection(4);
            break;
        }
    }

    auto vectorFieldRenderMode = mGameController->GetVectorFieldRenderMode();
    switch (vectorFieldRenderMode)
    {
        case VectorFieldRenderMode::None:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(0);
            break;
        }

        case VectorFieldRenderMode::PointVelocity:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(1);
            break;
        }

        case VectorFieldRenderMode::PointForce:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(2);
            break;
        }

        case VectorFieldRenderMode::PointWaterVelocity:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(3);
            break;
        }

        default:
        {
            assert(vectorFieldRenderMode == VectorFieldRenderMode::PointWaterMomentum);
            mVectorFieldRenderModeRadioBox->SetSelection(4);
            break;
        }
    }
}