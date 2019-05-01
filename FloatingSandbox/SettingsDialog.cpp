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
const long ID_MODULATE_WIND_CHECKBOX = wxNewId();
const long ID_PLAY_BREAK_SOUNDS_CHECKBOX = wxNewId();
const long ID_PLAY_STRESS_SOUNDS_CHECKBOX = wxNewId();
const long ID_PLAY_WIND_SOUND_CHECKBOX = wxNewId();
const long ID_PLAY_SINKING_MUSIC_CHECKBOX = wxNewId();

SettingsDialog::SettingsDialog(
    wxWindow* parent,
    std::shared_ptr<GameController> gameController,
    std::shared_ptr<SoundController> soundController,
    ResourceLoader const & resourceLoader)
    : mParent(parent)
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
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
    // Air
    //

    wxPanel * airPanel = new wxPanel(notebook);

    airPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateAirPanel(airPanel);

    notebook->AddPage(airPanel, "Air");


    //
    // Waves
    //

    wxPanel * wavesPanel = new wxPanel(notebook);

    wavesPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    PopulateWavesPanel(wavesPanel);

    notebook->AddPage(wavesPanel, "Waves");


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

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
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

void SettingsDialog::OnModulateWindCheckBoxClick(wxCommandEvent & /*event*/)
{
    mWindGustAmplitudeSlider->Enable(mModulateWindCheckBox->IsChecked());

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnTextureOceanRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    ReconciliateOceanRenderModeSettings();

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnDepthOceanRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    ReconciliateOceanRenderModeSettings();

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnDepthOceanColorStartChanged(wxColourPickerEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnDepthOceanColorEndChanged(wxColourPickerEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnFlatOceanRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    ReconciliateOceanRenderModeSettings();

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnFlatOceanColorChanged(wxColourPickerEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnSeeShipThroughOceanCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnTextureLandRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    ReconciliateLandRenderModeSettings();

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnFlatLandRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    ReconciliateLandRenderModeSettings();

    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnFlatLandColorChanged(wxColourPickerEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnFlatSkyColorChanged(wxColourPickerEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnTextureShipRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnStructureShipRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnShowStressCheckBoxClick(wxCommandEvent & /*event*/)
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
        mGameController->GetSpringStrengthAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinSpringStrengthAdjustment(),
            1.0f,
            mGameController->GetMaxSpringStrengthAdjustment()));

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
        "Adjusts the density of sea water, and thus the buoyancy it exerts on physical bodies.",
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

void SettingsDialog::PopulateAirPanel(wxPanel * panel)
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
        "The number of clouds in the world's sky. This is the total number of clouds in the world; at any moment in time, the number of clouds that are visible will be less than or equal to this value.",
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

void SettingsDialog::PopulateWavesPanel(wxPanel * panel)
{
    wxGridSizer* gridSizer = new wxGridSizer(2, 4, 0, 0);

    //
    // Row 1
    //


    // Basal Wave Height

    mBasalWaveHeightSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Basal Wave Height",
        "The height of basal sea water waves (m).",
        static_cast<float>(mGameController->GetBasalWaveHeight()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinBasalWaveHeight(),
            mGameController->GetMaxBasalWaveHeight()));

    gridSizer->Add(mBasalWaveHeightSlider.get(), 1, wxALL, SliderBorder);


    // Basal Wave Length Adjustment

    mBasalWaveLengthAdjustmentSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "BasalWave Width Adjust",
        "TODO.",
        static_cast<float>(mGameController->GetBasalWaveLengthAdjustment()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinBasalWaveLengthAdjustment(),
            0.1f,
            mGameController->GetMaxBasalWaveLengthAdjustment()));

    gridSizer->Add(mBasalWaveLengthAdjustmentSlider.get(), 1, wxALL, SliderBorder);


    // Basal Wave Speed Adjustment

    mBasalWaveSpeedAdjustmentSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Basal Wave Speed Adjust",
        "TODO.",
        static_cast<float>(mGameController->GetBasalWaveSpeedAdjustment()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinBasalWaveSpeedAdjustment(),
            mGameController->GetMaxBasalWaveSpeedAdjustment()));

    gridSizer->Add(mBasalWaveSpeedAdjustmentSlider.get(), 1, wxALL, SliderBorder);



    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateWorldPanel(wxPanel * panel)
{
    wxGridSizer* gridSizer = new wxGridSizer(2, 4, 0, 0);

    //
    // Row 1
    //

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


    // Rot Accelerator

    mRotAcceler8rSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Rot Acceler8r",
        "Adjusts the speed with which materials rot when exposed to sea water.",
        mGameController->GetRotAcceler8r(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<ExponentialSliderCore>(
            mGameController->GetMinRotAcceler8r(),
            1.0f,
            mGameController->GetMaxRotAcceler8r()));

    gridSizer->Add(mRotAcceler8rSlider.get(), 1, wxALL, SliderBorder);



    //
    // Row 2
    //

    // Luminiscence

    mLuminiscenceSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Luminiscence Adjust",
        "Adjusts the quantity of light emitted by luminiscent materials.",
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

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    static constexpr int StaticBoxTopMargin = 7;
    static constexpr int StaticBoxInsetMargin = 10;
    static constexpr int CellBorder = 8;

    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);


    //
    // Row 1
    //

    // Ocean
    {
        wxStaticBox * oceanBox = new wxStaticBox(panel, wxID_ANY, _("Sea"));

        wxBoxSizer * oceanBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        oceanBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * oceanSizer = new wxGridBagSizer(0, 0);

            // Ocean Render Mode
            {
                wxStaticBox * oceanRenderModeBox = new wxStaticBox(oceanBox, wxID_ANY, _("Draw Mode"));

                wxBoxSizer * oceanRenderModeBoxSizer1 = new wxBoxSizer(wxVERTICAL);
                oceanRenderModeBoxSizer1->AddSpacer(StaticBoxTopMargin);

                {
                    wxFlexGridSizer * oceanRenderModeBoxSizer2 = new wxFlexGridSizer(3, 5, 5);
                    oceanRenderModeBoxSizer2->SetFlexibleDirection(wxHORIZONTAL);

                    mTextureOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a static pattern.");
                    mTextureOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnTextureOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mTextureOceanRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    oceanRenderModeBoxSizer2->AddSpacer(0);

                    oceanRenderModeBoxSizer2->AddSpacer(0);

                    mDepthOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Depth Gradient"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a vertical color gradient.");
                    mDepthOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnDepthOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mDepthOceanColorStartPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorStartPicker->SetToolTip("Sets the starting (top) color of the gradient.");
                    mDepthOceanColorStartPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorStartChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorStartPicker, 0, wxALL, 0);

                    mDepthOceanColorEndPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorEndPicker->SetToolTip("Sets the ending (bottom) color of the gradient.");
                    mDepthOceanColorEndPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorEndChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorEndPicker, 0, wxALL, 0);

                    mFlatOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a single color.");
                    mFlatOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnFlatOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mFlatOceanRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatOceanColorPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanColorPicker->SetToolTip("Sets the single color of the ocean.");
                    mFlatOceanColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatOceanColorChanged, this);
                    oceanRenderModeBoxSizer2->Add(mFlatOceanColorPicker, 0, wxALL, 0);

                    oceanRenderModeBoxSizer2->AddSpacer(0);

                    oceanRenderModeBoxSizer1->Add(oceanRenderModeBoxSizer2, 0, wxALL, StaticBoxInsetMargin);
                }

                oceanRenderModeBox->SetSizerAndFit(oceanRenderModeBoxSizer1);

                oceanSizer->Add(
                    oceanRenderModeBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // See Ship Through Water
            {
                mSeeShipThroughOceanCheckBox = new wxCheckBox(oceanBox, wxID_ANY,
                    _("See Ship Through Water"), wxDefaultPosition, wxDefaultSize);
                mSeeShipThroughOceanCheckBox->SetToolTip("Shows the ship either behind the sea water or in front of it.");
                mSeeShipThroughOceanCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,&SettingsDialog::OnSeeShipThroughOceanCheckBoxClick, this);

                oceanSizer->Add(
                    mSeeShipThroughOceanCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Ocean Transparency
            {
                mOceanTransparencySlider = std::make_unique<SliderControl>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    "Transparency",
                    "Adjusts the transparency of sea water.",
                    mGameController->GetOceanTransparency(),
                    [this](float /*value*/)
                    {
                        // Remember we're dirty now
                        this->mApplyButton->Enable(true);
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                oceanSizer->Add(
                    mOceanTransparencySlider.get(),
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxALL,
                    CellBorder);
            }

            oceanBoxSizer1->Add(oceanSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        oceanBox->SetSizerAndFit(oceanBoxSizer1);

        gridSizer->Add(
            oceanBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 3),
            wxALL,
            CellBorder);
    }

    // Land
    {
        wxStaticBox * landBox = new wxStaticBox(panel, wxID_ANY, _("Land"));

        wxBoxSizer * landBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        landBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * landSizer = new wxGridBagSizer(0, 0);

            // Land Render Mode
            {
                wxStaticBox * landRenderModeBox = new wxStaticBox(landBox, wxID_ANY, _("Draw Mode"));

                wxBoxSizer * landRenderModeBoxSizer1 = new wxBoxSizer(wxVERTICAL);
                landRenderModeBoxSizer1->AddSpacer(StaticBoxTopMargin);

                {
                    wxFlexGridSizer* landRenderModeBoxSizer2 = new wxFlexGridSizer(3, 5, 5);
                    landRenderModeBoxSizer2->SetFlexibleDirection(wxHORIZONTAL);

                    mTextureLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureLandRenderModeRadioButton->SetToolTip("Draws the ocean floor using a static image.");
                    mTextureLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnTextureLandRenderModeRadioButtonClick, this);
                    landRenderModeBoxSizer2->Add(mTextureLandRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    landRenderModeBoxSizer2->AddSpacer(0);

                    landRenderModeBoxSizer2->AddSpacer(0);

                    mFlatLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatLandRenderModeRadioButton->SetToolTip("Draws the ocean floor using a static color.");
                    mFlatLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnFlatLandRenderModeRadioButtonClick, this);
                    landRenderModeBoxSizer2->Add(mFlatLandRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatLandColorPicker = new wxColourPickerCtrl(landRenderModeBox, wxID_ANY);
                    mFlatLandColorPicker->SetToolTip("Sets the single color of the ocean floor.");
                    mFlatLandColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatLandColorChanged, this);
                    landRenderModeBoxSizer2->Add(mFlatLandColorPicker, 0, wxALL, 0);

                    landRenderModeBoxSizer2->AddSpacer(0);

                    landRenderModeBoxSizer1->Add(landRenderModeBoxSizer2, 0, wxALL, StaticBoxInsetMargin);
                }

                landRenderModeBox->SetSizerAndFit(landRenderModeBoxSizer1);

                landSizer->Add(
                    landRenderModeBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            landBoxSizer1->Add(landSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        landBox->SetSizerAndFit(landBoxSizer1);

        gridSizer->Add(
            landBox,
            wxGBPosition(0, 3),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Sky
    {
        wxStaticBox * skyBox = new wxStaticBox(panel, wxID_ANY, _("Sky"));

        wxBoxSizer * skyBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        skyBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * skySizer = new wxGridBagSizer(0, 0);

            // Sky color
            {
                mFlatSkyColorPicker = new wxColourPickerCtrl(skyBox, wxID_ANY);
                mFlatSkyColorPicker->SetToolTip("Sets the color of the sky. Duh.");
                mFlatSkyColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatSkyColorChanged, this);

                skySizer->Add(
                    mFlatSkyColorPicker,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            skyBoxSizer1->Add(skySizer, 0, wxALL, StaticBoxInsetMargin);
        }

        skyBox->SetSizerAndFit(skyBoxSizer1);

        gridSizer->Add(
            skyBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Ship
    {
        wxStaticBox * shipBox = new wxStaticBox(panel, wxID_ANY, _("Ship"));

        wxBoxSizer * shipBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        shipBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * shipSizer = new wxGridBagSizer(0, 0);

            // Ship Render Mode
            {
                wxStaticBox * shipRenderModeBox = new wxStaticBox(shipBox, wxID_ANY, _("Draw Mode"));

                wxBoxSizer * shipRenderModeBoxSizer1 = new wxBoxSizer(wxVERTICAL);
                shipRenderModeBoxSizer1->AddSpacer(StaticBoxTopMargin);

                {
                    wxFlexGridSizer* shipRenderModeBoxSizer2 = new wxFlexGridSizer(1, 5, 5);
                    shipRenderModeBoxSizer2->SetFlexibleDirection(wxHORIZONTAL);

                    mTextureShipRenderModeRadioButton = new wxRadioButton(shipRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureShipRenderModeRadioButton->SetToolTip("Draws the ship using its texture image.");
                    mTextureShipRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnTextureShipRenderModeRadioButtonClick, this);
                    shipRenderModeBoxSizer2->Add(mTextureShipRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mStructureShipRenderModeRadioButton = new wxRadioButton(shipRenderModeBox, wxID_ANY, _("Structure"),
                        wxDefaultPosition, wxDefaultSize);
                    mStructureShipRenderModeRadioButton->SetToolTip("Draws the ship using its structure.");
                    mStructureShipRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnStructureShipRenderModeRadioButtonClick, this);
                    shipRenderModeBoxSizer2->Add(mStructureShipRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    shipRenderModeBoxSizer1->Add(shipRenderModeBoxSizer2, 0, wxALL, StaticBoxInsetMargin);
                }

                shipRenderModeBox->SetSizerAndFit(shipRenderModeBoxSizer1);

                shipSizer->Add(
                    shipRenderModeBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Show Stress
            {
                mShowStressCheckBox = new wxCheckBox(shipBox, wxID_ANY,
                    _("Show Stress"), wxDefaultPosition, wxDefaultSize);
                mShowStressCheckBox->SetToolTip("Enables or disables highlighting of the springs that are under heavy stress and close to rupture.");
                mShowStressCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnSeeShipThroughOceanCheckBoxClick, this);

                shipSizer->Add(
                    mShowStressCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            shipBoxSizer1->Add(shipSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        shipBox->SetSizerAndFit(shipBoxSizer1);

        gridSizer->Add(
            shipBox,
            wxGBPosition(1, 1),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Water
    {
        wxStaticBox * waterBox = new wxStaticBox(panel, wxID_ANY, _("Water"));

        wxBoxSizer * waterBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        waterBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * waterSizer = new wxGridBagSizer(0, 0);

            // Water contrast
            {
                mWaterContrastSlider = std::make_unique<SliderControl>(
                    waterBox,
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

                waterSizer->Add(
                    mWaterContrastSlider.get(),
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            waterBoxSizer1->Add(waterSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        waterBox->SetSizerAndFit(waterBoxSizer1);

        gridSizer->Add(
            waterBox,
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
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

    // Spring Stiffness

    mSpringStiffnessSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Spring Stiffness Adjust",
        "This setting is for testing physical instability of the mass-spring network with high stiffness values;"
        " it is not meant for improving the rigidity of physical bodies.",
        mGameController->GetSpringStiffnessAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinSpringStiffnessAdjustment(),
            mGameController->GetMaxSpringStiffnessAdjustment()),
        mWarningIcon.get());

    controlsSizer->Add(mSpringStiffnessSlider.get(), 1, wxALL, SliderBorder);

    // Spring Damping

    mSpringDampingSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Spring Damping Adjust",
        "This setting is for testing physical instability of the mass-spring network with different damping values;"
        " it is not meant for improving the rigidity of physical bodies.",
        mGameController->GetSpringDampingAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinSpringDampingAdjustment(),
            mGameController->GetMaxSpringDampingAdjustment()),
            mWarningIcon.get());

    controlsSizer->Add(mSpringDampingSlider.get(), 1, wxALL, SliderBorder);

    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    wxString debugShipRenderModeChoices[] =
    {
        _("No Debug"),
        _("Draw in Wireframe Mode"),
        _("Draw Only Points"),
        _("Draw Only Springs"),
        _("Draw Only Edge Springs"),
        _("Draw Decay")
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

    mStrengthSlider->SetValue(mGameController->GetSpringStrengthAdjustment());



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

    // Waves

    mBasalWaveHeightSlider->SetValue(mGameController->GetBasalWaveHeight());

    mBasalWaveLengthAdjustmentSlider->SetValue(mGameController->GetBasalWaveLengthAdjustment());

    mBasalWaveSpeedAdjustmentSlider->SetValue(mGameController->GetBasalWaveSpeedAdjustment());

    // World

    mLuminiscenceSlider->SetValue(mGameController->GetLuminiscenceAdjustment());

    mLightSpreadSlider->SetValue(mGameController->GetLightSpreadAdjustment());

    mRotAcceler8rSlider->SetValue(mGameController->GetRotAcceler8r());

    mSeaDepthSlider->SetValue(mGameController->GetSeaDepth());

    mOceanFloorBumpinessSlider->SetValue(mGameController->GetOceanFloorBumpiness());

    mOceanFloorDetailAmplificationSlider->SetValue(mGameController->GetOceanFloorDetailAmplification());

    // Interactions

    mDestroyRadiusSlider->SetValue(mGameController->GetDestroyRadius());

    mBombBlastRadiusSlider->SetValue(mGameController->GetBombBlastRadius());

    mAntiMatterBombImplosionStrengthSlider->SetValue(mGameController->GetAntiMatterBombImplosionStrength());

    mFloodRadiusSlider->SetValue(mGameController->GetFloodRadius());

    mFloodQuantitySlider->SetValue(mGameController->GetFloodQuantity());

    mUltraViolentCheckBox->SetValue(mGameController->GetUltraViolentMode());

    mGenerateDebrisCheckBox->SetValue(mGameController->GetDoGenerateDebris());

    mGenerateSparklesCheckBox->SetValue(mGameController->GetDoGenerateSparkles());

    mGenerateAirBubblesCheckBox->SetValue(mGameController->GetDoGenerateAirBubbles());

    // Render

    auto oceanRenderMode = mGameController->GetOceanRenderMode();
    switch (oceanRenderMode)
    {
        case OceanRenderMode::Texture:
        {
            mTextureOceanRenderModeRadioButton->SetValue(true);
            break;
        }

        case OceanRenderMode::Depth:
        {
            mDepthOceanRenderModeRadioButton->SetValue(true);
            break;
        }

        case OceanRenderMode::Flat:
        {
            mFlatOceanRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    auto depthOceanColorStart = mGameController->GetDepthOceanColorStart();
    mDepthOceanColorStartPicker->SetColour(wxColor(depthOceanColorStart.r, depthOceanColorStart.g, depthOceanColorStart.b));

    auto depthOceanColorEnd = mGameController->GetDepthOceanColorEnd();
    mDepthOceanColorEndPicker->SetColour(wxColor(depthOceanColorEnd.r, depthOceanColorEnd.g, depthOceanColorEnd.b));

    auto flatOceanColor = mGameController->GetFlatOceanColor();
    mFlatOceanColorPicker->SetColour(wxColor(flatOceanColor.r, flatOceanColor.g, flatOceanColor.b));

    ReconciliateOceanRenderModeSettings();

    mSeeShipThroughOceanCheckBox->SetValue(mGameController->GetShowShipThroughOcean());

    mOceanTransparencySlider->SetValue(mGameController->GetOceanTransparency());

    auto landRenderMode = mGameController->GetLandRenderMode();
    switch (landRenderMode)
    {
        case LandRenderMode::Texture:
        {
            mTextureLandRenderModeRadioButton->SetValue(true);
            break;
        }

        case LandRenderMode::Flat:
        {
            mFlatLandRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    auto flatLandColor = mGameController->GetFlatLandColor();
    mFlatLandColorPicker->SetColour(wxColor(flatLandColor.r, flatLandColor.g, flatLandColor.b));

    ReconciliateLandRenderModeSettings();

    auto flatSkyColor = mGameController->GetFlatSkyColor();
    mFlatSkyColorPicker->SetColour(wxColor(flatSkyColor.r, flatSkyColor.g, flatSkyColor.b));

    auto shipRenderMode = mGameController->GetShipRenderMode();
    switch (shipRenderMode)
    {
        case ShipRenderMode::Texture:
        {
            mTextureShipRenderModeRadioButton->SetValue(true);
            break;
        }

        case ShipRenderMode::Structure:
        {
            mStructureShipRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    mShowStressCheckBox->SetValue(mGameController->GetShowShipStress());

    mWaterContrastSlider->SetValue(mGameController->GetWaterContrast());

    // Sound

    mEffectsVolumeSlider->SetValue(mSoundController->GetMasterEffectsVolume());

    mToolsVolumeSlider->SetValue(mSoundController->GetMasterToolsVolume());

    mMusicVolumeSlider->SetValue(mSoundController->GetMasterMusicVolume());

    mPlayBreakSoundsCheckBox->SetValue(mSoundController->GetPlayBreakSounds());

    mPlayStressSoundsCheckBox->SetValue(mSoundController->GetPlayStressSounds());

    mPlayWindSoundCheckBox->SetValue(mSoundController->GetPlayWindSound());

    mPlaySinkingMusicCheckBox->SetValue(mSoundController->GetPlaySinkingMusic());




    mSpringStiffnessSlider->SetValue(mGameController->GetSpringStiffnessAdjustment());

    mSpringDampingSlider->SetValue(mGameController->GetSpringDampingAdjustment());

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

        case DebugShipRenderMode::Decay:
        {
            mDebugShipRenderModeRadioBox->SetSelection(5);
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

void SettingsDialog::ReconciliateOceanRenderModeSettings()
{
    mDepthOceanColorStartPicker->Enable(mDepthOceanRenderModeRadioButton->GetValue());
    mDepthOceanColorEndPicker->Enable(mDepthOceanRenderModeRadioButton->GetValue());
    mFlatOceanColorPicker->Enable(mFlatOceanRenderModeRadioButton->GetValue());
}

void SettingsDialog::ReconciliateLandRenderModeSettings()
{
    mFlatLandColorPicker->Enable(mFlatLandRenderModeRadioButton->GetValue());
}

void SettingsDialog::ApplySettings()
{
    assert(!!mGameController);


    mGameController->SetNumMechanicalDynamicsIterationsAdjustment(
        mMechanicalQualitySlider->GetValue());

    mGameController->SetSpringStrengthAdjustment(
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

    // Air

    mGameController->SetNumberOfStars(
        static_cast<size_t>(mNumberOfStarsSlider->GetValue()));

    mGameController->SetNumberOfClouds(
        static_cast<size_t>(mNumberOfCloudsSlider->GetValue()));

    mGameController->SetWindSpeedBase(
        mWindSpeedBaseSlider->GetValue());

    mGameController->SetDoModulateWind(mModulateWindCheckBox->IsChecked());

    mGameController->SetWindSpeedMaxFactor(
        mWindGustAmplitudeSlider->GetValue());

    // Waves

    mGameController->SetBasalWaveHeight(
        mBasalWaveHeightSlider->GetValue());

    mGameController->SetBasalWaveLengthAdjustment(
        mBasalWaveLengthAdjustmentSlider->GetValue());

    mGameController->SetBasalWaveSpeedAdjustment(
        mBasalWaveSpeedAdjustmentSlider->GetValue());

    // World

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

    mGameController->SetRotAcceler8r(
        mRotAcceler8rSlider->GetValue());


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


    // Render

    if (mTextureOceanRenderModeRadioButton->GetValue())
    {
        mGameController->SetOceanRenderMode(OceanRenderMode::Texture);
    }
    else if (mDepthOceanRenderModeRadioButton->GetValue())
    {
        mGameController->SetOceanRenderMode(OceanRenderMode::Depth);
    }
    else
    {
        assert(mFlatOceanRenderModeRadioButton->GetValue());
        mGameController->SetOceanRenderMode(OceanRenderMode::Flat);
    }

    auto depthOceanColorStart = mDepthOceanColorStartPicker->GetColour();
    mGameController->SetDepthOceanColorStart(
        rgbColor(depthOceanColorStart.Red(), depthOceanColorStart.Green(), depthOceanColorStart.Blue()));

    auto depthOceanColorEnd = mDepthOceanColorEndPicker->GetColour();
    mGameController->SetDepthOceanColorEnd(
        rgbColor(depthOceanColorEnd.Red(), depthOceanColorEnd.Green(), depthOceanColorEnd.Blue()));

    auto flatOceanColor = mFlatOceanColorPicker->GetColour();
    mGameController->SetFlatOceanColor(
        rgbColor(flatOceanColor.Red(), flatOceanColor.Green(), flatOceanColor.Blue()));

    mGameController->SetShowShipThroughOcean(mSeeShipThroughOceanCheckBox->IsChecked());

    mGameController->SetOceanTransparency(
        mOceanTransparencySlider->GetValue());

    if (mTextureLandRenderModeRadioButton->GetValue())
    {
        mGameController->SetLandRenderMode(LandRenderMode::Texture);
    }
    else
    {
        assert(mFlatLandRenderModeRadioButton->GetValue());
        mGameController->SetLandRenderMode(LandRenderMode::Flat);
    }

    auto flatLandColor = mFlatLandColorPicker->GetColour();
    mGameController->SetFlatLandColor(
        rgbColor(flatLandColor.Red(), flatLandColor.Green(), flatLandColor.Blue()));

    auto flatSkyColor = mFlatSkyColorPicker->GetColour();
    mGameController->SetFlatSkyColor(
        rgbColor(flatSkyColor.Red(), flatSkyColor.Green(), flatSkyColor.Blue()));

    if (mTextureShipRenderModeRadioButton->GetValue())
    {
        mGameController->SetShipRenderMode(ShipRenderMode::Texture);
    }
    else
    {
        assert(mStructureShipRenderModeRadioButton->GetValue());
        mGameController->SetShipRenderMode(ShipRenderMode::Structure);
    }

    mGameController->SetShowShipStress(mShowStressCheckBox->IsChecked());

    mGameController->SetWaterContrast(
        mWaterContrastSlider->GetValue());

    // Sound

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



    mGameController->SetSpringStiffnessAdjustment(
        mSpringStiffnessSlider->GetValue());

    mGameController->SetSpringDampingAdjustment(
        mSpringDampingSlider->GetValue());

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
    else if (4 == selectedDebugShipRenderMode)
    {
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::EdgeSprings);
    }
    else
    {
        assert(5 == selectedDebugShipRenderMode);
        mGameController->SetDebugShipRenderMode(DebugShipRenderMode::Decay);
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