/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLoadDialog.h"

#include "ShipDescriptionDialog.h"

#include <GameCore/Log.h>

#include <wx/sizer.h>

constexpr int MinDirCtrlWidth = 260;

ShipLoadDialog::ShipLoadDialog(
    wxWindow * parent,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    ResourceLoader const & resourceLoader)
	: mParent(parent)
    , mUIPreferencesManager(std::move(uiPreferencesManager))
{
	Create(
		mParent,
		wxID_ANY,
		_("Load Ship"),
		wxDefaultPosition,
        wxDefaultSize,
		wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP,
		_T("Load Ship Dialog"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    Bind(wxEVT_CLOSE_WINDOW, &ShipLoadDialog::OnCloseWindow, this);



    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);



    //
    // Directory tree and preview
    //

    {
        wxBoxSizer * hSizer1 = new wxBoxSizer(wxHORIZONTAL);


        // Directory tree

        assert(!mUIPreferencesManager->GetShipLoadDirectories().empty());

        mDirCtrl = new wxGenericDirCtrl(
            this,
            wxID_ANY,
            mUIPreferencesManager->GetShipLoadDirectories().front().string(),
            wxDefaultPosition,
            wxSize(MinDirCtrlWidth, 500),
            wxDIRCTRL_DIR_ONLY);

        mDirCtrl->ShowHidden(true); // When installing per-user, the Ships folder is under AppData, which is hidden
        mDirCtrl->SetMinSize(wxSize(MinDirCtrlWidth, 500));
        mDirCtrl->Bind(wxEVT_DIRCTRL_SELECTIONCHANGED, (wxObjectEventFunction)&ShipLoadDialog::OnDirCtrlDirSelected, this);

        hSizer1->Add(mDirCtrl, 0, wxEXPAND | wxALIGN_TOP);


        // Preview

        mShipPreviewPanel = new ShipPreviewPanel(this, resourceLoader);

        mShipPreviewPanel->Bind(fsEVT_SHIP_FILE_SELECTED, &ShipLoadDialog::OnShipFileSelected, this);
        mShipPreviewPanel->Bind(fsEVT_SHIP_FILE_CHOSEN, &ShipLoadDialog::OnShipFileChosen, this);
        mShipPreviewPanel->Bind(fsEVT_DIR_PREVIEW_COMPLETE, &ShipLoadDialog::OnDirectoryPreviewComplete, this);

        hSizer1->Add(mShipPreviewPanel, 1, wxALIGN_TOP | wxEXPAND);


        vSizer->Add(hSizer1, 1, wxEXPAND);
    }

    vSizer->AddSpacer(10);



    //
    // Recent directories combo and home button, and ship search box
    //

    {
        // |  | Label       |   | Label   | |
        // |  | Combo, Home |   | TextBox | |

        wxFlexGridSizer * gridSizer = new wxFlexGridSizer(2, 5, 0, 0);

        //
        // ROW 1
        //

        gridSizer->AddSpacer(10);

        {
            wxStaticText * recentDirsLabel = new wxStaticText(this, wxID_ANY, "Recent directories:");
            gridSizer->Add(recentDirsLabel, 4, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);

        {
            wxStaticText * searchLabel = new wxStaticText(this, wxID_ANY, "Search in this folder:");
            gridSizer->Add(searchLabel, 1, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);


        //
        // ROW 2
        //

        gridSizer->AddSpacer(10);

        {
            wxBoxSizer * hComboSizer = new wxBoxSizer(wxHORIZONTAL);

            // Combo

            wxArrayString emptyComboChoices;
            mRecentDirectoriesComboBox = new wxComboBox(
                this,
                wxID_ANY,
                "",
                wxDefaultPosition,
                wxDefaultSize,
                emptyComboChoices,
                wxCB_DROPDOWN | wxCB_READONLY);
            mRecentDirectoriesComboBox->Bind(wxEVT_COMBOBOX, &ShipLoadDialog::OnRecentDirectorySelected, this);
            hComboSizer->Add(mRecentDirectoriesComboBox, 1, wxEXPAND);

            hComboSizer->AddSpacer(4);

            // HomeDir button

            wxButton * homeDirButton = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(24, -1));
            wxBitmap homeBitmap(resourceLoader.GetIconFilepath("home").string(), wxBITMAP_TYPE_PNG);
            homeDirButton->SetBitmap(homeBitmap);
            homeDirButton->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&ShipLoadDialog::OnHomeDirButtonClicked, this);
            hComboSizer->Add(homeDirButton, 0, 0);

            gridSizer->Add(hComboSizer, 4, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);

        {
            // Search TextCtrl

            mShipSearchTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                "",
                wxDefaultPosition,
                wxDefaultSize,
                wxTE_PROCESS_ENTER);

            mShipSearchTextCtrl->Enable(false);

            mShipSearchTextCtrl->Bind(wxEVT_TEXT, &ShipLoadDialog::OnShipSearchTextCtrlText, this);
            mShipSearchTextCtrl->Bind(wxEVT_TEXT_ENTER, &ShipLoadDialog::OnShipSearchTextCtrlTextEnter, this);

            gridSizer->Add(mShipSearchTextCtrl, 1, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);


        // TODOOLD
        /*

        {
            wxBoxSizer * vSizer2 = new wxBoxSizer(wxVERTICAL);

            wxStaticText * recentDirsLabel = new wxStaticText(this, wxID_ANY, "Recent directories:");
            vSizer2->Add(recentDirsLabel, 0, wxALIGN_LEFT);

            wxBoxSizer * hComboSizer = new wxBoxSizer(wxHORIZONTAL);

            // Combo

            wxArrayString emptyComboChoices;
            mRecentDirectoriesComboBox = new wxComboBox(
                this,
                wxID_ANY,
                "",
                wxDefaultPosition,
                wxDefaultSize,
                emptyComboChoices,
                wxCB_DROPDOWN | wxCB_READONLY);
            mRecentDirectoriesComboBox->Bind(wxEVT_COMBOBOX, &ShipLoadDialog::OnRecentDirectorySelected, this);
            hComboSizer->Add(mRecentDirectoriesComboBox, 1, wxEXPAND);

            hComboSizer->AddSpacer(4);

            // HomeDir button

            wxButton * homeDirButton = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(24, -1));
            wxBitmap homeBitmap(resourceLoader.GetIconFilepath("home").string(), wxBITMAP_TYPE_PNG);
            homeDirButton->SetBitmap(homeBitmap);
            homeDirButton->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&ShipLoadDialog::OnHomeDirButtonClicked, this);
            hComboSizer->Add(homeDirButton, 0, 0);

            vSizer2->Add(hComboSizer, 0, wxEXPAND);

            hSizer2->Add(vSizer2, 4, 0);
        }

        hSizer2->AddSpacer(10);

        {
            wxBoxSizer * vSizer2 = new wxBoxSizer(wxVERTICAL);

            wxStaticText * searchLabel = new wxStaticText(this, wxID_ANY, "Search in this folder:");
            vSizer2->Add(searchLabel, 0, wxALIGN_LEFT);

            mShipSearchTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                "",
                wxDefaultPosition,
                wxDefaultSize,
                0);
            mShipSearchTextCtrl->Bind(wxEVT_TEXT, &ShipLoadDialog::OnShipSearchTextCtrlText, this);

            vSizer2->Add(mShipSearchTextCtrl, 0, wxEXPAND | wxALIGN_LEFT);

            hSizer2->Add(vSizer2, 1, 0);
        }

        hSizer2->AddSpacer(10);

        vSizer->Add(hSizer2, 0, wxEXPAND);
        */

        vSizer->Add(gridSizer, 0, wxEXPAND | wxALL);
    }

    vSizer->AddSpacer(10);



    //
    // Buttons
    //

    {

        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(10);

        mInfoButton = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(24, -1));
        wxBitmap infoBitmap(resourceLoader.GetIconFilepath("info").string(), wxBITMAP_TYPE_PNG);
        mInfoButton->SetBitmap(infoBitmap);
        mInfoButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnInfoButtonClicked, this);
        buttonsSizer->Add(mInfoButton, 0);

        buttonsSizer->AddStretchSpacer(1);

        mLoadButton = new wxButton(this, wxID_ANY, "Load");
        mLoadButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnLoadButton, this);
        buttonsSizer->Add(mLoadButton, 0);

        buttonsSizer->AddSpacer(20);

        wxButton * cancelButton = new wxButton(this, wxID_ANY, "Cancel");
        cancelButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnCancelButton, this);
        buttonsSizer->Add(cancelButton, 0);

        buttonsSizer->AddSpacer(10);

        vSizer->Add(buttonsSizer, 0, wxEXPAND);
    }

    vSizer->AddSpacer(10);


    //
    // Finalize layout
    //

    SetSizerAndFit(vSizer);

    // Size so that we have 3 columns of previews right away
    constexpr int Width = MinDirCtrlWidth + ShipPreviewPanel::MinPreviewWidth * 3 + 20;
    SetSize(wxSize(Width, 600 * Width / 800));

    Centre();
}

ShipLoadDialog::~ShipLoadDialog()
{
}

void ShipLoadDialog::Open()
{
    if (!IsShown())
    {
        // Reset our current ship selection
        mSelectedShipMetadata.reset();
        mSelectedShipFilepath.reset();

        // Disable controls
        mShipSearchTextCtrl->Enable(false);
        mInfoButton->Enable(false);
        mLoadButton->Enable(false);


        //
        // Load settings from preferences, if needed
        //

        if (mRecentDirectoriesComboBox->GetCount() == 0)
        {
            RepopulateRecentDirectoriesComboBox();

            assert(mRecentDirectoriesComboBox->GetCount() > 0);

            // Set the first one everywhere
            auto dir = mRecentDirectoriesComboBox->GetStrings().front();
            mDirCtrl->SetPath(dir);
            mRecentDirectoriesComboBox->SetValue(dir);
            mShipPreviewPanel->SetDirectory(dir.ToStdString());
        }


        //
        // Open dialog
        //

        mShipPreviewPanel->OnOpen();

        auto selectedPath = mDirCtrl->GetPath();
        if (!selectedPath.IsEmpty())
            mShipPreviewPanel->SetDirectory(std::filesystem::path(selectedPath.ToStdString()));

        Show(true);
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
    if (!!event.GetShipMetadata())
        mSelectedShipMetadata.emplace(*event.GetShipMetadata());
    else
        mSelectedShipMetadata.reset();
    mSelectedShipFilepath = event.GetShipFilepath();

    // Enable buttons
    mInfoButton->Enable(!!(event.GetShipMetadata()) && !!(event.GetShipMetadata()->Description));
    mLoadButton->Enable(true);

    // Continue processing
    event.Skip();
}

void ShipLoadDialog::OnShipFileChosen(fsShipFileChosenEvent & event)
{
    // Store selection
    mSelectedShipFilepath = event.GetShipFilepath();

    // Process
    OnShipFileChosen(*mSelectedShipFilepath);

    // Continue processing
    event.Skip();
}

void ShipLoadDialog::OnDirectoryPreviewComplete(fsDirPreviewCompleteEvent & event)
{
    mShipSearchTextCtrl->Enable(true);

    // Continue processing
    event.Skip();
}

void ShipLoadDialog::OnRecentDirectorySelected(wxCommandEvent & /*event*/)
{
    mDirCtrl->SetPath(mRecentDirectoriesComboBox->GetValue()); // Will send its own event
}

void ShipLoadDialog::OnShipSearchTextCtrlText(wxCommandEvent & event)
{
    mShipPreviewPanel->Search(event.GetString().ToStdString());
}

void ShipLoadDialog::OnShipSearchTextCtrlTextEnter(wxCommandEvent & /*event*/)
{
    mShipPreviewPanel->ChooseSearched();
}

void ShipLoadDialog::OnHomeDirButtonClicked(wxCommandEvent & /*event*/)
{
    assert(mUIPreferencesManager->GetShipLoadDirectories().size() >= 1);

    // Change combo
    mRecentDirectoriesComboBox->Select(0);

    // Change dir tree
    mDirCtrl->SetPath(mRecentDirectoriesComboBox->GetValue()); // Will send its own event
}

void ShipLoadDialog::OnInfoButtonClicked(wxCommandEvent & /*event*/)
{
    assert(!!mSelectedShipMetadata);

    if (!!(mSelectedShipMetadata->Description))
    {
        ShipDescriptionDialog shipDescriptionDialog(
            this,
            *mSelectedShipMetadata,
            false,
            mUIPreferencesManager);

        shipDescriptionDialog.ShowModal();
    }
}

void ShipLoadDialog::OnLoadButton(wxCommandEvent & /*event*/)
{
    assert(!!mSelectedShipFilepath);

    // Process
    OnShipFileChosen(*mSelectedShipFilepath);
}

void ShipLoadDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    Close();
}

void ShipLoadDialog::OnCloseWindow(wxCloseEvent & /*event*/)
{
    Close();
}

void ShipLoadDialog::OnDirectorySelected(std::filesystem::path directoryPath)
{
    // Reset our current selection
    mSelectedShipMetadata.reset();
    mSelectedShipFilepath.reset();

    // Disable controls
    mShipSearchTextCtrl->Enable(false);
    mInfoButton->Enable(false);
    mLoadButton->Enable(false);

    // Propagate to preview panel
    mShipPreviewPanel->SetDirectory(directoryPath);
}

void ShipLoadDialog::OnShipFileChosen(std::filesystem::path shipFilepath)
{
    LogMessage("ShipLoadDialog::OnShipFileChosen: ", shipFilepath);

    // Close ourselves
    Close();

    // Store directory in preferences
    auto dir = shipFilepath.parent_path();
    mUIPreferencesManager->AddShipLoadDirectory(dir);

    // Re-populate combo box
    RepopulateRecentDirectoriesComboBox();

    // Select this directory in the combo box
    mRecentDirectoriesComboBox->SetValue(dir.string());

    // Fire select event
    auto event = fsShipFileChosenEvent(
        fsEVT_SHIP_FILE_CHOSEN,
        this->GetId(),
        shipFilepath);

    ProcessWindowEvent(event);
}

void ShipLoadDialog::Close()
{
    mShipPreviewPanel->OnClose();

    // We just hide ourselves, so we can re-show ourselves again
    this->Hide();
}

void ShipLoadDialog::RepopulateRecentDirectoriesComboBox()
{
    assert(!mUIPreferencesManager->GetShipLoadDirectories().empty());

    mRecentDirectoriesComboBox->Clear();
    for (auto dir : mUIPreferencesManager->GetShipLoadDirectories())
    {
        if (std::filesystem::exists(dir))
        {
            mRecentDirectoriesComboBox->Append(dir.string());
        }
    }
}