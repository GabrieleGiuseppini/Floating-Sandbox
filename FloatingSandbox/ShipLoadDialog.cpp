/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLoadDialog.h"

#include <wx/sizer.h>

constexpr int Width = 800;
constexpr int Height = 600;
constexpr int DirColumnWidth = 260;

ShipLoadDialog::ShipLoadDialog(
    wxWindow * parent,
    std::shared_ptr<UIPreferences> uiPreferences,
    ResourceLoader const & resourceLoader)
	: mParent(parent)
    , mUIPreferences(std::move(uiPreferences))
{
	Create(
		mParent,
		wxID_ANY,
		_("Load Ship"),
		wxDefaultPosition,
        wxSize(Width, Height),
		wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX | wxFRAME_SHAPED,
		_T("Load Ship Dialog"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    Centre();


    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Directory and preview
    //

    wxBoxSizer * hSizer1 = new wxBoxSizer(wxHORIZONTAL);



    assert(!mUIPreferences->GetShipLoadDirectories().empty());

    mDirCtrl = new wxGenericDirCtrl(
        this,
        wxID_ANY,
        mUIPreferences->GetShipLoadDirectories().front().string(),
        wxDefaultPosition,
        wxSize(DirColumnWidth, -1),
        wxDIRCTRL_DIR_ONLY);

    Connect(mDirCtrl->GetId(), wxEVT_DIRCTRL_SELECTIONCHANGED, (wxObjectEventFunction)&ShipLoadDialog::OnDirectorySelected);

    hSizer1->Add(mDirCtrl, 0, wxEXPAND | wxALIGN_TOP);




    mShipPreviewPanel = new ShipPreviewPanel(this, resourceLoader);

    hSizer1->Add(mShipPreviewPanel, 1, wxALIGN_TOP | wxEXPAND);




    vSizer->Add(hSizer1, 1, wxEXPAND);

    vSizer->AddSpacer(10);



    //
    // Directories combo
    //

    wxBoxSizer * hSizer2 = new wxBoxSizer(wxHORIZONTAL);

    hSizer2->AddSpacer(10);



    wxArrayString comboChoices;
    for (auto dir : mUIPreferences->GetShipLoadDirectories())
    {
        comboChoices.Add(dir.string());
    }

    assert(!comboChoices.empty());

    mDirectoriesComboBox = new wxComboBox(
        this,
        wxID_ANY,
        comboChoices.front(),
        wxDefaultPosition,
        wxDefaultSize,
        comboChoices,
        wxCB_DROPDOWN | wxCB_READONLY);

    hSizer2->Add(mDirectoriesComboBox, 1, 0);

    hSizer2->AddSpacer(10);



    vSizer->Add(hSizer2, 0, wxEXPAND);

    vSizer->AddSpacer(10);


    //
    // Buttons
    //

    wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton * okButton = new wxButton(this, wxID_OK, "Load");
    buttonsSizer->Add(okButton, 0);

    buttonsSizer->AddSpacer(20);

    wxButton * cancelButton = new wxButton(this, wxID_CANCEL);
    buttonsSizer->Add(cancelButton, 0);

    buttonsSizer->AddSpacer(20);

    vSizer->Add(buttonsSizer, 0, wxALIGN_RIGHT);

    vSizer->AddSpacer(10);


    SetSizer(vSizer);
}

ShipLoadDialog::~ShipLoadDialog()
{
}

std::optional<std::filesystem::path> ShipLoadDialog::Open()
{
    this->Show();

    // TODO: return member, which is set by event handler for fsEVT_SHIP_FILE_SELECTED (decl'd by ShipPreviewControl)
    return std::nullopt;
}

void ShipLoadDialog::OnDirectorySelected(wxCommandEvent & /*event*/)
{
    std::filesystem::path selectedDirPath(mDirCtrl->GetPath().ToStdString());

    mShipPreviewPanel->SetDirectory(selectedDirPath);
}