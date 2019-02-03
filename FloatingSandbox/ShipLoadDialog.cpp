/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLoadDialog.h"

#include <GameCore/Log.h>

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
		wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP,
		_T("Load Ship Dialog"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    Centre();


    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Directory and preview
    //

    wxBoxSizer * hSizer1 = new wxBoxSizer(wxHORIZONTAL);



    // Directory

    assert(!mUIPreferences->GetShipLoadDirectories().empty());

    mDirCtrl = new wxGenericDirCtrl(
        this,
        wxID_ANY,
        mUIPreferences->GetShipLoadDirectories().front().string(),
        wxDefaultPosition,
        wxSize(DirColumnWidth, -1),
        wxDIRCTRL_DIR_ONLY);

    Connect(mDirCtrl->GetId(), wxEVT_DIRCTRL_SELECTIONCHANGED, (wxObjectEventFunction)&ShipLoadDialog::OnDirCtrlDirSelected);

    hSizer1->Add(mDirCtrl, 0, wxEXPAND | wxALIGN_TOP);


    // Preview

    mShipPreviewPanel = new ShipPreviewPanel(this, resourceLoader);

    mShipPreviewPanel->Bind(fsEVT_SHIP_FILE_SELECTED, &ShipLoadDialog::OnShipFileSelected, this);
    mShipPreviewPanel->Bind(fsEVT_SHIP_FILE_CHOSEN, &ShipLoadDialog::OnShipFileChosen, this);

    hSizer1->Add(mShipPreviewPanel, 1, wxALIGN_TOP | wxEXPAND);




    vSizer->Add(hSizer1, 1, wxEXPAND);

    vSizer->AddSpacer(10);



    //
    // Directories combo
    //

    wxBoxSizer * hSizer2 = new wxBoxSizer(wxHORIZONTAL);

    hSizer2->AddSpacer(10);

    wxArrayString comboChoices;
    mDirectoriesComboBox = new wxComboBox(
        this,
        wxID_ANY,
        "",
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

    mLoadButton = new wxButton(this, wxID_ANY, "Load");
    mLoadButton->Bind(wxID_ANY, &ShipLoadDialog::OnLoadButton, this);
    buttonsSizer->Add(mLoadButton, 0);

    buttonsSizer->AddSpacer(20);

    wxButton * cancelButton = new wxButton(this, wxID_ANY, "Cancel");
    cancelButton->Bind(wxID_ANY, &ShipLoadDialog::OnCancelButton, this);
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
    // Reset our current selection
    mSelectedShipFilepath.reset();

    // Disable load button
    mLoadButton->Enable(true);


    //
    // Load settings from preferences
    //

    assert(!mUIPreferences->GetShipLoadDirectories().empty());

    bool isFirst = true;
    mDirectoriesComboBox->Clear();
    for (auto dir : mUIPreferences->GetShipLoadDirectories())
    {
        if (std::filesystem::exists(dir))
        {
            mDirectoriesComboBox->Append(dir.string());

            if (isFirst)
            {
                // Set in dir tree
                mDirCtrl->SetPath(dir.string());

                // Set in preview
                mShipPreviewPanel->SetDirectory(dir.string());

                isFirst = false;
            }
        }
    }



    //
    // Open dialog
    //

    mLoadButton->Enable(false);

    mShipPreviewPanel->OnOpen();

    auto ret = this->ShowModal();

    mShipPreviewPanel->OnClose();

    if (ret == wxID_OK)
    {
        // TODOTEST
        LogMessage("ShipLoadDialog::Open: return=", !!mSelectedShipFilepath ? "<none>" : *mSelectedShipFilepath);


        //
        // Return user choice, if any
        //

        if (!!mSelectedShipFilepath)
        {
            mUIPreferences->AddShipLoadDirectory(*mSelectedShipFilepath);
        }

        return mSelectedShipFilepath;
    }
    else
    {
        return std::nullopt;
    }
}

void ShipLoadDialog::OnDirCtrlDirSelected(wxCommandEvent & /*event*/)
{
    std::filesystem::path selectedDirPath(mDirCtrl->GetPath().ToStdString());

    OnDirectorySelected(selectedDirPath);
}

void ShipLoadDialog::OnShipFileSelected(fsShipFileSelectedEvent & event)
{
    // Store selection
    mSelectedShipFilepath = event.GetShipFilepath();

    // Enable load button
    mLoadButton->Enable(true);
}

void ShipLoadDialog::OnShipFileChosen(fsShipFileChosenEvent & event)
{
    // Store selection
    mSelectedShipFilepath = event.GetShipFilepath();

    // Close
    this->EndModal(wxID_OK);
}

void ShipLoadDialog::OnDirectorySelected(std::filesystem::path directoryPath)
{
    // Reset our current selection
    mSelectedShipFilepath.reset();

    // Disable load button
    mLoadButton->Enable(true);

    // Propagate to preview panel
    mShipPreviewPanel->SetDirectory(directoryPath);
}