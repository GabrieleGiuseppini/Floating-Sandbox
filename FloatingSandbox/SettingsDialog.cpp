/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SettingsDialog.h"

#include "WxHelpers.h"

#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/FixedTickSliderCore.h>
#include <GameCore/IntegralLinearSliderCore.h>
#include <GameCore/LinearSliderCore.h>
#include <GameCore/Log.h>

#include <wx/gbsizer.h>
#include <wx/intl.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <algorithm>
#include <stdexcept>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

static int constexpr SliderWidth = 82; // Min
static int constexpr SliderHeight = 140;

static int constexpr IconInStaticBorderMargin = 4;
static int constexpr TopmostCellOverSliderHeight = 24;
static int constexpr InterCheckboxRowMargin = 4;
static int constexpr StaticBoxInsetMargin = 0;
static int constexpr CellBorderInner = 8;
static int constexpr CellBorderOuter = 4;

namespace /* anonymous */ {

/*
 * Comparer for sorting persistent settings.
 */
struct PersistedSettingsComparer
{
    bool operator()(PersistedSettingsMetadata const & m1, PersistedSettingsMetadata const & m2)
    {
        // m1 < m2
        // Rules:
        // - All user first, system next
        // - Among user, LastModified is last

        if (m1.Key.StorageType != m2.Key.StorageType)
            return m2.Key.StorageType == PersistedSettingsStorageTypes::System;

        assert(m1.Key.StorageType == m2.Key.StorageType);

        if (m1.Key == PersistedSettingsKey::MakeLastModifiedSettingsKey() || m2.Key == PersistedSettingsKey::MakeLastModifiedSettingsKey())
            return m2.Key == PersistedSettingsKey::MakeLastModifiedSettingsKey();

        return m1.Key.Name < m2.Key.Name;
    }
};

}

SettingsDialog::SettingsDialog(
    wxWindow * parent,
    std::shared_ptr<SettingsManager> settingsManager,
    std::shared_ptr<IGameControllerSettingsOptions> gameControllerSettingsOptions,
    ResourceLocator const & resourceLocator)
    : mParent(parent)
    , mSettingsManager(std::move(settingsManager))
    , mGameControllerSettingsOptions(std::move(gameControllerSettingsOptions))
    // State
    , mLiveSettings(mSettingsManager->MakeSettings())
    , mCheckpointSettings(mSettingsManager->MakeSettings())
    , mPersistedSettings()
{
    Create(
        mParent,
        wxID_ANY,
        _("Simulation Settings"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_NO_TASKBAR
        | /* wxFRAME_FLOAT_ON_PARENT */ wxSTAY_ON_TOP, // See https://trac.wxwidgets.org/ticket/18535
        wxS("Settings Window"));

    this->Bind(wxEVT_CLOSE_WINDOW, &SettingsDialog::OnCloseButton, this);

    // Set font
    {
        auto font = parent->GetFont();
        font.SetPointSize(8);
        SetFont(font);
    }

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    SetIcon(wxICON(BBB_SHIP_ICON));

    //
    // Populate and sort persisted settings
    //

    mPersistedSettings = mSettingsManager->ListPersistedSettings();

    PersistedSettingsComparer cmp;
    std::sort(
        mPersistedSettings.begin(),
        mPersistedSettings.end(),
        cmp);

    //
    // Load icons
    //

    mWarningIcon = std::make_unique<wxBitmap>(
        resourceLocator.GetIconFilePath("warning_icon").string(),
        wxBITMAP_TYPE_PNG);

    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxNB_TOP | wxNB_MULTILINE | wxNB_NOPAGETHEME);

    //
    // Mechanics and Thermodynamics
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateMechanicsAndThermodynamicsPanel(panel, resourceLocator);

        notebook->AddPage(panel, _("Mechanics and Thermodynamics"));
    }

    //
    // Water and Ocean
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateWaterAndOceanPanel(panel);

        notebook->AddPage(panel, _("Water and Ocean"));
    }

    //
    // Wind and Waves
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateWindAndWavesPanel(panel);

        notebook->AddPage(panel, _("Wind and Waves"));
    }

    //
    // Air and Sky
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateAirAndSkyPanel(panel);

        notebook->AddPage(panel, _("Air and Sky"));
    }

    //
    // Lights, Electricals, and Fishes
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateLightsElectricalAndFishesPanel(panel);

        notebook->AddPage(panel, _("Lights, Electricals, and Fishes"));
    }

    //
    // Tools
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateToolsPanel(panel, resourceLocator);

        notebook->AddPage(panel, _("Tools"));
    }

    //
    // Rendering
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateRenderingPanel(panel);

        notebook->AddPage(panel, _("Rendering"));
    }

    //
    // Sound and Advanced Settings
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateSoundAndAdvancedSettingsPanel(panel);

        notebook->AddPage(panel, _("Sound and Advanced Settings"));
    }

    //
    // Settings Management
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        PopulateSettingsManagementPanel(panel);

        notebook->AddPage(panel, _("Settings Management"));
    }

    dialogVSizer->Add(notebook, 0);
    dialogVSizer->Fit(notebook); // Workaround for multi-line bug

    dialogVSizer->AddSpacer(20);

    // Buttons

    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        mRevertToDefaultsButton = new wxButton(this, wxID_ANY, _("Revert to Defaults"));
        mRevertToDefaultsButton->SetToolTip(_("Resets all settings to their default values."));
        mRevertToDefaultsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRevertToDefaultsButton, this);
        buttonsSizer->Add(mRevertToDefaultsButton, 0, 0, 0);

        buttonsSizer->AddStretchSpacer(1);

        mOkButton = new wxButton(this, wxID_ANY, _("OK"));
        mOkButton->SetToolTip(_("Closes the window keeping all changes."));
        mOkButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnOkButton, this);
        buttonsSizer->Add(mOkButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mCancelButton = new wxButton(this, wxID_ANY, _("Cancel"));
        mCancelButton->SetToolTip(_("Reverts all changes effected since the window was last opened, and closes the window."));
        mCancelButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnCancelButton, this);
        buttonsSizer->Add(mCancelButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mUndoButton = new wxButton(this, wxID_ANY, _("Undo"));
        mUndoButton->SetToolTip(_("Reverts all changes effected since the window was last opened."));
        mUndoButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnUndoButton, this);
        buttonsSizer->Add(mUndoButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxEXPAND, 0);
    }

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
    if (IsShown())
        return; // Handle Ctrl^S while minimized

    assert(!!mSettingsManager);

    //
    // Initialize state
    //

    // Pull currently-enforced settings
    mSettingsManager->Pull(mLiveSettings);
    mLiveSettings.ClearAllDirty();

    // Save checkpoint for undo
    mCheckpointSettings = mLiveSettings;

    // Populate controls with live settings
    SyncControlsWithSettings(mLiveSettings);

    // Remember that the user hasn't changed anything yet in this session
    mHasBeenDirtyInCurrentSession = false;

    // Enable Revert to Defaults button only if settings are different than defaults
    mAreSettingsDirtyWrtDefaults = (mLiveSettings != mSettingsManager->GetDefaults());

    // Reconcile controls wrt dirty state
    ReconcileDirtyState();

    //
    // Open dialog
    //

    this->Raise();
    this->Show();
}

void SettingsDialog::OnOceanRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    if (mTextureOceanRenderModeRadioButton->GetValue())
    {
        mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderModeType::Texture);
    }
    else if (mDepthOceanRenderModeRadioButton->GetValue())
    {
        mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderModeType::Depth);
    }
    else
    {
        assert(mFlatOceanRenderModeRadioButton->GetValue());
        mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderModeType::Flat);
    }

    OnLiveSettingsChanged();

    ReconciliateOceanRenderModeSettings();
}

void SettingsDialog::OnLandRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    if (mTextureLandRenderModeRadioButton->GetValue())
    {
        mLiveSettings.SetValue(GameSettings::LandRenderMode, LandRenderModeType::Texture);
    }
    else
    {
        assert(mFlatLandRenderModeRadioButton->GetValue());
        mLiveSettings.SetValue(GameSettings::LandRenderMode, LandRenderModeType::Flat);
    }

    ReconciliateLandRenderModeSettings();

    OnLiveSettingsChanged();
}

void SettingsDialog::OnRevertToDefaultsButton(wxCommandEvent & /*event*/)
{
    //
    // Enforce default settings
    //

    mLiveSettings = mSettingsManager->GetDefaults();

    // Do not update checkpoint, allow user to revert to it

    // Enforce everything as a safety net, immediately
    mLiveSettings.MarkAllAsDirty();
    mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);

    // We are back in sync
    mLiveSettings.ClearAllDirty();

    assert(mSettingsManager->Pull() == mLiveSettings);

    // Re-populate controls with new values
    SyncControlsWithSettings(mLiveSettings);

    // Remember user has made changes wrt checkpoint
    mHasBeenDirtyInCurrentSession = true;

    // Remember we are clean now wrt defaults
    mAreSettingsDirtyWrtDefaults = false;

    ReconcileDirtyState();
}

void SettingsDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // Just close the dialog
    DoClose();
}

void SettingsDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    DoCancel();
}

void SettingsDialog::OnUndoButton(wxCommandEvent & /*event*/)
{
    assert(!!mSettingsManager);

    //
    // Undo changes done since last open, including eventual loads
    //

    mLiveSettings = mCheckpointSettings;

    // Just enforce anything in the checkpoint that is different than the current settings,
    // immediately
    mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
    mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);

    mLiveSettings.ClearAllDirty();

    assert(mSettingsManager->Pull() == mCheckpointSettings);

    // Re-populate controls with new values
    SyncControlsWithSettings(mLiveSettings);

    // Remember we are clean now
    mHasBeenDirtyInCurrentSession = false;
    ReconcileDirtyState();
}

void SettingsDialog::OnCloseButton(wxCloseEvent & /*event*/)
{
    DoCancel();
}

/////////////////////////////////////////////////////////////////////////////

void SettingsDialog::DoCancel()
{
    assert(!!mSettingsManager);

    if (mHasBeenDirtyInCurrentSession)
    {
        //
        // Undo changes done since last open, including eventual loads
        //

        mLiveSettings = mCheckpointSettings;

        // Just enforce anything in the checkpoint that is different than the current settings,
        // immediately
        mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
        mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);
    }

    //
    // Close the dialog
    //

    DoClose();
}

void SettingsDialog::DoClose()
{
    this->Hide();
}

void SettingsDialog::PopulateMechanicsAndThermodynamicsPanel(
    wxPanel * panel,
    ResourceLocator const & resourceLocator)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Mechanics
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Mechanics"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Simulation Quality
            {
                mMechanicalQualitySlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Simulation Quality"),
                    _("Higher values improve the rigidity of simulated structures, at the expense of longer computation times."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::NumMechanicalDynamicsIterationsAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<FixedTickSliderCore>(
                        0.5f,
                        mGameControllerSettingsOptions->GetMinNumMechanicalDynamicsIterationsAdjustment(),
                        mGameControllerSettingsOptions->GetMaxNumMechanicalDynamicsIterationsAdjustment()),
                    mWarningIcon.get());

                sizer->Add(
                    mMechanicalQualitySlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Strength Adjust
            {
                mStrengthSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Strength Adjust"),
                    _("Adjusts the breaking point of springs under stress. Has no effect on the rigidity of a ship."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SpringStrengthAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinSpringStrengthAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxSpringStrengthAdjustment()));

                sizer->Add(
                    mStrengthSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Global Damping Adjust
            {
                mGlobalDampingAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Global Damping Adjust"),
                    _("Adjusts the global damping of velocities."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::GlobalDampingAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinGlobalDampingAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxGlobalDampingAdjustment()));

                sizer->Add(
                    mGlobalDampingAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Thermodynamics
    //

    {
        wxStaticBoxSizer * thermodynamicsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Thermodynamics"));

        {
            wxGridBagSizer * thermodynamicsSizer = new wxGridBagSizer(0, 0);

            // Thermal Conductivity Adjustment
            {
                mThermalConductivityAdjustmentSlider = new SliderControl<float>(
                    thermodynamicsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Thermal Conductivity Adjust"),
                    _("Adjusts the speed with which heat propagates along materials."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::ThermalConductivityAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinThermalConductivityAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxThermalConductivityAdjustment()));

                thermodynamicsSizer->Add(
                    mThermalConductivityAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Heat Dissipation Adjustment
            {
                mHeatDissipationAdjustmentSlider = new SliderControl<float>(
                    thermodynamicsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Heat Dissipation Adjust"),
                    _("Adjusts the speed with which materials dissipate or accumulate heat to or from air and water."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::HeatDissipationAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinHeatDissipationAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxHeatDissipationAdjustment()));

                thermodynamicsSizer->Add(
                    mHeatDissipationAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            thermodynamicsBoxSizer->Add(thermodynamicsSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            thermodynamicsBoxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Ultra-Violent Mode
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Ultra-Violent Mode"));

        {
            boxSizer->AddStretchSpacer(1);
        }

        {
            wxBitmap bitmap = wxBitmap(
                resourceLocator.GetIconFilePath("uv_mode_icon").string(),
                wxBITMAP_TYPE_PNG);

            mUltraViolentToggleButton = new wxBitmapToggleButton(boxSizer->GetStaticBox(), wxID_ANY, bitmap);
            mUltraViolentToggleButton->SetToolTip(_("Enables or disables amplification of tool forces and inflicted damages."));
            mUltraViolentToggleButton->Bind(
                wxEVT_TOGGLEBUTTON,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::UltraViolentMode, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(
                mUltraViolentToggleButton,
                1,
                wxALL | wxALIGN_CENTER_HORIZONTAL,
                StaticBoxInsetMargin);
        }

        {
            boxSizer->AddStretchSpacer(1);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 2),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Combustion
    //

    {
        wxStaticBoxSizer * combustionBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Combustion"));

        {
            wxGridBagSizer * combustionSizer = new wxGridBagSizer(0, 0);

            // Ignition Temperature Adjustment
            {
                mIgnitionTemperatureAdjustmentSlider = new SliderControl<float>(
                    combustionBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Burning Point Adjust"),
                    _("Adjusts the temperature at which materials ignite."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::IgnitionTemperatureAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinIgnitionTemperatureAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxIgnitionTemperatureAdjustment()));

                combustionSizer->Add(
                    mIgnitionTemperatureAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Melting Temperature Adjustment
            {
                mMeltingTemperatureAdjustmentSlider = new SliderControl<float>(
                    combustionBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Melting Point Adjust"),
                    _("Adjusts the temperature at which materials melt."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::MeltingTemperatureAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinMeltingTemperatureAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxMeltingTemperatureAdjustment()));

                combustionSizer->Add(
                    mMeltingTemperatureAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Combustion Speed Adjustment
            {
                mCombustionSpeedAdjustmentSlider = new SliderControl<float>(
                    combustionBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Combustion Speed Adjust"),
                    _("Adjusts the rate with which materials consume when burning."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::CombustionSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinCombustionSpeedAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxCombustionSpeedAdjustment()));

                combustionSizer->Add(
                    mCombustionSpeedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Combustion Heat Adjustment
            {
                mCombustionHeatAdjustmentSlider = new SliderControl<float>(
                    combustionBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Combustion Heat Adjust"),
                    _("Adjusts the heat generated by fire; together with the maximum number of burning particles, determines the speed with which fire spreads to adjacent particles."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::CombustionHeatAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinCombustionHeatAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxCombustionHeatAdjustment()));

                combustionSizer->Add(
                    mCombustionHeatAdjustmentSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Max Particles
            {
                mMaxBurningParticlesSlider = new SliderControl<unsigned int>(
                    combustionBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Max Burning Particles"),
                    _("The maximum number of particles that may burn at any given moment in time; together with the combustion heat adjustment, determines the speed with which fire spreads to adjacent particles. Warning: higher values require more computing resources, with the risk of slowing the simulation down!"),
                    [this](unsigned int value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::MaxBurningParticles, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<unsigned int>>(
                        mGameControllerSettingsOptions->GetMinMaxBurningParticles(),
                        mGameControllerSettingsOptions->GetMaxMaxBurningParticles()),
                    mWarningIcon.get());

                combustionSizer->Add(
                    mMaxBurningParticlesSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            combustionBoxSizer->Add(combustionSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            combustionBoxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateWaterAndOceanPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Water
    //

    {
        wxStaticBoxSizer * waterBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Water"));

        {
            wxGridBagSizer * waterSizer = new wxGridBagSizer(0, 0);

            // Water Density
            {
                mWaterDensitySlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Density Adjust"),
                    _("Adjusts the density of sea water, and thus the buoyancy it exerts on physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDensityAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDensityAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterDensityAdjustment()));

                waterSizer->Add(
                    mWaterDensitySlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Friction Drag
            {
                mWaterFrictionDragSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Friction Drag Adjust"),
                    _("Adjusts the frictional drag force (or 'skin' drag) exerted by sea water on physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterFrictionDragAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterFrictionDragAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxWaterFrictionDragAdjustment()));

                waterSizer->Add(
                    mWaterFrictionDragSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Pressure Drag
            {
                mWaterPressureDragSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Pressure Drag Adjust"),
                    _("Adjusts the pressure drag force (or 'form' drag) exerted by sea water on physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterPressureDragAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterPressureDragAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxWaterPressureDragAdjustment()));

                waterSizer->Add(
                    mWaterPressureDragSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Intake
            {
                mWaterIntakeSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Intake Adjust"),
                    _("Adjusts the speed with which sea water enters or leaves a physical body."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterIntakeAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterIntakeAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxWaterIntakeAdjustment()));

                waterSizer->Add(
                    mWaterIntakeSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Diffusion Speed
            {
                mWaterDiffusionSpeedSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Diffusion Speed"),
                    _("Adjusts the speed with which water propagates within a physical body."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDiffusionSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDiffusionSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterDiffusionSpeedAdjustment()));

                waterSizer->Add(
                    mWaterDiffusionSpeedSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Crazyness
            {
                mWaterCrazynessSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Fluid Crazyness"),
                    _("Adjusts how \"splashy\" water flows inside a physical body."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterCrazyness, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterCrazyness(),
                        mGameControllerSettingsOptions->GetMaxWaterCrazyness()));

                waterSizer->Add(
                    mWaterCrazynessSlider,
                    wxGBPosition(0, 5),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Water Temperature
            {
                mWaterTemperatureSlider = new SliderControl<float>(
                    waterBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Temperature"),
                    _("The temperature of water (K)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterTemperature, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterTemperature(),
                        mGameControllerSettingsOptions->GetMaxWaterTemperature()));

                waterSizer->Add(
                    mWaterTemperatureSlider,
                    wxGBPosition(0, 6),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            waterBoxSizer->Add(waterSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            waterBoxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 5),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Ocean
    //

    {
        wxStaticBoxSizer * oceanBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Ocean"));

        {
            wxGridBagSizer * oceanSizer = new wxGridBagSizer(0, 0);

            // Ocean Depth
            {
                mOceanDepthSlider = new SliderControl<float>(
                    oceanBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Depth"),
                    _("The ocean depth (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SeaDepth, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinSeaDepth(),
                        300.0f,
                        mGameControllerSettingsOptions->GetMaxSeaDepth()));

                oceanSizer->Add(
                    mOceanDepthSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            oceanBoxSizer->Add(oceanSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            oceanBoxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Ocean Floor
    //

    {
        wxStaticBoxSizer * oceanFloorBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Ocean Floor"));

        {
            wxGridBagSizer * oceanFloorSizer = new wxGridBagSizer(0, 0);

            // Ocean Floor Bumpiness
            {
                mOceanFloorBumpinessSlider = new SliderControl<float>(
                    oceanFloorBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Bumpiness"),
                    _("Adjusts how much the ocean floor rolls up and down."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorBumpiness, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorBumpiness(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorBumpiness()));

                oceanFloorSizer->Add(
                    mOceanFloorBumpinessSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Restore Ocean Floor Terrain
            {
                wxButton * restoreDefaultTerrainButton = new wxButton(oceanFloorBoxSizer->GetStaticBox(), wxID_ANY, _("Restore Default Terrain"));
                restoreDefaultTerrainButton->SetToolTip(_("Reverts the user-drawn ocean floor terrain to the default terrain."));
                restoreDefaultTerrainButton->Bind(
                    wxEVT_BUTTON,
                    [this](wxCommandEvent &)
                    {
                        mLiveSettings.ClearAllDirty();

                        mLiveSettings.SetValue<OceanFloorTerrain>(
                            GameSettings::OceanFloorTerrain,
                            mSettingsManager->GetDefaults().GetValue<OceanFloorTerrain>(GameSettings::OceanFloorTerrain));

                        OnLiveSettingsChanged();
                    });

                auto sizer = oceanFloorSizer->Add(
                    restoreDefaultTerrainButton,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Ocean Floor Detail Amplification
            {
                mOceanFloorDetailAmplificationSlider = new SliderControl<float>(
                    oceanFloorBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Detail"),
                    _("Adjusts the contrast of the user-drawn ocean floor terrain. Setting this to zero disables the ability to adjust the ocean floor."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorDetailAmplification, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorDetailAmplification(),
                        10.0f,
                        mGameControllerSettingsOptions->GetMaxOceanFloorDetailAmplification()));

                oceanFloorSizer->Add(
                    mOceanFloorDetailAmplificationSlider,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Ocean Floor Elasticity
            {
                mOceanFloorElasticitySlider = new SliderControl<float>(
                    oceanFloorBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Elasticity"),
                    _("Adjusts the elasticity of collisions with the ocean floor."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorElasticity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorElasticity(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorElasticity()));

                oceanFloorSizer->Add(
                    mOceanFloorElasticitySlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Ocean Floor Friction
            {
                mOceanFloorFrictionSlider = new SliderControl<float>(
                    oceanFloorBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Friction"),
                    _("Adjusts the friction exherted by the ocean floor."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorFriction, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorFriction(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorFriction()));

                oceanFloorSizer->Add(
                    mOceanFloorFrictionSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            oceanFloorSizer->AddGrowableRow(1);

            oceanFloorBoxSizer->Add(oceanFloorSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            oceanFloorBoxSizer,
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Rotting
    //

    {
        wxStaticBoxSizer * rottingBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Rotting"));

        {
            wxGridBagSizer * rottingSizer = new wxGridBagSizer(0, 0);

            // Rot Accelerator
            {
                mRotAcceler8rSlider = new SliderControl<float>(
                    rottingBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Rot Acceler8r"),
                    _("Adjusts the speed with which materials rot when exposed to sea water. Set to zero to disable rotting altogether."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RotAcceler8r, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinRotAcceler8r(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxRotAcceler8r()));

                rottingSizer->Add(
                    mRotAcceler8rSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            rottingBoxSizer->Add(rottingSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            rottingBoxSizer,
            wxGBPosition(1, 3),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateWindAndWavesPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Wind
    //

    {
        wxStaticBoxSizer * windBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Wind"));

        {
            wxGridBagSizer * windSizer = new wxGridBagSizer(0, 0);

            // Zero wind
            {
                wxButton * zeroWindButton = new wxButton(windBoxSizer->GetStaticBox(), wxID_ANY, _T("Zero"));
                zeroWindButton->SetToolTip(_("Sets wind speed to zero."));
                zeroWindButton->Bind(
                    wxEVT_BUTTON,
                    [this](wxCommandEvent &)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WindSpeedBase, 0.0f);
                        mWindSpeedBaseSlider->SetValue(0.0f);
                        this->OnLiveSettingsChanged();
                    });

                auto sizer = windSizer->Add(
                    zeroWindButton,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Wind Speed Base
            {
                mWindSpeedBaseSlider = new SliderControl<float>(
                    windBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Base Speed"),
                    _("The base speed of wind (Km/h), before modulation takes place. Wind speed in turn determines ocean wave characteristics such as their height, speed, and width."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WindSpeedBase, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWindSpeedBase(),
                        mGameControllerSettingsOptions->GetMaxWindSpeedBase()));

                windSizer->Add(
                    mWindSpeedBaseSlider,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Modulate Wind
            {
                mModulateWindCheckBox = new wxCheckBox(windBoxSizer->GetStaticBox(), wxID_ANY, _("Modulate Wind"));
                mModulateWindCheckBox->SetToolTip(_("Enables or disables simulation of wind variations, alternating between dead calm and high-speed gusts."));
                mModulateWindCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoModulateWind, event.IsChecked());
                        OnLiveSettingsChanged();

                        mWindGustAmplitudeSlider->Enable(mModulateWindCheckBox->IsChecked());
                    });

                auto sizer = windSizer->Add(
                    mModulateWindCheckBox,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Wind Gust Amplitude
            {
                mWindGustAmplitudeSlider = new SliderControl<float>(
                    windBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Gust Amplitude"),
                    _("The amplitude of wind gusts, as a multiplier of the base wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WindSpeedMaxFactor, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWindSpeedMaxFactor(),
                        mGameControllerSettingsOptions->GetMaxWindSpeedMaxFactor()));

                windSizer->Add(
                    mWindGustAmplitudeSlider,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            windSizer->AddGrowableRow(1);

            windBoxSizer->Add(windSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            windBoxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Oceanic Waves
    //

    {
        wxStaticBoxSizer * wavesBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Oceanic Waves"));

        {
            wxGridBagSizer * wavesSizer = new wxGridBagSizer(0, 0);

            // Basal Wave Height Adjustment
            {
                mBasalWaveHeightAdjustmentSlider = new SliderControl<float>(
                    wavesBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Height Adjust"),
                    _("Adjusts the height of ocean waves wrt their optimal value, which is determined by wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BasalWaveHeightAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBasalWaveHeightAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBasalWaveHeightAdjustment()));

                wavesSizer->Add(
                    mBasalWaveHeightAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Basal Wave Length Adjustment
            {
                mBasalWaveLengthAdjustmentSlider = new SliderControl<float>(
                    wavesBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Width Adjust"),
                    _("Adjusts the width of ocean waves wrt their optimal value, which is determined by wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BasalWaveLengthAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinBasalWaveLengthAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxBasalWaveLengthAdjustment()));

                wavesSizer->Add(
                    mBasalWaveLengthAdjustmentSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Basal Wave Speed Adjustment
            {
                mBasalWaveSpeedAdjustmentSlider = new SliderControl<float>(
                    wavesBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Speed Adjust"),
                    _("Adjusts the speed of ocean waves wrt their optimal value, which is determined by wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BasalWaveSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBasalWaveSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBasalWaveSpeedAdjustment()));

                wavesSizer->Add(
                    mBasalWaveSpeedAdjustmentSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            wavesBoxSizer->Add(wavesSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            wavesBoxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Displacement Waves
    //

    {
        wxStaticBoxSizer * displacementWavesBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Displacement Waves"));

        {
            wxGridBagSizer * displacementWavesSizer = new wxGridBagSizer(0, 0);

            // Displace Water
            {
                mDoDisplaceWaterCheckBox = new wxCheckBox(displacementWavesBoxSizer->GetStaticBox(), wxID_ANY, _("Displace Water"));
                mDoDisplaceWaterCheckBox->SetToolTip(_("Enables or disables displacement of water due to interactions with physical objects."));
                mDoDisplaceWaterCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoDisplaceWater, event.IsChecked());
                        OnLiveSettingsChanged();

                        mWaterDisplacementWaveHeightAdjustmentSlider->Enable(mDoDisplaceWaterCheckBox->IsChecked());
                    });

                auto sizer = displacementWavesSizer->Add(
                    mDoDisplaceWaterCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Water Displacement Wave Height Adjust
            {
                mWaterDisplacementWaveHeightAdjustmentSlider = new SliderControl<float>(
                    displacementWavesBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Height Adjust"),
                    _("Adjusts the magnitude of the waves caused by water displacement."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDisplacementWaveHeightAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDisplacementWaveHeightAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterDisplacementWaveHeightAdjustment()));

                displacementWavesSizer->Add(
                    mWaterDisplacementWaveHeightAdjustmentSlider,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Wave Smoothness Adjust
            {
                mWaveSmoothnessAdjustmentSlider = new SliderControl<float>(
                    displacementWavesBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Smoothness Adjust"),
                    _("Adjusts the smoothness of waves caused by water displacement."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaveSmoothnessAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaveSmoothnessAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaveSmoothnessAdjustment()));

                displacementWavesSizer->Add(
                    mWaveSmoothnessAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            displacementWavesSizer->AddGrowableRow(1);

            displacementWavesBoxSizer->Add(displacementWavesSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            displacementWavesBoxSizer,
            wxGBPosition(0, 3),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Wave Phenomena
    //

    {
        wxStaticBoxSizer * wavePhenomenaBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Wave Phenomena"));

        {
            wxGridBagSizer * wavePhenomenaSizer = new wxGridBagSizer(0, 0);

            // Tsunami Rate
            {
                mTsunamiRateSlider = new SliderControl<std::chrono::minutes::rep>(
                    wavePhenomenaBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Tsunami Rate"),
                    _("The expected time between two automatically-generated tsunami waves (minutes). Set to zero to disable automatic generation of tsunami waves altogether."),
                    [this](std::chrono::minutes::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::TsunamiRate, std::chrono::minutes(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::minutes::rep>>(
                        mGameControllerSettingsOptions->GetMinTsunamiRate().count(),
                        mGameControllerSettingsOptions->GetMaxTsunamiRate().count()));

                wavePhenomenaSizer->Add(
                    mTsunamiRateSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Rogue Wave Rate
            {
                mRogueWaveRateSlider = new SliderControl<std::chrono::seconds::rep>(
                    wavePhenomenaBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Rogue Wave Rate"),
                    _("The expected time between two automatically-generated rogue waves (seconds). Set to zero to disable automatic generation of rogue waves altogether."),
                    [this](std::chrono::seconds::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RogueWaveRate, std::chrono::seconds(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::seconds::rep>>(
                        mGameControllerSettingsOptions->GetMinRogueWaveRate().count(),
                        mGameControllerSettingsOptions->GetMaxRogueWaveRate().count()));

                wavePhenomenaSizer->Add(
                    mRogueWaveRateSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            wavePhenomenaBoxSizer->Add(wavePhenomenaSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            wavePhenomenaBoxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Storms
    //

    {
        wxStaticBoxSizer * stormsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Storms"));

        {
            wxGridBagSizer * stormsSizer = new wxGridBagSizer(0, 0);

            // Storm Strength Adjustment
            {
                mStormStrengthAdjustmentSlider = new SliderControl<float>(
                    stormsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Strength Adjust"),
                    _("Adjusts the strength of storms."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::StormStrengthAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinStormStrengthAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxStormStrengthAdjustment()));

                stormsSizer->Add(
                    mStormStrengthAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Do rain with storm
            {
                mDoRainWithStormCheckBox = new wxCheckBox(stormsBoxSizer->GetStaticBox(), wxID_ANY, _("Spawn Rain"));
                mDoRainWithStormCheckBox->SetToolTip(_("Enables or disables generation of rain during a storm."));
                mDoRainWithStormCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoRainWithStorm, event.IsChecked());
                        OnLiveSettingsChanged();

                        mRainFloodAdjustmentSlider->Enable(event.IsChecked());
                    });

                auto sizer = stormsSizer->Add(
                    mDoRainWithStormCheckBox,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Rain Flood Adjustment
            {
                mRainFloodAdjustmentSlider = new SliderControl<float>(
                    stormsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Rain Flood Adjust"),
                    _("Adjusts the extent to which rain floods exposed areas of a ship."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RainFloodAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinRainFloodAdjustment(),
                        10000.0f,
                        mGameControllerSettingsOptions->GetMaxRainFloodAdjustment()));

                stormsSizer->Add(
                    mRainFloodAdjustmentSlider,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Storm Duration
            {
                mStormDurationSlider = new SliderControl<std::chrono::seconds::rep>(
                    stormsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Duration"),
                    _("The duration of a storm (s)."),
                    [this](std::chrono::seconds::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::StormDuration, std::chrono::seconds(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::seconds::rep>>(
                        mGameControllerSettingsOptions->GetMinStormDuration().count(),
                        mGameControllerSettingsOptions->GetMaxStormDuration().count()));

                stormsSizer->Add(
                    mStormDurationSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Storm Rate
            {
                mStormRateSlider = new SliderControl<std::chrono::minutes::rep>(
                    stormsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Rate"),
                    _("The expected time between two automatically-generated storms (minutes). Set to zero to disable automatic generation of storms altogether."),
                    [this](std::chrono::minutes::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::StormRate, std::chrono::minutes(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::minutes::rep>>(
                        mGameControllerSettingsOptions->GetMinStormRate().count(),
                        mGameControllerSettingsOptions->GetMaxStormRate().count()));

                stormsSizer->Add(
                    mStormRateSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            stormsSizer->AddGrowableRow(1);

            stormsBoxSizer->Add(stormsSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            stormsBoxSizer,
            wxGBPosition(1, 2),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateAirAndSkyPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Air
    //

    {
        wxStaticBoxSizer * airBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Air"));

        {
            wxGridBagSizer * airSizer = new wxGridBagSizer(0, 0);

            // Air Friction Drag
            {
                mAirFrictionDragSlider = new SliderControl<float>(
                    airBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Friction Drag Adjust"),
                    _("Adjusts the frictional drag force (or 'skin' drag) exerted by air on physical bodies."),
                    [this](float value)
                {
                    this->mLiveSettings.SetValue(GameSettings::AirFrictionDragAdjustment, value);
                    this->OnLiveSettingsChanged();
                },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinAirFrictionDragAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxAirFrictionDragAdjustment()));

                airSizer->Add(
                    mAirFrictionDragSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Air Pressure Drag
            {
                mAirPressureDragSlider = new SliderControl<float>(
                    airBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Pressure Drag Adjust"),
                    _("Adjusts the pressure drag force (or 'form' drag) exerted by air on physical bodies."),
                    [this](float value)
                {
                    this->mLiveSettings.SetValue(GameSettings::AirPressureDragAdjustment, value);
                    this->OnLiveSettingsChanged();
                },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinAirPressureDragAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxAirPressureDragAdjustment()));

                airSizer->Add(
                    mAirPressureDragSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Air Temperature
            {
                mAirTemperatureSlider = new SliderControl<float>(
                    airBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Temperature"),
                    _("The temperature of air (K)."),
                    [this](float value)
                {
                    this->mLiveSettings.SetValue(GameSettings::AirTemperature, value);
                    this->OnLiveSettingsChanged();
                },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinAirTemperature(),
                        mGameControllerSettingsOptions->GetMaxAirTemperature()));

                airSizer->Add(
                    mAirTemperatureSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Air Bubbles Density
            {
                mAirBubbleDensitySlider = new SliderControl<float>(
                    airBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Air Bubbles Density"),
                    _("The density of air bubbles generated when water enters a ship."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::AirBubblesDensity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinAirBubblesDensity(),
                        mGameControllerSettingsOptions->GetMaxAirBubblesDensity()));

                airSizer->Add(
                    mAirBubbleDensitySlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            airBoxSizer->Add(airSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            airBoxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Smoke
    //

    {
        wxStaticBoxSizer * smokeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Smoke"));

        {
            wxGridBagSizer * smokeSizer = new wxGridBagSizer(0, 0);

            // Smoke Density Adjust
            {
                mSmokeEmissionDensityAdjustmentSlider = new SliderControl<float>(
                    smokeBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Density Adjust"),
                    _("Adjusts the density of smoke particles."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SmokeEmissionDensityAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinSmokeEmissionDensityAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxSmokeEmissionDensityAdjustment()));

                smokeSizer->Add(
                    mSmokeEmissionDensityAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Smoke Persistence Adjust
            {
                mSmokeParticleLifetimeAdjustmentSlider = new SliderControl<float>(
                    smokeBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Persistence Adjust"),
                    _("Adjusts how long it takes for smoke to vanish."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SmokeParticleLifetimeAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinSmokeParticleLifetimeAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxSmokeParticleLifetimeAdjustment()));

                smokeSizer->Add(
                    mSmokeParticleLifetimeAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }


            smokeBoxSizer->Add(smokeSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            smokeBoxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Sky
    //

    {
        wxStaticBoxSizer * skyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Sky"));

        {
            wxGridBagSizer * skySizer = new wxGridBagSizer(0, 0);

            // Number of Stars
            {
                mNumberOfStarsSlider = new SliderControl<unsigned int>(
                    skyBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Number of Stars"),
                    _("The number of stars in the sky."),
                    [this](unsigned int value)
                {
                    this->mLiveSettings.SetValue(GameSettings::NumberOfStars, value);
                    this->OnLiveSettingsChanged();
                },
                    std::make_unique<IntegralLinearSliderCore<unsigned int>>(
                        mGameControllerSettingsOptions->GetMinNumberOfStars(),
                        mGameControllerSettingsOptions->GetMaxNumberOfStars()));

                skySizer->Add(
                    mNumberOfStarsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Number of Clouds
            {
                mNumberOfCloudsSlider = new SliderControl<unsigned int>(
                    skyBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Number of Clouds"),
                    _("The number of clouds in the world's sky. This is the total number of clouds in the world; at any moment in time, the number of clouds that are visible will be less than or equal to this value."),
                    [this](unsigned int value)
                {
                    this->mLiveSettings.SetValue(GameSettings::NumberOfClouds, value);
                    this->OnLiveSettingsChanged();
                },
                    std::make_unique<IntegralLinearSliderCore<unsigned int>>(
                        mGameControllerSettingsOptions->GetMinNumberOfClouds(),
                        mGameControllerSettingsOptions->GetMaxNumberOfClouds()));

                skySizer->Add(
                    mNumberOfCloudsSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Do daylight cycle
            {
                mDoDayLightCycleCheckBox = new wxCheckBox(skyBoxSizer->GetStaticBox(), wxID_ANY, _("Automatic Daylight Cycle"));
                mDoDayLightCycleCheckBox->SetToolTip(_("Enables or disables automatic cycling of daylight."));
                mDoDayLightCycleCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoDayLightCycle, event.IsChecked());
                        OnLiveSettingsChanged();

                        mDayLightCycleDurationSlider->Enable(event.IsChecked());
                    });

                auto sizer = skySizer->Add(
                    mDoDayLightCycleCheckBox,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                sizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Daylight Cycle Duration
            {
                mDayLightCycleDurationSlider = new SliderControl<std::chrono::minutes::rep>(
                    skyBoxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Daylight Cycle Duration"),
                    _("The duration of a full daylight cycle (minutes)."),
                    [this](std::chrono::minutes::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::DayLightCycleDuration, std::chrono::minutes(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::minutes::rep>>(
                        mGameControllerSettingsOptions->GetMinDayLightCycleDuration().count(),
                        mGameControllerSettingsOptions->GetMaxDayLightCycleDuration().count()));

                skySizer->Add(
                    mDayLightCycleDurationSlider,
                    wxGBPosition(1, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            skySizer->AddGrowableRow(1);

            skyBoxSizer->Add(skySizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            skyBoxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateLightsElectricalAndFishesPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Lights
    //

    {
        wxStaticBoxSizer * lightsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Lights"));

        {
            wxGridBagSizer * lightsSizer = new wxGridBagSizer(0, 0);

            // Luminiscence Adjust
            {
                mLuminiscenceSlider = new SliderControl<float>(
                    lightsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Luminiscence Adjust"),
                    _("Adjusts the quantity of light emitted by luminiscent materials."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::LuminiscenceAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinLuminiscenceAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxLuminiscenceAdjustment()));

                lightsSizer->Add(
                    mLuminiscenceSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Light Spread
            {
                mLightSpreadSlider = new SliderControl<float>(
                    lightsBoxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Spread Adjust"),
                    _("Adjusts how wide light emitted by luminiscent materials spreads out."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::LightSpreadAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinLightSpreadAdjustment(),
                        mGameControllerSettingsOptions->GetMaxLightSpreadAdjustment()));

                lightsSizer->Add(
                    mLightSpreadSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            lightsBoxSizer->Add(lightsSizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            lightsBoxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Electricals
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Electricals"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Generate Engine Wake
            {
                mGenerateEngineWakeCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Generate Engine Wake"));
                mGenerateEngineWakeCheckBox->SetToolTip(_("Enables or disables generation of wakes when engines are running underwater."));
                mGenerateEngineWakeCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoGenerateEngineWakeParticles, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                auto cellSizer = sizer->Add(
                    mGenerateEngineWakeCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                cellSizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Engine Thrust Adjust
            {
                mEngineThrustAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Engine Thrust Adjust"),
                    _("Adjusts the thrust exerted by engines."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::EngineThrustAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinEngineThrustAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxEngineThrustAdjustment()));

                sizer->Add(
                    mEngineThrustAdjustmentSlider,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Water Pump Power Adjust
            {
                mWaterPumpPowerAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Water Pump Power Adjust"),
                    _("Adjusts the power of water pumps."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterPumpPowerAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterPumpPowerAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxWaterPumpPowerAdjustment()));

                sizer->Add(
                    mWaterPumpPowerAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Heat Generation Adjustment
            {
                mElectricalElementHeatProducedAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Heat Generation Adjust"),
                    _("Adjusts the amount of heat generated by working electrical elements, such as lamps and generators."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::ElectricalElementHeatProducedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinElectricalElementHeatProducedAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxElectricalElementHeatProducedAdjustment()));

                sizer->Add(
                    mElectricalElementHeatProducedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Fishes
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Fishes"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Number of Fishes
            {
                mNumberOfFishesSlider = new SliderControl<unsigned int>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Number of Fishes"),
                    _("The number of fishes in the ocean."),
                    [this](unsigned int value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::NumberOfFishes, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<unsigned int>>(
                        mGameControllerSettingsOptions->GetMinNumberOfFishes(),
                        mGameControllerSettingsOptions->GetMaxNumberOfFishes()));

                sizer->Add(
                    mNumberOfFishesSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Fish Size Multiplier
            {
                mFishSizeMultiplierSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Size Multiplier"),
                    _("Magnifies or minimizes the physical size of fishes."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FishSizeMultiplier, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFishSizeMultiplier(),
                        mGameControllerSettingsOptions->GetMaxFishSizeMultiplier()));

                sizer->Add(
                    mFishSizeMultiplierSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Fish Speed Adjustment
            {
                mFishSpeedAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Speed Adjust"),
                    _("Adjusts the speed of fishes."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FishSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFishSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxFishSpeedAdjustment()));

                sizer->Add(
                    mFishSpeedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Do shoaling
            {
                mDoFishShoalingCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Enable Shoaling"));
                mDoFishShoalingCheckBox->SetToolTip(_("Enables or disables shoaling behavior in fishes."));
                mDoFishShoalingCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoFishShoaling, event.IsChecked());
                        OnLiveSettingsChanged();

                        mFishShoalRadiusAdjustmentSlider->Enable(event.IsChecked());
                    });

                auto cellSizer = sizer->Add(
                    mDoFishShoalingCheckBox,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);

                cellSizer->SetMinSize(-1, TopmostCellOverSliderHeight);
            }

            // Shoal Radius Adjustment
            {
                mFishShoalRadiusAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Shoal Radius Adjust"),
                    _("Adjusts the radius of the neighborhood tracked by fishes in a shoal."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FishShoalRadiusAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinFishShoalRadiusAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxFishShoalRadiusAdjustment()));

                sizer->Add(
                    mFishShoalRadiusAdjustmentSlider,
                    wxGBPosition(1, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateToolsPanel(
    wxPanel * panel,
    ResourceLocator const & resourceLocator)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Smash Tool
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Smash Tool"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("smash_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Destroy Radius
            {
                mDestroyRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Destroy Radius"),
                    _("The starting radius of the damage caused by destructive tools (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::DestroyRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinDestroyRadius(),
                        mGameControllerSettingsOptions->GetMaxDestroyRadius()));

                sizer->Add(
                    mDestroyRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Bombs
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Bombs"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("impact_bomb_cursor").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("rc_bomb_cursor").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("timer_bomb_cursor").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("am_bomb_cursor").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(3, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Bomb Blast Radius
            {
                mBombBlastRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Blast Radius"),
                    _("The radius of bomb explosions (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BombBlastRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBombBlastRadius(),
                        mGameControllerSettingsOptions->GetMaxBombBlastRadius()));

                sizer->Add(
                    mBombBlastRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(4, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Bomb Blast Force Adjustment
            {
                mBombBlastForceAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Blast Force Adjust"),
                    _("Adjusts the blast force generated by bomb explosions."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BombBlastForceAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBombBlastForceAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBombBlastForceAdjustment()));

                sizer->Add(
                    mBombBlastForceAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(4, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Bomb Blast Heat
            {
                mBombBlastHeatSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Blast Heat"),
                    _("The heat generated by bomb explosions (KJ/s)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BombBlastHeat, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinBombBlastHeat(),
                        40000.0f,
                        mGameControllerSettingsOptions->GetMaxBombBlastHeat()));

                sizer->Add(
                    mBombBlastHeatSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(4, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Anti-matter Bomb Implosion Strength
            {
                mAntiMatterBombImplosionStrengthSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("AM Bomb Implosion Strength"),
                    _("Adjusts the strength of the initial anti-matter bomb implosion."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::AntiMatterBombImplosionStrength, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinAntiMatterBombImplosionStrength(),
                        mGameControllerSettingsOptions->GetMaxAntiMatterBombImplosionStrength()));

                sizer->Add(
                    mAntiMatterBombImplosionStrengthSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(4, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Blast Tool
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Blast Tool"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("blast_cursor_up_1").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Blast Tool Radius
            {
                mBlastToolRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Radius"),
                    _("The radius of the blast tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BlastToolRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBlastToolRadius(),
                        mGameControllerSettingsOptions->GetMaxBlastToolRadius()));

                sizer->Add(
                    mBlastToolRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Blast Tool Force Adjustment
            {
                mBlastToolForceAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Force Adjust"),
                    _("Adjusts the blast force generated by the Blast tool."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BlastToolForceAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBlastToolForceAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBlastToolForceAdjustment()));

                sizer->Add(
                    mBlastToolForceAdjustmentSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 5),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    /////////////////////////////////////

    //
    // Scrub/Rot Tool
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Scrub/Rot Tool"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("scrub_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("rot_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Scrub/Rot Radius
            {
                mScrubRotRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Radius"),
                    _("How wide an area is affected by the scrub/rot tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::ScrubRotRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinScrubRotRadius(),
                        mGameControllerSettingsOptions->GetMaxScrubRotRadius()));

                sizer->Add(
                    mScrubRotRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Flood Tool
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Flood Tool"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("flood_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Flood Radius
            {
                mFloodRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Radius"),
                    _("How wide an area is flooded or drained by the flood tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FloodRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFloodRadius(),
                        mGameControllerSettingsOptions->GetMaxFloodRadius()));

                sizer->Add(
                    mFloodRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Flood Quantity
            {
                mFloodQuantitySlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Flow"),
                    _("How much water is injected or drained by the flood tool (m3)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FloodQuantity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFloodQuantity(),
                        mGameControllerSettingsOptions->GetMaxFloodQuantity()));

                sizer->Add(
                    mFloodQuantitySlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 1),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Repair Tool
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Repair Tool"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("repair_structure_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Repair Radius
            {
                mRepairRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Radius"),
                    _("Adjusts the radius of the repair tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RepairRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinRepairRadius(),
                        mGameControllerSettingsOptions->GetMaxRepairRadius()));

                sizer->Add(
                    mRepairRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Repair Speed Adjustment
            {
                mRepairSpeedAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Speed Adjust"),
                    _("Adjusts the speed with which the repair tool attracts particles to repair damage. Warning: at high speeds the repair tool might become destructive!"),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RepairSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinRepairSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxRepairSpeedAdjustment()));

                sizer->Add(
                    mRepairSpeedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 3),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // HeatBlaster
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("HeatBlaster"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Icon
            {
                wxBitmap bitmap = wxBitmap(
                    resourceLocator.GetCursorFilePath("heat_blaster_heat_cursor_up").string(),
                    wxBITMAP_TYPE_PNG);

                auto staticBitmap = new wxStaticBitmap(boxSizer->GetStaticBox(), wxID_ANY, bitmap);

                sizer->Add(
                    staticBitmap,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxTOP | wxLEFT,
                    IconInStaticBorderMargin);
            }

            // Radius
            {
                mHeatBlasterRadiusSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Radius"),
                    _("The radius of HeatBlaster tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::HeatBlasterRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinHeatBlasterRadius(),
                        mGameControllerSettingsOptions->GetMaxHeatBlasterRadius()));

                sizer->Add(
                    mHeatBlasterRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Heat flow
            {
                mHeatBlasterHeatFlowSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Heat"),
                    _("The heat produced by the HeatBlaster tool (KJ/s)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::HeatBlasterHeatFlow, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinHeatBlasterHeatFlow(),
                        2000.0f,
                        mGameControllerSettingsOptions->GetMaxHeatBlasterHeatFlow()));

                sizer->Add(
                    mHeatBlasterHeatFlowSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 5),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Sea
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Sea"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Ocean Render Mode
            {
                wxStaticBoxSizer * oceanRenderModeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, boxSizer->GetStaticBox(), _("Draw Mode"));

                {
                    wxGridBagSizer * oceanRenderModeSizer = new wxGridBagSizer(3, 3);

                    mTextureOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a static pattern."));
                    mTextureOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);

                    oceanRenderModeSizer->Add(mTextureOceanRenderModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mTextureOceanComboBox = new wxBitmapComboBox(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                        wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_READONLY);
                    for (auto const & entry : mGameControllerSettingsOptions->GetTextureOceanAvailableThumbnails())
                    {
                        mTextureOceanComboBox->Append(
                            entry.first,
                            WxHelpers::MakeBitmap(entry.second));
                    }
                    mTextureOceanComboBox->SetToolTip(_("Sets the texture to use for the ocean."));
                    mTextureOceanComboBox->Bind(
                        wxEVT_COMBOBOX,
                        [this](wxCommandEvent & /*event*/)
                        {
                            mLiveSettings.SetValue(GameSettings::TextureOceanTextureIndex, static_cast<size_t>(mTextureOceanComboBox->GetSelection()));
                            OnLiveSettingsChanged();
                        });

                    oceanRenderModeSizer->Add(mTextureOceanComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL | wxEXPAND, 0);

                    //

                    mDepthOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, _("Depth Gradient"));
                    mDepthOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a vertical color gradient."));
                    mDepthOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);

                    oceanRenderModeSizer->Add(mDepthOceanRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mDepthOceanColorStartPicker = new wxColourPickerCtrl(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, wxColour("WHITE"));
                    mDepthOceanColorStartPicker->SetToolTip(_("Sets the starting (top) color of the gradient."));
                    mDepthOceanColorStartPicker->Bind(
                        wxEVT_COLOURPICKER_CHANGED,
                        [this](wxColourPickerEvent & event)
                        {
                            auto const color = event.GetColour();

                            mLiveSettings.SetValue(
                                GameSettings::DepthOceanColorStart,
                                rgbColor(color.Red(), color.Green(), color.Blue()));

                            OnLiveSettingsChanged();
                        });

                    oceanRenderModeSizer->Add(mDepthOceanColorStartPicker, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 0);

                    mDepthOceanColorEndPicker = new wxColourPickerCtrl(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, wxColour("WHITE"));
                    mDepthOceanColorEndPicker->SetToolTip(_("Sets the ending (bottom) color of the gradient."));
                    mDepthOceanColorEndPicker->Bind(
                        wxEVT_COLOURPICKER_CHANGED,
                        [this](wxColourPickerEvent & event)
                        {
                            auto color = event.GetColour();

                            mLiveSettings.SetValue(
                                GameSettings::DepthOceanColorEnd,
                                rgbColor(color.Red(), color.Green(), color.Blue()));

                            OnLiveSettingsChanged();
                        });

                    oceanRenderModeSizer->Add(mDepthOceanColorEndPicker, wxGBPosition(1, 2), wxGBSpan(1, 1), wxALL, 0);

                    //

                    mFlatOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, _("Flat"));
                    mFlatOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a single color."));
                    mFlatOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);

                    oceanRenderModeSizer->Add(mFlatOceanRenderModeRadioButton, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatOceanColorPicker = new wxColourPickerCtrl(oceanRenderModeBoxSizer->GetStaticBox(), wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanColorPicker->SetToolTip(_("Sets the single color of the ocean."));
                    mFlatOceanColorPicker->Bind(
                        wxEVT_COLOURPICKER_CHANGED,
                        [this](wxColourPickerEvent & event)
                        {
                            auto color = event.GetColour();

                            mLiveSettings.SetValue(
                                GameSettings::FlatOceanColor,
                                rgbColor(color.Red(), color.Green(), color.Blue()));

                            OnLiveSettingsChanged();
                        });

                    oceanRenderModeSizer->Add(mFlatOceanColorPicker, wxGBPosition(2, 1), wxGBSpan(1, 1), wxALL, 0);

                    oceanRenderModeBoxSizer->Add(oceanRenderModeSizer, 0, wxALL, StaticBoxInsetMargin);
                }

                sizer->Add(
                    oceanRenderModeBoxSizer,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // High-Quality Rendering
            {
                mOceanRenderDetailModeDetailedCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("High-Quality Rendering"));
                mOceanRenderDetailModeDetailedCheckBox->SetToolTip(_("Renders the ocean with additional details. Requires more computational resources."));
                mOceanRenderDetailModeDetailedCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::OceanRenderDetail, event.IsChecked() ? OceanRenderDetailType::Detailed : OceanRenderDetailType::Basic);
                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mOceanRenderDetailModeDetailedCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // See Ship Through Water
            {
                mSeeShipThroughOceanCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("See Ship Through Water"));
                mSeeShipThroughOceanCheckBox->SetToolTip(_("Shows the ship either behind the sea water or in front of it."));
                mSeeShipThroughOceanCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::ShowShipThroughOcean, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mSeeShipThroughOceanCheckBox,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Ocean Transparency
            {
                mOceanTransparencySlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Transparency"),
                    _("Adjusts the transparency of sea water."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanTransparency, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                sizer->Add(
                    mOceanTransparencySlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(3, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Ocean Darkening Rate
            {
                mOceanDarkeningRateSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Darkening Rate"),
                    _("Adjusts the rate at which the ocean darkens with depth."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanDarkeningRate, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        0.0f,
                        0.2f,
                        1.0f));

                sizer->Add(
                    mOceanDarkeningRateSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(3, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 1, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(2, 5),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Land
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Land"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Land Render Mode
            {
                wxStaticBoxSizer * landRenderModeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, boxSizer->GetStaticBox(), _("Draw Mode"));

                {
                    wxGridBagSizer * landRenderModeSizer = new wxGridBagSizer(5, 5);

                    mTextureLandRenderModeRadioButton = new wxRadioButton(landRenderModeBoxSizer->GetStaticBox(), wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureLandRenderModeRadioButton->SetToolTip(_("Draws the ocean floor using a static image."));
                    mTextureLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnLandRenderModeRadioButtonClick, this);

                    landRenderModeSizer->Add(mTextureLandRenderModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mTextureLandComboBox = new wxBitmapComboBox(landRenderModeBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString,
                        wxDefaultPosition, wxSize(140, -1), wxArrayString(), wxCB_READONLY);
                    for (auto const & entry : mGameControllerSettingsOptions->GetTextureLandAvailableThumbnails())
                    {
                        mTextureLandComboBox->Append(
                            entry.first,
                            WxHelpers::MakeBitmap(entry.second));
                    }
                    mTextureLandComboBox->SetToolTip(_("Sets the texture to use for the ocean floor."));
                    mTextureLandComboBox->Bind(
                        wxEVT_COMBOBOX,
                        [this](wxCommandEvent & /*event*/)
                        {
                            mLiveSettings.SetValue(GameSettings::TextureLandTextureIndex, static_cast<size_t>(mTextureLandComboBox->GetSelection()));
                            OnLiveSettingsChanged();
                        });

                    landRenderModeSizer->Add(mTextureLandComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL, 0);

                    mFlatLandRenderModeRadioButton = new wxRadioButton(landRenderModeBoxSizer->GetStaticBox(), wxID_ANY, _("Flat"));
                    mFlatLandRenderModeRadioButton->SetToolTip(_("Draws the ocean floor using a static color."));
                    mFlatLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnLandRenderModeRadioButtonClick, this);

                    landRenderModeSizer->Add(mFlatLandRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatLandColorPicker = new wxColourPickerCtrl(landRenderModeBoxSizer->GetStaticBox(), wxID_ANY);
                    mFlatLandColorPicker->SetToolTip(_("Sets the single color of the ocean floor."));
                    mFlatLandColorPicker->Bind(
                        wxEVT_COLOURPICKER_CHANGED,
                        [this](wxColourPickerEvent & event)
                        {
                            auto color = event.GetColour();

                            mLiveSettings.SetValue(
                                GameSettings::FlatLandColor,
                                rgbColor(color.Red(), color.Green(), color.Blue()));

                            OnLiveSettingsChanged();
                        });

                    landRenderModeSizer->Add(mFlatLandColorPicker, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 0);

                    landRenderModeBoxSizer->Add(landRenderModeSizer, 0, wxALL, StaticBoxInsetMargin);
                }

                sizer->Add(
                    landRenderModeBoxSizer,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 5),
            wxGBSpan(1, 5),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Sky
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Sky"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Sky color
            {
                mFlatSkyColorPicker = new wxColourPickerCtrl(boxSizer->GetStaticBox(), wxID_ANY);
                mFlatSkyColorPicker->SetToolTip(_("Sets the color of the sky. Duh."));
                mFlatSkyColorPicker->Bind(
                    wxEVT_COLOURPICKER_CHANGED,
                    [this](wxColourPickerEvent & event)
                    {
                        auto color = event.GetColour();

                        mLiveSettings.SetValue(
                            GameSettings::FlatSkyColor,
                            rgbColor(color.Red(), color.Green(), color.Blue()));

                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mFlatSkyColorPicker,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 5),
            wxGBSpan(1, 2),
            wxALL | wxALIGN_LEFT,
            CellBorderInner);
    }

    // Lamp Light
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Lamp Light"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Lamp Light color
            {
                mFlatLampLightColorPicker = new wxColourPickerCtrl(boxSizer->GetStaticBox(), wxID_ANY);
                mFlatLampLightColorPicker->SetToolTip(_("Sets the color of lamp lights."));
                mFlatLampLightColorPicker->Bind(
                    wxEVT_COLOURPICKER_CHANGED,
                    [this](wxColourPickerEvent & event)
                    {
                        auto color = event.GetColour();

                        mLiveSettings.SetValue(
                            GameSettings::FlatLampLightColor,
                            rgbColor(color.Red(), color.Green(), color.Blue()));

                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mFlatLampLightColorPicker,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 8),
            wxGBSpan(1, 2),
            wxALL | wxALIGN_RIGHT,
            CellBorderInner);
    }

    // Heat
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Heat"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Heat render mode
            {
                wxString heatRenderModeChoices[] =
                {
                    _("Incandescence"),
                    _("Heat Overlay"),
                    _("None")
                };

                mHeatRenderModeRadioBox = new wxRadioBox(boxSizer->GetStaticBox(), wxID_ANY, _("Heat Draw Options"), wxDefaultPosition, wxDefaultSize,
                    WXSIZEOF(heatRenderModeChoices), heatRenderModeChoices, 1, wxRA_SPECIFY_COLS);
                mHeatRenderModeRadioBox->SetToolTip(_("Selects how heat is rendered on the ship."));
                mHeatRenderModeRadioBox->Bind(
                    wxEVT_RADIOBOX,
                    [this](wxCommandEvent & event)
                    {
                        auto const selectedHeatRenderMode = event.GetSelection();
                        if (0 == selectedHeatRenderMode)
                        {
                            mLiveSettings.SetValue(GameSettings::HeatRenderMode, HeatRenderModeType::Incandescence);
                        }
                        else if (1 == selectedHeatRenderMode)
                        {
                            mLiveSettings.SetValue(GameSettings::HeatRenderMode, HeatRenderModeType::HeatOverlay);
                        }
                        else
                        {
                            assert(2 == selectedHeatRenderMode);
                            mLiveSettings.SetValue(GameSettings::HeatRenderMode, HeatRenderModeType::None);
                        }

                        mHeatSensitivitySlider->Enable(selectedHeatRenderMode != 2);

                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mHeatRenderModeRadioBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Heat sensitivity
            {
                mHeatSensitivitySlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Heat Boost"),
                    _("Lowers the temperature at which materials start emitting red radiation, hence making incandescence more visible at lower temperatures."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::HeatSensitivity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                sizer->Add(
                    mHeatSensitivitySlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Flame size adjustment
            {
                mShipFlameSizeAdjustmentSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Flame Size Adjust"),
                    _("Adjusts the size of flames."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::ShipFlameSizeAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinShipFlameSizeAdjustment(),
                        mGameControllerSettingsOptions->GetMaxShipFlameSizeAdjustment()));

                sizer->Add(
                    mShipFlameSizeAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(2, 0),
            wxGBSpan(1, 5),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Water
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Water"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Default Water Color
            {
                mDefaultWaterColorPicker = new wxColourPickerCtrl(boxSizer->GetStaticBox(), wxID_ANY);
                mDefaultWaterColorPicker->SetToolTip(_("Sets the color of water which is used when ocean render mode is set to 'Texture'."));
                mDefaultWaterColorPicker->Bind(
                    wxEVT_COLOURPICKER_CHANGED,
                    [this](wxColourPickerEvent & event)
                    {
                        auto const color = event.GetColour();

                        mLiveSettings.SetValue(
                            GameSettings::DefaultWaterColor,
                            rgbColor(color.Red(), color.Green(), color.Blue()));

                        OnLiveSettingsChanged();
                    });

                sizer->Add(
                    mDefaultWaterColorPicker,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL,
                    CellBorderInner);
            }

            // Water Contrast
            {
                mWaterContrastSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    -1,
                    _("Contrast"),
                    _("Adjusts the contrast of water inside physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterContrast, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                sizer->Add(
                    mWaterContrastSlider,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorderInner);
            }

            // Water Level of Detail
            {
                mWaterLevelOfDetailSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Level of Detail"),
                    _("Adjusts how detailed water inside a physical body looks."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterLevelOfDetail, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterLevelOfDetail(),
                        mGameControllerSettingsOptions->GetMaxWaterLevelOfDetail()));

                sizer->Add(
                    mWaterLevelOfDetailSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(2, 5),
            wxGBSpan(1, 5),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateSoundAndAdvancedSettingsPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Sound
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Sound"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Effects volume
            {
                mEffectsVolumeSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Effects Volume"),
                    _("Adjusts the volume of sounds generated by the simulation."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::MasterEffectsVolume, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        100.0f));

                sizer->Add(
                    mEffectsVolumeSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Tools volume
            {
                mToolsVolumeSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Tools Volume"),
                    _("Adjusts the volume of sounds generated by interactive tools."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::MasterToolsVolume, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        100.0f));

                sizer->Add(
                    mToolsVolumeSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Checkboxes
            {
                wxStaticBoxSizer * checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, boxSizer->GetStaticBox());

                {
                    mPlayBreakSoundsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Play Break Sounds"));
                    mPlayBreakSoundsCheckBox->SetToolTip(_("Enables or disables the generation of sounds when materials break."));
                    mPlayBreakSoundsCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::PlayBreakSounds, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                    checkboxesSizer->Add(mPlayBreakSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
                }

                {
                    mPlayStressSoundsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Play Stress Sounds"));
                    mPlayStressSoundsCheckBox->SetToolTip(_("Enables or disables the generation of sounds when materials are under stress."));
                    mPlayStressSoundsCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::PlayStressSounds, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                    checkboxesSizer->Add(mPlayStressSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
                }

                {
                    mPlayWindSoundCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Play Wind Sounds"));
                    mPlayWindSoundCheckBox->SetToolTip(_("Enables or disables the generation of wind sounds."));
                    mPlayWindSoundCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::PlayWindSound, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                    checkboxesSizer->Add(mPlayWindSoundCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
                }

                {
                    mPlayAirBubbleSurfaceSoundCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Play Bubbles' Surface Sounds"));
                    mPlayAirBubbleSurfaceSoundCheckBox->SetToolTip(_("Enables or disables the bubbling sound when air bubbles come to the surface."));
                    mPlayAirBubbleSurfaceSoundCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::PlayAirBubbleSurfaceSound, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                    checkboxesSizer->Add(mPlayAirBubbleSurfaceSoundCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
                }

                sizer->Add(
                    checkboxesSizer,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Advanced
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Advanced"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Spring Stiffness
            {
                mSpringStiffnessSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Spring Stiffness Adjust"),
                    _("This setting is for testing physical instability of the mass-spring network with high stiffness values;"
                        " it is not meant for improving the rigidity of physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SpringStiffnessAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinSpringStiffnessAdjustment(),
                        mGameControllerSettingsOptions->GetMaxSpringStiffnessAdjustment()),
                        mWarningIcon.get());

                sizer->Add(
                    mSpringStiffnessSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Spring Damping
            {
                mSpringDampingSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderWidth,
                    SliderHeight,
                    _("Spring Damping Adjust"),
                    _("This setting is for testing physical instability of the mass-spring network with different damping values;"
                        " it is not meant for improving the rigidity of physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::SpringDampingAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinSpringDampingAdjustment(),
                        mGameControllerSettingsOptions->GetMaxSpringDampingAdjustment()),
                        mWarningIcon.get());

                sizer->Add(
                    mSpringDampingSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 2),
            wxGBSpan(1, 2),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Ship Debug Draw Options
    {
        wxString debugShipRenderModeChoices[] =
        {
            _("No Debug"),
            _("Draw in Wireframe Mode"),
            _("Draw Only Points"),
            _("Draw Only Springs"),
            _("Draw Only Edge Springs"),
            _("Draw Decay"),
            _("Draw Structure")
        };

        mDebugShipRenderModeRadioBox = new wxRadioBox(panel, wxID_ANY, _("Ship Debug Draw Options"), wxDefaultPosition, wxDefaultSize,
            WXSIZEOF(debugShipRenderModeChoices), debugShipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
        mDebugShipRenderModeRadioBox->Bind(
            wxEVT_RADIOBOX,
            [this](wxCommandEvent & event)
            {
                auto const selectedDebugShipRenderMode = event.GetSelection();
                if (0 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::None);
                }
                else if (1 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::Wireframe);
                }
                else if (2 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::Points);
                }
                else if (3 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::Springs);
                }
                else if (4 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::EdgeSprings);
                }
                else if (5 == selectedDebugShipRenderMode)
                {
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::Decay);
                }
                else
                {
                    assert(6 == selectedDebugShipRenderMode);
                    mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderModeType::Structure);
                }

                OnLiveSettingsChanged();
            });

        gridSizer->Add(
            mDebugShipRenderModeRadioBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Extra Draw Options
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Extra Draw Options"));

        {
            mDrawExplosionsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Draw Explosions"));
            mDrawExplosionsCheckBox->SetToolTip(_("Enables or disables rendering of explosions."));
            mDrawExplosionsCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::DrawExplosions, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mDrawExplosionsCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mDrawFlamesCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Draw Flames"));
            mDrawFlamesCheckBox->SetToolTip(_("Enables or disables rendering of flames."));
            mDrawFlamesCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::DrawFlames, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mDrawFlamesCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mShowFrontiersCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Show Frontiers"));
            mShowFrontiersCheckBox->SetToolTip(_("Enables or disables visualization of the frontiers of the ship."));
            mShowFrontiersCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::ShowShipFrontiers, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mShowFrontiersCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mShowAABBsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Show AABBs"));
            mShowAABBsCheckBox->SetToolTip(_("Enables or disables visualization of the AABBs (Axis-Aligned Bounding Boxes)."));
            mShowAABBsCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::ShowAABBs, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mShowAABBsCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mShowStressCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Show Stress"));
            mShowStressCheckBox->SetToolTip(_("Enables or disables highlighting of the springs that are under heavy stress and close to rupture."));
            mShowStressCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::ShowShipStress, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mShowStressCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mDrawHeatBlasterFlameCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Draw HeatBlaster Flame"));
            mDrawHeatBlasterFlameCheckBox->SetToolTip(_("Renders flames out of the HeatBlaster tool."));
            mDrawHeatBlasterFlameCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::DrawHeatBlasterFlame, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mDrawHeatBlasterFlameCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 1),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Vector Field Draw Options
    {
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
        mVectorFieldRenderModeRadioBox->SetToolTip(_("Enables or disables rendering of vector fields."));
        mVectorFieldRenderModeRadioBox->Bind(
            wxEVT_RADIOBOX,
            [this](wxCommandEvent & event)
            {
                auto selectedVectorFieldRenderMode = event.GetSelection();
                switch (selectedVectorFieldRenderMode)
                {
                    case 0:
                    {
                        mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderModeType::None);
                        break;
                    }

                    case 1:
                    {
                        mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderModeType::PointVelocity);
                        break;
                    }

                    case 2:
                    {
                        mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderModeType::PointForce);
                        break;
                    }

                    case 3:
                    {
                        mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderModeType::PointWaterVelocity);
                        break;
                    }

                    default:
                    {
                        assert(4 == selectedVectorFieldRenderMode);
                        mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderModeType::PointWaterMomentum);
                        break;
                    }
                }

                OnLiveSettingsChanged();
            });

        gridSizer->Add(
            mVectorFieldRenderModeRadioBox,
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Side-Effects
    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Side-Effects"));

        {
            mGenerateDebrisCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Generate Debris"));
            mGenerateDebrisCheckBox->SetToolTip(_("Enables or disables generation of debris when using destructive tools."));
            mGenerateDebrisCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::DoGenerateDebris, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mGenerateDebrisCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        {
            mGenerateSparklesForCutsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, _("Generate Sparkles"));
            mGenerateSparklesForCutsCheckBox->SetToolTip(_("Enables or disables generation of sparkles when using the saw tool on metal."));
            mGenerateSparklesForCutsCheckBox->Bind(
                wxEVT_COMMAND_CHECKBOX_CLICKED,
                [this](wxCommandEvent & event)
                {
                    mLiveSettings.SetValue(GameSettings::DoGenerateSparklesForCuts, event.IsChecked());
                    OnLiveSettingsChanged();
                });

            boxSizer->Add(mGenerateSparklesForCutsCheckBox, 0, wxALL | wxALIGN_LEFT, InterCheckboxRowMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(1, 3),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateSettingsManagementPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Load settings
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Load Settings"));

        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            // Col 1

            {
                mPersistedSettingsListCtrl = new wxListCtrl(
                    boxSizer->GetStaticBox(),
                    wxID_ANY,
                    wxDefaultPosition,
                    wxSize(250, 370),
                    wxBORDER_STATIC /*https://trac.wxwidgets.org/ticket/18549*/ | wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);

                mPersistedSettingsListCtrl->AppendColumn(
                    wxEmptyString,
                    wxLIST_FORMAT_LEFT,
                    // TODOTEST
                    //wxLIST_AUTOSIZE_USEHEADER);
                    mPersistedSettingsListCtrl->GetSize().GetWidth() - 10);

                for (size_t p = 0; p < mPersistedSettings.size(); ++p)
                {
                    InsertPersistedSettingInCtrl(p, mPersistedSettings[p].Key);
                }

                if (!mPersistedSettings.empty())
                {
                    // Select first item
                    mPersistedSettingsListCtrl->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                }

                mPersistedSettingsListCtrl->Bind(
                    wxEVT_LIST_ITEM_SELECTED,
                    [this](wxListEvent &)
                    {
                        ReconciliateLoadPersistedSettings();
                    });

                mPersistedSettingsListCtrl->Bind(
                    wxEVT_LIST_ITEM_ACTIVATED,
                    [this](wxListEvent & event)
                    {
                        assert(event.GetIndex() != wxNOT_FOUND);

                        LoadPersistedSettings(static_cast<size_t>(event.GetIndex()), true);
                    });

                hSizer->Add(mPersistedSettingsListCtrl, 0, wxALL | wxEXPAND, 5);
            }

            // Col 2

            {
                wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

                {
                    auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Description:"));

                    vSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
                }

                {
                    mPersistedSettingsDescriptionTextCtrl = new wxTextCtrl(
                        boxSizer->GetStaticBox(),
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxSize(250, 120),
                        wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

                    vSizer->Add(mPersistedSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
                }

                {
                    mApplyPersistedSettingsButton = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, _("Apply Saved Settings"));
                    mApplyPersistedSettingsButton->SetToolTip(_("Loads the selected settings and applies them on top of the current settings."));
                    mApplyPersistedSettingsButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent & /*event*/)
                        {
                            auto const selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

                            assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
                            assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());

                            if (selectedIndex != wxNOT_FOUND)
                            {
                                LoadPersistedSettings(static_cast<size_t>(selectedIndex), false);
                            }
                        });

                    vSizer->Add(mApplyPersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mRevertToPersistedSettingsButton = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, _("Revert to Saved Settings"));
                    mRevertToPersistedSettingsButton->SetToolTip(_("Reverts all settings to the selected settings."));
                    mRevertToPersistedSettingsButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent & /*event*/)
                        {
                            auto const selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

                            assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
                            assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());

                            if (selectedIndex != wxNOT_FOUND)
                            {
                                LoadPersistedSettings(static_cast<size_t>(selectedIndex), true);
                            }
                        });

                    vSizer->Add(mRevertToPersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mReplacePersistedSettingsButton = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, _("Replace Saved Settings with Current"));
                    mReplacePersistedSettingsButton->SetToolTip(_("Overwrites the selected settings with the current settings."));
                    mReplacePersistedSettingsButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent & /*event*/)
                        {
                            auto const selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

                            assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
                            assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());
                            assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

                            if (selectedIndex != wxNOT_FOUND)
                            {
                                auto const & metadata = mPersistedSettings[selectedIndex];

                                wxString message;
                                message.Printf(_("Are you sure you want to replace settings \"%s\" with the current settings?"), metadata.Key.Name.c_str());
                                auto const result = wxMessageBox(
                                    message,
                                    _("Warning"),
                                    wxCANCEL | wxOK);

                                if (result == wxOK)
                                {
                                    // Save
                                    SavePersistedSettings(metadata);

                                    // Reconciliate load UI
                                    ReconciliateLoadPersistedSettings();
                                }
                            }
                        });

                    vSizer->Add(mReplacePersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mDeletePersistedSettingsButton = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, _("Delete Saved Settings"));
                    mDeletePersistedSettingsButton->SetToolTip(_("Deletes the selected settings."));
                    mDeletePersistedSettingsButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent & /*event*/)
                        {
                            auto const selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

                            assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
                            assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());
                            assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

                            if (selectedIndex != wxNOT_FOUND)
                            {
                                auto const & metadata = mPersistedSettings[selectedIndex];

                                // Ask user whether they're sure
                                wxString message;
                                message.Printf(_("Are you sure you want to delete settings \"%s\"?"), metadata.Key.Name.c_str());
                                auto const result = wxMessageBox(
                                    message,
                                    _("Warning"),
                                    wxCANCEL | wxOK);

                                if (result == wxOK)
                                {
                                    try
                                    {
                                        // Delete
                                        mSettingsManager->DeletePersistedSettings(metadata.Key);
                                    }
                                    catch (std::runtime_error const & ex)
                                    {
                                        OnPersistenceError(std::string("Error deleting settings: ") + ex.what());
                                        return;
                                    }

                                    // Remove from list box
                                    mPersistedSettingsListCtrl->DeleteItem(selectedIndex);

                                    // Remove from mPersistedSettings
                                    mPersistedSettings.erase(mPersistedSettings.cbegin() + selectedIndex);

                                    // Reconciliate with UI
                                    ReconciliateLoadPersistedSettings();
                                }
                            }
                        });

                    vSizer->Add(mDeletePersistedSettingsButton, 0, wxALL | wxEXPAND, 5);
                }

                hSizer->Add(vSizer, 0, 0, 0);
            }

            boxSizer->Add(hSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);

        ReconciliateLoadPersistedSettings();
    }

    //
    // Save settings
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Save Settings"));

        {
            {
                auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Name:"));

                boxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
            }

            {
                wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
                validator.SetCharIncludes(
                    wxS("abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "0123456789"
                        " "
                        "_-"));
                validator.SuppressBellOnError();

                mSaveSettingsNameTextCtrl = new wxTextCtrl(
                    boxSizer->GetStaticBox(),
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxDefaultSize,
                    0,
                    validator);

                mSaveSettingsNameTextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent &)
                    {
                        ReconciliateSavePersistedSettings();
                    });

                boxSizer->Add(mSaveSettingsNameTextCtrl, 0, wxALL | wxEXPAND, 5);
            }

            {
                auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Description:"));

                boxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
            }

            {
                mSaveSettingsDescriptionTextCtrl = new wxTextCtrl(
                    boxSizer->GetStaticBox(),
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxSize(250, 120),
                    wxTE_MULTILINE | wxTE_WORDWRAP);

                mSaveSettingsDescriptionTextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent &)
                    {
                        ReconciliateSavePersistedSettings();
                    });

                boxSizer->Add(mSaveSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
            }

            {
                mSaveSettingsButton = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, _("Save Current Settings"));
                mSaveSettingsButton->SetToolTip(_("Saves the current settings using the specified name."));
                mSaveSettingsButton->Bind(
                    wxEVT_BUTTON,
                    [this](wxCommandEvent &)
                    {
                        assert(!mSaveSettingsNameTextCtrl->IsEmpty()); // Guaranteed by UI

                        if (mSaveSettingsNameTextCtrl->IsEmpty())
                            return;

                        auto const settingsMetadata = PersistedSettingsMetadata(
                            PersistedSettingsKey(
                                mSaveSettingsNameTextCtrl->GetValue().ToStdString(),
                                PersistedSettingsStorageTypes::User),
                            mSaveSettingsDescriptionTextCtrl->GetValue().ToStdString());


                        //
                        // Check if settings with this name already exist
                        //

                        {
                            auto it = std::find_if(
                                mPersistedSettings.cbegin(),
                                mPersistedSettings.cend(),
                                [&settingsMetadata](auto const & sm)
                                {
                                    return sm.Key == settingsMetadata.Key;
                                });

                            if (it != mPersistedSettings.cend())
                            {
                                // Ask user if sure
                                wxString message;
                                message.Printf(_("Settings \"%s\" already exist; do you want to replace them with the current settings?"), settingsMetadata.Key.Name.c_str());
                                auto const result = wxMessageBox(
                                    message,
                                    _("Warning"),
                                    wxCANCEL | wxOK);

                                if (result == wxCANCEL)
                                {
                                    // Abort
                                    return;
                                }
                            }
                        }

                        //
                        // Save settings
                        //

                        // Save
                        SavePersistedSettings(settingsMetadata);

                        // Find index for insertion
                        PersistedSettingsComparer cmp;
                        auto const it = std::lower_bound(
                            mPersistedSettings.begin(),
                            mPersistedSettings.end(),
                            settingsMetadata,
                            cmp);

                        if (it != mPersistedSettings.end()
                            && it->Key == settingsMetadata.Key)
                        {
                            // It's a replace

                            // Replace in persisted settings
                            it->Description = settingsMetadata.Description;
                        }
                        else
                        {
                            // It's an insert

                            auto const insertIdx = std::distance(mPersistedSettings.begin(), it);

                            // Insert into persisted settings
                            mPersistedSettings.insert(it, settingsMetadata);

                            // Insert in list control
                            InsertPersistedSettingInCtrl(insertIdx, settingsMetadata.Key);
                        }

                        // Reconciliate load UI
                        ReconciliateLoadPersistedSettings();

                        // Clear name and description
                        mSaveSettingsNameTextCtrl->Clear();
                        mSaveSettingsDescriptionTextCtrl->Clear();

                        // Reconciliate save UI
                        ReconciliateSavePersistedSettings();
                    });

                boxSizer->Add(mSaveSettingsButton, 0, wxALL | wxEXPAND, 5);
            }
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderInner);

        ReconciliateSavePersistedSettings();
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::SyncControlsWithSettings(Settings<GameSettings> const & settings)
{
    //
    // Mechanics and Thermodynamics
    //

    mMechanicalQualitySlider->SetValue(settings.GetValue<float>(GameSettings::NumMechanicalDynamicsIterationsAdjustment));
    mStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStrengthAdjustment));
    mGlobalDampingAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::GlobalDampingAdjustment));
    mThermalConductivityAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ThermalConductivityAdjustment));
    mHeatDissipationAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::HeatDissipationAdjustment));
    mIgnitionTemperatureAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::IgnitionTemperatureAdjustment));
    mMeltingTemperatureAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::MeltingTemperatureAdjustment));
    mCombustionSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::CombustionSpeedAdjustment));
    mCombustionHeatAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::CombustionHeatAdjustment));
    mMaxBurningParticlesSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::MaxBurningParticles));
    mUltraViolentToggleButton->SetValue(settings.GetValue<bool>(GameSettings::UltraViolentMode));

    //
    // Water and Ocean
    //

    mWaterDensitySlider->SetValue(settings.GetValue<float>(GameSettings::WaterDensityAdjustment));
    mWaterFrictionDragSlider->SetValue(settings.GetValue<float>(GameSettings::WaterFrictionDragAdjustment));
    mWaterPressureDragSlider->SetValue(settings.GetValue<float>(GameSettings::WaterPressureDragAdjustment));
    mWaterIntakeSlider->SetValue(settings.GetValue<float>(GameSettings::WaterIntakeAdjustment));
    mWaterCrazynessSlider->SetValue(settings.GetValue<float>(GameSettings::WaterCrazyness));
    mWaterDiffusionSpeedSlider->SetValue(settings.GetValue<float>(GameSettings::WaterDiffusionSpeedAdjustment));
    mWaterTemperatureSlider->SetValue(settings.GetValue<float>(GameSettings::WaterTemperature));
    mOceanDepthSlider->SetValue(settings.GetValue<float>(GameSettings::SeaDepth));
    mOceanFloorBumpinessSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorBumpiness));
    mOceanFloorDetailAmplificationSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorDetailAmplification));
    mOceanFloorElasticitySlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorElasticity));
    mOceanFloorFrictionSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorFriction));
    mRotAcceler8rSlider->SetValue(settings.GetValue<float>(GameSettings::RotAcceler8r));

    //
    // Wind and Waves
    //

    mWindSpeedBaseSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedBase));
    mModulateWindCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoModulateWind));
    mWindGustAmplitudeSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedMaxFactor));
    mWindGustAmplitudeSlider->Enable(settings.GetValue<bool>(GameSettings::DoModulateWind));
    mBasalWaveHeightAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveHeightAdjustment));
    mBasalWaveLengthAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveLengthAdjustment));
    mBasalWaveSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveSpeedAdjustment));
    mDoDisplaceWaterCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoDisplaceWater));
    mWaterDisplacementWaveHeightAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::WaterDisplacementWaveHeightAdjustment));
    mWaterDisplacementWaveHeightAdjustmentSlider->Enable(settings.GetValue<bool>(GameSettings::DoDisplaceWater));
    mWaveSmoothnessAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::WaveSmoothnessAdjustment));
    mTsunamiRateSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::TsunamiRate).count());
    mRogueWaveRateSlider->SetValue(settings.GetValue<std::chrono::seconds>(GameSettings::RogueWaveRate).count());
    mStormStrengthAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::StormStrengthAdjustment));
    mDoRainWithStormCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoRainWithStorm));
    mRainFloodAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::RainFloodAdjustment));
    mRainFloodAdjustmentSlider->Enable(settings.GetValue<bool>(GameSettings::DoRainWithStorm));
    mStormDurationSlider->SetValue(settings.GetValue<std::chrono::seconds>(GameSettings::StormDuration).count());
    mStormRateSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::StormRate).count());

    //
    // Air and Sky
    //

    mAirFrictionDragSlider->SetValue(settings.GetValue<float>(GameSettings::AirFrictionDragAdjustment));
    mAirPressureDragSlider->SetValue(settings.GetValue<float>(GameSettings::AirPressureDragAdjustment));
    mAirTemperatureSlider->SetValue(settings.GetValue<float>(GameSettings::AirTemperature));
    mAirBubbleDensitySlider->SetValue(settings.GetValue<float>(GameSettings::AirBubblesDensity));
    mSmokeEmissionDensityAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::SmokeEmissionDensityAdjustment));
    mSmokeParticleLifetimeAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::SmokeParticleLifetimeAdjustment));
    mNumberOfStarsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfStars));
    mNumberOfCloudsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfClouds));
    mDoDayLightCycleCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoDayLightCycle));
    mDayLightCycleDurationSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::DayLightCycleDuration).count());
    mDayLightCycleDurationSlider->Enable(settings.GetValue<bool>(GameSettings::DoDayLightCycle));

    //
    // Lights, Electricals, and Fishes
    //

    mLuminiscenceSlider->SetValue(settings.GetValue<float>(GameSettings::LuminiscenceAdjustment));
    mLightSpreadSlider->SetValue(settings.GetValue<float>(GameSettings::LightSpreadAdjustment));
    mGenerateEngineWakeCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateEngineWakeParticles));
    mEngineThrustAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::EngineThrustAdjustment));
    mWaterPumpPowerAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::WaterPumpPowerAdjustment));
    mElectricalElementHeatProducedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ElectricalElementHeatProducedAdjustment));
    mNumberOfFishesSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfFishes));
    mFishSizeMultiplierSlider->SetValue(settings.GetValue<float>(GameSettings::FishSizeMultiplier));
    mFishSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::FishSpeedAdjustment));
    mDoFishShoalingCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoFishShoaling));
    mFishShoalRadiusAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::FishShoalRadiusAdjustment));
    mFishShoalRadiusAdjustmentSlider->Enable(settings.GetValue<bool>(GameSettings::DoFishShoaling));

    //
    // Tools
    //

    mDestroyRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::DestroyRadius));
    mBombBlastRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastRadius));
    mBombBlastForceAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastForceAdjustment));
    mBombBlastHeatSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastHeat));
    mAntiMatterBombImplosionStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::AntiMatterBombImplosionStrength));
    mBlastToolRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::BlastToolRadius));
    mBlastToolForceAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BlastToolForceAdjustment));
    mScrubRotRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::ScrubRotRadius));
    mFloodRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::FloodRadius));
    mFloodQuantitySlider->SetValue(settings.GetValue<float>(GameSettings::FloodQuantity));
    mRepairRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::RepairRadius));
    mRepairSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::RepairSpeedAdjustment));
    mHeatBlasterRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::HeatBlasterRadius));
    mHeatBlasterHeatFlowSlider->SetValue(settings.GetValue<float>(GameSettings::HeatBlasterHeatFlow));

    //
    // Rendering
    //

    switch (settings.GetValue<OceanRenderModeType>(GameSettings::OceanRenderMode))
    {
        case OceanRenderModeType::Texture:
        {
            mTextureOceanRenderModeRadioButton->SetValue(true);
            break;
        }

        case OceanRenderModeType::Depth:
        {
            mDepthOceanRenderModeRadioButton->SetValue(true);
            break;
        }

        case OceanRenderModeType::Flat:
        {
            mFlatOceanRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    mTextureOceanComboBox->Select(static_cast<int>(settings.GetValue<size_t>(GameSettings::TextureOceanTextureIndex)));

    auto const depthOceanColorStart = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorStart);
    mDepthOceanColorStartPicker->SetColour(wxColor(depthOceanColorStart.r, depthOceanColorStart.g, depthOceanColorStart.b));

    auto const depthOceanColorEnd = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorEnd);
    mDepthOceanColorEndPicker->SetColour(wxColor(depthOceanColorEnd.r, depthOceanColorEnd.g, depthOceanColorEnd.b));

    auto const flatOceanColor = settings.GetValue<rgbColor>(GameSettings::FlatOceanColor);
    mFlatOceanColorPicker->SetColour(wxColor(flatOceanColor.r, flatOceanColor.g, flatOceanColor.b));

    mOceanRenderDetailModeDetailedCheckBox->SetValue(settings.GetValue<OceanRenderDetailType>(GameSettings::OceanRenderDetail) == OceanRenderDetailType::Detailed);
    mSeeShipThroughOceanCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipThroughOcean));
    mOceanTransparencySlider->SetValue(settings.GetValue<float>(GameSettings::OceanTransparency));
    mOceanDarkeningRateSlider->SetValue(settings.GetValue<float>(GameSettings::OceanDarkeningRate));

    ReconciliateOceanRenderModeSettings();

    switch (settings.GetValue<LandRenderModeType>(GameSettings::LandRenderMode))
    {
        case LandRenderModeType::Texture:
        {
            mTextureLandRenderModeRadioButton->SetValue(true);
            break;
        }

        case LandRenderModeType::Flat:
        {
            mFlatLandRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    mTextureLandComboBox->Select(static_cast<int>(settings.GetValue<size_t>(GameSettings::TextureLandTextureIndex)));

    auto const flatLandColor = settings.GetValue<rgbColor>(GameSettings::FlatLandColor);
    mFlatLandColorPicker->SetColour(wxColor(flatLandColor.r, flatLandColor.g, flatLandColor.b));

    ReconciliateLandRenderModeSettings();

    auto const flatSkyColor = settings.GetValue<rgbColor>(GameSettings::FlatSkyColor);
    mFlatSkyColorPicker->SetColour(wxColor(flatSkyColor.r, flatSkyColor.g, flatSkyColor.b));

    auto flatLampLightColor = settings.GetValue<rgbColor>(GameSettings::FlatLampLightColor);
    mFlatLampLightColorPicker->SetColour(wxColor(flatLampLightColor.r, flatLampLightColor.g, flatLampLightColor.b));

    auto const heatRenderMode = settings.GetValue<HeatRenderModeType>(GameSettings::HeatRenderMode);
    switch (heatRenderMode)
    {
        case HeatRenderModeType::Incandescence:
        {
            mHeatRenderModeRadioBox->SetSelection(0);
            break;
        }

        case HeatRenderModeType::HeatOverlay:
        {
            mHeatRenderModeRadioBox->SetSelection(1);
            break;
        }

        case HeatRenderModeType::None:
        {
            mHeatRenderModeRadioBox->SetSelection(2);
            break;
        }
    }

    mHeatSensitivitySlider->SetValue(settings.GetValue<float>(GameSettings::HeatSensitivity));
    mHeatSensitivitySlider->Enable(heatRenderMode != HeatRenderModeType::None);
    mShipFlameSizeAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ShipFlameSizeAdjustment));

    auto const defaultWaterColor = settings.GetValue<rgbColor>(GameSettings::DefaultWaterColor);
    mDefaultWaterColorPicker->SetColour(wxColor(defaultWaterColor.r, defaultWaterColor.g, defaultWaterColor.b));

    mWaterContrastSlider->SetValue(settings.GetValue<float>(GameSettings::WaterContrast));
    mWaterLevelOfDetailSlider->SetValue(settings.GetValue<float>(GameSettings::WaterLevelOfDetail));

    //
    // Sound and Advanced Settings
    //

    mEffectsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterEffectsVolume));
    mToolsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterToolsVolume));
    mPlayBreakSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayBreakSounds));
    mPlayStressSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayStressSounds));
    mPlayWindSoundCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayWindSound));
    mPlayAirBubbleSurfaceSoundCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayAirBubbleSurfaceSound));

    mSpringStiffnessSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStiffnessAdjustment));
    mSpringDampingSlider->SetValue(settings.GetValue<float>(GameSettings::SpringDampingAdjustment));

    switch (settings.GetValue<DebugShipRenderModeType>(GameSettings::DebugShipRenderMode))
    {
        case DebugShipRenderModeType::None:
        {
            mDebugShipRenderModeRadioBox->SetSelection(0);
            break;
        }

        case DebugShipRenderModeType::Wireframe:
        {
            mDebugShipRenderModeRadioBox->SetSelection(1);
            break;
        }

        case DebugShipRenderModeType::Points:
        {
            mDebugShipRenderModeRadioBox->SetSelection(2);
            break;
        }

        case DebugShipRenderModeType::Springs:
        {
            mDebugShipRenderModeRadioBox->SetSelection(3);
            break;
        }

        case DebugShipRenderModeType::EdgeSprings:
        {
            mDebugShipRenderModeRadioBox->SetSelection(4);
            break;
        }

        case DebugShipRenderModeType::Decay:
        {
            mDebugShipRenderModeRadioBox->SetSelection(5);
            break;
        }

        case DebugShipRenderModeType::Structure:
        {
            mDebugShipRenderModeRadioBox->SetSelection(6);
            break;
        }
    }

    mDrawExplosionsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawExplosions));
    mDrawFlamesCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawFlames));
    mShowFrontiersCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipFrontiers));
    mShowAABBsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowAABBs));
    mShowStressCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipStress));
    mDrawHeatBlasterFlameCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawHeatBlasterFlame));

    switch (settings.GetValue<VectorFieldRenderModeType>(GameSettings::VectorFieldRenderMode))
    {
        case VectorFieldRenderModeType::None:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(0);
            break;
        }

        case VectorFieldRenderModeType::PointVelocity:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(1);
            break;
        }

        case VectorFieldRenderModeType::PointForce:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(2);
            break;
        }

        case VectorFieldRenderModeType::PointWaterVelocity:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(3);
            break;
        }

        case VectorFieldRenderModeType::PointWaterMomentum:
        {
            mVectorFieldRenderModeRadioBox->SetSelection(4);
            break;
        }
    }

    mGenerateDebrisCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateDebris));
    mGenerateSparklesForCutsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateSparklesForCuts));
}

void SettingsDialog::ReconciliateOceanRenderModeSettings()
{
    mTextureOceanComboBox->Enable(mTextureOceanRenderModeRadioButton->GetValue());
    mDepthOceanColorStartPicker->Enable(mDepthOceanRenderModeRadioButton->GetValue());
    mDepthOceanColorEndPicker->Enable(mDepthOceanRenderModeRadioButton->GetValue());
    mFlatOceanColorPicker->Enable(mFlatOceanRenderModeRadioButton->GetValue());
}

void SettingsDialog::ReconciliateLandRenderModeSettings()
{
    mTextureLandComboBox->Enable(mTextureLandRenderModeRadioButton->GetValue());
    mFlatLandColorPicker->Enable(mFlatLandRenderModeRadioButton->GetValue());
}

void SettingsDialog::OnLiveSettingsChanged()
{
    assert(!!mSettingsManager);

    // Enforce settings that have just changed
    mSettingsManager->EnforceDirtySettings(mLiveSettings);

    // We're back in sync
    mLiveSettings.ClearAllDirty();

    // Remember that we have changed since we were opened
    mHasBeenDirtyInCurrentSession = true;
    mAreSettingsDirtyWrtDefaults = true; // Best effort, assume each change deviates from defaults
    ReconcileDirtyState();
}

void SettingsDialog::ReconcileDirtyState()
{
    //
    // Update buttons' state based on dirty state
    //

    mRevertToDefaultsButton->Enable(mAreSettingsDirtyWrtDefaults);
    mUndoButton->Enable(mHasBeenDirtyInCurrentSession);
}

long SettingsDialog::GetSelectedPersistedSettingIndexFromCtrl() const
{
    return mPersistedSettingsListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void SettingsDialog::InsertPersistedSettingInCtrl(
    int index,
    PersistedSettingsKey const & psKey)
{
    mPersistedSettingsListCtrl->InsertItem(
        index,
        psKey.Name);

    if (psKey.StorageType == PersistedSettingsStorageTypes::System
        || psKey == PersistedSettingsKey::MakeLastModifiedSettingsKey())
    {
        // Make it bold
        auto font = mPersistedSettingsListCtrl->GetItemFont(index);
        font.SetWeight(wxFONTWEIGHT_BOLD);
        mPersistedSettingsListCtrl->SetItemFont(index, font);
    }
}

void SettingsDialog::LoadPersistedSettings(size_t index, bool withDefaults)
{
    assert(index < mPersistedSettings.size());

    if (index < mPersistedSettings.size())
    {
        if (withDefaults)
        {
            //
            // Apply loaded settings to {Defaults}
            //

            mLiveSettings = mSettingsManager->GetDefaults();

            mSettingsManager->LoadPersistedSettings(
                mPersistedSettings[index].Key,
                mLiveSettings);

            // Make sure we enforce everything
            mLiveSettings.MarkAllAsDirty();
        }
        else
        {
            //
            // Apply loaded settings to {Current}
            //

            mSettingsManager->LoadPersistedSettings(
                mPersistedSettings[index].Key,
                mLiveSettings);
        }

        // Enforce, immediate
        mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);

        // We're back in sync
        mLiveSettings.ClearAllDirty();

        // Remember that we have changed since we were opened
        mHasBeenDirtyInCurrentSession = true;
        mAreSettingsDirtyWrtDefaults = true; // Best effort, assume each change deviates from defaults
        ReconcileDirtyState();

        // Re-populate controls
        SyncControlsWithSettings(mLiveSettings);
    }
}

void SettingsDialog::ReconciliateLoadPersistedSettings()
{
    auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

    assert(selectedIndex == wxNOT_FOUND || static_cast<size_t>(selectedIndex) < mPersistedSettings.size());

    // Enable as long as there's a selection
    mApplyPersistedSettingsButton->Enable(selectedIndex != wxNOT_FOUND);
    mRevertToPersistedSettingsButton->Enable(selectedIndex != wxNOT_FOUND);

    // Enable as long as there's a selection for a user setting that's not the "last-modified" setting
    mReplacePersistedSettingsButton->Enable(
        selectedIndex != wxNOT_FOUND
        && mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User
        && mPersistedSettings[selectedIndex].Key != PersistedSettingsKey::MakeLastModifiedSettingsKey());

    // Enable as long as there's a selection for a user setting that's not the "last-modified" setting
    mDeletePersistedSettingsButton->Enable(
        selectedIndex != wxNOT_FOUND
        && mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User
        && mPersistedSettings[selectedIndex].Key != PersistedSettingsKey::MakeLastModifiedSettingsKey());

    if (selectedIndex != wxNOT_FOUND)
    {
        // Set description content
        mPersistedSettingsDescriptionTextCtrl->SetValue(mPersistedSettings[selectedIndex].Description);
    }
    else
    {
        // Clear description content
        mPersistedSettingsDescriptionTextCtrl->Clear();
    }
}

void SettingsDialog::SavePersistedSettings(PersistedSettingsMetadata const & metadata)
{
    // Only save settings different than default
    mLiveSettings.SetDirtyWithDiff(mSettingsManager->GetDefaults());

    // Save settings
    try
    {
        // Save dirty settings
        mSettingsManager->SaveDirtySettings(
            metadata.Key.Name,
            metadata.Description,
            mLiveSettings);
    }
    catch (std::runtime_error const & ex)
    {
        OnPersistenceError(std::string("Error saving settings: ") + ex.what());
        return;
    }

    // We are in sync (well, we were even before saving)
    mLiveSettings.ClearAllDirty();
}

void SettingsDialog::ReconciliateSavePersistedSettings()
{
    // Enable save button if we have name and description
    mSaveSettingsButton->Enable(
        !mSaveSettingsNameTextCtrl->IsEmpty()
        && !mSaveSettingsDescriptionTextCtrl->IsEmpty());
}

void SettingsDialog::OnPersistenceError(std::string const & errorMessage) const
{
    wxMessageBox(errorMessage, _("Error"), wxICON_ERROR);
}