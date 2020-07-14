/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "PreferencesDialog.h"

#include <GameCore/ExponentialSliderCore.h>
#include <GameCore/LinearSliderCore.h>

#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/stattext.h>

static constexpr int Border = 10;

static int constexpr StaticBoxTopMargin = 7;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr CellBorder = 8;

static int constexpr SliderWidth = 40;
static int constexpr SliderHeight = 140;

static constexpr int MaxZoomIncrementPosition = 200;
static constexpr int MaxPanIncrementPosition = 200;

PreferencesDialog::PreferencesDialog(
    wxWindow* parent,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    std::function<void()> onChangeCallback)
    : mParent(parent)
    , mUIPreferencesManager(std::move(uiPreferencesManager))
    , mOnChangeCallback(std::move(onChangeCallback))
{
    Create(
        mParent,
        wxID_ANY,
        _("Game Preferences"),
        wxDefaultPosition,
        wxSize(400, -1),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED,
        _T("Preferences Window"));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


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
    // Game Preferences
    //

    wxPanel * gamePanel = new wxPanel(notebook);

    PopulateGamePanel(gamePanel);

    notebook->AddPage(gamePanel, "Game");


    //
    // Ship Preferences
    //

    wxPanel * shipsPanel = new wxPanel(notebook);

    PopulateShipPanel(shipsPanel);

    notebook->AddPage(shipsPanel, "Ships");


    //
    // Global Sound and Music
    //

    wxPanel * musicPanel = new wxPanel(notebook);

    PopulateMusicPanel(musicPanel);

    notebook->AddPage(musicPanel, "Global Sound and Music");


    dialogVSizer->Add(notebook, 1, wxEXPAND);

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

    this->Show();
}

void PreferencesDialog::OnScreenshotDirPickerChanged(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetScreenshotsFolderPath(mScreenshotDirPickerCtrl->GetPath().ToStdString());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowStartupTip(mShowTipOnStartupCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnCheckForUpdatesAtStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetCheckUpdatesAtStartup(mCheckForUpdatesAtStartupCheckBox->GetValue());

    if (mCheckForUpdatesAtStartupCheckBox->GetValue())
    {
        mUIPreferencesManager->ResetUpdateBlacklist();
    }

    mOnChangeCallback();
}

void PreferencesDialog::OnSaveSettingsOnExitCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetSaveSettingsOnExit(mSaveSettingsOnExitCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetDoShowTsunamiNotifications(mShowTsunamiNotificationsCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnZoomIncrementSpinCtrl(wxSpinEvent & event)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetZoomIncrement(ZoomIncrementSpinToZoomIncrement(event.GetPosition()));

    mOnChangeCallback();
}

void PreferencesDialog::OnPanIncrementSpinCtrl(wxSpinEvent & event)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetPanIncrement(PanIncrementSpinToPanIncrement(event.GetPosition()));

    mOnChangeCallback();
}

void PreferencesDialog::OnShowStatusTextCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowStatusText(mShowStatusTextCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowExtendedStatusTextCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowExtendedStatusText(mShowExtendedStatusTextCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowShipDescriptionsAtShipLoad(mShowShipDescriptionAtShipLoadCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoZoomAtShipLoadCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetDoAutoZoomAtShipLoad(mAutoZoomAtShipLoadCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoShowSwitchboardCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetAutoShowSwitchboard(mAutoShowSwitchboardCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnShowElectricalNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetDoShowElectricalNotifications(mShowElectricalNotificationsCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnAutoTexturizationModeRadioButtonClick(wxCommandEvent & /*event*/)
{
    if (mFlatStructureAutoTexturizationModeRadioButton->GetValue())
    {
        mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().Mode = ShipAutoTexturizationModeType::FlatStructure;
    }
    else
    {
        assert(mMaterialTexturesAutoTexturizationModeRadioButton->GetValue());
        mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().Mode = ShipAutoTexturizationModeType::MaterialTextures;
    }

    ReconciliateShipAutoTexturizationModeSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnForceDefaultAutoTexturizationSettingsOntoShipCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShipAutoTexturizationForceDefaultsOntoShipDefinition(mForceDefaultAutoTexturizationSettingsOntoShipCheckBox->GetValue());

    mOnChangeCallback();
}

void PreferencesDialog::OnGlobalMuteCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetGlobalMute(mGlobalMuteCheckBox->GetValue());

    ReconcileSoundSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnPlayBackgroundMusicCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetPlayBackgroundMusic(mPlayBackgroundMusicCheckBox->GetValue());

    ReconcileSoundSettings();

    mOnChangeCallback();
}

void PreferencesDialog::OnPlaySinkingMusicCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetPlaySinkingMusic(mPlaySinkingMusicCheckBox->GetValue());

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
    //
    // | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
    //     | X |       | X   X |
    //

    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);

    gridSizer->SetFlexibleDirection(wxHORIZONTAL);
    gridSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);


    //
    // Row 1
    //

    {
        wxStaticText * screenshotDirStaticText = new wxStaticText(panel, wxID_ANY, "Screenshot directory:");

        gridSizer->Add(
            screenshotDirStaticText,
            wxGBPosition(0, 1),
            wxGBSpan(1, 5), // Take entire row
            wxRIGHT | wxLEFT | wxEXPAND | wxALIGN_BOTTOM,
            Border);
    }

    //
    // Row 2
    //

    {
        mScreenshotDirPickerCtrl = new wxDirPickerCtrl(
            panel,
            wxID_ANY,
            _T(""),
            _("Select directory that screenshots will be saved to:"),
            wxDefaultPosition,
            wxSize(-1, -1),
            wxDIRP_DIR_MUST_EXIST | wxDIRP_USE_TEXTCTRL);
        mScreenshotDirPickerCtrl->SetToolTip("Sets the directory into which in-game screenshots are automatically saved.");
        mScreenshotDirPickerCtrl->Bind(wxEVT_DIRPICKER_CHANGED, &PreferencesDialog::OnScreenshotDirPickerChanged, this);

        gridSizer->Add(
            mScreenshotDirPickerCtrl,
            wxGBPosition(1, 1),
            wxGBSpan(1, 5), // Take entire row
            wxRIGHT | wxLEFT | wxEXPAND,
            Border);
    }

    //
    // Row 3
    //

    gridSizer->Add(1, 15, wxGBPosition(2, 0), wxGBSpan(7, 1), wxEXPAND);

    //
    // Row 4
    //

    {
        mShowTipOnStartupCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Tips on Startup"), wxDefaultPosition, wxDefaultSize, 0);
        mShowTipOnStartupCheckBox->SetToolTip("Enables or disables the tips shown when the game starts.");
        mShowTipOnStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTipOnStartupCheckBoxClicked, this);

        gridSizer->Add(
            mShowTipOnStartupCheckBox,
            wxGBPosition(3, 1),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    {
        mZoomIncrementSpinCtrl = new wxSpinCtrl(panel, wxID_ANY, _T("Zoom Increment"), wxDefaultPosition, wxSize(75, -1),
            wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
        mZoomIncrementSpinCtrl->SetRange(1, MaxZoomIncrementPosition);
        mZoomIncrementSpinCtrl->SetToolTip("Changes the amount by which zoom changes when using the zoom controls.");
        mZoomIncrementSpinCtrl->Bind(wxEVT_SPINCTRL, &PreferencesDialog::OnZoomIncrementSpinCtrl, this);

        gridSizer->Add(
            mZoomIncrementSpinCtrl,
            wxGBPosition(3, 4),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
            Border);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, "Zoom Increment", wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        gridSizer->Add(
            label,
            wxGBPosition(3, 5),
            wxGBSpan(1, 1),
            wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
            Border);
    }


    //
    // Row 5
    //

    {
        mCheckForUpdatesAtStartupCheckBox = new wxCheckBox(panel, wxID_ANY, _("Check for Updates on Startup"), wxDefaultPosition, wxDefaultSize, 0);
        mCheckForUpdatesAtStartupCheckBox->SetToolTip("Enables or disables checking for new versions when the game starts.");
        mCheckForUpdatesAtStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnCheckForUpdatesAtStartupCheckBoxClicked, this);

        gridSizer->Add(
            mCheckForUpdatesAtStartupCheckBox,
            wxGBPosition(4, 1),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    {
        mPanIncrementSpinCtrl = new wxSpinCtrl(panel, wxID_ANY, _T("Pan Increment"), wxDefaultPosition, wxSize(75, -1),
            wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
        mPanIncrementSpinCtrl->SetRange(1, MaxPanIncrementPosition);
        mPanIncrementSpinCtrl->SetToolTip("Changes the amount by which the camera position changes when using the pan controls.");
        mPanIncrementSpinCtrl->Bind(wxEVT_SPINCTRL, &PreferencesDialog::OnPanIncrementSpinCtrl, this);

        gridSizer->Add(
            mPanIncrementSpinCtrl,
            wxGBPosition(4, 4),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
            Border);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, "Pan Increment", wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        gridSizer->Add(
            label,
            wxGBPosition(4, 5),
            wxGBSpan(1, 1),
            wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxBOTTOM | wxRIGHT,
            Border);
    }

    //
    // Row 6
    //

    {
        mSaveSettingsOnExitCheckBox = new wxCheckBox(panel, wxID_ANY, _("Save Settings on Exit"), wxDefaultPosition, wxDefaultSize, 0);
        mSaveSettingsOnExitCheckBox->SetToolTip("Enables or disables saving the last-modified settings when exiting the game.");
        mSaveSettingsOnExitCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnSaveSettingsOnExitCheckBoxClicked, this);

        gridSizer->Add(
            mSaveSettingsOnExitCheckBox,
            wxGBPosition(5, 1),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    {
        mShowStatusTextCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Status Text"), wxDefaultPosition, wxDefaultSize, 0);
        mShowStatusTextCheckBox->SetToolTip("Enables or disables the display of game performance information, such as frame rate and time elapsed.");
        mShowStatusTextCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowStatusTextCheckBoxClicked, this);

        gridSizer->Add(
            mShowStatusTextCheckBox,
            wxGBPosition(5, 4),
            wxGBSpan(1, 2),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    //
    // Row 7
    //

    {
        mShowTsunamiNotificationsCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Tsunami Notifications"), wxDefaultPosition, wxDefaultSize, 0);
        mShowTsunamiNotificationsCheckBox->SetToolTip("Enables or disables notifications when a tsunami is being spawned.");
        mShowTsunamiNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked, this);

        gridSizer->Add(
            mShowTsunamiNotificationsCheckBox,
            wxGBPosition(6, 1),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    {
        mShowExtendedStatusTextCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Extended Status Text"), wxDefaultPosition, wxDefaultSize, 0);
        mShowExtendedStatusTextCheckBox->SetToolTip("Enables or disables the display of extended game performance information, such as update/render ratio and counts of primitives being rendered.");
        mShowExtendedStatusTextCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowExtendedStatusTextCheckBoxClicked, this);

        gridSizer->Add(
            mShowExtendedStatusTextCheckBox,
            wxGBPosition(6, 4),
            wxGBSpan(1, 2),
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
            Border);
    }

    //
    // Add spacers
    //

    gridSizer->Add(1, 1, wxGBPosition(0, 0), wxGBSpan(7, 1), wxEXPAND);
    gridSizer->AddGrowableCol(0, 1);
    gridSizer->AddGrowableCol(2, 1);

    gridSizer->AddGrowableCol(3, 1);
    gridSizer->Add(1, 1, wxGBPosition(0, 6), wxGBSpan(7, 1), wxEXPAND);
    gridSizer->AddGrowableCol(6, 1);


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void PreferencesDialog::PopulateShipPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Auto-Texturization
    //

    {
        wxStaticBox * autoTexturizationBox = new wxStaticBox(panel, wxID_ANY, _("Auto-Texturization"));

        wxBoxSizer * autoTexturizationBoxSizer = new wxBoxSizer(wxVERTICAL);
        autoTexturizationBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * autoTexturizationSizer = new wxGridBagSizer(0, 0);

            // Texturization Mode
            {
                wxStaticBox * texturizationModeBox = new wxStaticBox(autoTexturizationBox, wxID_ANY, _("Mode"));

                wxBoxSizer * texturizationModeBoxSizer1 = new wxBoxSizer(wxVERTICAL);
                texturizationModeBoxSizer1->AddSpacer(StaticBoxTopMargin);

                {
                    wxGridBagSizer * texturizationModeBoxSizer2 = new wxGridBagSizer(5, 3);

                    mFlatStructureAutoTexturizationModeRadioButton = new wxRadioButton(texturizationModeBox, wxID_ANY, _("Flat Structure"),
                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
                    mFlatStructureAutoTexturizationModeRadioButton->SetToolTip("When a ship does not have a high-definition image, generates one using the materials' matte colors. Changes to this setting are only visible after a new ship is loaded.");
                    mFlatStructureAutoTexturizationModeRadioButton->Bind(wxEVT_RADIOBUTTON, &PreferencesDialog::OnAutoTexturizationModeRadioButtonClick, this);
                    texturizationModeBoxSizer2->Add(mFlatStructureAutoTexturizationModeRadioButton, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    //

                    mMaterialTexturesAutoTexturizationModeRadioButton = new wxRadioButton(texturizationModeBox, wxID_ANY, _("Material Textures"),
                        wxDefaultPosition, wxDefaultSize);
                    mMaterialTexturesAutoTexturizationModeRadioButton->SetToolTip("When a ship does not have a high-definition image, generates one using material-specific textures. Changes to this setting are only visible after a new ship is loaded.");
                    mMaterialTexturesAutoTexturizationModeRadioButton->Bind(wxEVT_RADIOBUTTON, &PreferencesDialog::OnAutoTexturizationModeRadioButtonClick, this);
                    texturizationModeBoxSizer2->Add(mMaterialTexturesAutoTexturizationModeRadioButton, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL | wxALIGN_CENTER_VERTICAL, 0);

                    texturizationModeBoxSizer1->Add(texturizationModeBoxSizer2, 0, wxALL, StaticBoxInsetMargin);
                }

                texturizationModeBox->SetSizerAndFit(texturizationModeBoxSizer1);

                autoTexturizationSizer->Add(
                    texturizationModeBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            // Force default settings onto ship
            {
                mForceDefaultAutoTexturizationSettingsOntoShipCheckBox = new wxCheckBox(autoTexturizationBox, wxID_ANY, _("Force Defaults onto Ships"), wxDefaultPosition, wxDefaultSize, 0);
                mForceDefaultAutoTexturizationSettingsOntoShipCheckBox->SetToolTip("Override individual ships' auto-texturization settings with these defaults. This setting is not saved, and it will revert to OFF the next time the game is played.");
                mForceDefaultAutoTexturizationSettingsOntoShipCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnForceDefaultAutoTexturizationSettingsOntoShipCheckBoxClicked, this);

                autoTexturizationSizer->Add(
                    mForceDefaultAutoTexturizationSettingsOntoShipCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Material Texture Magnification
            {
                mMaterialTextureMagnificationSlider = new SliderControl<float>(
                    autoTexturizationBox,
                    SliderWidth,
                    SliderHeight,
                    "Texture Magnification",
                    "Changes the level of detail of materials' textures. Changes to this setting are only visible after a new ship is loaded.",
                    [this](float value)
                    {
                        assert(!!mUIPreferencesManager);
                        mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().MaterialTextureMagnification = value;
                        mOnChangeCallback();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        0.1f,
                        1.0f,
                        2.0f));

                autoTexturizationSizer->Add(
                    mMaterialTextureMagnificationSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Material Texture Transparency
            {
                mMaterialTextureTransparencySlider = new SliderControl<float>(
                    autoTexturizationBox,
                    SliderWidth,
                    SliderHeight,
                    "Texture Transparency",
                    "Changes the transparency of materials' textures. Changes to this setting are only visible after a new ship is loaded.",
                    [this](float value)
                    {
                        assert(!!mUIPreferencesManager);
                        mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().MaterialTextureTransparency = value;
                        mOnChangeCallback();
                    },
                    std::make_unique<LinearSliderCore>(
                        0.0f,
                        1.0f));

                autoTexturizationSizer->Add(
                    mMaterialTextureTransparencySlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(2, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            autoTexturizationBoxSizer->Add(autoTexturizationSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        autoTexturizationBox->SetSizerAndFit(autoTexturizationBoxSizer);

        gridSizer->Add(
            autoTexturizationBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Misc
    //

    {
        wxStaticBox * miscBox = new wxStaticBox(panel, wxID_ANY, _("Miscellaneous"));

        wxBoxSizer * miscBoxSizer = new wxBoxSizer(wxVERTICAL);
        miscBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * miscSizer = new wxGridBagSizer(0, 0);

            // Show Ship Description at Ship Load
            {
                mShowShipDescriptionAtShipLoadCheckBox = new wxCheckBox(miscBox, wxID_ANY, _("Show Ship Descriptions at Load"), wxDefaultPosition, wxDefaultSize, 0);
                mShowShipDescriptionAtShipLoadCheckBox->SetToolTip("Enables or disables the window showing ship descriptions when ships are loaded.");
                mShowShipDescriptionAtShipLoadCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked, this);

                miscSizer->Add(
                    mShowShipDescriptionAtShipLoadCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Auto-Zoom
            {
                mAutoZoomAtShipLoadCheckBox = new wxCheckBox(miscBox, wxID_ANY, _("Auto-Zoom at Ship Load"), wxDefaultPosition, wxDefaultSize, 0);
                mAutoZoomAtShipLoadCheckBox->SetToolTip("Enables or disables auto-zooming when loading a new ship.");
                mAutoZoomAtShipLoadCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnAutoZoomAtShipLoadCheckBoxClicked, this);

                miscSizer->Add(
                    mAutoZoomAtShipLoadCheckBox,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Auto-Show Switchboard
            {
                mAutoShowSwitchboardCheckBox = new wxCheckBox(miscBox, wxID_ANY, _("Open Electrical Panel at Load"), wxDefaultPosition, wxDefaultSize, 0);
                mAutoShowSwitchboardCheckBox->SetToolTip("Enables or disables automatic showing of the ship's electrical panel when a ship with interactive electrical elements is loaded.");
                mAutoShowSwitchboardCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnAutoShowSwitchboardCheckBoxClicked, this);

                miscSizer->Add(
                    mAutoShowSwitchboardCheckBox,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Show Electrical Notifications
            {
                mShowElectricalNotificationsCheckBox = new wxCheckBox(miscBox, wxID_ANY, _("Show Electrical Notifications"), wxDefaultPosition, wxDefaultSize, 0);
                mShowElectricalNotificationsCheckBox->SetToolTip("Enables or disables visual notifications when an electrical element changes state.");
                mShowElectricalNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowElectricalNotificationsCheckBoxClicked, this);

                miscSizer->Add(
                    mShowElectricalNotificationsCheckBox,
                    wxGBPosition(3, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            miscBoxSizer->Add(miscSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        miscBox->SetSizerAndFit(miscBoxSizer);

        gridSizer->Add(
            miscBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
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
            mGlobalMuteCheckBox = new wxCheckBox(panel, wxID_ANY, _("Mute All Sounds"), wxDefaultPosition, wxDefaultSize, 0);

            mGlobalMuteCheckBox->SetToolTip("Mutes or allows all sounds.");

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
                        SliderWidth,
                        SliderHeight,
                        "Background Music Volume",
                        "Adjusts the volume of background music.",
                        [this](float value)
                        {
                            this->mUIPreferencesManager->SetBackgroundMusicVolume(value);
                            this->mOnChangeCallback();
                        },
                        std::make_unique<LinearSliderCore>(
                            0.0f,
                            100.0f));

                    gridSizer->Add(
                        mBackgroundMusicVolumeSlider,
                        wxGBPosition(0, 1),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,
                        Border);
                }

                // Sinking Music Volume
                {
                    mSinkingMusicVolumeSlider = new SliderControl<float>(
                        panel,
                        SliderWidth,
                        SliderHeight,
                        "Farewell Music Volume",
                        "Adjusts the volume of the music played when a ship is sinking.",
                        [this](float value)
                        {
                            this->mUIPreferencesManager->SetGameMusicVolume(value);
                            this->mOnChangeCallback();
                        },
                        std::make_unique<LinearSliderCore>(
                            0.0f,
                            100.0f));

                    gridSizer->Add(
                        mSinkingMusicVolumeSlider,
                        wxGBPosition(0, 3),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxALL,
                        Border);
                }
            }

            //
            // Row 2
            //

            {
                // Play background music
                {
                    mPlayBackgroundMusicCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Background Music"), wxDefaultPosition, wxDefaultSize, 0);

                    mPlayBackgroundMusicCheckBox->SetToolTip("Enables or disables background music while playing the game.");

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
                    mPlaySinkingMusicCheckBox = new wxCheckBox(panel, wxID_ANY, _("Play Farewell Music"), wxDefaultPosition, wxDefaultSize, 0);

                    mPlaySinkingMusicCheckBox->SetToolTip("Enables or disables playing sorrow music when a ship starts sinking.");

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
            1,
            wxEXPAND,
            0);
    }

    // Finalize panel
    panel->SetSizerAndFit(vSizer);
}

void PreferencesDialog::ReadSettings()
{
    assert(!!mUIPreferencesManager);

    mScreenshotDirPickerCtrl->SetPath(mUIPreferencesManager->GetScreenshotsFolderPath().string());

    mShowTipOnStartupCheckBox->SetValue(mUIPreferencesManager->GetShowStartupTip());
    mCheckForUpdatesAtStartupCheckBox->SetValue(mUIPreferencesManager->GetCheckUpdatesAtStartup());
    mSaveSettingsOnExitCheckBox->SetValue(mUIPreferencesManager->GetSaveSettingsOnExit());
    mShowTsunamiNotificationsCheckBox->SetValue(mUIPreferencesManager->GetDoShowTsunamiNotifications());
    mZoomIncrementSpinCtrl->SetValue(ZoomIncrementToZoomIncrementSpin(mUIPreferencesManager->GetZoomIncrement()));
    mPanIncrementSpinCtrl->SetValue(PanIncrementToPanIncrementSpin(mUIPreferencesManager->GetPanIncrement()));
    mShowStatusTextCheckBox->SetValue(mUIPreferencesManager->GetShowStatusText());
    mShowExtendedStatusTextCheckBox->SetValue(mUIPreferencesManager->GetShowExtendedStatusText());

    mShowShipDescriptionAtShipLoadCheckBox->SetValue(mUIPreferencesManager->GetShowShipDescriptionsAtShipLoad());
    mAutoZoomAtShipLoadCheckBox->SetValue(mUIPreferencesManager->GetDoAutoZoomAtShipLoad());
    mAutoShowSwitchboardCheckBox->SetValue(mUIPreferencesManager->GetAutoShowSwitchboard());
    mShowElectricalNotificationsCheckBox->SetValue(mUIPreferencesManager->GetDoShowElectricalNotifications());
    switch (mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().Mode)
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
    mForceDefaultAutoTexturizationSettingsOntoShipCheckBox->SetValue(mUIPreferencesManager->GetShipAutoTexturizationForceDefaultsOntoShipDefinition());
    mMaterialTextureMagnificationSlider->SetValue(mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().MaterialTextureMagnification);
    mMaterialTextureTransparencySlider->SetValue(mUIPreferencesManager->GetShipAutoTexturizationDefaultSettings().MaterialTextureTransparency);

    mGlobalMuteCheckBox->SetValue(mUIPreferencesManager->GetGlobalMute());
    mBackgroundMusicVolumeSlider->SetValue(mUIPreferencesManager->GetBackgroundMusicVolume());
    mPlayBackgroundMusicCheckBox->SetValue(mUIPreferencesManager->GetPlayBackgroundMusic());
    mSinkingMusicVolumeSlider->SetValue(mUIPreferencesManager->GetGameMusicVolume());
    mPlaySinkingMusicCheckBox->SetValue(mUIPreferencesManager->GetPlaySinkingMusic());

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

float PreferencesDialog::PanIncrementSpinToPanIncrement(int spinPosition)
{
    return static_cast<float>(spinPosition);
}

int PreferencesDialog::PanIncrementToPanIncrementSpin(float panIncrement)
{
    return static_cast<int>(panIncrement);
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