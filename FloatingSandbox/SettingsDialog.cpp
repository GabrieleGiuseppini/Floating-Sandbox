/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SettingsDialog.h"

#include <GameLib/Log.h>
#include <UILib/ExponentialSliderCore.h>
#include <UILib/FixedTickSliderCore.h>
#include <UILib/LinearSliderCore.h>

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
const long ID_SEE_SHIP_THROUGH_SEA_WATER_CHECKBOX = wxNewId();
const long ID_SHOW_STRESS_CHECKBOX = wxNewId();
const long ID_WIREFRAME_MODE_CHECKBOX = wxNewId();
const long ID_PLAY_BREAK_SOUNDS_CHECKBOX = wxNewId();
const long ID_PLAY_STRESS_SOUNDS_CHECKBOX = wxNewId();
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

void SettingsDialog::OnShipRenderModeRadioBox(wxCommandEvent & /*event*/)
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

void SettingsDialog::OnWireframeModeCheckBoxClick(wxCommandEvent & /*event*/)
{
    mShipRenderModeRadioBox->Enable(!mWireframeModeCheckBox->IsChecked());

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

    mGameController->SetStiffnessAdjustment(
        mStiffnessSlider->GetValue());

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

    mGameController->SetWindSpeed(
        mWindSpeedSlider->GetValue());



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

    mGameController->SetUltraViolentMode(mUltraViolentCheckBox->IsChecked());

    mGameController->SetDoGenerateDebris(mGenerateDebrisCheckBox->IsChecked());

    mGameController->SetDoGenerateSparkles(mGenerateSparklesCheckBox->IsChecked());



    mGameController->SetWaterContrast(
        mWaterContrastSlider->GetValue());

    mGameController->SetSeaWaterTransparency(
        mSeaWaterTransparencySlider->GetValue());

    mGameController->SetShowShipThroughSeaWater(mSeeShipThroughSeaWaterCheckBox->IsChecked());

    auto selectedShipRenderMode = mShipRenderModeRadioBox->GetSelection();
    if (0 == selectedShipRenderMode)
    {
        mGameController->SetShipRenderMode(ShipRenderMode::Points);
    }
    else if (1 == selectedShipRenderMode)
    {
        mGameController->SetShipRenderMode(ShipRenderMode::Springs);
    }
    else if (2 == selectedShipRenderMode)
    {
        mGameController->SetShipRenderMode(ShipRenderMode::EdgeSprings);
    }
    else if (3 == selectedShipRenderMode)
    {
        mGameController->SetShipRenderMode(ShipRenderMode::Structure);
    }
    else
    {
        assert(4 == selectedShipRenderMode);
        mGameController->SetShipRenderMode(ShipRenderMode::Texture);
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

    mGameController->SetShowShipStress(mShowStressCheckBox->IsChecked());

    mGameController->SetWireframeMode(mWireframeModeCheckBox->IsChecked());


    mSoundController->SetMasterEffectsVolume(
        mEffectsVolumeSlider->GetValue());

    mSoundController->SetMasterToolsVolume(
        mToolsVolumeSlider->GetValue());

    mSoundController->SetMasterMusicVolume(
        mMusicVolumeSlider->GetValue());

    mSoundController->SetPlayBreakSounds(mPlayBreakSoundsCheckBox->IsChecked());

    mSoundController->SetPlayStressSounds(mPlayStressSoundsCheckBox->IsChecked());

    mSoundController->SetPlaySinkingMusic(mPlaySinkingMusicCheckBox->IsChecked());
}

void SettingsDialog::PopulateMechanicsPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);


    // Mechanical quality

    mMechanicalQualitySlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Simulation Quality",
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
        *mWarningIcon,
        "Higher values improve the rigidity of simulated structures, at the expense of longer computation times");

    controlsSizer->Add(mMechanicalQualitySlider.get(), 1, wxALL, SliderBorder);


    // Stiffness

    mStiffnessSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Stiffness Adjust",
        mGameController->GetStiffnessAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinStiffnessAdjustment(),
            mGameController->GetMaxStiffnessAdjustment()),
        *mWarningIcon,
        "Higher values cause physical instability, but provide for a nice effect!");

    controlsSizer->Add(mStiffnessSlider.get(), 1, wxALL, SliderBorder);


    // Strength

    mStrengthSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Strength Adjust",
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
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);


    // Number of Stars

    mNumberOfStarsSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Number of Stars",
        static_cast<float>(mGameController->GetNumberOfStars()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            static_cast<float>(mGameController->GetMinNumberOfStars()),
            static_cast<float>(mGameController->GetMaxNumberOfStars())));

    controlsSizer->Add(mNumberOfStarsSlider.get(), 1, wxALL, SliderBorder);


    // Number of Clouds

    mNumberOfCloudsSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Number of Clouds",
        static_cast<float>(mGameController->GetNumberOfClouds()),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            static_cast<float>(mGameController->GetMinNumberOfClouds()),
            static_cast<float>(mGameController->GetMaxNumberOfClouds())));

    controlsSizer->Add(mNumberOfCloudsSlider.get(), 1, wxALL, SliderBorder);


    // Wind Speed

    mWindSpeedSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Wind Speed",
        mGameController->GetWindSpeed(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWindSpeed(),
            mGameController->GetMaxWindSpeed()));

    controlsSizer->Add(mWindSpeedSlider.get(), 1, wxALL, SliderBorder);



    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
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
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);


    // Destroy Radius

    mDestroyRadiusSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Destroy Radius",
        mGameController->GetDestroyRadius(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinDestroyRadius(),
            mGameController->GetMaxDestroyRadius()));

    controlsSizer->Add(mDestroyRadiusSlider.get(), 1, wxALL, SliderBorder);


    // Bomb Blast Radius

    mBombBlastRadiusSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "Bomb Blast Radius",
        mGameController->GetBombBlastRadius(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinBombBlastRadius(),
            mGameController->GetMaxBombBlastRadius()));

    controlsSizer->Add(mBombBlastRadiusSlider.get(), 1, wxALL, SliderBorder);


    // Anti-matter Bomb Implosion Strength

    mAntiMatterBombImplosionStrengthSlider = std::make_unique<SliderControl>(
        panel,
        SliderWidth,
        SliderHeight,
        "AM Bomb Implosion Strength",
        mGameController->GetAntiMatterBombImplosionStrength(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinAntiMatterBombImplosionStrength(),
            mGameController->GetMaxAntiMatterBombImplosionStrength()));

    controlsSizer->Add(mAntiMatterBombImplosionStrengthSlider.get(), 1, wxALL, SliderBorder);


    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    mUltraViolentCheckBox = new wxCheckBox(panel, ID_ULTRA_VIOLENT_CHECKBOX, _("Ultra-Violent Mode"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Ultra-Violent Mode Checkbox"));
    Connect(ID_ULTRA_VIOLENT_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnUltraViolentCheckBoxClick);
    checkboxesSizer->Add(mUltraViolentCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mGenerateDebrisCheckBox = new wxCheckBox(panel, ID_GENERATE_DEBRIS_CHECKBOX, _("Generate Debris"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Generate Debris Checkbox"));
    Connect(ID_GENERATE_DEBRIS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnGenerateDebrisCheckBoxClick);
    checkboxesSizer->Add(mGenerateDebrisCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mGenerateSparklesCheckBox = new wxCheckBox(panel, ID_GENERATE_SPARKLES_CHECKBOX, _("Generate Sparkles"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Generate Sparkles Checkbox"));
    Connect(ID_GENERATE_SPARKLES_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnGenerateSparklesCheckBoxClick);
    checkboxesSizer->Add(mGenerateSparklesCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
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
        _("Draw Only Points"),
        _("Draw Only Springs"),
        _("Draw Only Edge Springs"),
        _("Draw Structure"),
        _("Draw Image")
    };

    mShipRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Ship Draw Options"), wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(shipRenderModeChoices), shipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    Connect(mShipRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnShipRenderModeRadioBox);

    checkboxesSizer->Add(mShipRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);


    wxString vectorFieldRenderModeChoices[] =
    {
        _("None"),
        _("Point Velocities"),
        _("Point Forces"),
        _("Point Water Velocities"),
        _("Point Water Momenta")
    };

    mVectorFieldRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Vector Field Draw Options"), wxDefaultPosition, wxSize(-1,-1),
        WXSIZEOF(vectorFieldRenderModeChoices), vectorFieldRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    Connect(mVectorFieldRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnVectorFieldRenderModeRadioBox);

    checkboxesSizer->Add(mVectorFieldRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);


    mShowStressCheckBox = new wxCheckBox(panel, ID_SHOW_STRESS_CHECKBOX, _("Show Stress"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Show Stress Checkbox"));
    Connect(ID_SHOW_STRESS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnShowStressCheckBoxClick);

    checkboxesSizer->Add(mShowStressCheckBox, 0, wxALL | wxALIGN_LEFT, 5);


    mWireframeModeCheckBox = new wxCheckBox(panel, ID_WIREFRAME_MODE_CHECKBOX, _("Wireframe Mode"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Wireframe Mode Checkbox"));
    Connect(ID_WIREFRAME_MODE_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnWireframeModeCheckBoxClick);

    checkboxesSizer->Add(mWireframeModeCheckBox, 0, wxALL | wxALIGN_LEFT, 5);


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
    Connect(ID_PLAY_BREAK_SOUNDS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlayBreakSoundsCheckBoxClick);
    checkboxesSizer->Add(mPlayBreakSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlayStressSoundsCheckBox = new wxCheckBox(panel, ID_PLAY_STRESS_SOUNDS_CHECKBOX, _("Play Stress Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Stress Sounds Checkbox"));
    Connect(ID_PLAY_STRESS_SOUNDS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlayStressSoundsCheckBoxClick);
    checkboxesSizer->Add(mPlayStressSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlaySinkingMusicCheckBox = new wxCheckBox(panel, ID_PLAY_SINKING_MUSIC_CHECKBOX, _("Play Farewell Music"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Sinking Music Checkbox"));
    Connect(ID_PLAY_SINKING_MUSIC_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnPlaySinkingMusicCheckBoxClick);
    checkboxesSizer->Add(mPlaySinkingMusicCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::ReadSettings()
{
    assert(!!mGameController);


    mMechanicalQualitySlider->SetValue(mGameController->GetNumMechanicalDynamicsIterationsAdjustment());

    mStiffnessSlider->SetValue(mGameController->GetStiffnessAdjustment());

    mStrengthSlider->SetValue(mGameController->GetStrengthAdjustment());



    mWaterDensitySlider->SetValue(mGameController->GetWaterDensityAdjustment());

    mWaterDragSlider->SetValue(mGameController->GetWaterDragAdjustment());

    mWaterIntakeSlider->SetValue(mGameController->GetWaterIntakeAdjustment());

    mWaterCrazynessSlider->SetValue(mGameController->GetWaterCrazyness());

    mWaterDiffusionSpeedSlider->SetValue(mGameController->GetWaterDiffusionSpeedAdjustment());

    mWaterLevelOfDetailSlider->SetValue(mGameController->GetWaterLevelOfDetail());



    mNumberOfStarsSlider->SetValue(static_cast<float>(mGameController->GetNumberOfStars()));

    mNumberOfCloudsSlider->SetValue(static_cast<float>(mGameController->GetNumberOfClouds()));

    mWindSpeedSlider->SetValue(mGameController->GetWindSpeed());



    mWaveHeightSlider->SetValue(mGameController->GetWaveHeight());

    mLuminiscenceSlider->SetValue(mGameController->GetLuminiscenceAdjustment());

    mLightSpreadSlider->SetValue(mGameController->GetLightSpreadAdjustment());

    mSeaDepthSlider->SetValue(mGameController->GetSeaDepth());

    mOceanFloorBumpinessSlider->SetValue(mGameController->GetOceanFloorBumpiness());

    mOceanFloorDetailAmplificationSlider->SetValue(mGameController->GetOceanFloorDetailAmplification());



    mDestroyRadiusSlider->SetValue(mGameController->GetDestroyRadius());

    mBombBlastRadiusSlider->SetValue(mGameController->GetBombBlastRadius());

    mAntiMatterBombImplosionStrengthSlider->SetValue(mGameController->GetAntiMatterBombImplosionStrength());

    mUltraViolentCheckBox->SetValue(mGameController->GetUltraViolentMode());

    mGenerateDebrisCheckBox->SetValue(mGameController->GetDoGenerateDebris());

    mGenerateSparklesCheckBox->SetValue(mGameController->GetDoGenerateSparkles());



    mWaterContrastSlider->SetValue(mGameController->GetWaterContrast());

    mSeaWaterTransparencySlider->SetValue(mGameController->GetSeaWaterTransparency());

    mSeeShipThroughSeaWaterCheckBox->SetValue(mGameController->GetShowShipThroughSeaWater());

    auto shipRenderMode = mGameController->GetShipRenderMode();
    switch (shipRenderMode)
    {
        case ShipRenderMode::Points:
        {
            mShipRenderModeRadioBox->SetSelection(0);
            break;
        }

        case ShipRenderMode::Springs:
        {
            mShipRenderModeRadioBox->SetSelection(1);
            break;
        }

        case ShipRenderMode::EdgeSprings:
        {
            mShipRenderModeRadioBox->SetSelection(2);
            break;
        }

        case ShipRenderMode::Structure:
        {
            mShipRenderModeRadioBox->SetSelection(3);
            break;
        }

        case ShipRenderMode::Texture:
        {
            mShipRenderModeRadioBox->SetSelection(4);
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

    mShowStressCheckBox->SetValue(mGameController->GetShowShipStress());

    mWireframeModeCheckBox->SetValue(mGameController->GetWireframeMode());
    mShipRenderModeRadioBox->Enable(!mGameController->GetWireframeMode());


    mEffectsVolumeSlider->SetValue(mSoundController->GetMasterEffectsVolume());

    mToolsVolumeSlider->SetValue(mSoundController->GetMasterToolsVolume());

    mMusicVolumeSlider->SetValue(mSoundController->GetMasterMusicVolume());

    mPlayBreakSoundsCheckBox->SetValue(mSoundController->GetPlayBreakSounds());

    mPlayStressSoundsCheckBox->SetValue(mSoundController->GetPlayStressSounds());

    mPlaySinkingMusicCheckBox->SetValue(mSoundController->GetPlaySinkingMusic());
}