/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "PreferencesDialog.h"

#include <wx/gbsizer.h>
#include <wx/stattext.h>

static constexpr int Border = 10;

PreferencesDialog::PreferencesDialog(
    wxWindow* parent,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager)
    : mParent(parent)
    , mUIPreferencesManager(std::move(uiPreferencesManager))
{
    Create(
        mParent,
        wxID_ANY,
        _("Preferences"),
        wxDefaultPosition,
        wxSize(400, -1),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED,
        _T("Preferences Window"));


    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel * mainPanel = new wxPanel(this);
    mainPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    PopulateMainPanel(mainPanel);

    dialogVSizer->Add(mainPanel, 1, wxEXPAND);

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
}

void PreferencesDialog::OnShowTipOnStartupCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowStartupTip(mShowTipOnStartupCheckBox->GetValue());
}

void PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowShipDescriptionsAtShipLoad(mShowShipDescriptionAtShipLoadCheckBox->GetValue());
}

void PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowTsunamiNotifications(mShowTsunamiNotificationsCheckBox->GetValue());
}

void PreferencesDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // Close ourselves
    this->Close();
}

void PreferencesDialog::PopulateMainPanel(wxPanel * panel)
{
    wxGridBagSizer* gridSizer = new wxGridBagSizer(0, 0);


    //
    // Row 1
    //

    wxBoxSizer* screenshotDirSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText * screenshotDirStaticText = new wxStaticText(panel, wxID_ANY, "Screenshot directory:", wxDefaultPosition, wxDefaultSize, 0);
    screenshotDirSizer->Add(screenshotDirStaticText, 1, wxALIGN_LEFT | wxEXPAND, 0);

    mScreenshotDirPickerCtrl = new wxDirPickerCtrl(
        panel,
        wxID_ANY,
        _T(""),
        _("Select directory that screenshots will be saved to:"),
        wxDefaultPosition,
        wxSize(-1, -1),
        wxDIRP_DIR_MUST_EXIST | wxDIRP_USE_TEXTCTRL);
    mScreenshotDirPickerCtrl->SetToolTip("Sets the directory into which in-game screenshots are automatically saved.");
    mScreenshotDirPickerCtrl->SetMinSize(wxSize(540, -1));

    mScreenshotDirPickerCtrl->Bind(wxEVT_DIRPICKER_CHANGED, &PreferencesDialog::OnScreenshotDirPickerChanged, this);

    screenshotDirSizer->Add(mScreenshotDirPickerCtrl, 1, wxALIGN_LEFT | wxEXPAND, 0);

    gridSizer->Add(
        screenshotDirSizer,
        wxGBPosition(0, 0),
        wxGBSpan(1, 4), // Take entire row
        wxALL | wxEXPAND,
        Border);


    //
    // Row 2
    //

    mShowTipOnStartupCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Tips on Startup"), wxDefaultPosition, wxDefaultSize, 0);

    mShowTipOnStartupCheckBox->SetToolTip("Enables or disables the tips shown when the game starts.");

    mShowTipOnStartupCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTipOnStartupCheckBoxClicked, this);

    gridSizer->Add(
        mShowTipOnStartupCheckBox,
        wxGBPosition(1, 0),
        wxGBSpan(1, 1),
        wxLEFT | wxRIGHT | wxBOTTOM,
        Border);


    //
    // Row 3
    //

    mShowShipDescriptionAtShipLoadCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Ship Descriptions at Load"), wxDefaultPosition, wxDefaultSize, 0);

    mShowShipDescriptionAtShipLoadCheckBox->SetToolTip("Enables or disables the window showing ship descriptions when ships are loaded.");

    mShowShipDescriptionAtShipLoadCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowShipDescriptionAtShipLoadCheckBoxClicked, this);

    gridSizer->Add(
        mShowShipDescriptionAtShipLoadCheckBox,
        wxGBPosition(2, 0),
        wxGBSpan(1, 1),
        wxLEFT | wxRIGHT | wxBOTTOM,
        Border);


    //
    // Row 4
    //

    mShowTsunamiNotificationsCheckBox = new wxCheckBox(panel, wxID_ANY, _("Show Tsunami Notifications"), wxDefaultPosition, wxDefaultSize, 0);

    mShowTsunamiNotificationsCheckBox->SetToolTip("Enables or disables visual notifications when a tsunami is being spawned.");

    mShowTsunamiNotificationsCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PreferencesDialog::OnShowTsunamiNotificationsCheckBoxClicked, this);

    gridSizer->Add(
        mShowTsunamiNotificationsCheckBox,
        wxGBPosition(3, 0),
        wxGBSpan(1, 1),
        wxLEFT | wxRIGHT | wxBOTTOM,
        Border);


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void PreferencesDialog::ReadSettings()
{
    assert(!!mUIPreferencesManager);

    mScreenshotDirPickerCtrl->SetPath(mUIPreferencesManager->GetScreenshotsFolderPath().string());

    mShowTipOnStartupCheckBox->SetValue(mUIPreferencesManager->GetShowStartupTip());
    mShowShipDescriptionAtShipLoadCheckBox->SetValue(mUIPreferencesManager->GetShowShipDescriptionsAtShipLoad());
    mShowTsunamiNotificationsCheckBox->SetValue(mUIPreferencesManager->GetShowTsunamiNotifications());
}