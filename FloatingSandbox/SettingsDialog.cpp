/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SettingsDialog.h"

#include <GameLib/Log.h>
#include <UILib/ExponentialSliderCore.h>
#include <UILib/LinearSliderCore.h>

#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

static constexpr int SliderWidth = 40;
static constexpr int SliderHeight = 140;

const long ID_ULTRA_VIOLENT_CHECKBOX = wxNewId();
const long ID_QUICK_WATER_FIX_CHECKBOX = wxNewId();
const long ID_SHOW_STRESS_CHECKBOX = wxNewId();

SettingsDialog::SettingsDialog(
    wxWindow* parent,
    std::shared_ptr<GameController> gameController)
    : mParent(parent)
    , mGameController(std::move(gameController))
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
    // Lay out the dialog
    //

    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->AddSpacer(10);
    

    // Controls 1

    wxBoxSizer* controls1Sizer = new wxBoxSizer(wxHORIZONTAL);

    controls1Sizer->AddSpacer(20);


    // Stiffness

    mStiffnessSlider = std::make_unique<SliderControl>(
        this,
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
            mGameController->GetMaxStiffnessAdjustment()));

    controls1Sizer->Add(mStiffnessSlider.get(), 0);

    controls1Sizer->AddSpacer(20);


    // Strength

    mStrengthSlider = std::make_unique<SliderControl>(
        this,
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

    controls1Sizer->Add(mStrengthSlider.get(), 0);
    
    controls1Sizer->AddSpacer(20);


    // Buoyancy
    
    mBuoyancySlider = std::make_unique<SliderControl>(
        this,
        SliderWidth,
        SliderHeight,
        "Buoyancy Adjust",
        mGameController->GetBuoyancyAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinBuoyancyAdjustment(),
            mGameController->GetMaxBuoyancyAdjustment()));
    
    controls1Sizer->Add(mBuoyancySlider.get(), 0);
    
    controls1Sizer->AddSpacer(20);


    // Water Pressure

    mWaterPressureSlider = std::make_unique<SliderControl>(
        this,
        SliderWidth,
        SliderHeight,
        "Water Pressure Adjust",
        mGameController->GetWaterPressureAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinWaterPressureAdjustment(),
            mGameController->GetMaxWaterPressureAdjustment()));

    controls1Sizer->Add(mWaterPressureSlider.get(), 0);

    controls1Sizer->AddSpacer(20);


    // Wave Height

    mWaveHeightSlider = std::make_unique<SliderControl>(
        this,
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
    
    controls1Sizer->Add(mWaveHeightSlider.get(), 0);

    controls1Sizer->AddSpacer(20);


    // Check boxes 1

    wxStaticBoxSizer* checkboxesSizer1 = new wxStaticBoxSizer(wxVERTICAL, this);

    mUltraViolentCheckBox = new wxCheckBox(this, ID_ULTRA_VIOLENT_CHECKBOX, _("Ultra-Violent Mode"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Ultra-Violent Checkbox"));
    Connect(ID_ULTRA_VIOLENT_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnUltraViolentCheckBoxClick);
    checkboxesSizer1->Add(mUltraViolentCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controls1Sizer->Add(checkboxesSizer1, 0);

    controls1Sizer->AddSpacer(20);


    mainSizer->Add(controls1Sizer);

    mainSizer->AddSpacer(20);




    // Controls 2

    wxBoxSizer* controls2Sizer = new wxBoxSizer(wxHORIZONTAL);

    controls2Sizer->AddSpacer(20);

    
    // Water Transparency

    mWaterTransparencySlider = std::make_unique<SliderControl>(
        this,
        SliderWidth,
        SliderHeight,
        "Water Transparency",
        mGameController->GetWaterTransparency(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            1.0f));
    
    controls2Sizer->Add(mWaterTransparencySlider.get(), 0);

    controls2Sizer->AddSpacer(20);


    // Light Diffusion

    mLightDiffusionSlider = std::make_unique<SliderControl>(
        this,
        SliderWidth,
        SliderHeight,
        "Light Diffusion Adjust",
        mGameController->GetLightDiffusionAdjustment(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            1.0f));

    controls2Sizer->Add(mLightDiffusionSlider.get(), 0);

    controls2Sizer->AddSpacer(20);


    // Sea Depth

    mSeaDepthSlider = std::make_unique<SliderControl>(
        this,
        SliderWidth,
        SliderHeight,
        "Ocean Depth",
        mGameController->GetSeaDepth(),
        [this](float /*value*/)
        {
            // Remember we're dirty now
            this->mApplyButton->Enable(true);
        },
        std::make_unique<LinearSliderCore>(
            mGameController->GetMinSeaDepth(),
            mGameController->GetMaxSeaDepth()));

    controls2Sizer->Add(mSeaDepthSlider.get(), 0);

    controls2Sizer->AddSpacer(20);


    // Destroy Radius

    mDestroyRadiusSlider = std::make_unique<SliderControl>(
        this,
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

    controls2Sizer->Add(mDestroyRadiusSlider.get(), 0);

    controls2Sizer->AddSpacer(20);
    

    // Bomb Blast Radius

    mBombBlastRadiusSlider = std::make_unique<SliderControl>(
        this,
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

    controls2Sizer->Add(mBombBlastRadiusSlider.get(), 0);

    controls2Sizer->AddSpacer(20);


    // Check boxes 2

    wxStaticBoxSizer* checkboxesSizer2 = new wxStaticBoxSizer(wxVERTICAL, this);

    mQuickWaterFixCheckBox = new wxCheckBox(this, ID_QUICK_WATER_FIX_CHECKBOX, _("See Ship Through Water"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Quick Water Fix Checkbox"));
    Connect(ID_QUICK_WATER_FIX_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnQuickWaterFixCheckBoxClick);
    checkboxesSizer2->Add(mQuickWaterFixCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    wxString shipRenderModeChoices[] =
    {
        _("Draw Only Points"),
        _("Draw Only Springs"),
        _("Draw Structure"),
        _("Draw Image")
    };

    mShipRenderModeRadioBox = new wxRadioBox(this, wxID_ANY, _("Ship Draw Options"), wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(shipRenderModeChoices), shipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
    Connect(mShipRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnShipRenderModeRadioBox);
    checkboxesSizer2->Add(mShipRenderModeRadioBox, 0, wxALL | wxALIGN_LEFT, 5);

    mShowStressCheckBox = new wxCheckBox(this, ID_SHOW_STRESS_CHECKBOX, _("Show Stress"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Show Stress Checkbox"));
    Connect(ID_SHOW_STRESS_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&SettingsDialog::OnShowStressCheckBoxClick);
    checkboxesSizer2->Add(mShowStressCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controls2Sizer->Add(checkboxesSizer2, 0);

    controls2Sizer->AddSpacer(20);



    mainSizer->Add(controls2Sizer);

    mainSizer->AddSpacer(20);


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

    mainSizer->Add(buttonsSizer, 0, wxALIGN_RIGHT);
    
    mainSizer->AddSpacer(20);



    //
    // Finalize dialog
    //

    SetSizerAndFit(mainSizer);
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

void SettingsDialog::OnQuickWaterFixCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnUltraViolentCheckBoxClick(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnShipRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    // Remember we're dirty now
    mApplyButton->Enable(true);
}

void SettingsDialog::OnShowStressCheckBoxClick(wxCommandEvent & /*event*/)
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

    mGameController->SetStiffnessAdjustment(
        mStiffnessSlider->GetValue());

    mGameController->SetStrengthAdjustment(
        mStrengthSlider->GetValue());

    mGameController->SetBuoyancyAdjustment(
        mBuoyancySlider->GetValue());

    mGameController->SetWaterPressureAdjustment(
        mWaterPressureSlider->GetValue());

    mGameController->SetWaveHeight(
        mWaveHeightSlider->GetValue());

    mGameController->SetWaterTransparency(
        mWaterTransparencySlider->GetValue());

    mGameController->SetLightDiffusionAdjustment(
        mLightDiffusionSlider->GetValue());

    mGameController->SetSeaDepth(
        mSeaDepthSlider->GetValue());

    mGameController->SetDestroyRadius(
        mDestroyRadiusSlider->GetValue());

    mGameController->SetBombBlastRadius(
        mBombBlastRadiusSlider->GetValue());

    mGameController->SetUltraViolentMode(mUltraViolentCheckBox->IsChecked());

    mGameController->SetShowShipThroughWater(mQuickWaterFixCheckBox->IsChecked());

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
        mGameController->SetShipRenderMode(ShipRenderMode::Structure);
    }
    else
    {
        assert(3 == selectedShipRenderMode);
        mGameController->SetShipRenderMode(ShipRenderMode::Texture);
    }

    mGameController->SetShowShipStress(mShowStressCheckBox->IsChecked());
}

void SettingsDialog::ReadSettings()
{
    assert(!!mGameController);

    mUltraViolentCheckBox->SetValue(mGameController->GetUltraViolentMode());

    mQuickWaterFixCheckBox->SetValue(mGameController->GetShowShipThroughWater());

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

        case ShipRenderMode::Structure:
        {
            mShipRenderModeRadioBox->SetSelection(2);
            break;
        }

        case ShipRenderMode::Texture:
        {
            mShipRenderModeRadioBox->SetSelection(3);
            break;
        }
    }

    mShowStressCheckBox->SetValue(mGameController->GetShowShipStress());
}

