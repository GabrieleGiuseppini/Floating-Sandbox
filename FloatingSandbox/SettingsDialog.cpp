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
#include <wx/stattext.h>
#include <wx/string.h>

#include <algorithm>
#include <stdexcept>

static int constexpr SliderWidth = 40;
static int constexpr SliderHeight = 140;
static int constexpr SliderBorder = 10;

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
			// - Among user, LastPlayed is last

			if (m1.Key.StorageType != m2.Key.StorageType)
				return m2.Key.StorageType == PersistedSettingsStorageTypes::System;

			assert(m1.Key.StorageType == m2.Key.StorageType);

			if (m1.Key == PersistedSettingsKey::MakeLastPlayedSettingsKey() || m2.Key == PersistedSettingsKey::MakeLastPlayedSettingsKey())
				return m2.Key == PersistedSettingsKey::MakeLastPlayedSettingsKey();

			return m1.Key.Name < m2.Key.Name;
		}
	};

}

SettingsDialog::SettingsDialog(
    wxWindow* parent,
    std::shared_ptr<SettingsManager> settingsManager,
	std::shared_ptr<IGameControllerSettingsOptions> gameControllerSettingsOptions,
    ResourceLoader const & resourceLoader)
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
        _("Settings"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_NO_TASKBAR
			| /* wxFRAME_FLOAT_ON_PARENT */ wxSTAY_ON_TOP, // See https://trac.wxwidgets.org/ticket/18535
        _T("Settings Window"));

    this->Bind(wxEVT_CLOSE_WINDOW, &SettingsDialog::OnCloseButton, this);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    SetIcon(wxICON(AAA_SHIP_ICON));


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
    // Mechanics, fluids, lights
    //

    wxPanel * mechanicsFluidsLightsPanel = new wxPanel(notebook);

    PopulateMechanicsFluidsLightsPanel(mechanicsFluidsLightsPanel);

    notebook->AddPage(mechanicsFluidsLightsPanel, "Mechanics, Fluids, and Lights");


    //
    // Heat
    //

    wxPanel * heatPanel = new wxPanel(notebook);

    PopulateHeatPanel(heatPanel);

    notebook->AddPage(heatPanel, "Heat and Combustion");


    //
    // Ocean and Sky
    //

    wxPanel * oceanAndSkyPanel = new wxPanel(notebook);

    PopulateOceanAndSkyPanel(oceanAndSkyPanel);

    notebook->AddPage(oceanAndSkyPanel, "Ocean and Sky");


    //
    // Wind and Waves
    //

    wxPanel * windAndWavesPanel = new wxPanel(notebook);

    PopulateWindAndWavesPanel(windAndWavesPanel);

    notebook->AddPage(windAndWavesPanel, "Wind and Waves");


    //
    // Interactions
    //

    wxPanel * interactionsPanel = new wxPanel(notebook);

    PopulateInteractionsPanel(interactionsPanel);

    notebook->AddPage(interactionsPanel, "Interactions");


    //
    // Rendering
    //

    wxPanel * renderingPanel = new wxPanel(notebook);

    PopulateRenderingPanel(renderingPanel);

    notebook->AddPage(renderingPanel, "Rendering");


    //
    // Sound
    //

    wxPanel * soundPanel = new wxPanel(notebook);

    PopulateSoundPanel(soundPanel);

    notebook->AddPage(soundPanel, "Sound");


    //
    // Advanced
    //

    wxPanel * advancedPanel = new wxPanel(notebook);

    PopulateAdvancedPanel(advancedPanel);

    notebook->AddPage(advancedPanel, "Advanced");


	//
	// Settings Management
	//

	wxPanel * settingsManagementPanel = new wxPanel(notebook);

	PopulateSettingsManagementPanel(settingsManagementPanel);

	notebook->AddPage(settingsManagementPanel, "Settings Management");



    dialogVSizer->Add(notebook, 0, wxEXPAND);

    dialogVSizer->AddSpacer(20);


    // Buttons

    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

		mRevertToDefaultsButton = new wxButton(this, wxID_ANY, "Revert to Defaults");
		mRevertToDefaultsButton->SetToolTip("Resets all settings to their default values.");
		mRevertToDefaultsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRevertToDefaultsButton, this);
		buttonsSizer->Add(mRevertToDefaultsButton, 0, 0, 0);

		buttonsSizer->AddStretchSpacer(1);

        mOkButton = new wxButton(this, wxID_ANY, "OK");
		mOkButton->SetToolTip("Closes the window keeping all changes.");
        mOkButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnOkButton, this);
        buttonsSizer->Add(mOkButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mCancelButton = new wxButton(this, wxID_ANY, "Cancel");
		mCancelButton->SetToolTip("Reverts all changes effected since the window was last opened, and closes the window.");
        mCancelButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnCancelButton, this);
        buttonsSizer->Add(mCancelButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mUndoButton = new wxButton(this, wxID_ANY, "Undo");
		mUndoButton->SetToolTip("Reverts all changes effected since the window was last opened.");
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
	SyncSettingsWithControls(mLiveSettings);

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

void SettingsDialog::OnUltraViolentCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::UltraViolentMode, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnGenerateDebrisCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::DoGenerateDebris, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnGenerateSparklesCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::DoGenerateSparkles, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnGenerateAirBubblesCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::DoGenerateAirBubbles, event.IsChecked());
    mAirBubbleDensitySlider->Enable(event.IsChecked());

    OnLiveSettingsChanged();
}

void SettingsDialog::OnModulateWindCheckBoxClick(wxCommandEvent & event)
{
    mLiveSettings.SetValue<bool>(GameSettings::DoModulateWind, event.IsChecked());    
    OnLiveSettingsChanged();

    mWindGustAmplitudeSlider->Enable(mModulateWindCheckBox->IsChecked());
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
		mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderMode::Texture);
	}
	else if (mDepthOceanRenderModeRadioButton->GetValue())
	{
		mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderMode::Depth);
	}
	else
	{
		assert(mFlatOceanRenderModeRadioButton->GetValue());
		mLiveSettings.SetValue(GameSettings::OceanRenderMode, OceanRenderMode::Flat);
	}

	OnLiveSettingsChanged();

    ReconciliateOceanRenderModeSettings();
}

void SettingsDialog::OnTextureOceanChanged(wxCommandEvent & /*event*/)
{
	mLiveSettings.SetValue(GameSettings::TextureOceanTextureIndex, static_cast<size_t>(mTextureOceanComboBox->GetSelection()));
    OnLiveSettingsChanged();
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

void SettingsDialog::OnSeeShipThroughOceanCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::ShowShipThroughOcean, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnLandRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
	if (mTextureLandRenderModeRadioButton->GetValue())
	{
		mLiveSettings.SetValue(GameSettings::LandRenderMode, LandRenderMode::Texture);
	}
	else
	{
		assert(mFlatLandRenderModeRadioButton->GetValue());
		mLiveSettings.SetValue(GameSettings::LandRenderMode, LandRenderMode::Flat);
	}

    ReconciliateLandRenderModeSettings();

    OnLiveSettingsChanged();
}

void SettingsDialog::OnTextureLandChanged(wxCommandEvent & /*event*/)
{
	mLiveSettings.SetValue(GameSettings::TextureLandTextureIndex, static_cast<size_t>(mTextureLandComboBox->GetSelection()));
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

void SettingsDialog::OnShipRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
	if (mTextureShipRenderModeRadioButton->GetValue())
	{
		mLiveSettings.SetValue(GameSettings::ShipRenderMode, ShipRenderMode::Texture);
	}
	else
	{
		assert(mStructureShipRenderModeRadioButton->GetValue());
		mLiveSettings.SetValue(GameSettings::ShipRenderMode, ShipRenderMode::Structure);
	}
	
    OnLiveSettingsChanged();
}

void SettingsDialog::OnShowStressCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::ShowShipStress, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnDrawHeatOverlayCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::DrawHeatOverlay, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnDrawHeatBlasterFlameCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::DrawHeatBlasterFlame, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnShipFlameRenderModeRadioButtonClick(wxCommandEvent & /*event*/)
{
	if (mMode1ShipFlameRenderModeRadioButton->GetValue())
	{
		mLiveSettings.SetValue(GameSettings::ShipFlameRenderMode, ShipFlameRenderMode::Mode1);
	}
	else if (mMode2ShipFlameRenderModeRadioButton->GetValue())
	{
		mLiveSettings.SetValue(GameSettings::ShipFlameRenderMode, ShipFlameRenderMode::Mode2);
	}
	else
	{
		assert(mNoDrawShipFlameRenderModeRadioButton->GetValue());
		mLiveSettings.SetValue(GameSettings::ShipFlameRenderMode, ShipFlameRenderMode::NoDraw);
	}
	
    OnLiveSettingsChanged();
}

void SettingsDialog::OnDebugShipRenderModeRadioBox(wxCommandEvent & /*event*/)
{
	auto selectedDebugShipRenderMode = mDebugShipRenderModeRadioBox->GetSelection();
	if (0 == selectedDebugShipRenderMode)
	{
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::None);
	}
	else if (1 == selectedDebugShipRenderMode)
	{
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::Wireframe);
	}
	else if (2 == selectedDebugShipRenderMode)
	{
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::Points);
	}
	else if (3 == selectedDebugShipRenderMode)
	{
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::Springs);
	}
	else if (4 == selectedDebugShipRenderMode)
	{
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::EdgeSprings);
	}
	else
	{
		assert(5 == selectedDebugShipRenderMode);
		mLiveSettings.SetValue(GameSettings::DebugShipRenderMode, DebugShipRenderMode::Decay);
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
			mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderMode::None);
			break;
		}

		case 1:
		{
			mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderMode::PointVelocity);
			break;
		}

		case 2:
		{
			mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderMode::PointForce);
			break;
		}

		case 3:
		{
			mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderMode::PointWaterVelocity);
			break;
		}

		default:
		{
			assert(4 == selectedVectorFieldRenderMode);
			mLiveSettings.SetValue(GameSettings::VectorFieldRenderMode, VectorFieldRenderMode::PointWaterMomentum);
			break;
		}
	}

    OnLiveSettingsChanged();
}

void SettingsDialog::OnPlayBreakSoundsCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::PlayBreakSounds, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnPlayStressSoundsCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::PlayStressSounds, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnPlayWindSoundCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::PlayWindSound, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnPlaySinkingMusicCheckBoxClick(wxCommandEvent & event)
{
	mLiveSettings.SetValue(GameSettings::PlaySinkingMusic, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnPersistedSettingsListCtrlSelected(wxListEvent & /*event*/)
{
	ReconciliateLoadPersistedSettings();
}

void SettingsDialog::OnPersistedSettingsListCtrlActivated(wxListEvent & event)
{
	LoadPersistedSettings(event.GetIndex());
}

void SettingsDialog::OnLoadAndApplyPersistedSettingsButton(wxCommandEvent & /*event*/)
{
	auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

	assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
	assert(selectedIndex < mPersistedSettings.size());

	if (selectedIndex != wxNOT_FOUND)
	{
		LoadPersistedSettings(selectedIndex);
	}
}

void SettingsDialog::OnReplacePersistedSettingsButton(wxCommandEvent & /*event*/)
{
	auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();
	
	assert(selectedIndex != wxNOT_FOUND); // Enforced by UI
	assert(selectedIndex < mPersistedSettings.size());
	assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

	if (selectedIndex != wxNOT_FOUND)
	{
		auto const & metadata = mPersistedSettings[selectedIndex];

		auto result = wxMessageBox(
			"Are you sure you want to replace settings \"" + metadata.Key.Name + "\" with the current settings?",
			"Warning",
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
	assert(selectedIndex < mPersistedSettings.size());
	assert(mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User); // Enforced by UI

	if (selectedIndex != wxNOT_FOUND)
	{
		auto const & metadata = mPersistedSettings[selectedIndex];

		// Ask user whether they're sure
		auto result = wxMessageBox(
			"Are you sure you want to delete settings \"" + metadata.Key.Name + "\"?",
			"Warning",
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
			auto result = wxMessageBox(
				"Settings \"" + settingsMetadata.Key.Name + "\" already exist; do you want to replace them with the current settings?",
				"Warning",
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

void SettingsDialog::OnRevertToDefaultsButton(wxCommandEvent& /*event*/)
{
	//
	// Enforce default settings
	//

	mLiveSettings = mSettingsManager->GetDefaults();

	// Do not update checkpoint, allow user to revert to it

	// Enforce everything as a safety net
	mLiveSettings.MarkAllAsDirty();
	mSettingsManager->EnforceDirtySettings(mLiveSettings);

	// We are back in sync
	mLiveSettings.ClearAllDirty();

	assert(mSettingsManager->Pull() == mLiveSettings);

	// Re-populate controls with new values
	SyncSettingsWithControls(mLiveSettings);

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

    // Just enforce anything in the checkpoint that is different than the current settings
    mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
    mSettingsManager->EnforceDirtySettings(mLiveSettings);

    mLiveSettings.ClearAllDirty();

    assert(mSettingsManager->Pull() == mCheckpointSettings);

    // Re-populate controls with new values
	SyncSettingsWithControls(mLiveSettings);

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

        // Just enforce anything in the checkpoint that is different than the current settings
        mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
        mSettingsManager->EnforceDirtySettings(mLiveSettings);
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

void SettingsDialog::PopulateMechanicsFluidsLightsPanel(wxPanel * panel)
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
                    "Simulation Quality",
                    "Higher values improve the rigidity of simulated structures, at the expense of longer computation times.",
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

            // Strength
            {
                mStrengthSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Strength Adjust",
                    "Adjusts the strength of springs.",
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

            // Rot Accelerator
            {
                mRotAcceler8rSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Rot Acceler8r",
                    "Adjusts the speed with which materials rot when exposed to sea water.",
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
                    wxGBPosition(0, 2),
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
            wxGBSpan(1, 3),
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
                    "Luminiscence Adjust",
                    "Adjusts the quantity of light emitted by luminiscent materials.",
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
                    "Light Spread Adjust",
                    "Adjusts how wide light emitted by luminiscent materials spreads out.",
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
            wxGBPosition(0, 3),
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
                    "Water Density Adjust",
                    "Adjusts the density of sea water, and thus the buoyancy it exerts on physical bodies.",
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

            // Water Drag
            {
                mWaterDragSlider = new SliderControl<float>(
                    fluidsBox,
                    SliderWidth,
                    SliderHeight,
                    "Water Drag Adjust",
                    "Adjusts the drag force exerted by sea water on physical bodies.",
					[this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterDragAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterDragAdjustment(),
                        1.0f,
                        mGameControllerSettingsOptions->GetMaxWaterDragAdjustment()));

                fluidsSizer->Add(
                    mWaterDragSlider,
                    wxGBPosition(0, 1),
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
                    "Water Intake Adjust",
                    "Adjusts the speed with which sea water enters or leaves a physical body.",
					[this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::WaterIntakeAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinWaterIntakeAdjustment(),
                        mGameControllerSettingsOptions->GetMaxWaterIntakeAdjustment()));

                fluidsSizer->Add(
                    mWaterIntakeSlider,
                    wxGBPosition(0, 2),
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
                    "Water Crazyness",
                    "Adjusts how \"splashy\" water flows inside a physical body.",
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
                    wxGBPosition(0, 3),
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
                    "Water Diffusion Speed",
                    "Adjusts the speed with which water propagates within a physical body.",
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
                    wxGBPosition(0, 4),
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
            wxGBSpan(1, 5),
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
                    "Thermal Conductivity Adjust",
                    "Adjusts the speed with which heat propagates along materials.",
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
                    "Heat Dissipation Adjust",
                    "Adjusts the speed with which materials dissipate or accumulate heat to or from air and water.",
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
                    "Burning Point Adjust",
                    "Adjusts the temperature at which materials ignite.",
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
                    "Melting Point Adjust",
                    "Adjusts the temperature at which materials melt.",
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
                    "Combustion Speed Adjust",
                    "Adjusts the rate with which materials consume when burning.",
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
                    "Combustion Heat Adjust",
                    "Adjusts the heat generated by fire; together with the maximum number of burning particles, determines the speed with which fire spreads to adjacent particles.",
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
                    "Air Temperature",
                    "The temperature of air (K).",                    
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
                    "Water Temperature",
                    "The temperature of water (K).",                    
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
                    "Heat Generation Adjust",
                    "Adjusts the amount of heat generated by working electrical elements, such as lamps and generators.",                    
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
                    "Radius",
                    "The radius of HeatBlaster tool (m).",                    
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
                    "Heat",
                    "The heat produced by the HeatBlaster tool (KJ/s).",                    
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
                    "Max Burning Particles",
                    "The maximum number of particles that may burn at any given moment in time; together with the combustion heat adjustment, determines the speed with which fire spreads to adjacent particles. Warning: higher values require more computing resources, with the risk of slowing the simulation down!",                    
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

void SettingsDialog::PopulateOceanAndSkyPanel(wxPanel * panel)
{
    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);

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
                    "Ocean Depth",
                    "The ocean depth (m).",                    
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
                    "Ocean Floor Bumpiness",
                    "Adjusts how much the ocean floor rolls up and down.",                    
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

            // Ocean Floor Detail Amplification
            {
                mOceanFloorDetailAmplificationSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    -1,
                    "Ocean Floor Detail",
                    "Adjusts the contrast of the user-drawn ocean floor terrain",
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
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

			// Restore Ocean Floor Terrain
			{
				wxButton * restoreDefaultTerrainButton = new wxButton(oceanBox, wxID_ANY, "Restore Default Terrain");
				restoreDefaultTerrainButton->SetToolTip("Reverts the user-drawn ocean floor terrain to the default terrain.");
				restoreDefaultTerrainButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRestoreDefaultTerrainButton, this);

				oceanSizer->Add(
					restoreDefaultTerrainButton,
					wxGBPosition(1, 2),
					wxGBSpan(1, 1),
					wxEXPAND | wxALL,
					CellBorder);
			}

            oceanBoxSizer->Add(oceanSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        oceanBox->SetSizerAndFit(oceanBoxSizer);

        gridSizer->Add(
            oceanBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 3),
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

            // Number of Stars
            {
                mNumberOfStarsSlider = new SliderControl<unsigned int>(
                    skyBox,
                    SliderWidth,
                    SliderHeight,
                    "Number of Stars",
                    "The number of stars in the sky.",                    
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
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Number of Clouds
            {
                mNumberOfCloudsSlider = new SliderControl<unsigned int>(
                    skyBox,
                    SliderWidth,
                    SliderHeight,
                    "Number of Clouds",
                    "The number of clouds in the world's sky. This is the total number of clouds in the world; at any moment in time, the number of clouds that are visible will be less than or equal to this value.",                    
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
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            skyBoxSizer->Add(skySizer, 0, wxALL, StaticBoxInsetMargin);
        }

        skyBox->SetSizerAndFit(skyBoxSizer);

        gridSizer->Add(
            skyBox,
            wxGBPosition(0, 3),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }



    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateWindAndWavesPanel(wxPanel * panel)
{
    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);

    //
    // Wind
    //

    {
        wxStaticBox * windBox = new wxStaticBox(panel, wxID_ANY, _("Wind"));

        wxBoxSizer * windBoxSizer = new wxBoxSizer(wxVERTICAL);
        windBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * windSizer = new wxGridBagSizer(0, 0);

            // Wind Speed Base
            {
                mWindSpeedBaseSlider = new SliderControl<float>(
                    windBox,
                    SliderWidth,
                    SliderHeight,
                    "Wind Speed Base",
                    "The base speed of wind (Km/h), before modulation takes place. Wind speed in turn determines ocean wave characteristics such as their height, speed, and width.",                    
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
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Wind modulation
            {
                wxBoxSizer * windModulationBoxSizer = new wxBoxSizer(wxVERTICAL);

                // Modulate Wind
                {
                    mModulateWindCheckBox = new wxCheckBox(windBox, wxID_ANY, _("Modulate Wind"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Modulate Wind Checkbox"));
                    mModulateWindCheckBox->SetToolTip("Enables or disables simulation of wind variations, alternating between dead calm and high-speed gusts.");
					mModulateWindCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnModulateWindCheckBoxClick, this);

                    windModulationBoxSizer->Add(mModulateWindCheckBox, 0, wxALIGN_LEFT, 0);
                }

                // Wind Gust Amplitude
                {
                    mWindGustAmplitudeSlider = new SliderControl<float>(
                        windBox,
                        SliderWidth,
                        -1,
                        "Wind Gust Amplitude",
                        "The amplitude of wind gusts, as a multiplier of the base wind speed.",                        
                        [this](float value)
                        {
                            this->mLiveSettings.SetValue(GameSettings::WindSpeedMaxFactor, value);
                            this->OnLiveSettingsChanged();
                        },
                        std::make_unique<LinearSliderCore>(
                            mGameControllerSettingsOptions->GetMinWindSpeedMaxFactor(),
                            mGameControllerSettingsOptions->GetMaxWindSpeedMaxFactor()));

                    windModulationBoxSizer->Add(mWindGustAmplitudeSlider, 1, wxEXPAND, 0);
                }

                windSizer->Add(
                    windModulationBoxSizer,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            windBoxSizer->Add(windSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        windBox->SetSizerAndFit(windBoxSizer);

        gridSizer->Add(
            windBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
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
                    "Wave Height Adjust",
                    "Adjusts the height of ocean waves wrt their optimal value, which is determined by wind speed.",                    
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
                    "Wave Width Adjust",
                    "Adjusts the width of ocean waves wrt their optimal value, which is determined by wind speed.",                    
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
                    "Wave Speed Adjust",
                    "Adjusts the speed of ocean waves wrt their optimal value, which is determined by wind speed.",                    
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
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
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
                mTsunamiRateSlider = new SliderControl<float>(
                    abnormalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    "Tsunami Rate",
                    "The expected time between two automatically-generated tsunami waves (minutes). Set to zero to disable automatic generation of tsunami waves altogether.",                    
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::TsunamiRate, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinTsunamiRate(),
                        mGameControllerSettingsOptions->GetMaxTsunamiRate()));

                abnormalWavesSizer->Add(
                    mTsunamiRateSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Rogue Wave Rate
            {
                mRogueWaveRateSlider = new SliderControl<float>(
                    abnormalWavesBox,
                    SliderWidth,
                    SliderHeight,
                    "Rogue Wave Rate",
                    "The expected time between two automatically-generated rogue waves (minutes). Set to zero to disable automatic generation of rogue waves altogether.",                    
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::RogueWaveRate, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mGameControllerSettingsOptions->GetMinRogueWaveRate(),
                        mGameControllerSettingsOptions->GetMaxRogueWaveRate()));

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
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }



    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateInteractionsPanel(wxPanel * panel)
{
    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);

    //
    // Tools
    //

    {
        wxStaticBox * toolsBox = new wxStaticBox(panel, wxID_ANY, _("Tools"));

        wxBoxSizer * toolsBoxSizer = new wxBoxSizer(wxVERTICAL);
        toolsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(0, 0);

            // Destroy Radius
            {
                mDestroyRadiusSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    "Destroy Radius",
                    "The starting radius of the damage caused by destructive tools (m).",                    
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
                    "Bomb Blast Radius",
                    "The radius of bomb explosions (m).",                    
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
                    "Bomb Blast Heat",
                    "The heat generated by bomb explosions (KJ/s).",                    
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
                    "AM Bomb Implosion Strength",
                    "Adjusts the strength of the initial anti-matter bomb implosion.",                    
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

            // Flood Radius
            {
                mFloodRadiusSlider = new SliderControl<float>(
                    toolsBox,
                    SliderWidth,
                    SliderHeight,
                    "Flood Radius",
                    "How wide an area is flooded by the flood tool (m).",                    
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
                    wxGBPosition(1, 0),
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
                    "Flood Quantity",
                    "How much water is injected by the flood tool (m3).",                    
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
                    wxGBPosition(1, 1),
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
                    "Repair Radius",
                    "Adjusts the radius of the repair tool (m).",                    
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
                    wxGBPosition(1, 2),
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
                    "Repair Speed Adjust",
                    "Adjusts the speed with which the repair tool attracts particles to repair damage. Warning: at high speeds the repair tool might become destructive!",                    
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
                    wxGBPosition(1, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Checkboxes
            {
                wxBoxSizer* checkboxesSizer = new wxBoxSizer(wxVERTICAL);

                {
                    mUltraViolentCheckBox = new wxCheckBox(toolsBox, wxID_ANY, _("Ultra-Violent Mode"));
                    mUltraViolentCheckBox->SetToolTip("Enables or disables amplification of tool forces and inflicted damages.");
					mUltraViolentCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnUltraViolentCheckBoxClick, this);

                    checkboxesSizer->Add(mUltraViolentCheckBox, 0, wxALIGN_LEFT, 0);

                    checkboxesSizer->AddStretchSpacer(1);
                }

                toolsSizer->Add(
                    checkboxesSizer,
                    wxGBPosition(2, 0),
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
    // Side-Effects
    //

    {
        wxStaticBox * sideEffectsBox = new wxStaticBox(panel, wxID_ANY, _("Side-Effects"));

        wxBoxSizer * sideEffectsBoxSizer = new wxBoxSizer(wxVERTICAL);
        sideEffectsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * sideEffectsSizer = new wxGridBagSizer(0, 0);

            // Air Bubbles
            {
                wxBoxSizer * airBubblesBoxSizer = new wxBoxSizer(wxVERTICAL);

                // Generate Air Bubbles
                {
                    mGenerateAirBubblesCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Air Bubbles"));
                    mGenerateAirBubblesCheckBox->SetToolTip("Enables or disables generation of air bubbles when water enters a physical body.");
					mGenerateAirBubblesCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnGenerateAirBubblesCheckBoxClick, this);

                    airBubblesBoxSizer->Add(mGenerateAirBubblesCheckBox, 0, wxALIGN_LEFT, 0);
                }

                // Air Bubbles Density
                {
                    mAirBubbleDensitySlider = new SliderControl<float>(
                        sideEffectsBox,
                        SliderWidth,
                        -1,
                        "Air Bubbles Density",
                        "The density of air bubbles generated when water enters a ship.",
                        [this](float value)
                        {
                            this->mLiveSettings.SetValue(GameSettings::AirBubblesDensity, value);
                            this->OnLiveSettingsChanged();
                        },
                        std::make_unique<LinearSliderCore>(
                            mGameControllerSettingsOptions->GetMinAirBubblesDensity(),
                            mGameControllerSettingsOptions->GetMaxAirBubblesDensity()));

                    airBubblesBoxSizer->Add(mAirBubbleDensitySlider, 1, wxEXPAND, 0);
                }

                sideEffectsSizer->Add(
                    airBubblesBoxSizer,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Checkboxes
            {
                wxBoxSizer* checkboxesSizer = new wxBoxSizer(wxVERTICAL);

                {
                    mGenerateDebrisCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Debris"));
                    mGenerateDebrisCheckBox->SetToolTip("Enables or disables generation of debris when using destructive tools.");
					mGenerateDebrisCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnGenerateDebrisCheckBoxClick, this);

                    checkboxesSizer->Add(mGenerateDebrisCheckBox, 0, wxALIGN_LEFT, 0);

                    checkboxesSizer->AddSpacer(5);

                    mGenerateSparklesCheckBox = new wxCheckBox(sideEffectsBox, wxID_ANY, _("Generate Sparkles"));
                    mGenerateSparklesCheckBox->SetToolTip("Enables or disables generation of sparkles when using the saw tool on metal.");
					mGenerateSparklesCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnGenerateSparklesCheckBoxClick, this);

                    checkboxesSizer->Add(mGenerateSparklesCheckBox, 0, wxALIGN_LEFT, 0);

                    checkboxesSizer->AddStretchSpacer(1);
                }

                sideEffectsSizer->Add(
                    checkboxesSizer,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            sideEffectsBoxSizer->Add(sideEffectsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        sideEffectsBox->SetSizerAndFit(sideEffectsBoxSizer);

        gridSizer->Add(
            sideEffectsBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);


    //
    // Row 1
    //

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
                    mTextureOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a static pattern.");
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
                    mTextureOceanComboBox->SetToolTip("Sets the texture to use for the ocean.");
                    mTextureOceanComboBox->Bind(wxEVT_COMBOBOX, &SettingsDialog::OnTextureOceanChanged, this);
                    oceanRenderModeBoxSizer2->Add(mTextureOceanComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL | wxEXPAND, 0);

                    //

                    mDepthOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Depth Gradient"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a vertical color gradient.");
                    mDepthOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mDepthOceanColorStartPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorStartPicker->SetToolTip("Sets the starting (top) color of the gradient.");
                    mDepthOceanColorStartPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorStartChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorStartPicker, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 0);

                    mDepthOceanColorEndPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mDepthOceanColorEndPicker->SetToolTip("Sets the ending (bottom) color of the gradient.");
                    mDepthOceanColorEndPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnDepthOceanColorEndChanged, this);
                    oceanRenderModeBoxSizer2->Add(mDepthOceanColorEndPicker, wxGBPosition(1, 2), wxGBSpan(1, 1), wxALL, 0);

                    //

                    mFlatOceanRenderModeRadioButton = new wxRadioButton(oceanRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanRenderModeRadioButton->SetToolTip("Draws the ocean using a single color.");
                    mFlatOceanRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnOceanRenderModeRadioButtonClick, this);
                    oceanRenderModeBoxSizer2->Add(mFlatOceanRenderModeRadioButton, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatOceanColorPicker = new wxColourPickerCtrl(oceanRenderModeBox, wxID_ANY, wxColour("WHITE"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatOceanColorPicker->SetToolTip("Sets the single color of the ocean.");
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
                mOceanTransparencySlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    "Transparency",
                    "Adjusts the transparency of sea water.",
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
                    wxGBSpan(2, 1),
                    wxALL,
                    CellBorder);
            }

            // Ocean Darkening Rate
            {
                mOceanDarkeningRateSlider = new SliderControl<float>(
                    oceanBox,
                    SliderWidth,
                    SliderHeight,
                    "Darkening Rate",
                    "Adjusts the rate at which the ocean darkens with depth.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(GameSettings::OceanDarkeningRate, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                oceanSizer->Add(
                    mOceanDarkeningRateSlider,
                    wxGBPosition(0,2),
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
                    wxGridBagSizer* landRenderModeBoxSizer2 = new wxGridBagSizer(5, 5);

                    mTextureLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Texture"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mTextureLandRenderModeRadioButton->SetToolTip("Draws the ocean floor using a static image.");
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
                    mTextureLandComboBox->SetToolTip("Sets the texture to use for the ocean floor.");
                    mTextureLandComboBox->Bind(wxEVT_COMBOBOX, &SettingsDialog::OnTextureLandChanged, this);
                    landRenderModeBoxSizer2->Add(mTextureLandComboBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxALL, 0);

                    mFlatLandRenderModeRadioButton = new wxRadioButton(landRenderModeBox, wxID_ANY, _("Flat"),
                        wxDefaultPosition, wxDefaultSize);
                    mFlatLandRenderModeRadioButton->SetToolTip("Draws the ocean floor using a static color.");
                    mFlatLandRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnLandRenderModeRadioButtonClick, this);
                    landRenderModeBoxSizer2->Add(mFlatLandRenderModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mFlatLandColorPicker = new wxColourPickerCtrl(landRenderModeBox, wxID_ANY);
                    mFlatLandColorPicker->SetToolTip("Sets the single color of the ocean floor.");
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
            wxGBPosition(1, 2),
            wxGBSpan(1, 1),
            wxALL,
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
                mDrawHeatOverlayCheckBox->SetToolTip("Renders heat over ships.");
                mDrawHeatOverlayCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnDrawHeatOverlayCheckBoxClick, this);

                renderSizer->Add(
                    mDrawHeatOverlayCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Fire Render Mode
            {
                wxStaticBox * fireRenderModeBox = new wxStaticBox(heatBox, wxID_ANY, _("Fire Draw Mode"));

                wxBoxSizer * fireRenderModeBoxSizer1 = new wxBoxSizer(wxVERTICAL);
                fireRenderModeBoxSizer1->AddSpacer(StaticBoxTopMargin);

                {
                    wxFlexGridSizer* fireRenderModeBoxSizer2 = new wxFlexGridSizer(1, 5, 5);
                    fireRenderModeBoxSizer2->SetFlexibleDirection(wxHORIZONTAL);

                    mMode1ShipFlameRenderModeRadioButton = new wxRadioButton(fireRenderModeBox, wxID_ANY, _("Mode 1"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mMode1ShipFlameRenderModeRadioButton->SetToolTip("Changes the way flames are drawn.");
                    mMode1ShipFlameRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnShipFlameRenderModeRadioButtonClick, this);
                    fireRenderModeBoxSizer2->Add(mMode1ShipFlameRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mMode2ShipFlameRenderModeRadioButton = new wxRadioButton(fireRenderModeBox, wxID_ANY, _("Mode 2"),
                        wxDefaultPosition, wxDefaultSize);
                    mMode2ShipFlameRenderModeRadioButton->SetToolTip("Changes the way flames are drawn.");
                    mMode2ShipFlameRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnShipFlameRenderModeRadioButtonClick, this);
                    fireRenderModeBoxSizer2->Add(mMode2ShipFlameRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mNoDrawShipFlameRenderModeRadioButton = new wxRadioButton(fireRenderModeBox, wxID_ANY, _("Not Drawn"),
                        wxDefaultPosition, wxDefaultSize);
                    mNoDrawShipFlameRenderModeRadioButton->SetToolTip("Changes the way flames are drawn.");
                    mNoDrawShipFlameRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnShipFlameRenderModeRadioButtonClick, this);
                    fireRenderModeBoxSizer2->Add(mNoDrawShipFlameRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    fireRenderModeBoxSizer1->Add(fireRenderModeBoxSizer2, 0, wxALL, StaticBoxInsetMargin);
                }

                fireRenderModeBox->SetSizerAndFit(fireRenderModeBoxSizer1);

                renderSizer->Add(
                    fireRenderModeBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Heat blaster flame
            {
                mDrawHeatBlasterFlameCheckBox = new wxCheckBox(heatBox, wxID_ANY,
                    _("Draw HeatBlaster Flame"), wxDefaultPosition, wxDefaultSize);
                mDrawHeatBlasterFlameCheckBox->SetToolTip("Renders flames out of the HeatBlaster tool.");
                mDrawHeatBlasterFlameCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnDrawHeatBlasterFlameCheckBoxClick, this);

                renderSizer->Add(
                    mDrawHeatBlasterFlameCheckBox,
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
                    "Heat Overlay Transparency",
                    "Adjusts the transparency of the heat overlay.",
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

            // Flame size adjustment
            {
                mShipFlameSizeAdjustmentSlider = new SliderControl<float>(
                    heatBox,
                    SliderWidth,
                    SliderHeight,
                    "Flame Size Adjust",
                    "Adjusts the size of flames.",
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
                    wxGBPosition(0, 2),
                    wxGBSpan(3, 1),
                    wxALL,
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
                    mTextureShipRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnShipRenderModeRadioButtonClick, this);
                    shipRenderModeBoxSizer2->Add(mTextureShipRenderModeRadioButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    mStructureShipRenderModeRadioButton = new wxRadioButton(shipRenderModeBox, wxID_ANY, _("Structure"),
                        wxDefaultPosition, wxDefaultSize);
                    mStructureShipRenderModeRadioButton->SetToolTip("Draws the ship using its structure.");
                    mStructureShipRenderModeRadioButton->Bind(wxEVT_RADIOBUTTON, &SettingsDialog::OnShipRenderModeRadioButtonClick, this);
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

            // Water contrast
            {
                mWaterContrastSlider = new SliderControl<float>(
                    waterBox,
                    SliderWidth,
                    SliderHeight,
                    "Water Contrast",
                    "Adjusts the contrast of water inside physical bodies.",
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
                    wxALL,
                    CellBorder);
            }

            {
                // Water Level of Detail

                mWaterLevelOfDetailSlider = new SliderControl<float>(
                    waterBox,
                    SliderWidth,
                    SliderHeight,
                    "Water Level of Detail",
                    "Adjusts how detailed water inside a physical body looks.",
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
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            waterBoxSizer1->Add(waterSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        waterBox->SetSizerAndFit(waterBoxSizer1);

        gridSizer->Add(
            waterBox,
            wxGBPosition(2, 2),
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

    mEffectsVolumeSlider = new SliderControl<float>(
        panel,
        SliderWidth,
        SliderHeight,
        "Effects Volume",
        "Adjusts the volume of sounds generated by the simulation.",
        [this](float value)
        {
            this->mLiveSettings.SetValue(GameSettings::MasterEffectsVolume, value);
            this->OnLiveSettingsChanged();
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mEffectsVolumeSlider, 1, wxALL, SliderBorder);


    // Tools volume

    mToolsVolumeSlider = new SliderControl<float>(
        panel,
        SliderWidth,
        SliderHeight,
        "Tools Volume",
        "Adjusts the volume of sounds generated by interactive tools.",
        [this](float value)
        {
            this->mLiveSettings.SetValue(GameSettings::MasterToolsVolume, value);
            this->OnLiveSettingsChanged();
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mToolsVolumeSlider, 1, wxALL, SliderBorder);


    // Music volume

    mMusicVolumeSlider = new SliderControl<float>(
        panel,
        SliderWidth,
        SliderHeight,
        "Music Volume",
        "Adjusts the volume of music.",
        [this](float value)
        {
            this->mLiveSettings.SetValue(GameSettings::MasterMusicVolume, value);
            this->OnLiveSettingsChanged();
        },
        std::make_unique<LinearSliderCore>(
            0.0f,
            100.0f));

    controlsSizer->Add(mMusicVolumeSlider, 1, wxALL, SliderBorder);


    // Check boxes

    wxStaticBoxSizer* checkboxesSizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    mPlayBreakSoundsCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Break Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Break Sounds Checkbox"));
    mPlayBreakSoundsCheckBox->SetToolTip("Enables or disables the generation of sounds when materials break.");
	mPlayBreakSoundsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnPlayBreakSoundsCheckBoxClick, this);
    checkboxesSizer->Add(mPlayBreakSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlayStressSoundsCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Stress Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Stress Sounds Checkbox"));
    mPlayStressSoundsCheckBox->SetToolTip("Enables or disables the generation of sounds when materials are under stress.");
	mPlayStressSoundsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnPlayStressSoundsCheckBoxClick, this);
    checkboxesSizer->Add(mPlayStressSoundsCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlayWindSoundCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Wind Sounds"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Wind Sound Checkbox"));
    mPlayWindSoundCheckBox->SetToolTip("Enables or disables the generation of wind sounds.");
	mPlayWindSoundCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnPlayWindSoundCheckBoxClick, this);
    checkboxesSizer->Add(mPlayWindSoundCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    mPlaySinkingMusicCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Farewell Music"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("Play Sinking Music Checkbox"));
    mPlaySinkingMusicCheckBox->SetToolTip("Enables or disables playing \"Nearer My God to Thee\" when a ship starts sinking.");
	mPlaySinkingMusicCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnPlaySinkingMusicCheckBoxClick, this);
    checkboxesSizer->Add(mPlaySinkingMusicCheckBox, 0, wxALL | wxALIGN_LEFT, 5);

    controlsSizer->Add(checkboxesSizer, 0, wxALL, SliderBorder);


    // Finalize panel

    panel->SetSizerAndFit(controlsSizer);
}

void SettingsDialog::PopulateAdvancedPanel(wxPanel * panel)
{
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

    // Spring Stiffness

    mSpringStiffnessSlider = new SliderControl<float>(
        panel,
        SliderWidth,
        SliderHeight,
        "Spring Stiffness Adjust",
        "This setting is for testing physical instability of the mass-spring network with high stiffness values;"
        " it is not meant for improving the rigidity of physical bodies.",
        [this](float value)
        {
            this->mLiveSettings.SetValue(GameSettings::SpringStiffnessAdjustment, value);
            this->OnLiveSettingsChanged();
        },
        std::make_unique<LinearSliderCore>(
            mGameControllerSettingsOptions->GetMinSpringStiffnessAdjustment(),
            mGameControllerSettingsOptions->GetMaxSpringStiffnessAdjustment()),
        mWarningIcon.get());

    controlsSizer->Add(mSpringStiffnessSlider, 1, wxALL, SliderBorder);

    // Spring Damping

    mSpringDampingSlider = new SliderControl<float>(
        panel,
        SliderWidth,
        SliderHeight,
        "Spring Damping Adjust",
        "This setting is for testing physical instability of the mass-spring network with different damping values;"
        " it is not meant for improving the rigidity of physical bodies.",
        [this](float value)
        {
            this->mLiveSettings.SetValue(GameSettings::SpringDampingAdjustment, value);
            this->OnLiveSettingsChanged();
        },
        std::make_unique<LinearSliderCore>(
            mGameControllerSettingsOptions->GetMinSpringDampingAdjustment(),
            mGameControllerSettingsOptions->GetMaxSpringDampingAdjustment()),
            mWarningIcon.get());

    controlsSizer->Add(mSpringDampingSlider, 1, wxALL, SliderBorder);

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
					"",
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
					auto label = new wxStaticText(loadSettingsBox, wxID_ANY, "Description:");

					col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
				}

				{
					mPersistedSettingsDescriptionTextCtrl = new wxTextCtrl(
						loadSettingsBox,						
						wxID_ANY,
						_(""),
						wxDefaultPosition,
						wxSize(250, 120),
						wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

					col2BoxSizer->Add(mPersistedSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
				}

				{
					mLoadAndApplyPersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, "Load and Apply");
					mLoadAndApplyPersistedSettingsButton->SetToolTip("Loads the selected settings and applies them to the game.");
					mLoadAndApplyPersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnLoadAndApplyPersistedSettingsButton, this);

					col2BoxSizer->Add(mLoadAndApplyPersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

					mReplacePersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, "Replace with Current");
					mReplacePersistedSettingsButton->SetToolTip("Overwrites the selected settings with the current settings.");
					mReplacePersistedSettingsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnReplacePersistedSettingsButton, this);

					col2BoxSizer->Add(mReplacePersistedSettingsButton, 0, wxALL | wxEXPAND, 5);

					mDeletePersistedSettingsButton = new wxButton(loadSettingsBox, wxID_ANY, "Delete Settings");
					mDeletePersistedSettingsButton->SetToolTip("Deletes the selected settings.");
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
					auto label = new wxStaticText(saveSettingsBox, wxID_ANY, "Name:");

					col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
				}

				{
					wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);					
					validator.SetCharIncludes(
						"abcdefghijklmnopqrstuvwxyz"
						"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						"0123456789"
						" "
						"_-");
					validator.SuppressBellOnError();

					mSaveSettingsNameTextCtrl = new wxTextCtrl(
						saveSettingsBox,
						wxID_ANY,
						_(""),
						wxDefaultPosition,
						wxDefaultSize,
						0,
						validator);

					mSaveSettingsNameTextCtrl->Bind(wxEVT_TEXT, &SettingsDialog::OnSaveSettingsTextEdited, this);

					col2BoxSizer->Add(mSaveSettingsNameTextCtrl, 0, wxALL | wxEXPAND, 5);
				}

				{
					auto label = new wxStaticText(saveSettingsBox, wxID_ANY, "Description:");

					col2BoxSizer->Add(label, 0, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 5);
				}

				{
					mSaveSettingsDescriptionTextCtrl = new wxTextCtrl(
						saveSettingsBox,
						wxID_ANY,
						_(""),
						wxDefaultPosition,
						wxSize(250, 120),
						wxTE_MULTILINE | wxTE_WORDWRAP);

					mSaveSettingsDescriptionTextCtrl->Bind(wxEVT_TEXT, &SettingsDialog::OnSaveSettingsTextEdited, this);

					col2BoxSizer->Add(mSaveSettingsDescriptionTextCtrl, 0, wxALL | wxEXPAND, 5);
				}

				{
					mSaveSettingsButton = new wxButton(saveSettingsBox, wxID_ANY, "Save Current Settings");
					mSaveSettingsButton->SetToolTip("Saves the current settings using the specified name.");
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

void SettingsDialog::SyncSettingsWithControls(Settings<GameSettings> const & settings)
{
    // Mechanics, Fluids, Lights

    mMechanicalQualitySlider->SetValue(settings.GetValue<float>(GameSettings::NumMechanicalDynamicsIterationsAdjustment));

    mStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStrengthAdjustment));

    mRotAcceler8rSlider->SetValue(settings.GetValue<float>(GameSettings::RotAcceler8r));

    mWaterDensitySlider->SetValue(settings.GetValue<float>(GameSettings::WaterDensityAdjustment));

    mWaterDragSlider->SetValue(settings.GetValue<float>(GameSettings::WaterDragAdjustment));

    mWaterIntakeSlider->SetValue(settings.GetValue<float>(GameSettings::WaterIntakeAdjustment));

    mWaterCrazynessSlider->SetValue(settings.GetValue<float>(GameSettings::WaterCrazyness));

    mWaterDiffusionSpeedSlider->SetValue(settings.GetValue<float>(GameSettings::WaterDiffusionSpeedAdjustment));

    mLuminiscenceSlider->SetValue(settings.GetValue<float>(GameSettings::LuminiscenceAdjustment));

    mLightSpreadSlider->SetValue(settings.GetValue<float>(GameSettings::LightSpreadAdjustment));

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

    // Ocean and Sky

    mOceanDepthSlider->SetValue(settings.GetValue<float>(GameSettings::SeaDepth));

    mOceanFloorBumpinessSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorBumpiness));

    mOceanFloorDetailAmplificationSlider->SetValue(settings.GetValue<float>(GameSettings::OceanFloorDetailAmplification));

    mNumberOfStarsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfStars));

    mNumberOfCloudsSlider->SetValue(settings.GetValue<unsigned int>(GameSettings::NumberOfClouds));

    mWindSpeedBaseSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedBase));

    mModulateWindCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoModulateWind));

    mWindGustAmplitudeSlider->SetValue(settings.GetValue<float>(GameSettings::WindSpeedMaxFactor));
    mWindGustAmplitudeSlider->Enable(settings.GetValue<bool>(GameSettings::DoModulateWind));

    // Waves

    mBasalWaveHeightAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveHeightAdjustment));

    mBasalWaveLengthAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveLengthAdjustment));

    mBasalWaveSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::BasalWaveSpeedAdjustment));

    mTsunamiRateSlider->SetValue(settings.GetValue<float>(GameSettings::TsunamiRate));

    mRogueWaveRateSlider->SetValue(settings.GetValue<float>(GameSettings::RogueWaveRate));

    // Interactions

    mDestroyRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::DestroyRadius));

    mBombBlastRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastRadius));

    mBombBlastHeatSlider->SetValue(settings.GetValue<float>(GameSettings::BombBlastHeat));

    mAntiMatterBombImplosionStrengthSlider->SetValue(settings.GetValue<float>(GameSettings::AntiMatterBombImplosionStrength));

    mFloodRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::FloodRadius));

    mFloodQuantitySlider->SetValue(settings.GetValue<float>(GameSettings::FloodQuantity));

    mRepairRadiusSlider->SetValue(settings.GetValue<float>(GameSettings::RepairRadius));

    mRepairSpeedAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::RepairSpeedAdjustment));

    mUltraViolentCheckBox->SetValue(settings.GetValue<bool>(GameSettings::UltraViolentMode));

    mGenerateDebrisCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateDebris));

    mGenerateSparklesCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateSparkles));

    mGenerateAirBubblesCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DoGenerateAirBubbles));

    mAirBubbleDensitySlider->SetValue(settings.GetValue<float>(GameSettings::AirBubblesDensity));
    mAirBubbleDensitySlider->Enable(settings.GetValue<bool>(GameSettings::DoGenerateAirBubbles));

    // Render

    auto oceanRenderMode = settings.GetValue<OceanRenderMode>(GameSettings::OceanRenderMode);
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

    mTextureOceanComboBox->Select(static_cast<int>(settings.GetValue<size_t>(GameSettings::TextureOceanTextureIndex)));

    auto depthOceanColorStart = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorStart);
    mDepthOceanColorStartPicker->SetColour(wxColor(depthOceanColorStart.r, depthOceanColorStart.g, depthOceanColorStart.b));

    auto depthOceanColorEnd = settings.GetValue<rgbColor>(GameSettings::DepthOceanColorEnd);
    mDepthOceanColorEndPicker->SetColour(wxColor(depthOceanColorEnd.r, depthOceanColorEnd.g, depthOceanColorEnd.b));

    auto flatOceanColor = settings.GetValue<rgbColor>(GameSettings::FlatOceanColor);
    mFlatOceanColorPicker->SetColour(wxColor(flatOceanColor.r, flatOceanColor.g, flatOceanColor.b));

    ReconciliateOceanRenderModeSettings();

    mSeeShipThroughOceanCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipThroughOcean));

    mOceanTransparencySlider->SetValue(settings.GetValue<float>(GameSettings::OceanTransparency));

    mOceanDarkeningRateSlider->SetValue(settings.GetValue<float>(GameSettings::OceanDarkeningRate));

    auto landRenderMode = settings.GetValue<LandRenderMode>(GameSettings::LandRenderMode);
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

    mTextureLandComboBox->Select(static_cast<int>(settings.GetValue<size_t>(GameSettings::TextureLandTextureIndex)));

    auto flatLandColor = settings.GetValue<rgbColor>(GameSettings::FlatLandColor);
    mFlatLandColorPicker->SetColour(wxColor(flatLandColor.r, flatLandColor.g, flatLandColor.b));

    ReconciliateLandRenderModeSettings();

    auto flatSkyColor = settings.GetValue<rgbColor>(GameSettings::FlatSkyColor);
    mFlatSkyColorPicker->SetColour(wxColor(flatSkyColor.r, flatSkyColor.g, flatSkyColor.b));

    auto shipRenderMode = settings.GetValue<ShipRenderMode>(GameSettings::ShipRenderMode);
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

    mShowStressCheckBox->SetValue(settings.GetValue<bool>(GameSettings::ShowShipStress));

    mWaterContrastSlider->SetValue(settings.GetValue<float>(GameSettings::WaterContrast));

    mWaterLevelOfDetailSlider->SetValue(settings.GetValue<float>(GameSettings::WaterLevelOfDetail));

    mDrawHeatOverlayCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawHeatOverlay));

    mHeatOverlayTransparencySlider->SetValue(settings.GetValue<float>(GameSettings::HeatOverlayTransparency));

    auto shipFlameRenderMode = settings.GetValue<ShipFlameRenderMode>(GameSettings::ShipFlameRenderMode);
    switch (shipFlameRenderMode)
    {
        case ShipFlameRenderMode::Mode1:
        {
            mMode1ShipFlameRenderModeRadioButton->SetValue(true);
            break;
        }

        case ShipFlameRenderMode::Mode2:
        {
            mMode2ShipFlameRenderModeRadioButton->SetValue(true);
            break;
        }

        case ShipFlameRenderMode::NoDraw:
        {
            mNoDrawShipFlameRenderModeRadioButton->SetValue(true);
            break;
        }
    }

    mDrawHeatBlasterFlameCheckBox->SetValue(settings.GetValue<bool>(GameSettings::DrawHeatBlasterFlame));

    mShipFlameSizeAdjustmentSlider->SetValue(settings.GetValue<float>(GameSettings::ShipFlameSizeAdjustment));

    // Sound

    mEffectsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterEffectsVolume));

    mToolsVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterToolsVolume));

    mMusicVolumeSlider->SetValue(settings.GetValue<float>(GameSettings::MasterMusicVolume));

    mPlayBreakSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayBreakSounds));

    mPlayStressSoundsCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayStressSounds));

    mPlayWindSoundCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlayWindSound));

    mPlaySinkingMusicCheckBox->SetValue(settings.GetValue<bool>(GameSettings::PlaySinkingMusic));

    // Advanced

    mSpringStiffnessSlider->SetValue(settings.GetValue<float>(GameSettings::SpringStiffnessAdjustment));

    mSpringDampingSlider->SetValue(settings.GetValue<float>(GameSettings::SpringDampingAdjustment));

    auto debugShipRenderMode = settings.GetValue<DebugShipRenderMode>(GameSettings::DebugShipRenderMode);
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

    auto vectorFieldRenderMode = settings.GetValue<VectorFieldRenderMode>(GameSettings::VectorFieldRenderMode);
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
		|| psKey == PersistedSettingsKey::MakeLastPlayedSettingsKey())
	{
		// Make it bold
		auto font = mPersistedSettingsListCtrl->GetItemFont(index);
		font.SetWeight(wxFONTWEIGHT_BOLD);
		mPersistedSettingsListCtrl->SetItemFont(index, font);
	}
}

void SettingsDialog::LoadPersistedSettings(int index)
{
	assert(index < mPersistedSettings.size());

	if (index < mPersistedSettings.size())
	{
		// Load
		//
		// Only loaded settings will be marked as dirty on output
		mLiveSettings = mSettingsManager->LoadPersistedSettings(mPersistedSettings[index].Key);

		// Enforce and reconcile
		OnLiveSettingsChanged();
	}
}

void SettingsDialog::ReconciliateLoadPersistedSettings()
{
	auto selectedIndex = GetSelectedPersistedSettingIndexFromCtrl();

	assert(selectedIndex == wxNOT_FOUND || selectedIndex < mPersistedSettings.size());

	// Enable as long as there's a selection
	mLoadAndApplyPersistedSettingsButton->Enable(selectedIndex != wxNOT_FOUND);

	// Enable as long as there's a selection for a user setting that's not the "last-played" setting
	mReplacePersistedSettingsButton->Enable(
		selectedIndex != wxNOT_FOUND
		&& mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User
		&& mPersistedSettings[selectedIndex].Key != PersistedSettingsKey::MakeLastPlayedSettingsKey());

	// Enable as long as there's a selection for a user setting that's not the "last-played" setting
	mDeletePersistedSettingsButton->Enable(
		selectedIndex != wxNOT_FOUND
		&& mPersistedSettings[selectedIndex].Key.StorageType == PersistedSettingsStorageTypes::User
		&& mPersistedSettings[selectedIndex].Key != PersistedSettingsKey::MakeLastPlayedSettingsKey());

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
	wxMessageBox(errorMessage, wxT("Error"), wxICON_ERROR);
}