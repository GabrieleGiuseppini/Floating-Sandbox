/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "PreferencesDialog.h"

#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/FixedSetSliderCore.h>
#include <GameCore/IntegralLinearSliderCore.h>
#include <GameCore/LinearSliderCore.h>

#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/stattext.h>

#include <algorithm>

static int constexpr Border = 10;

static int constexpr StaticBoxTopMargin = 7;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr CellBorderInner = 8;
static int constexpr CellBorderOuter = 4;

static int constexpr SliderWidth = 82; // Min
static int constexpr SliderHeight = 140;

static int constexpr MaxZoomIncrementPosition = 200;
static int constexpr MaxPanIncrementPosition = 200;
static float constexpr CameraSpeedAdjustmentSpinFactor = 100.0f;

PreferencesDialog::PreferencesDialog(
    wxWindow* parent,
    UIPreferencesManager & uiPreferencesManager,
    std::function<void()> onChangeCallback,
    ResourceLocator const & resourceLocator)
    : mParent(parent)
    , mUIPreferencesManager(uiPreferencesManager)
    , mOnChangeCallback(std::move(onChangeCallback))
    , mAvailableLanguages(mUIPreferencesManager.GetAvailableLanguages())
    , mHasWarnedAboutLanguageSettingChanges(false)
{
    Create(
        mParent,
        wxID_ANY,
        _("Game Preferences"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED,
        wxS("Preferences Window"));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

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

    {
        wxNotebook * notebook = new wxNotebook(
            this,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxNB_TOP | wxNB_NOPAGETHEME);

        //
        // Game Preferences
        //

        wxPanel * gamePanel = new wxPanel(notebook);

        PopulateGamePanel(gamePanel);

        notebook->AddPage(gamePanel, _("Game"));


        //
        // Ship Preferences
        //

        wxPanel * shipsPanel = new wxPanel(notebook);

        PopulateShipPanel(shipsPanel);

        notebook->AddPage(shipsPanel, _("Ships"));


        //
        // NPC Preferences
        //

        wxPanel * npcsPanel = new wxPanel(notebook);

        PopulateNpcPanel(npcsPanel);

        notebook->AddPage(npcsPanel, _("NPCs"));


        //
        // Global Sound and Music
        //

        wxPanel * musicPanel = new wxPanel(notebook);

        PopulateMusicPanel(musicPanel);

        notebook->AddPage(musicPanel, _("Global Sound and Music"));

        dialogVSizer->Add(notebook, 0);
        dialogVSizer->Fit(notebook);
    }

    dialogVSizer->AddSpacer(20);

    // Buttons

    wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonsSizer->AddSpacer(20);

    mOkButton = new wxButton(this, wxID_ANY, _("Done"));
    mOkButton->Bind(wxEVT_BUTTON, &PreferencesDialog::OnOkButton, this);
    buttonsSizer->Add(mOkButton, 0);

    buttonsSizer->AddSpacer(20);

    dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);

    dialogVSizer->AddSpacer(20);



    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::Open()
{
    ReadSettings();

    mHasWarnedAboutLanguageSettingChanges = false;

    this->Show();
}

void PreferencesDialog::OnScreenshotDirPickerChanged(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetScreenshotsFolderPath(mScreenshotDirPickerCtrl->GetPath().ToStdString());

    mOnChangeCallback();
}

void PreferencesDialog::OnStartInFullScreenCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetStartInFullScreen(mStartInFullScreenCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetShowStartupTip(mShowTipOnStartupCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnCheckForUpdatesAtStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetCheckUpdatesAtStartup(mCheckForUpdatesAtStartupCheckBox->GetValue());

    if (mCheckForUpdatesAtStartupCheckBox->GetValue())
    {
        mUIPreferencesManager.ResetUpdateBlacklist();
    }

    mOnChangeCallback();
}

void PreferencesDialog::OnSaveSettingsOnExitCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetSaveSettingsOnExit(mSaveSettingsOnExitCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetDoShowTsunamiNotifications(mShowTsunamiNotificationsCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnZoomIncrementSpinCtrl(wxSpinEvent & event)
{
    mUIPreferencesManager.SetZoomIncrement(ZoomIncrementSpinToZoomIncrement(event.GetPosition()));

    mOnChangeCallback();
}

void PreferencesDialog::OnPanIncrementSpinCtrl(wxSpinEvent & event)
{
    mUIPreferencesManager.SetPanIncrement(PanIncrementSpinToPanIncrement(event.GetPosition()));

    mOnChangeCallback();
}

void PreferencesDialog::OnCameraSpeedAdjustmentSpinCtrl(wxSpinEvent & event)
{
    mUIPreferencesManager.SetCameraSpeedAdjustment(CameraSpeedAdjustmentSpinToCameraSpeedAdjustment(event.GetPosition()));

    mOnChangeCallback();
}

void PreferencesDialog::OnShowStatusTextCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetShowStatusText(mShowStatusTextCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowExtendedStatusTextCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetShowExtendedStatusText(mShowExtendedStatusTextCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnLanguagesListBoxSelected(wxCommandEvent & /*event*/)
{
    if (mLanguagesListBox->GetSelection() != wxNOT_FOUND)
    {
        size_t languageIndex = static_cast<size_t>(mLanguagesListBox->GetSelection());

        std::optional<std::string> desiredLanguageIdentifier;
        if (languageIndex == 0)
        {
            // Default
            desiredLanguageIdentifier = std::nullopt;
        }
        else
        {
            --languageIndex;

            assert(languageIndex < mAvailableLanguages.size());
            desiredLanguageIdentifier = mAvailableLanguages[languageIndex].Identifier;
        }

        if (desiredLanguageIdentifier != mUIPreferencesManager.GetDesiredLanguage()
            && !mHasWarnedAboutLanguageSettingChanges)
        {
            wxMessageBox(
                _("Please note that a restart is required for language changes to take effect."),
                _("Restart Required"),
                wxOK | wxICON_INFORMATION | wxCENTRE);

            mHasWarnedAboutLanguageSettingChanges = true;
        }

        mUIPreferencesManager.SetDesiredLanguage(desiredLanguageIdentifier);
    }

    mOnChangeCallback();
}

void PreferencesDialog::OnReloadLastLoadedShipOnStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetReloadLastLoadedShipOnStartup(mReloadLastLoadedShipOnStartupCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetShowShipDescriptionsAtShipLoad(mShowShipDescriptionAtShipLoadCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnContinuousAutoFocusOnShipCheckBoxClicked(wxCommandEvent & /*event*/)
{
    bool const value = mContinuousAutoFocusOnShipCheckBox->GetValue();
    mUIPreferencesManager.SetAutoFocusTarget(value ? std::optional<AutoFocusTargetKindType>(AutoFocusTargetKindType::Ship) : std::nullopt);

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoFocusOnShipLoadCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetDoAutoFocusOnShipLoad(mAutoFocusOnShipLoadCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoShowSwitchboardCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetAutoShowSwitchboard(mAutoShowSwitchboardCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowElectricalNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetDoShowElectricalNotifications(mShowElectricalNotificationsCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoTexturizationModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    if (mFlatStructureAutoTexturizationModeRadioButton->GetValue())
    {
        mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().Mode = ShipAutoTexturizationModeType::FlatStructure;
    }
    else
    {
        assert(mMaterialTexturesAutoTexturizationModeRadioButton->GetValue());
        mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().Mode = ShipAutoTexturizationModeType::MaterialTextures;
    }

    ReconciliateShipAutoTexturizationModeSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnForceSharedAutoTexturizationSettingsOntoShipCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetShipAutoTexturizationForceSharedSettingsOntoShipDefinition(mForceSharedAutoTexturizationSettingsOntoShipCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoFocusOnNpcPlacementCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetDoAutoFocusOnNpcPlacement(mAutoFocusOnNpcPlacementCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowNpcNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetDoShowNpcNotifications(mShowNpcNotificationsCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnGlobalMuteCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetGlobalMute(mGlobalMuteCheckBox->GetValue());

    ReconcileSoundSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnPlayBackgroundMusicCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetPlayBackgroundMusic(mPlayBackgroundMusicCheckBox->GetValue());

    ReconcileSoundSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnPlaySinkingMusicCheckBoxClicked(wxCommandEvent & /*event*/)
{
    mUIPreferencesManager.SetPlaySinkingMusic(mPlaySinkingMusicCheckBox->GetValue());

    ReconcileSoundSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // Close ourselves
    this->Close();
}

void PreferencesDialog::PopulateGamePanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // User interface
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("User Interface"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            sizer->SetFlexibleDirection(wxHORIZONTAL);
            sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);
            int constexpr UserInterfaceBorder = 3;

            // | 0 | 1 | 2 | 3 |
            // | X |   | X   X |

            //
            // Row 1
            //

            {
                mStartInFullScreenCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Start in Full Screen"), wxDefaultPosition, wxDefaultSize, 0);
                mStartInFullScreenCheckBox->SetToolTip(_("Selects whether the game starts in full-screen mode or as a normal window."));
                mStartInFullScreenCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnStartInFullScreenCheckBoxClicked, this);

                sizer->Add(
                    mStartInFullScreenCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM,
                    UserInterfaceBorder);
            }

            {
                mZoomIncrementSpinCtrl = new wxSpinCtrl(boxSizer->GetStaticBox(), wxID_ANY, _("Zoom Increment"), wxDefaultPosition, wxSize(75, -1),
                    wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
                mZoomIncrementSpinCtrl->SetRange(1, MaxZoomIncrementPosition);
                mZoomIncrementSpinCtrl->SetToolTip(_("Changes the amount by which zoom changes when using the zoom controls."));
                mZoomIncrementSpinCtrl->Bind(wxEVT_SPINCTRL, &PreferencesDialog::OnZoomIncrementSpinCtrl, this);

                sizer->Add(
                    mZoomIncrementSpinCtrl,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            {
                auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Zoom Increment"), wxDefaultPosition, wxDefaultSize,
                    wxALIGN_LEFT);

                sizer->Add(
                    label,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 2
            //

            {
                mShowTipOnStartupCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Tips on Startup"), wxDefaultPosition, wxDefaultSize, 0);
                mShowTipOnStartupCheckBox->SetToolTip(_("Enables or disables the tips shown when the game starts."));
                mShowTipOnStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTipOnStartupCheckBoxClicked, this);

                sizer->Add(
                    mShowTipOnStartupCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM,
                    UserInterfaceBorder);
            }

            {
                mPanIncrementSpinCtrl = new wxSpinCtrl(boxSizer->GetStaticBox(), wxID_ANY, _("Pan Increment"), wxDefaultPosition, wxSize(75, -1),
                    wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
                mPanIncrementSpinCtrl->SetRange(1, MaxPanIncrementPosition);
                mPanIncrementSpinCtrl->SetToolTip(_("Changes the amount by which the camera position changes when using the pan controls."));
                mPanIncrementSpinCtrl->Bind(wxEVT_SPINCTRL, &PreferencesDialog::OnPanIncrementSpinCtrl, this);

                sizer->Add(
                    mPanIncrementSpinCtrl,
                    wxGBPosition(1, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            {
                auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Pan Increment"), wxDefaultPosition, wxDefaultSize,
                    wxALIGN_LEFT);

                sizer->Add(
                    label,
                    wxGBPosition(1, 3),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 3
            //

            {
                mCheckForUpdatesAtStartupCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Check for Updates on Startup"), wxDefaultPosition, wxDefaultSize, 0);
                mCheckForUpdatesAtStartupCheckBox->SetToolTip(_("Enables or disables checking for new versions when the game starts."));
                mCheckForUpdatesAtStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnCheckForUpdatesAtStartupCheckBoxClicked, this);

                sizer->Add(
                    mCheckForUpdatesAtStartupCheckBox,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM,
                    UserInterfaceBorder);
            }

            {
                mCameraSpeedAdjustmentSpinCtrl = new wxSpinCtrl(boxSizer->GetStaticBox(), wxID_ANY, _("Camera Speed"), wxDefaultPosition, wxSize(75, -1),
                    wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
                mCameraSpeedAdjustmentSpinCtrl->SetRange(
                    CameraSpeedAdjustmentToCameraSpeedAdjustmentSpin(mUIPreferencesManager.GetMinCameraSpeedAdjustment()),
                    CameraSpeedAdjustmentToCameraSpeedAdjustmentSpin(mUIPreferencesManager.GetMaxCameraSpeedAdjustment()));
                mCameraSpeedAdjustmentSpinCtrl->SetToolTip(_("Adjusts the speed of the camera movements."));
                mCameraSpeedAdjustmentSpinCtrl->Bind(wxEVT_SPINCTRL, &PreferencesDialog::OnCameraSpeedAdjustmentSpinCtrl, this);

                sizer->Add(
                    mCameraSpeedAdjustmentSpinCtrl,
                    wxGBPosition(2, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            {
                auto label = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Camera Speed"), wxDefaultPosition, wxDefaultSize,
                    wxALIGN_LEFT);

                sizer->Add(
                    label,
                    wxGBPosition(2, 3),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 4
            //

            {
                mSaveSettingsOnExitCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Save Settings on Exit"), wxDefaultPosition, wxDefaultSize, 0);
                mSaveSettingsOnExitCheckBox->SetToolTip(_("Enables or disables saving the last-modified settings when exiting the game."));
                mSaveSettingsOnExitCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnSaveSettingsOnExitCheckBoxClicked, this);

                sizer->Add(
                    mSaveSettingsOnExitCheckBox,
                    wxGBPosition(3, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM,
                    UserInterfaceBorder);
            }

            {
                mShowStatusTextCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Status Text"), wxDefaultPosition, wxDefaultSize, 0);
                mShowStatusTextCheckBox->SetToolTip(_("Enables or disables the display of game performance information, such as frame rate and time elapsed."));
                mShowStatusTextCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowStatusTextCheckBoxClicked, this);

                sizer->Add(
                    mShowStatusTextCheckBox,
                    wxGBPosition(3, 2),
                    wxGBSpan(1, 2),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 5
            //

            {
                mShowTsunamiNotificationsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Tsunami Notifications"), wxDefaultPosition, wxDefaultSize, 0);
                mShowTsunamiNotificationsCheckBox->SetToolTip(_("Enables or disables notifications when a tsunami is being spawned."));
                mShowTsunamiNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked, this);

                sizer->Add(
                    mShowTsunamiNotificationsCheckBox,
                    wxGBPosition(4, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM,
                    UserInterfaceBorder);
            }

            {
                mShowExtendedStatusTextCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Extended Status Text"), wxDefaultPosition, wxDefaultSize, 0);
                mShowExtendedStatusTextCheckBox->SetToolTip(_("Enables or disables the display of extended game performance information, such as update/render ratio and counts of primitives being rendered."));
                mShowExtendedStatusTextCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowExtendedStatusTextCheckBoxClicked, this);

                sizer->Add(
                    mShowExtendedStatusTextCheckBox,
                    wxGBPosition(4, 2),
                    wxGBSpan(1, 2),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 6
            //

            {
                wxStaticText * displayUnitsSystemStaticText = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Units system:"));

                sizer->Add(
                    displayUnitsSystemStaticText,
                    wxGBPosition(5, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 7
            //

            {
                wxString choices[3] = {
                    _("SI (Kelvin)"),
                    _("SI (Celsius)"),
                    _("USCS")
                };

                mDisplayUnitsSettingsComboBox = new wxComboBox(
                    boxSizer->GetStaticBox(),
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxDefaultSize,
                    3,
                    choices,
                    wxCB_DROPDOWN | wxCB_READONLY);

                mDisplayUnitsSettingsComboBox->SetToolTip(_("Sets the units system to use when displaying physical quantities."));
                mDisplayUnitsSettingsComboBox->Bind(
                    wxEVT_COMBOBOX,
                    [this](wxCommandEvent &)
                    {
                        switch (mDisplayUnitsSettingsComboBox->GetSelection())
                        {
                            case 0:
                            {
                                mUIPreferencesManager.SetDisplayUnitsSystem(UnitsSystem::SI_Kelvin);
                                break;
                            }

                            case 1:
                            {
                                mUIPreferencesManager.SetDisplayUnitsSystem(UnitsSystem::SI_Celsius);
                                break;
                            }

                            case 2:
                            {
                                mUIPreferencesManager.SetDisplayUnitsSystem(UnitsSystem::USCS);
                                break;
                            }

                            case wxNOT_FOUND:
                            {
                                // Nop
                                break;
                            }

                            default:
                            {
                                assert(false);
                            }
                        }

                        mOnChangeCallback();
                    });

                sizer->Add(
                    mDisplayUnitsSettingsComboBox,
                    wxGBPosition(6, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxLEFT | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 8
            //

            {
                wxStaticText * screenshotDirStaticText = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, _("Screenshot directory:"));

                sizer->Add(
                    screenshotDirStaticText,
                    wxGBPosition(7, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    UserInterfaceBorder);
            }

            //
            // Row 9
            //

            {
                mScreenshotDirPickerCtrl = new wxDirPickerCtrl(
                    boxSizer->GetStaticBox(),
                    wxID_ANY,
                    wxEmptyString,
                    _("Select directory that screenshots will be saved to:"),
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxDIRP_DIR_MUST_EXIST | wxDIRP_USE_TEXTCTRL);
                mScreenshotDirPickerCtrl->SetToolTip(_("Sets the directory into which in-game screenshots are automatically saved."));
                mScreenshotDirPickerCtrl->Bind(wxEVT_DIRPICKER_CHANGED, &PreferencesDialog::OnScreenshotDirPickerChanged, this);

                sizer->Add(
                    mScreenshotDirPickerCtrl,
                    wxGBPosition(8, 0),
                    wxGBSpan(1, 4),
                    wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,
                    UserInterfaceBorder);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    //
    // Language
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Language"));

        // Language list
        {
            mLanguagesListBox = new wxListBox(
                boxSizer->GetStaticBox(),
                wxID_ANY,
                wxDefaultPosition,
                wxDefaultSize,
                0,
                NULL,
                wxLB_SINGLE | wxLB_NEEDED_SB);

            mLanguagesListBox->Append(_("Default Language (from system)"));

            for (size_t l = 0; l < mAvailableLanguages.size(); ++l)
            {
                mLanguagesListBox->Append(mAvailableLanguages[l].Name);
            }

            mLanguagesListBox->Bind(wxEVT_LISTBOX, &PreferencesDialog::OnLanguagesListBoxSelected, this);

            boxSizer->Add(
                mLanguagesListBox,
                1,
                wxALL,
                Border);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorderOuter);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizerAndFit(gridSizer);
}

void PreferencesDialog::PopulateShipPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Auto-Texturization
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Auto-Texturization"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Texturization Mode
            {
                wxStaticBoxSizer * texturizationModeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, boxSizer->GetStaticBox(), _("Mode"));

                {
                    wxGridBagSizer * texturizationModeSizer = new wxGridBagSizer(5, 3);

                    mFlatStructureAutoTexturizationModeRadioButton = new wxRadioButton(texturizationModeBoxSizer->GetStaticBox(), wxID_ANY,
                        _("Flat Structure"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mFlatStructureAutoTexturizationModeRadioButton->SetToolTip(_("When a ship does not have a high-definition image, generates one using the materials' matte colors. Changes to this setting will only be visible after the next ship is loaded."));
                    mFlatStructureAutoTexturizationModeRadioButton->Bind(wxEVT_RADIOBUTTON, &PreferencesDialog::OnAutoTexturizationModeRadioButtonClick, this);
                    texturizationModeSizer->Add(mFlatStructureAutoTexturizationModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    //

                    mMaterialTexturesAutoTexturizationModeRadioButton = new wxRadioButton(texturizationModeBoxSizer->GetStaticBox(), wxID_ANY,
                        _("Material Textures"), wxDefaultPosition, wxDefaultSize);
                    mMaterialTexturesAutoTexturizationModeRadioButton->SetToolTip(_("When a ship does not have a high-definition image, generates one using material-specific textures. Changes to this setting will only be visible after the next ship is loaded."));
                    mMaterialTexturesAutoTexturizationModeRadioButton->Bind(wxEVT_RADIOBUTTON, &PreferencesDialog::OnAutoTexturizationModeRadioButtonClick, this);
                    texturizationModeSizer->Add(mMaterialTexturesAutoTexturizationModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    texturizationModeBoxSizer->Add(texturizationModeSizer, 1, wxALL, StaticBoxInsetMargin);
                }

                sizer->Add(
                    texturizationModeBoxSizer,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorderInner);
            }

            // Force shared settings onto ship
            {
                mForceSharedAutoTexturizationSettingsOntoShipCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Force Defaults onto Ships"), wxDefaultPosition, wxDefaultSize, 0);
                mForceSharedAutoTexturizationSettingsOntoShipCheckBox->SetToolTip(_("Override individual ships' auto-texturization settings with these defaults. This setting is not saved, and it will revert to OFF the next time the game is played."));
                mForceSharedAutoTexturizationSettingsOntoShipCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnForceSharedAutoTexturizationSettingsOntoShipCheckBoxClicked, this);

                sizer->Add(
                    mForceSharedAutoTexturizationSettingsOntoShipCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Material Texture Magnification
            {
                mMaterialTextureMagnificationSlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderControl<float>::DirectionType::Vertical,
                    SliderWidth,
                    SliderHeight,
                    _("Texture Magnification"),
                    _("Changes the level of detail of materials' textures. Changes to this setting will only be visible after the next ship is loaded."),
                    [this](float value)
                    {
                        mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().MaterialTextureMagnification = value;
                        mOnChangeCallback();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        0.1f,
                        1.0f,
                        2.0f));

                sizer->Add(
                    mMaterialTextureMagnificationSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Material Texture Transparency
            {
                mMaterialTextureTransparencySlider = new SliderControl<float>(
                    boxSizer->GetStaticBox(),
                    SliderControl<float>::DirectionType::Vertical,
                    SliderWidth,
                    SliderHeight,
                    _("Texture Transparency"),
                    _("Changes the transparency of materials' textures. Changes to this setting will only be visible after the next ship is loaded."),
                    [this](float value)
                    {
                        mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().MaterialTextureTransparency = value;
                        mOnChangeCallback();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                sizer->Add(
                    mMaterialTextureTransparencySlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
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
    // Misc
    //

    {
        wxStaticBoxSizer * boxSizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Miscellaneous"));

        {
            wxGridBagSizer * sizer = new wxGridBagSizer(0, 0);

            // Reload last loaded ship on startup
            {
                mReloadLastLoadedShipOnStartupCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Reload Previous Ship on Startup"), wxDefaultPosition, wxDefaultSize, 0);
                mReloadLastLoadedShipOnStartupCheckBox->SetToolTip(_("When checked, the game starts with the ship that had been loaded when the game was last played."));
                mReloadLastLoadedShipOnStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnReloadLastLoadedShipOnStartupCheckBoxClicked, this);

                sizer->Add(
                    mReloadLastLoadedShipOnStartupCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Show Ship Description at Ship Load
            {
                mShowShipDescriptionAtShipLoadCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Ship Descriptions at Load"), wxDefaultPosition, wxDefaultSize, 0);
                mShowShipDescriptionAtShipLoadCheckBox->SetToolTip(_("Enables or disables the window showing ship descriptions when ships are loaded."));
                mShowShipDescriptionAtShipLoadCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked, this);

                sizer->Add(
                    mShowShipDescriptionAtShipLoadCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Continous Auto-Focus on Ship
            {
                mContinuousAutoFocusOnShipCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Continuous Auto-Focus"), wxDefaultPosition, wxDefaultSize, 0);
                mContinuousAutoFocusOnShipCheckBox->SetToolTip(_("Enables or disables automatic focus on the ship."));
                mContinuousAutoFocusOnShipCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnContinuousAutoFocusOnShipCheckBoxClicked, this);

                sizer->Add(
                    mContinuousAutoFocusOnShipCheckBox,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Auto-Focus on Ship Load
            {
                mAutoFocusOnShipLoadCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Auto-Focus at Ship Load"), wxDefaultPosition, wxDefaultSize, 0);
                mAutoFocusOnShipLoadCheckBox->SetToolTip(_("Enables or disables auto-focus when a ship is loaded."));
                mAutoFocusOnShipLoadCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnAutoFocusOnShipLoadCheckBoxClicked, this);

                sizer->Add(
                    mAutoFocusOnShipLoadCheckBox,
                    wxGBPosition(3, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Auto-Show Switchboard
            {
                mAutoShowSwitchboardCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Open Electrical Panel at Load"), wxDefaultPosition, wxDefaultSize, 0);
                mAutoShowSwitchboardCheckBox->SetToolTip(_("Enables or disables automatic showing of the ship's electrical panel when a ship with interactive electrical elements is loaded."));
                mAutoShowSwitchboardCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnAutoShowSwitchboardCheckBoxClicked, this);

                sizer->Add(
                    mAutoShowSwitchboardCheckBox,
                    wxGBPosition(4, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            // Show Electrical Notifications
            {
                mShowElectricalNotificationsCheckBox = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY,
                    _("Show Electrical Notifications"), wxDefaultPosition, wxDefaultSize, 0);
                mShowElectricalNotificationsCheckBox->SetToolTip(_("Enables or disables visual notifications when an electrical element changes state."));
                mShowElectricalNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowElectricalNotificationsCheckBoxClicked, this);

                sizer->Add(
                    mShowElectricalNotificationsCheckBox,
                    wxGBPosition(5, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorderInner);
            }

            boxSizer->Add(sizer, 0, wxALL, StaticBoxInsetMargin);
        }

        gridSizer->Add(
            boxSizer,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorderOuter);
    }

    // Finalize panel
    panel->SetSizerAndFit(gridSizer);
}

void PreferencesDialog::PopulateNpcPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Max NPCs
    {
        mMaxNpcsSlider = new SliderControl<size_t>(
            panel,
            SliderControl<size_t>::DirectionType::Vertical,
            SliderWidth,
            SliderHeight,
            _("Max NPCs"),
            _("Changes the maximum number of NPCs. Warning: higher values require more computing resources, with the risk of slowing the simulation down! Changes to this setting will only be visible after the next ship is loaded."),
            [this](size_t value)
            {
                mUIPreferencesManager.SetMaxNpcs(value);
                mOnChangeCallback();
            },
            FixedSetSliderCore<size_t>::FromPowersOfTwo(
                mUIPreferencesManager.GetMinMaxNpcs(),
                mUIPreferencesManager.GetMaxMaxNpcs()),
            mWarningIcon.get());

        gridSizer->Add(
            mMaxNpcsSlider,
            wxGBPosition(0, 0),
            wxGBSpan(2, 1),
            wxEXPAND | wxALL,
            CellBorderInner);
    }

    // NPCs per Group
    {
        mNpcsPerGroupSlider = new SliderControl<size_t>(
            panel,
            SliderControl<size_t>::DirectionType::Vertical,
            SliderWidth,
            SliderHeight,
            _("NPCs per Group"),
            _("Changes the number of NPCs spawned when a group is added."),
            [this](size_t value)
            {
                mUIPreferencesManager.SetNpcsPerGroup(value);
                mOnChangeCallback();
            },
            std::make_unique<IntegralLinearSliderCore<size_t>>(
                mUIPreferencesManager.GetMinNpcsPerGroup(),
                mUIPreferencesManager.GetMaxNpcsPerGroup()));

        gridSizer->Add(
            mNpcsPerGroupSlider,
            wxGBPosition(0, 1),
            wxGBSpan(2, 1),
            wxEXPAND | wxALL,
            CellBorderInner);
    }

    // Checkboxes

    {
        wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

        // Auto-Focus on Npc Placement
        {
            mAutoFocusOnNpcPlacementCheckBox = new wxCheckBox(panel, wxID_ANY,
                _("Auto-Focus at NPC Add"), wxDefaultPosition, wxDefaultSize, 0);
            mAutoFocusOnNpcPlacementCheckBox->SetToolTip(_("Enables or disables auto-focus when an NPC is added."));
            mAutoFocusOnNpcPlacementCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnAutoFocusOnNpcPlacementCheckBoxClicked, this);

            vSizer->Add(
                mAutoFocusOnNpcPlacementCheckBox,
                0,
                wxALIGN_LEFT | wxALL,
                Border);
        }

        // Show NPC Notifications
        {
            mShowNpcNotificationsCheckBox = new wxCheckBox(panel, wxID_ANY,
                _("Show NPC Notifications"), wxDefaultPosition, wxDefaultSize, 0);
            mShowNpcNotificationsCheckBox->SetToolTip(_("Enables or disables notifications about NPCs."));
            mShowNpcNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowNpcNotificationsCheckBoxClicked, this);

            vSizer->Add(
                mShowNpcNotificationsCheckBox,
                0,
                wxALIGN_LEFT | wxALL,
                Border);
        }

        gridSizer->Add(
            vSizer,
            wxGBPosition(0, 2),
            wxGBSpan(1, 1),
            wxALL,
            CellBorderInner);
    }

    // Finalize panel

    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);
    vSizer->AddStretchSpacer(1);
    vSizer->Add(
        gridSizer,
        0,
        wxALIGN_CENTER,
        0);
    vSizer->AddStretchSpacer(1);

    panel->SetSizerAndFit(vSizer);
}

void PreferencesDialog::PopulateMusicPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Row 1
    //

    {
        // Global mute
        {
            mGlobalMuteCheckBox = new wxCheckBox(panel, wxID_ANY,
                _("Mute All Sounds"), wxDefaultPosition, wxDefaultSize, 0);
            mGlobalMuteCheckBox->SetToolTip(_("Mutes or allows all sounds."));
            mGlobalMuteCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnGlobalMuteCheckBoxClicked, this);

            vSizer->Add(
                mGlobalMuteCheckBox,
                0,
                wxALIGN_LEFT | wxALL,
                Border);
        }
    }

    //
    // Row 2
    //

    {

        wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);

        {
            //
            // Row 1
            //

            {
                // Background music volume
                {
                    mBackgroundMusicVolumeSlider = new SliderControl<float>(
                        panel,
                        SliderControl<float>::DirectionType::Vertical,
                        SliderWidth,
                        SliderHeight,
                        _("Background Music Volume"),
                        _("Adjusts the volume of background music."),
                        [this](float value)
                        {
                            this->mUIPreferencesManager.SetBackgroundMusicVolume(value);
                            this->mOnChangeCallback();
                        },
                        std::make_unique<LinearSliderCore>(
                            0.0f,
                            100.0f));

                    gridSizer->Add(
                        mBackgroundMusicVolumeSlider,
                        wxGBPosition(0, 1),
                        wxGBSpan(1, 1),
                        wxEXPAND | wxALL,
                        Border);
                }

                // Sinking Music Volume
                {
                    mSinkingMusicVolumeSlider = new SliderControl<float>(
                        panel,
                        SliderControl<float>::DirectionType::Vertical,
                        SliderWidth,
                        SliderHeight,
                        _("Farewell Music Volume"),
                        _("Adjusts the volume of the music played when a ship is sinking."),
                        [this](float value)
                        {
                            this->mUIPreferencesManager.SetGameMusicVolume(value);
                            this->mOnChangeCallback();
                        },
                        std::make_unique<LinearSliderCore>(
                            0.0f,
                            100.0f));

                    gridSizer->Add(
                        mSinkingMusicVolumeSlider,
                        wxGBPosition(0, 3),
                        wxGBSpan(1, 1),
                        wxEXPAND | wxALL,
                        Border);
                }
            }

            //
            // Row 2
            //

            {
                // Play background music
                {
                    mPlayBackgroundMusicCheckBox = new wxCheckBox(panel, wxID_ANY,
                        _("Play Background Music"), wxDefaultPosition, wxDefaultSize, 0);
                    mPlayBackgroundMusicCheckBox->SetToolTip(_("Enables or disables background music while playing the game."));
                    mPlayBackgroundMusicCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnPlayBackgroundMusicCheckBoxClicked, this);

                    gridSizer->Add(
                        mPlayBackgroundMusicCheckBox,
                        wxGBPosition(1, 1),
                        wxGBSpan(1, 1),
                        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,
                        Border);
                }

                // Play sinking music
                {
                    mPlaySinkingMusicCheckBox = new wxCheckBox(panel, wxID_ANY,
                        _("Play Farewell Music"), wxDefaultPosition, wxDefaultSize, 0);
                    mPlaySinkingMusicCheckBox->SetToolTip(_("Enables or disables playing sorrow music when a ship starts sinking."));
                    mPlaySinkingMusicCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnPlaySinkingMusicCheckBoxClicked, this);

                    gridSizer->Add(
                        mPlaySinkingMusicCheckBox,
                        wxGBPosition(1, 3),
                        wxGBSpan(1, 1),
                        wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,
                        Border);
                }
            }


            //
            // Add spacers
            //

            // Col 0
            gridSizer->Add(
                1,
                0,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxEXPAND);

            // Col 2
            gridSizer->Add(
                1,
                0,
                wxGBPosition(0, 2),
                wxGBSpan(2, 1),
                wxEXPAND);

            // Col 4
            gridSizer->Add(
                1,
                0,
                wxGBPosition(0, 4),
                wxGBSpan(2, 1),
                wxEXPAND);

            gridSizer->AddGrowableCol(0);
            gridSizer->AddGrowableCol(2);
            gridSizer->AddGrowableCol(4);
        }

        vSizer->Add(
            gridSizer,
            0,
            wxEXPAND,
            0);
    }

    // Finalize panel
    panel->SetSizerAndFit(vSizer);
}

void PreferencesDialog::ReadSettings()
{
    mScreenshotDirPickerCtrl->SetPath(mUIPreferencesManager.GetScreenshotsFolderPath().string());

    mStartInFullScreenCheckBox->SetValue(mUIPreferencesManager.GetStartInFullScreen());
    mShowTipOnStartupCheckBox->SetValue(mUIPreferencesManager.GetShowStartupTip());
    mCheckForUpdatesAtStartupCheckBox->SetValue(mUIPreferencesManager.GetCheckUpdatesAtStartup());
    mSaveSettingsOnExitCheckBox->SetValue(mUIPreferencesManager.GetSaveSettingsOnExit());
    mShowTsunamiNotificationsCheckBox->SetValue(mUIPreferencesManager.GetDoShowTsunamiNotifications());
    mZoomIncrementSpinCtrl->SetValue(ZoomIncrementToZoomIncrementSpin(mUIPreferencesManager.GetZoomIncrement()));
    mPanIncrementSpinCtrl->SetValue(PanIncrementToPanIncrementSpin(mUIPreferencesManager.GetPanIncrement()));
    mCameraSpeedAdjustmentSpinCtrl->SetValue(CameraSpeedAdjustmentToCameraSpeedAdjustmentSpin(mUIPreferencesManager.GetCameraSpeedAdjustment()));
    mShowStatusTextCheckBox->SetValue(mUIPreferencesManager.GetShowStatusText());
    mShowExtendedStatusTextCheckBox->SetValue(mUIPreferencesManager.GetShowExtendedStatusText());
    mLanguagesListBox->SetSelection(GetLanguagesListBoxIndex(mUIPreferencesManager.GetDesiredLanguage()));
    switch (mUIPreferencesManager.GetDisplayUnitsSystem())
    {
        case UnitsSystem::SI_Kelvin:
        {
            mDisplayUnitsSettingsComboBox->SetSelection(0);
            break;
        }

        case UnitsSystem::SI_Celsius:
        {
            mDisplayUnitsSettingsComboBox->SetSelection(1);
            break;
        }

        case UnitsSystem::USCS:
        {
            mDisplayUnitsSettingsComboBox->SetSelection(2);
            break;
        }
    }

    mReloadLastLoadedShipOnStartupCheckBox->SetValue(mUIPreferencesManager.GetReloadLastLoadedShipOnStartup());
    mShowShipDescriptionAtShipLoadCheckBox->SetValue(mUIPreferencesManager.GetShowShipDescriptionsAtShipLoad());
    mContinuousAutoFocusOnShipCheckBox->SetValue(mUIPreferencesManager.GetAutoFocusTarget() == AutoFocusTargetKindType::Ship);
    mAutoFocusOnShipLoadCheckBox->SetValue(mUIPreferencesManager.GetDoAutoFocusOnShipLoad());
    mAutoShowSwitchboardCheckBox->SetValue(mUIPreferencesManager.GetAutoShowSwitchboard());
    mShowElectricalNotificationsCheckBox->SetValue(mUIPreferencesManager.GetDoShowElectricalNotifications());
    switch (mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().Mode)
    {
        case ShipAutoTexturizationModeType::FlatStructure:
        {
            mFlatStructureAutoTexturizationModeRadioButton->SetValue(true);
            break;
        }

        case ShipAutoTexturizationModeType::MaterialTextures:
        {
            mMaterialTexturesAutoTexturizationModeRadioButton->SetValue(true);
            break;
        }
    }
    mForceSharedAutoTexturizationSettingsOntoShipCheckBox->SetValue(mUIPreferencesManager.GetShipAutoTexturizationForceSharedSettingsOntoShipDefinition());
    mMaterialTextureMagnificationSlider->SetValue(mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().MaterialTextureMagnification);
    mMaterialTextureTransparencySlider->SetValue(mUIPreferencesManager.GetShipAutoTexturizationSharedSettings().MaterialTextureTransparency);

    mMaxNpcsSlider->SetValue(mUIPreferencesManager.GetMaxNpcs());
    mNpcsPerGroupSlider->SetValue(mUIPreferencesManager.GetNpcsPerGroup());
    mAutoFocusOnNpcPlacementCheckBox->SetValue(mUIPreferencesManager.GetDoAutoFocusOnNpcPlacement());
    mShowNpcNotificationsCheckBox->SetValue(mUIPreferencesManager.GetDoShowNpcNotifications());

    mGlobalMuteCheckBox->SetValue(mUIPreferencesManager.GetGlobalMute());
    mBackgroundMusicVolumeSlider->SetValue(mUIPreferencesManager.GetBackgroundMusicVolume());
    mPlayBackgroundMusicCheckBox->SetValue(mUIPreferencesManager.GetPlayBackgroundMusic());
    mSinkingMusicVolumeSlider->SetValue(mUIPreferencesManager.GetGameMusicVolume());
    mPlaySinkingMusicCheckBox->SetValue(mUIPreferencesManager.GetPlaySinkingMusic());

    ReconcileSoundSettings();
}

float PreferencesDialog::ZoomIncrementSpinToZoomIncrement(int spinPosition)
{
    return 1.0f + static_cast<float>(spinPosition) * 2.0f / static_cast<float>(MaxZoomIncrementPosition);
}

int PreferencesDialog::ZoomIncrementToZoomIncrementSpin(float zoomIncrement)
{
    return static_cast<int>(std::round((zoomIncrement - 1.0f) * static_cast<float>(MaxZoomIncrementPosition) / 2.0f));
}

int PreferencesDialog::PanIncrementSpinToPanIncrement(int spinPosition)
{
    return spinPosition;
}

int PreferencesDialog::PanIncrementToPanIncrementSpin(int panIncrement)
{
    return panIncrement;
}

float PreferencesDialog::CameraSpeedAdjustmentSpinToCameraSpeedAdjustment(int spinPosition) const
{
    return static_cast<float>(spinPosition) / CameraSpeedAdjustmentSpinFactor;
}

int PreferencesDialog::CameraSpeedAdjustmentToCameraSpeedAdjustmentSpin(float cameraSpeedAdjustment) const
{
    return static_cast<int>(cameraSpeedAdjustment * CameraSpeedAdjustmentSpinFactor);
}

void PreferencesDialog::ReconciliateShipAutoTexturizationModeSettings()
{
    mMaterialTextureMagnificationSlider->Enable(mMaterialTexturesAutoTexturizationModeRadioButton->GetValue());
    mMaterialTextureTransparencySlider->Enable(mMaterialTexturesAutoTexturizationModeRadioButton->GetValue());
}

void PreferencesDialog::ReconcileSoundSettings()
{
    mBackgroundMusicVolumeSlider->Enable(!mGlobalMuteCheckBox->GetValue() && mPlayBackgroundMusicCheckBox->GetValue());
    mSinkingMusicVolumeSlider->Enable(!mGlobalMuteCheckBox->GetValue() && mPlaySinkingMusicCheckBox->GetValue());
}

int PreferencesDialog::GetLanguagesListBoxIndex(std::optional<std::string> languageIdentifier) const
{
    if (languageIdentifier.has_value())
    {
        auto const it = std::find_if(
            mAvailableLanguages.cbegin(),
            mAvailableLanguages.cend(),
            [&languageIdentifier](auto const & li)
            {
                return li.Identifier == *languageIdentifier;
            });

        assert(it != mAvailableLanguages.cend());
        return static_cast<int>(std::distance(mAvailableLanguages.cbegin(), it)) + 1;
    }
    else
    {
        // System-chosen
        return 0;
    }
}