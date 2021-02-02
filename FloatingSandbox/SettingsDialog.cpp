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

static int constexpr SliderWidth = 40;
static int constexpr SliderHeight = 140;

static int constexpr StaticBoxTopMargin = 7;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr CellBorder = 8;

namespace /* anonymous */ {

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
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_NO_TASKBAR
        | /* wxFRAME_FLOAT_ON_PARENT */ wxSTAY_ON_TOP, // See https://trac.wxwidgets.org/ticket/18535
        wxS("Settings Window"));

    this->Bind(wxEVT_CLOSE_WINDOW, &SettingsDialog::OnCloseButton, this);

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
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);


    //
    // Mechanics, Air, Fluids
    //

    wxPanel * mechanicsAirFluidsPanel = new wxPanel(notebook);

    PopulateMechanicsAirFluidsPanel(mechanicsAirFluidsPanel);

    notebook->AddPage(mechanicsAirFluidsPanel, _("Mechanics, Air, and Fluids"));


    //
    // Heat
    //

    wxPanel * heatPanel = new wxPanel(notebook);

    PopulateHeatPanel(heatPanel);

    notebook->AddPage(heatPanel, _("Heat and Combustion"));


    //
    // Ocean, Smoke, Sky
    //

    wxPanel * oceanSmokeSkyPanel = new wxPanel(notebook);

    PopulateOceanSmokeSkyPanel(oceanSmokeSkyPanel);

    notebook->AddPage(oceanSmokeSkyPanel, _("Ocean, Smoke, and Sky"));


    //
    // Wind, Waves, Fishes, Lights
    //

    wxPanel * windWavesFishesLightsPanel = new wxPanel(notebook);

    PopulateWindWavesFishesLightsPanel(windWavesFishesLightsPanel);

    notebook->AddPage(windWavesFishesLightsPanel, _("Wind, Waves, Fishes, and Lights"));


    //
    // Interactions
    //

    wxPanel * interactionsPanel = new wxPanel(notebook);

    PopulateInteractionsPanel(interactionsPanel);

    notebook->AddPage(interactionsPanel, _("Interactions"));


    //
    // Rendering
    //

    wxPanel * renderingPanel = new wxPanel(notebook);

    PopulateRenderingPanel(renderingPanel);

    notebook->AddPage(renderingPanel, _("Rendering"));


    //
    // Sound and Advanced
    //

    wxPanel * soundAndAdvancedPanel = new wxPanel(notebook);

    PopulateSoundAndAdvancedPanel(soundAndAdvancedPanel);

    notebook->AddPage(soundAndAdvancedPanel, _("Sound and Advanced Settings"));


    //
    // Settings Management
    //

    wxPanel * settingsManagementPanel = new wxPanel(notebook);

    PopulateSettingsManagementPanel(settingsManagementPanel);

    notebook->AddPage(settingsManagementPanel, _("Settings Management"));



    dialogVSizer->Add(notebook, 0, wxEXPAND);

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

void SettingsDialog::OnRestoreDefaultTerrainButton(wxCommandEvent & /*event*/)
{
    mLiveSettings.ClearAllDirty();

    mLiveSettings.SetValue<OceanFloorTerrain>(
        GameSettings::OceanFloorTerrain,
        mSettingsManager->GetDefaults().GetValue<OceanFloorTerrain>(GameSettings::OceanFloorTerrain));

    OnLiveSettingsChanged();
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

void SettingsDialog::OnDepthOceanColorStartChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::DepthOceanColorStart,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnDepthOceanColorEndChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::DepthOceanColorEnd,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnFlatOceanColorChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::FlatOceanColor,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
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

void SettingsDialog::OnFlatLandColorChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::FlatLandColor,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnFlatSkyColorChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::FlatSkyColor,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnFlatLampLightColorChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::FlatLampLightColor,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnDefaultWaterColorChanged(wxColourPickerEvent & event)
{
    auto color = event.GetColour();

    mLiveSettings.SetValue(
        GameSettings::DefaultWaterColor,
        rgbColor(color.Red(), color.Green(), color.Blue()));

    OnLiveSettingsChanged();
}

void SettingsDialog::OnDebugShipRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    auto selectedDebugShipRenderMode = mDebugShipRenderModeRadioBox->GetSelection();
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
}

void SettingsDialog::OnVectorFieldRenderModeRadioBox(wxCommandEvent & /*event*/)
{
    auto selectedVectorFieldRenderMode = mVectorFieldRenderModeRadioBox->GetSelection();
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
}

void SettingsDialog::OnPersistedSettingsListCtrlSelected(wxListEvent & /*event*/)
{
    ReconciliateLoadPersistedSettings();
}

void SettingsDialog::OnPersistedSettingsListCtrlActivated(wxListEvent & event)
{
    assert(event.GetIndex() != wxNOT_FOUND);

    LoadPersistedSettings(static_cast<size_t>(event.GetIndex()), true);
}

void SettingsDialog::OnApplyPersistedSettingsButton(wxCommandEvent & /*event*/)
{
    auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

    assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
    assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());

    if (selectedIndex != wxNOT_FOUND)
    {
        LoadPersistedSettings(static_cast<size_t>(selectedIndex), false);
    }
}

void SettingsDialog::OnRevertToPersistedSettingsButton(wxCommandEvent & /*event*/)
{
    auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

    assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
    assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());

    if (selectedIndex != wxNOT_FOUND)
    {
        LoadPersistedSettings(static_cast<size_t>(selectedIndex), true);
    }
}

void SettingsDialog::OnReplacePersistedSettingsButton(wxCommandEvent & /*event*/)
{
    auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

    assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
    assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());
    assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

    if (selectedIndex != wxNOT_FOUND)
    {
        auto const & metadata = mPersistedSettings[selectedIndex];

        wxString message;
        message.Printf(_("Are you sure you want to replace settings \"%s\" with the current settings?"), metadata.Key.Name.c_str());
        auto result = wxMessageBox(
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
}

void SettingsDialog::OnDeletePersistedSettingsButton(wxCommandEvent & /*event*/)
{
    auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

    assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
    assert(static_cast<size_t>(selectedIndex) < mPersistedSettings.size());
    assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

    if (selectedIndex != wxNOT_FOUND)
    {
        auto const & metadata = mPersistedSettings[selectedIndex];

        // Ask user whether they're sure
        wxString message;
        message.Printf(_("Are you sure you want to delete settings \"%s\"?"), metadata.Key.Name.c_str());
        auto result = wxMessageBox(
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
}

void SettingsDialog::OnSaveSettingsTextEdited(wxCommandEvent & /*event*/)
{
    ReconciliateSavePersistedSettings();
}

void SettingsDialog::OnSaveSettingsButton(wxCommandEvent & /*event*/)
{
    assert(!mSaveSettingsNameTextCtrl->IsEmpty()); // Guaranteed by UI

    if (mSaveSettingsNameTextCtrl->IsEmpty())
        return;

    auto settingsMetadata = PersistedSettingsMetadata(
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
            auto result = wxMessageBox(
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

void SettingsDialog::PopulateMechanicsAirFluidsPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Mechanics
    //

    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Simulation Quality
            {
                mMechanicalQualitySlider = new SliderControl<float>(
                    mechanicsBox,
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

                mechanicsSizer->Add(
                    mMechanicalQualitySlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Strength Adjust
            {
                mStrengthSlider = new SliderControl<float>(
                    mechanicsBox,
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

                mechanicsSizer->Add(
                    mStrengthSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping Adjust
            {
                mGlobalDampingAdjustmentSlider = new SliderControl<float>(
                    mechanicsBox,
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

                mechanicsSizer->Add(
                    mGlobalDampingAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Rot Accelerator
            {
                mRotAcceler8rSlider = new SliderControl<float>(
                    mechanicsBox,
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

                mechanicsSizer->Add(
                    mRotAcceler8rSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Air
    //

    {
        wxStaticBox * airBox = new wxStaticBox(panel, wxID_ANY, _("Air"));

        wxBoxSizer * airBoxSizer = new wxBoxSizer(wxVERTICAL);
        airBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * airSizer = new wxGridBagSizer(0, 0);

            // Air Friction Drag
            {
                mAirFrictionDragSlider = new SliderControl<float>(
                    airBox,
                    SliderWidth,
                    SliderHeight,
                    _("Air Friction Drag Adjust"),
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
                    CellBorder);
            }

            // Air Pressure Drag
            {
                mAirPressureDragSlider = new SliderControl<float>(
                    airBox,
                    SliderWidth,
                    SliderHeight,
                    _("Air Pressure Drag Adjust"),
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
                    CellBorder);
            }

            airBoxSizer->Add(airSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        airBox->SetSizerAndFit(airBoxSizer);

        gridSizer->Add(
            airBox,
            wxGBPosition(0, 4),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Fluids
    //

    {
        wxStaticBox * fluidsBox = new wxStaticBox(panel, wxID_ANY, _("Fluids"));

        wxBoxSizer * fluidsBoxSizer = new wxBoxSizer(wxVERTICAL);
        fluidsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * fluidsSizer = new wxGridBagSizer(0, 0);

            // Water Density
            {
                mWaterDensitySlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Density Adjust"),
                    _("Adjusts the density of sea water, and thus the buoyancy it exerts on physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDensityAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDensityAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterDensityAdjustment()));

                fluidsSizer->Add(
                    mWaterDensitySlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Friction Drag
            {
                mWaterFrictionDragSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Friction Drag Adjust"),
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

                fluidsSizer->Add(
                    mWaterFrictionDragSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Pressure Drag
            {
                mWaterPressureDragSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Pressure Drag Adjust"),
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

                fluidsSizer->Add(
                    mWaterPressureDragSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Intake
            {
                mWaterIntakeSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Intake Adjust"),
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

                fluidsSizer->Add(
                    mWaterIntakeSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Crazyness
            {
                mWaterCrazynessSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Crazyness"),
                    _("Adjusts how \"splashy\" water flows inside a physical body."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterCrazyness, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterCrazyness(),
                        mGameControllerSettingsOptions->GetMaxWaterCrazyness()));

                fluidsSizer->Add(
                    mWaterCrazynessSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Diffusion Speed
            {
                mWaterDiffusionSpeedSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Diffusion Speed"),
                    _("Adjusts the speed with which water propagates within a physical body."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDiffusionSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDiffusionSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterDiffusionSpeedAdjustment()));

                fluidsSizer->Add(
                    mWaterDiffusionSpeedSlider,
                    wxGBPosition(0, 5),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            fluidsBoxSizer->Add(
                fluidsSizer,
                0,
                wxEXPAND | wxALL, StaticBoxInsetMargin);
        }

        fluidsBox->SetSizerAndFit(fluidsBoxSizer);

        gridSizer->Add(
            fluidsBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 6),
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateHeatPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Physics
    {
        wxStaticBox * physicsBox = new wxStaticBox(panel, wxID_ANY, _("Physics"));

        wxBoxSizer * physicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        physicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * physicsSizer = new wxGridBagSizer(0, 0);

            // Thermal Conductivity Adjustment
            {
                mThermalConductivityAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mThermalConductivityAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Heat Dissipation Adjustment
            {
                mHeatDissipationAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mHeatDissipationAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Ignition Temperature Adjustment
            {
                mIgnitionTemperatureAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mIgnitionTemperatureAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Melting Temperature Adjustment
            {
                mMeltingTemperatureAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mMeltingTemperatureAdjustmentSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Combustion Speed Adjustment
            {
                mCombustionSpeedAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mCombustionSpeedAdjustmentSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Combustion Heat Adjustment
            {
                mCombustionHeatAdjustmentSlider = new SliderControl<float>(
                    physicsBox,
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

                physicsSizer->Add(
                    mCombustionHeatAdjustmentSlider,
                    wxGBPosition(0, 5),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            physicsBoxSizer->Add(physicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        physicsBox->SetSizerAndFit(physicsBoxSizer);

        gridSizer->Add(
            physicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL,
            CellBorder);
    }

    /////////////////////////////////////////////////////////////////////////////////

    // World
    {
        wxStaticBox * worldBox = new wxStaticBox(panel, wxID_ANY, _("World"));

        wxBoxSizer * worldBoxSizer = new wxBoxSizer(wxVERTICAL);
        worldBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * worldSizer = new wxGridBagSizer(0, 0);

            // Air Temperature
            {
                mAirTemperatureSlider = new SliderControl<float>(
                    worldBox,
                    SliderWidth,
                    SliderHeight,
                    _("Air Temperature"),
                    _("The temperature of air (K)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::AirTemperature, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinAirTemperature(),
                        mGameControllerSettingsOptions->GetMaxAirTemperature()));

                worldSizer->Add(
                    mAirTemperatureSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Temperature
            {
                mWaterTemperatureSlider = new SliderControl<float>(
                    worldBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Temperature"),
                    _("The temperature of water (K)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterTemperature, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterTemperature(),
                        mGameControllerSettingsOptions->GetMaxWaterTemperature()));

                worldSizer->Add(
                    mWaterTemperatureSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            worldBoxSizer->Add(worldSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        worldBox->SetSizerAndFit(worldBoxSizer);

        gridSizer->Add(
            worldBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Electrical
    {
        wxStaticBox * electricalBox = new wxStaticBox(panel, wxID_ANY, _("Electrical"));

        wxBoxSizer * electricalBoxSizer = new wxBoxSizer(wxVERTICAL);
        electricalBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * electricalSizer = new wxGridBagSizer(0, 0);

            // Heat Generation Adjustment
            {
                mElectricalElementHeatProducedAdjustmentSlider = new SliderControl<float>(
                    electricalBox,
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

                electricalSizer->Add(
                    mElectricalElementHeatProducedAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            electricalBoxSizer->Add(electricalSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        electricalBox->SetSizerAndFit(electricalBoxSizer);

        gridSizer->Add(
            electricalBox,
            wxGBPosition(1, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // HeatBlaster
    {
        wxStaticBox * heatBlasterBox = new wxStaticBox(panel, wxID_ANY, _("HeatBlaster"));

        wxBoxSizer * heatBlasterBoxSizer = new wxBoxSizer(wxVERTICAL);
        heatBlasterBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * heatBlasterSizer = new wxGridBagSizer(0, 0);

            // Radius
            {
                mHeatBlasterRadiusSlider = new SliderControl<float>(
                    heatBlasterBox,
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

                heatBlasterSizer->Add(
                    mHeatBlasterRadiusSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Heat flow
            {
                mHeatBlasterHeatFlowSlider = new SliderControl<float>(
                    heatBlasterBox,
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

                heatBlasterSizer->Add(
                    mHeatBlasterHeatFlowSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            heatBlasterBoxSizer->Add(heatBlasterSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        heatBlasterBox->SetSizerAndFit(heatBlasterBoxSizer);

        gridSizer->Add(
            heatBlasterBox,
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Fire
    {
        wxStaticBox * fireBox = new wxStaticBox(panel, wxID_ANY, _("Fire"));

        wxBoxSizer * fireBoxSizer = new wxBoxSizer(wxVERTICAL);
        fireBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * fireSizer = new wxGridBagSizer(0, 0);

            // Max Particles
            {
                mMaxBurningParticlesSlider = new SliderControl<unsigned int>(
                    fireBox,
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

                fireSizer->Add(
                    mMaxBurningParticlesSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            fireBoxSizer->Add(fireSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        fireBox->SetSizerAndFit(fireBoxSizer);

        gridSizer->Add(
            fireBox,
            wxGBPosition(1, 3),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateOceanSmokeSkyPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Row 1
    //

    //
    // Ocean
    //

    {
        wxStaticBox * oceanBox = new wxStaticBox(panel, wxID_ANY, _("Ocean"));

        wxBoxSizer * oceanBoxSizer = new wxBoxSizer(wxVERTICAL);
        oceanBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * oceanSizer = new wxGridBagSizer(0, 0);

            oceanSizer->AddGrowableRow(0, 1); // Slider above button

            // Ocean Depth
            {
                mOceanDepthSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    _("Ocean Depth"),
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
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Ocean Floor Bumpiness
            {
                mOceanFloorBumpinessSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    _("Ocean Floor Bumpiness"),
                    _("Adjusts how much the ocean floor rolls up and down."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorBumpiness, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorBumpiness(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorBumpiness()));

                oceanSizer->Add(
                    mOceanFloorBumpinessSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Restore Ocean Floor Terrain
            {
                wxButton * restoreDefaultTerrainButton = new wxButton(oceanBox, wxID_ANY, _("Restore Default Terrain"));
                restoreDefaultTerrainButton->SetToolTip(_("Reverts the user-drawn ocean floor terrain to the default terrain."));
                restoreDefaultTerrainButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRestoreDefaultTerrainButton, this);

                oceanSizer->Add(
                    restoreDefaultTerrainButton,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxRIGHT,
                    CellBorder);
            }

            // Ocean Floor Detail Amplification
            {
                mOceanFloorDetailAmplificationSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    -1,
                    _("Ocean Floor Detail"),
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

                oceanSizer->Add(
                    mOceanFloorDetailAmplificationSlider,
                    wxGBPosition(1, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    CellBorder);
            }

            // Ocean Floor Elasticity
            {
                mOceanFloorElasticitySlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    _("Ocean Floor Elasticity"),
                    _("Adjusts the elasticity of collisions with the ocean floor."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorElasticity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorElasticity(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorElasticity()));

                oceanSizer->Add(
                    mOceanFloorElasticitySlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Ocean Floor Friction
            {
                mOceanFloorFrictionSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    _("Ocean Floor Friction"),
                    _("Adjusts the friction exherted by the ocean floor."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanFloorFriction, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinOceanFloorFriction(),
                        mGameControllerSettingsOptions->GetMaxOceanFloorFriction()));

                oceanSizer->Add(
                    mOceanFloorFrictionSlider,
                    wxGBPosition(0, 4),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            oceanBoxSizer->Add(oceanSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        oceanBox->SetSizerAndFit(oceanBoxSizer);

        gridSizer->Add(
            oceanBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 5),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Smoke
    //

    {
        wxStaticBox * smokeBox = new wxStaticBox(panel, wxID_ANY, _("Smoke"));

        wxBoxSizer * smokeBoxSizer = new wxBoxSizer(wxVERTICAL);
        smokeBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * smokeSizer = new wxGridBagSizer(0, 0);

            // Smoke Density Adjust
            {
                mSmokeEmissionDensityAdjustmentSlider = new SliderControl<float>(
                    smokeBox,
                    SliderWidth,
                    SliderHeight,
                    _("Smoke Density Adjust"),
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
                    CellBorder);
            }

            // Smoke Persistence Adjust
            {
                mSmokeParticleLifetimeAdjustmentSlider = new SliderControl<float>(
                    smokeBox,
                    SliderWidth,
                    SliderHeight,
                    _("Smoke Persistence Adjust"),
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
                    CellBorder);
            }

            smokeBoxSizer->Add(smokeSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        smokeBox->SetSizerAndFit(smokeBoxSizer);

        gridSizer->Add(
            smokeBox,
            wxGBPosition(0, 5),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Row 2
    //

    //
    // Storms
    //

    {
        wxStaticBox * stormBox = new wxStaticBox(panel, wxID_ANY, _("Storms"));

        wxBoxSizer * stormBoxSizer = new wxBoxSizer(wxVERTICAL);
        stormBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * stormSizer = new wxGridBagSizer(0, 0);

            stormSizer->AddGrowableRow(1, 1); // Slider below checkbox

            // Storm Strength Adjustment
            {
                mStormStrengthAdjustmentSlider = new SliderControl<float>(
                    stormBox,
                    SliderWidth,
                    SliderHeight,
                    _("Storm Strength Adjust"),
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

                stormSizer->Add(
                    mStormStrengthAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Do rain with storm
            {
                mDoRainWithStormCheckBox = new wxCheckBox(stormBox, wxID_ANY,
                    _("Spawn Rain"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                mDoRainWithStormCheckBox->SetToolTip(_("Enables or disables generation of rain during a storm."));
                mDoRainWithStormCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoRainWithStorm, event.IsChecked());
                        OnLiveSettingsChanged();

                        mRainFloodAdjustmentSlider->Enable(event.IsChecked());
                    });

                stormSizer->Add(
                    mDoRainWithStormCheckBox,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Rain Flood Adjustment
            {
                mRainFloodAdjustmentSlider = new SliderControl<float>(
                    stormBox,
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

                stormSizer->Add(
                    mRainFloodAdjustmentSlider,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Storm Duration
            {
                mStormDurationSlider = new SliderControl<std::chrono::seconds::rep>(
                    stormBox,
                    SliderWidth,
                    SliderHeight,
                    _("Storm Duration"),
                    _("The duration of a storm (s)."),
                    [this](std::chrono::seconds::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::StormDuration, std::chrono::seconds(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::seconds::rep>>(
                        mGameControllerSettingsOptions->GetMinStormDuration().count(),
                        mGameControllerSettingsOptions->GetMaxStormDuration().count()));

                stormSizer->Add(
                    mStormDurationSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Storm Rate
            {
                mStormRateSlider = new SliderControl<std::chrono::minutes::rep>(
                    stormBox,
                    SliderWidth,
                    SliderHeight,
                    _("Storm Rate"),
                    _("The expected time between two automatically-generated storms (minutes). Set to zero to disable automatic generation of storms altogether."),
                    [this](std::chrono::minutes::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::StormRate, std::chrono::minutes(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::minutes::rep>>(
                        mGameControllerSettingsOptions->GetMinStormRate().count(),
                        mGameControllerSettingsOptions->GetMaxStormRate().count()));

                stormSizer->Add(
                    mStormRateSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            stormBoxSizer->Add(stormSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        stormBox->SetSizerAndFit(stormBoxSizer);

        gridSizer->Add(
            stormBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Sky
    //

    {
        wxStaticBox * skyBox = new wxStaticBox(panel, wxID_ANY, _("Sky"));

        wxBoxSizer * skyBoxSizer = new wxBoxSizer(wxVERTICAL);
        skyBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * skySizer = new wxGridBagSizer(0, 0);

            skySizer->AddGrowableRow(1, 1); // Slider below checkbox

            // Number of Stars
            {
                mNumberOfStarsSlider = new SliderControl<unsigned int>(
                    skyBox,
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
                    CellBorder);
            }

            // Number of Clouds
            {
                mNumberOfCloudsSlider = new SliderControl<unsigned int>(
                    skyBox,
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
                    CellBorder);
            }

            // Do daylight cycle
            {
                mDoDayLightCycleCheckBox = new wxCheckBox(skyBox, wxID_ANY,
                    _("Automatic Daylight Cycle"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                mDoDayLightCycleCheckBox->SetToolTip(_("Enables or disables automatic cycling of daylight."));
                mDoDayLightCycleCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoDayLightCycle, event.IsChecked());
                        OnLiveSettingsChanged();

                        mDayLightCycleDurationSlider->Enable(event.IsChecked());
                    });

                skySizer->Add(
                    mDoDayLightCycleCheckBox,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Daylight Cycle Duration
            {
                mDayLightCycleDurationSlider = new SliderControl<std::chrono::minutes::rep>(
                    skyBox,
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
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            skyBoxSizer->Add(skySizer, 0, wxALL, StaticBoxInsetMargin);
        }

        skyBox->SetSizerAndFit(skyBoxSizer);

        gridSizer->Add(
            skyBox,
            wxGBPosition(1, 4),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateWindWavesFishesLightsPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Wind
    //

    {
        wxStaticBox * windBox = new wxStaticBox(panel, wxID_ANY, _("Wind"));

        wxBoxSizer * windBoxSizer = new wxBoxSizer(wxVERTICAL);
        windBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * windSizer = new wxGridBagSizer(0, 0);

            windSizer->AddGrowableRow(1, 1);

            // Wind Speed Base
            {
                // Zero wind
                {
                    auto button = new wxButton(windBox, wxID_ANY, _T("Zero"), wxDefaultPosition, wxDefaultSize);
                    button->SetToolTip(_("Set wind speed to zero."));
                    button->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent & /*event*/)
                        {
                            this->mLiveSettings.SetValue(GameSettings::WindSpeedBase, 0.0f);
                            mWindSpeedBaseSlider->SetValue(0.0f);
                            this->OnLiveSettingsChanged();
                        });

                    windSizer->Add(
                        button,
                        wxGBPosition(0, 0),
                        wxGBSpan(1, 1),
                        wxEXPAND | wxLEFT | wxRIGHT,
                        CellBorder);
                }

                // Wind Speed Base
                {
                    mWindSpeedBaseSlider = new SliderControl<float>(
                        windBox,
                        SliderWidth,
                        -1,
                        _("Wind Speed Base"),
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
                        CellBorder);
                }
            }

            // Wind modulation
            {
                // Modulate Wind
                {
                    mModulateWindCheckBox = new wxCheckBox(windBox, wxID_ANY, _("Modulate Wind"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, wxEmptyString);
                    mModulateWindCheckBox->SetToolTip(_("Enables or disables simulation of wind variations, alternating between dead calm and high-speed gusts."));
                    mModulateWindCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                        {
                            mLiveSettings.SetValue<bool>(GameSettings::DoModulateWind, event.IsChecked());
                            OnLiveSettingsChanged();

                            mWindGustAmplitudeSlider->Enable(mModulateWindCheckBox->IsChecked());
                        });

                    windSizer->Add(
                        mModulateWindCheckBox,
                        wxGBPosition(0, 1),
                        wxGBSpan(1, 1),
                        wxEXPAND | wxLEFT | wxRIGHT,
                        CellBorder);
                }

                // Wind Gust Amplitude
                {
                    mWindGustAmplitudeSlider = new SliderControl<float>(
                        windBox,
                        SliderWidth,
                        -1,
                        _("Wind Gust Amplitude"),
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
                        CellBorder);
                }
            }

            windBoxSizer->Add(windSizer, 1, wxEXPAND | wxALL, StaticBoxInsetMargin);
        }

        windBox->SetSizerAndFit(windBoxSizer);

        gridSizer->Add(
            windBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Basal waves
    //

    {
        wxStaticBox * basalWavesBox = new wxStaticBox(panel, wxID_ANY, _("Basal Waves"));

        wxBoxSizer * basalWavesBoxSizer = new wxBoxSizer(wxVERTICAL);
        basalWavesBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * basalWavesSizer = new wxGridBagSizer(0, 0);

            // Basal Wave Height Adjustment
            {
                mBasalWaveHeightAdjustmentSlider = new SliderControl<float>(
                    basalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Wave Height Adjust"),
                    _("Adjusts the height of ocean waves wrt their optimal value, which is determined by wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BasalWaveHeightAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBasalWaveHeightAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBasalWaveHeightAdjustment()));

                basalWavesSizer->Add(
                    mBasalWaveHeightAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Basal Wave Length Adjustment
            {
                mBasalWaveLengthAdjustmentSlider = new SliderControl<float>(
                    basalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Wave Width Adjust"),
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

                basalWavesSizer->Add(
                    mBasalWaveLengthAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Basal Wave Speed Adjustment
            {
                mBasalWaveSpeedAdjustmentSlider = new SliderControl<float>(
                    basalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Wave Speed Adjust"),
                    _("Adjusts the speed of ocean waves wrt their optimal value, which is determined by wind speed."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BasalWaveSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBasalWaveSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxBasalWaveSpeedAdjustment()));

                basalWavesSizer->Add(
                    mBasalWaveSpeedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            basalWavesBoxSizer->Add(basalWavesSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        basalWavesBox->SetSizerAndFit(basalWavesBoxSizer);

        gridSizer->Add(
            basalWavesBox,
            wxGBPosition(0, 2),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Wave Phenomena
    //

    {
        wxStaticBox * abnormalWavesBox = new wxStaticBox(panel, wxID_ANY, _("Wave Phenomena"));

        wxBoxSizer * abnormalWavesBoxSizer = new wxBoxSizer(wxVERTICAL);
        abnormalWavesBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * abnormalWavesSizer = new wxGridBagSizer(0, 0);

            // Tsunami Rate
            {
                mTsunamiRateSlider = new SliderControl<std::chrono::minutes::rep>(
                    abnormalWavesBox,
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

                abnormalWavesSizer->Add(
                    mTsunamiRateSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Rogue Wave Rate
            {
                mRogueWaveRateSlider = new SliderControl<std::chrono::minutes::rep>(
                    abnormalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Rogue Wave Rate"),
                    _("The expected time between two automatically-generated rogue waves (minutes). Set to zero to disable automatic generation of rogue waves altogether."),
                    [this](std::chrono::minutes::rep value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RogueWaveRate, std::chrono::minutes(value));
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<std::chrono::minutes::rep>>(
                        mGameControllerSettingsOptions->GetMinRogueWaveRate().count(),
                        mGameControllerSettingsOptions->GetMaxRogueWaveRate().count()));

                abnormalWavesSizer->Add(
                    mRogueWaveRateSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            abnormalWavesBoxSizer->Add(abnormalWavesSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        abnormalWavesBox->SetSizerAndFit(abnormalWavesBoxSizer);

        gridSizer->Add(
            abnormalWavesBox,
            wxGBPosition(0, 5),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Fishes
    //

    {
        wxStaticBox * fishesBox = new wxStaticBox(panel, wxID_ANY, _("Fishes"));

        wxBoxSizer * fishesBoxSizer = new wxBoxSizer(wxVERTICAL);
        fishesBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * fishesSizer = new wxGridBagSizer(0, 0);

            // Number of Fishes
            {
                mNumberOfFishesSlider = new SliderControl<unsigned int>(
                    fishesBox,
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

                fishesSizer->Add(
                    mNumberOfFishesSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Fish Size Multiplier
            {
                mFishSizeMultiplierSlider = new SliderControl<float>(
                    fishesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Fish Size Multiplier"),
                    _("Magnifies or minimizes the physical size of fishes."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FishSizeMultiplier, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFishSizeMultiplier(),
                        mGameControllerSettingsOptions->GetMaxFishSizeMultiplier()));

                fishesSizer->Add(
                    mFishSizeMultiplierSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Fish Speed Adjustment
            {
                mFishSpeedAdjustmentSlider = new SliderControl<float>(
                    fishesBox,
                    SliderWidth,
                    SliderHeight,
                    _("Fish Speed Adjust"),
                    _("Adjusts the speed of fishes."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FishSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFishSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxFishSpeedAdjustment()));

                fishesSizer->Add(
                    mFishSpeedAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Do shoaling
            {
                mDoFishShoalingCheckBox = new wxCheckBox(fishesBox, wxID_ANY,
                    _("Enable Shoaling"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                mDoFishShoalingCheckBox->SetToolTip(_("Enables or disables shoaling behavior in fishes."));
                mDoFishShoalingCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue<bool>(GameSettings::DoFishShoaling, event.IsChecked());
                        OnLiveSettingsChanged();

                        mFishShoalRadiusAdjustmentSlider->Enable(event.IsChecked());
                    });

                fishesSizer->Add(
                    mDoFishShoalingCheckBox,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 3),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Shoal Radius Adjustment
            {
                mFishShoalRadiusAdjustmentSlider = new SliderControl<float>(
                    fishesBox,
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

                fishesSizer->Add(
                    mFishShoalRadiusAdjustmentSlider,
                    wxGBPosition(1, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            fishesBoxSizer->Add(fishesSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        fishesBox->SetSizerAndFit(fishesBoxSizer);

        gridSizer->Add(
            fishesBox,
            wxGBPosition(1, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Lights
    //

    {
        wxStaticBox * lightsBox = new wxStaticBox(panel, wxID_ANY, _("Lights"));

        wxBoxSizer * lightsBoxSizer = new wxBoxSizer(wxVERTICAL);
        lightsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * lightsSizer = new wxGridBagSizer(0, 0);

            // Luminiscence
            {
                mLuminiscenceSlider = new SliderControl<float>(
                    lightsBox,
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
                    CellBorder);
            }

            // Light Spread
            {
                mLightSpreadSlider = new SliderControl<float>(
                    lightsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Light Spread Adjust"),
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
                    CellBorder);
            }

            lightsBoxSizer->Add(lightsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        lightsBox->SetSizerAndFit(lightsBoxSizer);

        gridSizer->Add(
            lightsBox,
            wxGBPosition(1, 4),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateInteractionsPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Tools
    //

    {
        wxStaticBox * toolsBox = new wxStaticBox(panel, wxID_ANY, _("Tools"));

        wxBoxSizer * toolsBoxSizer = new wxBoxSizer(wxVERTICAL);
        toolsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(0, 0);

            toolsSizer->AddGrowableRow(1, 1);

            //
            // Row 1
            //

            // Destroy Radius
            {
                mDestroyRadiusSlider = new SliderControl<float>(
                    toolsBox,
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

                toolsSizer->Add(
                    mDestroyRadiusSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Bomb Blast Radius
            {
                mBombBlastRadiusSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Bomb Blast Radius"),
                    _("The radius of bomb explosions (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::BombBlastRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinBombBlastRadius(),
                        mGameControllerSettingsOptions->GetMaxBombBlastRadius()));

                toolsSizer->Add(
                    mBombBlastRadiusSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Bomb Blast Heat
            {
                mBombBlastHeatSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Bomb Blast Heat"),
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

                toolsSizer->Add(
                    mBombBlastHeatSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Anti-matter Bomb Implosion Strength
            {
                mAntiMatterBombImplosionStrengthSlider = new SliderControl<float>(
                    toolsBox,
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

                toolsSizer->Add(
                    mAntiMatterBombImplosionStrengthSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            //
            // Row 3
            //

            // Flood Radius
            {
                mFloodRadiusSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Flood Radius"),
                    _("How wide an area is flooded or drained by the flood tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FloodRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFloodRadius(),
                        mGameControllerSettingsOptions->GetMaxFloodRadius()));

                toolsSizer->Add(
                    mFloodRadiusSlider,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Flood Quantity
            {
                mFloodQuantitySlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Flood Quantity"),
                    _("How much water is injected or drained by the flood tool (m3)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::FloodQuantity, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinFloodQuantity(),
                        mGameControllerSettingsOptions->GetMaxFloodQuantity()));

                toolsSizer->Add(
                    mFloodQuantitySlider,
                    wxGBPosition(2, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Repair Radius
            {
                mRepairRadiusSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Repair Radius"),
                    _("Adjusts the radius of the repair tool (m)."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RepairRadius, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinRepairRadius(),
                        mGameControllerSettingsOptions->GetMaxRepairRadius()));

                toolsSizer->Add(
                    mRepairRadiusSlider,
                    wxGBPosition(2, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Repair Speed Adjustment
            {
                mRepairSpeedAdjustmentSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    _("Repair Speed Adjust"),
                    _("Adjusts the speed with which the repair tool attracts particles to repair damage. Warning: at high speeds the repair tool might become destructive!"),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RepairSpeedAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinRepairSpeedAdjustment(),
                        mGameControllerSettingsOptions->GetMaxRepairSpeedAdjustment()));

                toolsSizer->Add(
                    mRepairSpeedAdjustmentSlider,
                    wxGBPosition(2, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            toolsBoxSizer->Add(toolsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        toolsBox->SetSizerAndFit(toolsBoxSizer);

        gridSizer->Add(
            toolsBox,
            wxGBPosition(0, 0),
            wxGBSpan(2, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Air Bubbles
    //

    {
        wxStaticBox * airBubblesBox = new wxStaticBox(panel, wxID_ANY, _("Air Bubbles"));

        wxBoxSizer * airBubblesBoxSizer = new wxBoxSizer(wxVERTICAL);
        airBubblesBoxSizer->AddSpacer(StaticBoxTopMargin);
        airBubblesBoxSizer->AddSpacer(3);

        {
            wxBoxSizer * airBubblesSizer = new wxBoxSizer(wxVERTICAL);

            // Generate Air Bubbles
            {
                mGenerateAirBubblesCheckBox = new wxCheckBox(airBubblesBox, wxID_ANY, _("Generate Air Bubbles"));
                mGenerateAirBubblesCheckBox->SetToolTip(_("Enables or disables generation of air bubbles when water enters a physical body."));
                mGenerateAirBubblesCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoGenerateAirBubbles, event.IsChecked());
                        OnLiveSettingsChanged();

                        mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox->Enable(event.IsChecked());
                        mAirBubbleDensitySlider->Enable(event.IsChecked());
                    });

                airBubblesSizer->Add(mGenerateAirBubblesCheckBox, 0, wxALIGN_LEFT, 0);
            }

            airBubblesSizer->AddSpacer(3);

            // Displace ocean surface at air bubble surfacing
            {
                mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox = new wxCheckBox(airBubblesBox, wxID_ANY, _("Generate Waves"));
                mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox->SetToolTip(_("Enables or disables generation of waves when air bubbles surface above water."));
                mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoDisplaceOceanSurfaceAtAirBubblesSurfacing, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                airBubblesSizer->Add(mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox, 0, wxALIGN_LEFT, 0);
            }

            // Air Bubbles Density
            {
                mAirBubbleDensitySlider = new SliderControl<float>(
                    airBubblesBox,
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

                airBubblesSizer->Add(mAirBubbleDensitySlider, 1, wxEXPAND, 0);
            }

            airBubblesBoxSizer->Add(airBubblesSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        airBubblesBox->SetSizerAndFit(airBubblesBoxSizer);

        gridSizer->Add(
            airBubblesBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Side-Effects
    //

    {
        wxStaticBox * sideEffectsBox = new wxStaticBox(panel, wxID_ANY, _("Side-Effects"));

        wxBoxSizer * sideEffectsBoxSizer = new wxBoxSizer(wxVERTICAL);
        sideEffectsBoxSizer->AddSpacer(StaticBoxTopMargin);
        sideEffectsBoxSizer->AddSpacer(3);

        {
            wxBoxSizer * sideEffectsCheckboxSizer = new wxBoxSizer(wxVERTICAL);

            {
                mGenerateDebrisCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Debris"));
                mGenerateDebrisCheckBox->SetToolTip(_("Enables or disables generation of debris when using destructive tools."));
                mGenerateDebrisCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoGenerateDebris, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                sideEffectsCheckboxSizer->Add(mGenerateDebrisCheckBox, 0, wxALIGN_LEFT, 0);
            }

            sideEffectsCheckboxSizer->AddSpacer(3);

            {
                mGenerateSparklesForCutsCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Sparkles"));
                mGenerateSparklesForCutsCheckBox->SetToolTip(_("Enables or disables generation of sparkles when using the saw tool on metal."));
                mGenerateSparklesForCutsCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoGenerateSparklesForCuts, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                sideEffectsCheckboxSizer->Add(mGenerateSparklesForCutsCheckBox, 0, wxALIGN_LEFT, 0);
            }

            sideEffectsCheckboxSizer->AddSpacer(3);

            {
                mGenerateEngineWakeCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Engine Wake"));
                mGenerateEngineWakeCheckBox->SetToolTip(_("Enables or disables generation of wakes when engines are running underwater."));
                mGenerateEngineWakeCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DoGenerateEngineWakeParticles, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                sideEffectsCheckboxSizer->Add(mGenerateEngineWakeCheckBox, 0, wxALIGN_LEFT, 0);
            }

            sideEffectsCheckboxSizer->AddSpacer(40);

            {
                mUltraViolentCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Ultra-Violent Mode"));
                mUltraViolentCheckBox->SetToolTip(_("Enables or disables amplification of tool forces and inflicted damages."));
                mUltraViolentCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::UltraViolentMode, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                sideEffectsCheckboxSizer->Add(mUltraViolentCheckBox, 0, wxALIGN_LEFT, 0);
            }

            sideEffectsCheckboxSizer->AddStretchSpacer(1);

            sideEffectsBoxSizer->Add(sideEffectsCheckboxSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        sideEffectsBox->SetSizerAndFit(sideEffectsBoxSizer);

        gridSizer->Add(
            sideEffectsBox,
            wxGBPosition(0, 2),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Electrical
    //

    {
        wxStaticBox * electricalBox = new wxStaticBox(panel, wxID_ANY, _("Electrical"));

        wxBoxSizer * electricalBoxSizer = new wxBoxSizer(wxVERTICAL);
        electricalBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * electricalSizer = new wxGridBagSizer(0, 0);

            // Engine Thrust Adjust
            {
                mEngineThrustAdjustmentSlider = new SliderControl<float>(
                    electricalBox,
                    SliderWidth,
                    SliderHeight,
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

                electricalSizer->Add(
                    mEngineThrustAdjustmentSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Water Pump Power Adjust
            {
                mWaterPumpPowerAdjustmentSlider = new SliderControl<float>(
                    electricalBox,
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

                electricalSizer->Add(
                    mWaterPumpPowerAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            electricalBoxSizer->Add(electricalSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        electricalBox->SetSizerAndFit(electricalBoxSizer);

        gridSizer->Add(
            electricalBox,
            wxGBPosition(1, 1),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Sea
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
                    wxGridBagSizer * oceanRenderModeBoxSizer2 = new wxGridBagSizer(3, 3);

                    mTextureOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a static pattern."));
                    mTextureOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mTextureOceanRenderModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mTextureOceanComboBox = new wxBitmapComboBox(oceanRenderModeBox, wxID_ANY, wxEmptyString,
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
                    oceanRenderModeBoxSizer2->Add(mTextureOceanComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL | wxEXPAND, 0);

                    //

                    mDepthOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Depth Gradient"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a vertical color gradient."));
                    mDepthOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mDepthOceanColorStartPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorStartPicker->SetToolTip(_("Sets the starting (top) color of the gradient."));
                    mDepthOceanColorStartPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorStartChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorStartPicker, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 0);

                    mDepthOceanColorEndPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorEndPicker->SetToolTip(_("Sets the ending (bottom) color of the gradient."));
                    mDepthOceanColorEndPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorEndChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorEndPicker, wxGBPosition(1, 2), wxGBSpan(1, 1), wxALL, 0);

                    //

                    mFlatOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanRenderModeRadioButton->SetToolTip(_("Draws the ocean using a single color."));
                    mFlatOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mFlatOceanRenderModeRadioButton, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatOceanColorPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanColorPicker->SetToolTip(_("Sets the single color of the ocean."));
                    mFlatOceanColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatOceanColorChanged, this);
                    oceanRenderModeBoxSizer2->Add(mFlatOceanColorPicker, wxGBPosition(2, 1), wxGBSpan(1, 1), wxALL, 0);

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

            // High-Quality Rendering
            {
                mOceanRenderDetailModeDetailedCheckBox = new wxCheckBox(oceanBox, wxID_ANY,
                    _("High-Quality Rendering"), wxDefaultPosition, wxDefaultSize);
                mOceanRenderDetailModeDetailedCheckBox->SetToolTip(_("Renders the ocean with additional details. Requires more computational resources."));
                mOceanRenderDetailModeDetailedCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::OceanRenderDetail, event.IsChecked() ? OceanRenderDetailType::Detailed : OceanRenderDetailType::Basic);
                        OnLiveSettingsChanged();
                    });

                oceanSizer->Add(
                    mOceanRenderDetailModeDetailedCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // See Ship Through Water
            {
                mSeeShipThroughOceanCheckBox = new wxCheckBox(oceanBox, wxID_ANY,
                    _("See Ship Through Water"), wxDefaultPosition, wxDefaultSize);
                mSeeShipThroughOceanCheckBox->SetToolTip(_("Shows the ship either behind the sea water or in front of it."));
                mSeeShipThroughOceanCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::ShowShipThroughOcean, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                oceanSizer->Add(
                    mSeeShipThroughOceanCheckBox,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Ocean Transparency
            {
                mOceanTransparencySlider = new SliderControl<float>(
                    oceanBox,
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

                oceanSizer->Add(
                    mOceanTransparencySlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(3, 1),
                    wxALL,
                    CellBorder);
            }

            // Ocean Darkening Rate
            {
                mOceanDarkeningRateSlider = new SliderControl<float>(
                    oceanBox,
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

                oceanSizer->Add(
                    mOceanDarkeningRateSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(3, 1),
                    wxALL,
                    CellBorder);
            }

            oceanBoxSizer1->Add(oceanSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        oceanBox->SetSizerAndFit(oceanBoxSizer1);

        gridSizer->Add(
            oceanBox,
            wxGBPosition(0, 0),
            wxGBSpan(2, 2),
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
                    wxGridBagSizer * landRenderModeBoxSizer2 = new wxGridBagSizer(5, 5);

                    mTextureLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureLandRenderModeRadioButton->SetToolTip(_("Draws the ocean floor using a static image."));
                    mTextureLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnLandRenderModeRadioButtonClick, this);
                    landRenderModeBoxSizer2->Add(mTextureLandRenderModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mTextureLandComboBox = new wxBitmapComboBox(landRenderModeBox, wxID_ANY, wxEmptyString,
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
                    landRenderModeBoxSizer2->Add(mTextureLandComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL, 0);

                    mFlatLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatLandRenderModeRadioButton->SetToolTip(_("Draws the ocean floor using a static color."));
                    mFlatLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnLandRenderModeRadioButtonClick, this);
                    landRenderModeBoxSizer2->Add(mFlatLandRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatLandColorPicker = new wxColourPickerCtrl(landRenderModeBox, wxID_ANY);
                    mFlatLandColorPicker->SetToolTip(_("Sets the single color of the ocean floor."));
                    mFlatLandColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatLandColorChanged, this);
                    landRenderModeBoxSizer2->Add(mFlatLandColorPicker, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 0);

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
            wxGBPosition(0, 2),
            wxGBSpan(1, 2),
            wxALL | wxEXPAND,
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
                mFlatSkyColorPicker->SetToolTip(_("Sets the color of the sky. Duh."));
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
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_LEFT,
            CellBorder);
    }

    // Lamp Light
    {
        wxStaticBox * lampLightBox = new wxStaticBox(panel, wxID_ANY, _("Lamp Light"));

        wxBoxSizer * lampLightBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        lampLightBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * lampLightSizer = new wxGridBagSizer(0, 0);

            // Lamp Light color
            {
                mFlatLampLightColorPicker = new wxColourPickerCtrl(lampLightBox, wxID_ANY);
                mFlatLampLightColorPicker->SetToolTip(_("Sets the color of lamp lights."));
                mFlatLampLightColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnFlatLampLightColorChanged, this);

                lampLightSizer->Add(
                    mFlatLampLightColorPicker,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            lampLightBoxSizer1->Add(lampLightSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        lampLightBox->SetSizerAndFit(lampLightBoxSizer1);

        gridSizer->Add(
            lampLightBox,
            wxGBPosition(1, 3),
            wxGBSpan(1, 1),
            wxALL | wxALIGN_RIGHT,
            CellBorder);
    }

    // Heat
    {
        wxStaticBox * heatBox = new wxStaticBox(panel, wxID_ANY, _("Heat"));

        wxBoxSizer * heatBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        heatBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * renderSizer = new wxGridBagSizer(0, 0);

            // Draw heat overlay
            {
                mDrawHeatOverlayCheckBox = new wxCheckBox(heatBox, wxID_ANY,
                    _("Draw Heat Overlay"), wxDefaultPosition, wxDefaultSize);
                mDrawHeatOverlayCheckBox->SetToolTip(_("Renders heat over ships."));
                mDrawHeatOverlayCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DrawHeatOverlay, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                renderSizer->Add(
                    mDrawHeatOverlayCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Heat blaster flame
            {
                mDrawHeatBlasterFlameCheckBox = new wxCheckBox(heatBox, wxID_ANY,
                    _("Draw HeatBlaster Flame"), wxDefaultPosition, wxDefaultSize);
                mDrawHeatBlasterFlameCheckBox->SetToolTip(_("Renders flames out of the HeatBlaster tool."));
                mDrawHeatBlasterFlameCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::DrawHeatBlasterFlame, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                renderSizer->Add(
                    mDrawHeatBlasterFlameCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Flame size adjustment
            {
                mShipFlameSizeAdjustmentSlider = new SliderControl<float>(
                    heatBox,
                    SliderWidth,
                    -1,
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

                renderSizer->Add(
                    mShipFlameSizeAdjustmentSlider,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Heat overlay transparency
            {
                mHeatOverlayTransparencySlider = new SliderControl<float>(
                    heatBox,
                    SliderWidth,
                    SliderHeight,
                    _("Heat Overlay Transparency"),
                    _("Adjusts the transparency of the heat overlay."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::HeatOverlayTransparency, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                renderSizer->Add(
                    mHeatOverlayTransparencySlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(3, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            heatBoxSizer1->Add(renderSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        heatBox->SetSizerAndFit(heatBoxSizer1);

        gridSizer->Add(
            heatBox,
            wxGBPosition(2, 0),
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
            wxBoxSizer * shipSizer = new wxBoxSizer(wxVERTICAL);

            // Show Stress
            {
                mShowStressCheckBox = new wxCheckBox(shipBox, wxID_ANY,
                    _("Show Stress"), wxDefaultPosition, wxDefaultSize);
                mShowStressCheckBox->SetToolTip(_("Enables or disables highlighting of the springs that are under heavy stress and close to rupture."));
                mShowStressCheckBox->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [this](wxCommandEvent & event)
                    {
                        mLiveSettings.SetValue(GameSettings::ShowShipStress, event.IsChecked());
                        OnLiveSettingsChanged();
                    });

                shipSizer->Add(mShowStressCheckBox, 0, wxALL | wxALIGN_LEFT, 5);
            }

            shipBoxSizer1->Add(shipSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        shipBox->SetSizerAndFit(shipBoxSizer1);

        gridSizer->Add(
            shipBox,
            wxGBPosition(2, 1),
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

            waterSizer->AddGrowableRow(0, 1); // Slider above button

            // Water contrast
            {
                mWaterContrastSlider = new SliderControl<float>(
                    waterBox,
                    SliderWidth,
                    -1,
                    _("Water Contrast"),
                    _("Adjusts the contrast of water inside physical bodies."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterContrast, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                waterSizer->Add(
                    mWaterContrastSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Default Water Color
            {
                mDefaultWaterColorPicker = new wxColourPickerCtrl(waterBox, wxID_ANY);
                mDefaultWaterColorPicker->SetToolTip(_("Sets the color of water which is used when ocean render mode is set to 'Texture'."));
                mDefaultWaterColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDefaultWaterColorChanged, this);

                waterSizer->Add(
                    mDefaultWaterColorPicker,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL | wxALIGN_CENTER_HORIZONTAL,
                    CellBorder);
            }

            // Water Level of Detail
            {
                mWaterLevelOfDetailSlider = new SliderControl<float>(
                    waterBox,
                    SliderWidth,
                    SliderHeight,
                    _("Water Level of Detail"),
                    _("Adjusts how detailed water inside a physical body looks."),
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterLevelOfDetail, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterLevelOfDetail(),
                        mGameControllerSettingsOptions->GetMaxWaterLevelOfDetail()));

                waterSizer->Add(
                    mWaterLevelOfDetailSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxALL,
                    CellBorder);
            }

            waterBoxSizer1->Add(waterSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        waterBox->SetSizerAndFit(waterBoxSizer1);

        gridSizer->Add(
            waterBox,
            wxGBPosition(2, 2),
            wxGBSpan(1, 2),
            wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateSoundAndAdvancedPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);


    //
    // Row 1
    //

    // Sounds
    {
        wxStaticBox * soundBox = new wxStaticBox(panel, wxID_ANY, _("Sound"));

        wxBoxSizer * soundBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        soundBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * soundSizer = new wxGridBagSizer(0, 0);

            // Effects volume
            {
                mEffectsVolumeSlider = new SliderControl<float>(
                    soundBox,
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

                soundSizer->Add(
                    mEffectsVolumeSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Tools volume
            {
                mToolsVolumeSlider = new SliderControl<float>(
                    soundBox,
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

                soundSizer->Add(
                    mToolsVolumeSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Checkboxes
            {
                wxStaticBoxSizer * checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, soundBox);

                {
                    mPlayBreakSoundsCheckBox = new wxCheckBox(soundBox, wxID_ANY,
                        _("Play Break Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                    mPlayBreakSoundsCheckBox->SetToolTip(_("Enables or disables the generation of sounds when materials break."));
                    mPlayBreakSoundsCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                        {
                            mLiveSettings.SetValue(GameSettings::PlayBreakSounds, event.IsChecked());
                            OnLiveSettingsChanged();
                        });

                    checkboxesSizer->Add(mPlayBreakSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);
                }

                {
                    mPlayStressSoundsCheckBox = new wxCheckBox(soundBox, wxID_ANY,
                        _("Play Stress Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                    mPlayStressSoundsCheckBox->SetToolTip(_("Enables or disables the generation of sounds when materials are under stress."));
                    mPlayStressSoundsCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                        {
                            mLiveSettings.SetValue(GameSettings::PlayStressSounds, event.IsChecked());
                            OnLiveSettingsChanged();
                        });

                    checkboxesSizer->Add(mPlayStressSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);
                }

                {
                    mPlayWindSoundCheckBox = new wxCheckBox(soundBox, wxID_ANY,
                        _("Play Wind Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                    mPlayWindSoundCheckBox->SetToolTip(_("Enables or disables the generation of wind sounds."));
                    mPlayWindSoundCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                        {
                            mLiveSettings.SetValue(GameSettings::PlayWindSound, event.IsChecked());
                            OnLiveSettingsChanged();
                        });

                    checkboxesSizer->Add(mPlayWindSoundCheckBox, 0, wxALL | wxALIGN_LEFT, 5);
                }

                {
                    mPlayAirBubbleSurfaceSoundCheckBox = new wxCheckBox(soundBox, wxID_ANY,
                        _("Play Bubbles' Surface Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
                    mPlayAirBubbleSurfaceSoundCheckBox->SetToolTip(_("Enables or disables the bubbling sound when air bubbles come to the surface."));
                    mPlayAirBubbleSurfaceSoundCheckBox->Bind(
                        wxEVT_COMMAND_CHECKBOX_CLICKED,
                        [this](wxCommandEvent & event)
                        {
                            mLiveSettings.SetValue(GameSettings::PlayAirBubbleSurfaceSound, event.IsChecked());
                            OnLiveSettingsChanged();
                        });

                    checkboxesSizer->Add(mPlayAirBubbleSurfaceSoundCheckBox, 0, wxALL | wxALIGN_LEFT, 5);
                }

                soundSizer->Add(
                    checkboxesSizer,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            soundBoxSizer1->Add(soundSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        soundBox->SetSizerAndFit(soundBoxSizer1);

        gridSizer->Add(
            soundBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Advanced
    {
        wxStaticBox * advancedBox = new wxStaticBox(panel, wxID_ANY, _("Advanced"));

        wxBoxSizer * advancedBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        advancedBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * advancedSizer = new wxGridBagSizer(0, 0);

            // Spring Stiffness
            {
                mSpringStiffnessSlider = new SliderControl<float>(
                    advancedBox,
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

                advancedSizer->Add(
                    mSpringStiffnessSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Spring Damping
            {
                mSpringDampingSlider = new SliderControl<float>(
                    advancedBox,
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

                advancedSizer->Add(
                    mSpringDampingSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Checkboxes
            {
                wxStaticBoxSizer * checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, advancedBox);

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

                    mDebugShipRenderModeRadioBox = new wxRadioBox(advancedBox, wxID_ANY, _("Ship Debug Draw Options"), wxDefaultPosition, wxDefaultSize,
                        WXSIZEOF(debugShipRenderModeChoices), debugShipRenderModeChoices, 1, wxRA_SPECIFY_COLS);
                    Connect(mDebugShipRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnDebugShipRenderModeRadioBox);

                    checkboxesSizer->Add(
                        mDebugShipRenderModeRadioBox,
                        0,
                        wxEXPAND | wxALL,
                        5);
                }

                {
                    wxStaticBox * extrasBox = new wxStaticBox(advancedBox, wxID_ANY, _("Ship Extra Draw Options"));

                    wxBoxSizer * extrasBoxSizer = new wxBoxSizer(wxVERTICAL);
                    extrasBoxSizer->AddSpacer(StaticBoxTopMargin);
                    extrasBoxSizer->AddSpacer(3);

                    {
                        wxBoxSizer * extrasSizer = new wxBoxSizer(wxVERTICAL);

                        {
                            mDrawFlamesCheckBox = new wxCheckBox(extrasBox, wxID_ANY,
                                _("Draw Flames"), wxDefaultPosition, wxDefaultSize);
                            mDrawFlamesCheckBox->SetToolTip(_("Enables or disables rendering of flames."));
                            mDrawFlamesCheckBox->Bind(
                                wxEVT_COMMAND_CHECKBOX_CLICKED,
                                [this](wxCommandEvent & event)
                                {
                                    mLiveSettings.SetValue(GameSettings::DrawFlames, event.IsChecked());
                                    OnLiveSettingsChanged();
                                });

                            extrasSizer->Add(mDrawFlamesCheckBox, 0, wxALIGN_LEFT, 0);
                        }

                        extrasSizer->AddSpacer(3);

                        {
                            mShowFrontiersCheckBox = new wxCheckBox(extrasBox, wxID_ANY,
                                _("Show Frontiers"), wxDefaultPosition, wxDefaultSize);
                            mShowFrontiersCheckBox->SetToolTip(_("Enables or disables visualization of the frontiers of the ship."));
                            mShowFrontiersCheckBox->Bind(
                                wxEVT_COMMAND_CHECKBOX_CLICKED,
                                [this](wxCommandEvent & event)
                                {
                                    mLiveSettings.SetValue(GameSettings::ShowShipFrontiers, event.IsChecked());
                                    OnLiveSettingsChanged();
                                });

                            extrasSizer->Add(mShowFrontiersCheckBox, 0, wxALIGN_LEFT, 0);
                        }

                        extrasSizer->AddSpacer(3);

                        {
                            mShowAABBsCheckBox = new wxCheckBox(extrasBox, wxID_ANY,
                                _("Show AABBs"), wxDefaultPosition, wxDefaultSize);
                            mShowAABBsCheckBox->SetToolTip(_("Enables or disables visualization of the AABBs (Axis-Aligned Bounding Boxes)."));
                            mShowAABBsCheckBox->Bind(
                                wxEVT_COMMAND_CHECKBOX_CLICKED,
                                [this](wxCommandEvent & event)
                                {
                                    mLiveSettings.SetValue(GameSettings::ShowAABBs, event.IsChecked());
                                    OnLiveSettingsChanged();
                                });

                            extrasSizer->Add(mShowAABBsCheckBox, 0, wxALIGN_LEFT, 0);
                        }

                        extrasBoxSizer->Add(extrasSizer, 0, wxALL, StaticBoxInsetMargin);
                    }

                    extrasBox->SetSizerAndFit(extrasBoxSizer);

                    checkboxesSizer->Add(
                        extrasBox,
                        0,
                        wxEXPAND | wxALL,
                        5);
                }

                {
                    wxString vectorFieldRenderModeChoices[] =
                    {
                        _("None"),
                        _("Point Velocities"),
                        _("Point Forces"),
                        _("Point Water Velocities"),
                        _("Point Water Momenta")
                    };

                    mVectorFieldRenderModeRadioBox = new wxRadioBox(advancedBox, wxID_ANY, _("Vector Field Draw Options"), wxDefaultPosition, wxSize(-1, -1),
                        WXSIZEOF(vectorFieldRenderModeChoices), vectorFieldRenderModeChoices, 1, wxRA_SPECIFY_COLS);
                    mVectorFieldRenderModeRadioBox->SetToolTip(_("Enables or disables rendering of vector fields."));
                    Connect(mVectorFieldRenderModeRadioBox->GetId(), wxEVT_RADIOBOX, (wxObjectEventFunction)&SettingsDialog::OnVectorFieldRenderModeRadioBox);

                    checkboxesSizer->Add(
                        mVectorFieldRenderModeRadioBox,
                        0,
                        wxEXPAND | wxALL,
                        5);
                }

                advancedSizer->Add(
                    checkboxesSizer,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            advancedBoxSizer1->Add(advancedSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        advancedBox->SetSizerAndFit(advancedBoxSizer1);

        gridSizer->Add(
            advancedBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateSettingsManagementPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Load settings
    //

    {
        wxStaticBox * loadSettingsBox = new wxStaticBox(panel, wxID_ANY, _("Load Settings"));

        wxBoxSizer * loadSettingsBoxVSizer = new wxBoxSizer(wxVERTICAL);
        loadSettingsBoxVSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxBoxSizer * loadSettingsBoxHSizer = new wxBoxSizer(wxHORIZONTAL);

            // Col 1

            {
                mPersistedSettingsListCtrl = new wxListCtrl(
                    loadSettingsBox,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxSize(250, 370),
                    wxBORDER_STATIC /*https://trac.wxwidgets.org/ticket/18549*/ | wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);

                mPersistedSettingsListCtrl->AppendColumn(
                    wxEmptyString,
                    wxLIST_FORMAT_LEFT,
                    wxLIST_AUTOSIZE_USEHEADER);

                for (size_t p = 0; p < mPersistedSettings.size(); ++p)
                {
                    InsertPersistedSettingInCtrl(p, mPersistedSettings[p].Key);
                }

                if (!mPersistedSettings.empty())
                {
                    // Select first item
                    mPersistedSettingsListCtrl->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                }

                mPersistedSettingsListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &SettingsDialog::OnPersistedSettingsListCtrlSelected, this);
                mPersistedSettingsListCtrl->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SettingsDialog::OnPersistedSettingsListCtrlActivated, this);

                loadSettingsBoxHSizer->Add(mPersistedSettingsListCtrl, 0, wxALL | wxEXPAND, 5);
            }

            // Col 2

            {
                wxBoxSizer * col2BoxSizer = new wxBoxSizer(wxVERTICAL);

                {
                    auto label = new wxStaticText(loadSettingsBox, wxID_ANY, _("Description:"));

                    col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
                }

                {
                    mPersistedSettingsDescriptionTextCtrl = new wxTextCtrl(
                        loadSettingsBox,
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxSize(250, 120),
                        wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

                    col2BoxSizer->Add(mPersistedSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
                }

                {
                    mApplyPersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, _("Apply Saved Settings"));
                    mApplyPersistedSettingsButton->SetToolTip(_("Loads the selected settings and applies them on top of the current settings."));
                    mApplyPersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnApplyPersistedSettingsButton, this);

                    col2BoxSizer->Add(mApplyPersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mRevertToPersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, _("Revert to Saved Settings"));
                    mRevertToPersistedSettingsButton->SetToolTip(_("Reverts all settings to the selected settings."));
                    mRevertToPersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRevertToPersistedSettingsButton, this);

                    col2BoxSizer->Add(mRevertToPersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mReplacePersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, _("Replace Saved Settings with Current"));
                    mReplacePersistedSettingsButton->SetToolTip(_("Overwrites the selected settings with the current settings."));
                    mReplacePersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnReplacePersistedSettingsButton, this);

                    col2BoxSizer->Add(mReplacePersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

                    mDeletePersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, _("Delete Saved Settings"));
                    mDeletePersistedSettingsButton->SetToolTip(_("Deletes the selected settings."));
                    mDeletePersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnDeletePersistedSettingsButton, this);

                    col2BoxSizer->Add(mDeletePersistedSettingsButton, 0, wxALL | wxEXPAND, 5);
                }

                loadSettingsBoxHSizer->Add(col2BoxSizer, 0, 0, 0);
            }

            loadSettingsBoxVSizer->Add(loadSettingsBoxHSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        loadSettingsBox->SetSizerAndFit(loadSettingsBoxVSizer);

        gridSizer->Add(
            loadSettingsBox,
            wxGBPosition(0, 0),
            wxGBSpan(2, 1),
            wxEXPAND | wxALL,
            CellBorder);

        ReconciliateLoadPersistedSettings();
    }

    //
    // Save settings
    //

    {
        wxStaticBox * saveSettingsBox = new wxStaticBox(panel, wxID_ANY, _("Save Settings"));

        wxBoxSizer * saveSettingsBoxVSizer = new wxBoxSizer(wxVERTICAL);
        saveSettingsBoxVSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxBoxSizer * saveSettingsBoxHSizer = new wxBoxSizer(wxHORIZONTAL);

            {
                wxBoxSizer * col2BoxSizer = new wxBoxSizer(wxVERTICAL);

                {
                    auto label = new wxStaticText(saveSettingsBox, wxID_ANY, _("Name:"));

                    col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
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
                        saveSettingsBox,
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxDefaultSize,
                        0,
                        validator);

                    mSaveSettingsNameTextCtrl->Bind(wxEVT_TEXT, &SettingsDialog::OnSaveSettingsTextEdited, this);

                    col2BoxSizer->Add(mSaveSettingsNameTextCtrl, 0, wxALL | wxEXPAND, 5);
                }

                {
                    auto label = new wxStaticText(saveSettingsBox, wxID_ANY, _("Description:"));

                    col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
                }

                {
                    mSaveSettingsDescriptionTextCtrl = new wxTextCtrl(
                        saveSettingsBox,
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxSize(250, 120),
                        wxTE_MULTILINE | wxTE_WORDWRAP);

                    mSaveSettingsDescriptionTextCtrl->Bind(wxEVT_TEXT, &SettingsDialog::OnSaveSettingsTextEdited, this);

                    col2BoxSizer->Add(mSaveSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
                }

                {
                    mSaveSettingsButton = new wxButton(saveSettingsBox, wxID_ANY, _("Save Current Settings"));
                    mSaveSettingsButton->SetToolTip(_("Saves the current settings using the specified name."));
                    mSaveSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnSaveSettingsButton, this);

                    col2BoxSizer->Add(mSaveSettingsButton, 0, wxALL | wxEXPAND, 5);
                }

                saveSettingsBoxHSizer->Add(col2BoxSizer, 0, 0, 0);
            }

            saveSettingsBoxVSizer->Add(saveSettingsBoxHSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        saveSettingsBox->SetSizerAndFit(saveSettingsBoxVSizer);

        gridSizer->Add(
            saveSettingsBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);

        ReconciliateSavePersistedSettings();
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::SyncControlsWithSettings(Settings<GameSettings> const & settings)
{
    // Mechanics, Air, and Fluids

    mMechanicalQualitySlider->SetValue(settings.GetValue<float>(GameSettings::NumMechanicalDynamicsIterationsAdjustment));

    mStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStrengthAdjustment));

    mGlobalDampingAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::GlobalDampingAdjustment));

    mRotAcceler8rSlider->SetValue(settings.GetValue<float>(GameSettings::RotAcceler8r));

    mAirFrictionDragSlider->SetValue(settings.GetValue<float>(GameSettings::AirFrictionDragAdjustment));

    mAirPressureDragSlider->SetValue(settings.GetValue<float>(GameSettings::AirPressureDragAdjustment));

    mWaterDensitySlider->SetValue(settings.GetValue<float>(GameSettings::WaterDensityAdjustment));

    mWaterFrictionDragSlider->SetValue(settings.GetValue<float>(GameSettings::WaterFrictionDragAdjustment));

    mWaterPressureDragSlider->SetValue(settings.GetValue<float>(GameSettings::WaterPressureDragAdjustment));

    mWaterIntakeSlider->SetValue(settings.GetValue<float>(GameSettings::WaterIntakeAdjustment));

    mWaterCrazynessSlider->SetValue(settings.GetValue<float>(GameSettings::WaterCrazyness));

    mWaterDiffusionSpeedSlider->SetValue(settings.GetValue<float>(GameSettings::WaterDiffusionSpeedAdjustment));

    // Heat

    mThermalConductivityAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ThermalConductivityAdjustment));

    mHeatDissipationAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::HeatDissipationAdjustment));

    mIgnitionTemperatureAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::IgnitionTemperatureAdjustment));

    mMeltingTemperatureAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::MeltingTemperatureAdjustment));

    mCombustionSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::CombustionSpeedAdjustment));

    mCombustionHeatAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::CombustionHeatAdjustment));

    mAirTemperatureSlider->SetValue(settings.GetValue<float>(GameSettings::AirTemperature));

    mWaterTemperatureSlider->SetValue(settings.GetValue<float>(GameSettings::WaterTemperature));

    mElectricalElementHeatProducedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ElectricalElementHeatProducedAdjustment));

    mHeatBlasterRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::HeatBlasterRadius));

    mHeatBlasterHeatFlowSlider->SetValue(settings.GetValue<float>(GameSettings::HeatBlasterHeatFlow));

    mMaxBurningParticlesSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::MaxBurningParticles));

    // Ocean, Smoke, and Sky

    mOceanDepthSlider->SetValue(settings.GetValue<float>(GameSettings::SeaDepth));
    mOceanFloorBumpinessSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorBumpiness));
    mOceanFloorDetailAmplificationSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorDetailAmplification));
    mOceanFloorElasticitySlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorElasticity));
    mOceanFloorFrictionSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorFriction));

    mSmokeEmissionDensityAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::SmokeEmissionDensityAdjustment));
    mSmokeParticleLifetimeAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::SmokeParticleLifetimeAdjustment));

    mStormStrengthAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::StormStrengthAdjustment));
    mDoRainWithStormCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoRainWithStorm));
    mRainFloodAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::RainFloodAdjustment));
    mRainFloodAdjustmentSlider->Enable(settings.GetValue<bool>(GameSettings::DoRainWithStorm));
    mStormDurationSlider->SetValue(settings.GetValue<std::chrono::seconds>(GameSettings::StormDuration).count());
    mStormRateSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::StormRate).count());

    mNumberOfStarsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfStars));
    mNumberOfCloudsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfClouds));
    mDoDayLightCycleCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoDayLightCycle));
    mDayLightCycleDurationSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::DayLightCycleDuration).count());
    mDayLightCycleDurationSlider->Enable(settings.GetValue<bool>(GameSettings::DoDayLightCycle));

    // Wind, Waves, Fishes, and Lights

    mWindSpeedBaseSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedBase));
    mModulateWindCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoModulateWind));
    mWindGustAmplitudeSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedMaxFactor));
    mWindGustAmplitudeSlider->Enable(settings.GetValue<bool>(GameSettings::DoModulateWind));

    mBasalWaveHeightAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveHeightAdjustment));
    mBasalWaveLengthAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveLengthAdjustment));
    mBasalWaveSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveSpeedAdjustment));
    mTsunamiRateSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::TsunamiRate).count());
    mRogueWaveRateSlider->SetValue(settings.GetValue<std::chrono::minutes>(GameSettings::RogueWaveRate).count());

    mNumberOfFishesSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfFishes));
    mFishSizeMultiplierSlider->SetValue(settings.GetValue<float>(GameSettings::FishSizeMultiplier));
    mFishSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::FishSpeedAdjustment));
    mDoFishShoalingCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoFishShoaling));
    mFishShoalRadiusAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::FishShoalRadiusAdjustment));
    mFishShoalRadiusAdjustmentSlider->Enable(settings.GetValue<bool>(GameSettings::DoFishShoaling));

    mLuminiscenceSlider->SetValue(settings.GetValue<float>(GameSettings::LuminiscenceAdjustment));
    mLightSpreadSlider->SetValue(settings.GetValue<float>(GameSettings::LightSpreadAdjustment));

    // Interactions

    mDestroyRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::DestroyRadius));

    mBombBlastRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastRadius));

    mBombBlastHeatSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastHeat));

    mAntiMatterBombImplosionStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::AntiMatterBombImplosionStrength));

    mFloodRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::FloodRadius));

    mFloodQuantitySlider->SetValue(settings.GetValue<float>(GameSettings::FloodQuantity));

    mRepairRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::RepairRadius));

    mRepairSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::RepairSpeedAdjustment));

    mGenerateAirBubblesCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateAirBubbles));

    mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoDisplaceOceanSurfaceAtAirBubblesSurfacing));
    mDisplaceOceanFloorSurfaceAtAirBubbleSurfacingCheckBox->Enable(settings.GetValue<bool>(GameSettings::DoGenerateAirBubbles));

    mAirBubbleDensitySlider->SetValue(settings.GetValue<float>(GameSettings::AirBubblesDensity));
    mAirBubbleDensitySlider->Enable(settings.GetValue<bool>(GameSettings::DoGenerateAirBubbles));

    mGenerateDebrisCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateDebris));

    mGenerateSparklesForCutsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateSparklesForCuts));

    mGenerateEngineWakeCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateEngineWakeParticles));

    mUltraViolentCheckBox->SetValue(settings.GetValue<bool>(GameSettings::UltraViolentMode));

    mEngineThrustAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::EngineThrustAdjustment));

    mWaterPumpPowerAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::WaterPumpPowerAdjustment));

    // Render

    auto oceanRenderMode = settings.GetValue<OceanRenderModeType>(GameSettings::OceanRenderMode);
    switch (oceanRenderMode)
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

    auto depthOceanColorStart = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorStart);
    mDepthOceanColorStartPicker->SetColour(wxColor(depthOceanColorStart.r, depthOceanColorStart.g, depthOceanColorStart.b));

    auto depthOceanColorEnd = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorEnd);
    mDepthOceanColorEndPicker->SetColour(wxColor(depthOceanColorEnd.r, depthOceanColorEnd.g, depthOceanColorEnd.b));

    auto flatOceanColor = settings.GetValue<rgbColor>(GameSettings::FlatOceanColor);
    mFlatOceanColorPicker->SetColour(wxColor(flatOceanColor.r, flatOceanColor.g, flatOceanColor.b));

    ReconciliateOceanRenderModeSettings();

    mOceanRenderDetailModeDetailedCheckBox->SetValue(settings.GetValue<OceanRenderDetailType>(GameSettings::OceanRenderDetail) == OceanRenderDetailType::Detailed);

    mSeeShipThroughOceanCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipThroughOcean));

    mOceanTransparencySlider->SetValue(settings.GetValue<float>(GameSettings::OceanTransparency));

    mOceanDarkeningRateSlider->SetValue(settings.GetValue<float>(GameSettings::OceanDarkeningRate));

    auto landRenderMode = settings.GetValue<LandRenderModeType>(GameSettings::LandRenderMode);
    switch (landRenderMode)
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

    auto flatLandColor = settings.GetValue<rgbColor>(GameSettings::FlatLandColor);
    mFlatLandColorPicker->SetColour(wxColor(flatLandColor.r, flatLandColor.g, flatLandColor.b));

    ReconciliateLandRenderModeSettings();

    auto flatSkyColor = settings.GetValue<rgbColor>(GameSettings::FlatSkyColor);
    mFlatSkyColorPicker->SetColour(wxColor(flatSkyColor.r, flatSkyColor.g, flatSkyColor.b));

    mShowStressCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipStress));

    auto flatLampLightColor = settings.GetValue<rgbColor>(GameSettings::FlatLampLightColor);
    mFlatLampLightColorPicker->SetColour(wxColor(flatLampLightColor.r, flatLampLightColor.g, flatLampLightColor.b));

    auto defaultWaterColor = settings.GetValue<rgbColor>(GameSettings::DefaultWaterColor);
    mDefaultWaterColorPicker->SetColour(wxColor(defaultWaterColor.r, defaultWaterColor.g, defaultWaterColor.b));

    mWaterContrastSlider->SetValue(settings.GetValue<float>(GameSettings::WaterContrast));

    mWaterLevelOfDetailSlider->SetValue(settings.GetValue<float>(GameSettings::WaterLevelOfDetail));

    mDrawHeatOverlayCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawHeatOverlay));

    mHeatOverlayTransparencySlider->SetValue(settings.GetValue<float>(GameSettings::HeatOverlayTransparency));

    mDrawHeatBlasterFlameCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawHeatBlasterFlame));

    mShipFlameSizeAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ShipFlameSizeAdjustment));

    // Sound

    mEffectsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterEffectsVolume));

    mToolsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterToolsVolume));

    mPlayBreakSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayBreakSounds));

    mPlayStressSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayStressSounds));

    mPlayWindSoundCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayWindSound));

    mPlayAirBubbleSurfaceSoundCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayAirBubbleSurfaceSound));

    // Advanced

    mSpringStiffnessSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStiffnessAdjustment));

    mSpringDampingSlider->SetValue(settings.GetValue<float>(GameSettings::SpringDampingAdjustment));

    auto debugShipRenderMode = settings.GetValue<DebugShipRenderModeType>(GameSettings::DebugShipRenderMode);
    switch (debugShipRenderMode)
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

    mDrawFlamesCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawFlames));

    mShowFrontiersCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipFrontiers));

    mShowAABBsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowAABBs));

    auto vectorFieldRenderMode = settings.GetValue<VectorFieldRenderModeType>(GameSettings::VectorFieldRenderMode);
    switch (vectorFieldRenderMode)
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