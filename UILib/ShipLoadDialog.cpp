/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLoadDialog.h"

#include <GameCore/Log.h>

#include <UILib/ShipDescriptionDialog.h>

#include <wx/sizer.h>

constexpr int MinDirCtrlWidth = 260;
constexpr int MaxDirComboWidth = 650;

ShipLoadDialog::ShipLoadDialog(
    wxWindow * parent,
    std::vector<std::filesystem::path> const & shipLoadDirectories,
    ResourceLocator const & resourceLocator)
	: mParent(parent)
    , mShipLoadDirectories(shipLoadDirectories)
    , mResourceLocator(resourceLocator)
{
	Create(
		mParent,
		wxID_ANY,
        _("Load Ship"),
		wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX | wxFRAME_SHAPED
#if !defined(_DEBUG) || !defined(_WIN32)
            | wxSTAY_ON_TOP
#endif
            ,
        wxS("Load Ship Dialog"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    Bind(wxEVT_CLOSE_WINDOW, &ShipLoadDialog::OnCloseWindow, this);



    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Directory tree and preview
    //

    {
        wxBoxSizer * hSizer1 = new wxBoxSizer(wxHORIZONTAL);

        // Directory tree
        {
            assert(!mShipLoadDirectories.empty());

            LogMessage("ShipLoadDialog::cctor(): creating wxGenericDirCtrl...");

            auto const minSize = wxSize(MinDirCtrlWidth, 680);

            mDirCtrl = new wxGenericDirCtrl(
                this,
                wxID_ANY,
                mShipLoadDirectories.front().string(),
                wxDefaultPosition,
                minSize,
                wxDIRCTRL_DIR_ONLY);

            LogMessage("ShipLoadDialog::cctor(): ...wxGenericDirCtrl created.");

            mDirCtrl->ShowHidden(true); // When installing per-user, the Ships folder is under AppData, which is hidden
            mDirCtrl->SetMinSize(minSize);
            mDirCtrl->Bind(wxEVT_DIRCTRL_SELECTIONCHANGED, (wxObjectEventFunction)&ShipLoadDialog::OnDirCtrlDirSelected, this);

            hSizer1->Add(mDirCtrl, 0, wxEXPAND | wxALIGN_TOP);
        }


        // Preview
        {
            mShipPreviewWindow = new ShipPreviewWindow(this, mResourceLocator);

            mShipPreviewWindow->SetMinSize(wxSize(ShipPreviewWindow::CalculateMinWidthForColumns(3) + 40, -1));
            mShipPreviewWindow->Bind(fsEVT_SHIP_FILE_SELECTED, &ShipLoadDialog::OnShipFileSelected, this);
            mShipPreviewWindow->Bind(fsEVT_SHIP_FILE_CHOSEN, &ShipLoadDialog::OnShipFileChosen, this);

            hSizer1->Add(mShipPreviewWindow, 1, wxALIGN_TOP | wxEXPAND);
        }

        vSizer->Add(hSizer1, 1, wxEXPAND);
    }

    vSizer->AddSpacer(10);



    //
    // Recent directories combo and home button, and ship search box
    //

    {
        // |  | Label       |   | Label            | |
        // |  | Combo, Home |   | SearchBox [Next] | |

        wxFlexGridSizer * gridSizer = new wxFlexGridSizer(2, 5, 0, 0);

        gridSizer->AddGrowableCol(1, 4);
        gridSizer->AddGrowableCol(3, 1);

        //
        // ROW 1
        //

        gridSizer->AddSpacer(10);

        {
            wxStaticText * recentDirsLabel = new wxStaticText(this, wxID_ANY, _("Recent directories:"));
            gridSizer->Add(recentDirsLabel, 4, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);

        {
            wxStaticText * searchLabel = new wxStaticText(this, wxID_ANY, _("Search in this folder:"));
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
                wxEmptyString,
                wxDefaultPosition,
                wxDefaultSize,
                emptyComboChoices,
                wxCB_DROPDOWN | wxCB_READONLY);

			mRecentDirectoriesComboBox->SetMaxSize(wxSize(MaxDirComboWidth, -1));
            mRecentDirectoriesComboBox->Bind(wxEVT_COMBOBOX, &ShipLoadDialog::OnRecentDirectorySelected, this);

            hComboSizer->Add(mRecentDirectoriesComboBox, 1, wxALIGN_CENTRE_VERTICAL);

            hComboSizer->AddSpacer(4);

            // HomeDir button

            wxBitmap homeBitmap(mResourceLocator.GetIconFilePath("home").string(), wxBITMAP_TYPE_PNG);
            wxBitmapButton * homeDirButton = new wxBitmapButton(this, wxID_ANY, homeBitmap, wxDefaultPosition, wxDefaultSize);
            homeDirButton->SetToolTip(_("Go to the default Ships folder"));
            homeDirButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnHomeDirButtonClicked, this);

            hComboSizer->Add(homeDirButton, 0, wxALIGN_CENTRE_VERTICAL);

            gridSizer->Add(hComboSizer, 1, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);

        {
            wxBoxSizer * hSearchSizer = new wxBoxSizer(wxHORIZONTAL);

            // Search box

            mShipSearchCtrl = new wxSearchCtrl(
                this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(-1, 24),
                0);

			mShipSearchCtrl->ShowCancelButton(true);
            mShipSearchCtrl->Bind(wxEVT_TEXT, &ShipLoadDialog::OnShipSearchCtrlText, this);
            mShipSearchCtrl->Bind(wxEVT_SEARCHCTRL_SEARCH_BTN, &ShipLoadDialog::OnShipSearchCtrlSearchBtn, this);
            mShipSearchCtrl->Bind(wxEVT_SEARCHCTRL_CANCEL_BTN, &ShipLoadDialog::OnShipSearchCtrlCancelBtn, this);

            hSearchSizer->Add(mShipSearchCtrl, 1, wxALIGN_CENTRE_VERTICAL);

			// Search button

            wxBitmap searchNextBitmap(mResourceLocator.GetIconFilePath("right_arrow").string(), wxBITMAP_TYPE_PNG);
            mSearchNextButton = new wxBitmapButton(this, wxID_ANY, searchNextBitmap, wxDefaultPosition, wxDefaultSize);
            mSearchNextButton->SetToolTip(_("Go to the next search result"));
            mSearchNextButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnSearchNextButtonClicked, this);

            hSearchSizer->Add(mSearchNextButton, 0, wxALIGN_CENTRE_VERTICAL);

            gridSizer->Add(hSearchSizer, 1, wxALIGN_LEFT | wxEXPAND | wxALL);
        }

        gridSizer->AddSpacer(10);

        vSizer->Add(gridSizer, 0, wxEXPAND | wxALL);
    }

    vSizer->AddSpacer(10);



    //
    // Buttons
    //

    {

        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(10);

        wxBitmap infoBitmap(mResourceLocator.GetIconFilePath("info").string(), wxBITMAP_TYPE_PNG);
        mInfoButton = new wxBitmapButton(this, wxID_ANY, infoBitmap, wxDefaultPosition, wxDefaultSize);
        mInfoButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnInfoButtonClicked, this);
        buttonsSizer->Add(mInfoButton, 0);

        buttonsSizer->AddStretchSpacer(1);

        mLoadButton = new wxButton(this, wxID_ANY, _("Load"));
        mLoadButton->Bind(wxEVT_BUTTON, &ShipLoadDialog::OnLoadButton, this);
        buttonsSizer->Add(mLoadButton, 0);

        buttonsSizer->AddSpacer(20);

        wxButton * cancelButton = new wxButton(this, wxID_ANY, _("Cancel"));
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

    int const TotalWidth = MinDirCtrlWidth + mShipPreviewWindow->GetMinWidth() + 10;
    SetSize(wxSize(TotalWidth, 600 * TotalWidth / 800));

    Centre();
}

ShipLoadDialog::~ShipLoadDialog()
{
}

int ShipLoadDialog::ShowModal()
{
    // Reset our current ship selection
    mSelectedShipMetadata.reset();
    mSelectedShipFilepath.reset();
    mChosenShipFilepath.reset();

    // Disable controls
    mInfoButton->Enable(false);
    mLoadButton->Enable(false);

    // Clear search
    mShipSearchCtrl->Clear();
    mSearchNextButton->Enable(false);


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
    }


    //
    // Initialize preview panel
    //

    mShipPreviewWindow->OnOpen();

    auto selectedPath = mDirCtrl->GetPath();
    if (!selectedPath.IsEmpty())
        mShipPreviewWindow->SetDirectory(std::filesystem::path(selectedPath.ToStdString()));

    // Run self as modal
    return wxDialog::ShowModal();
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
}

void ShipLoadDialog::OnShipFileChosen(fsShipFileChosenEvent & event)
{
    // Store selection
    mSelectedShipFilepath = event.GetShipFilepath();

    // Process
    OnShipFileChosen(*mSelectedShipFilepath);

    // Do not continue processing, as OnShipFileChosen() will fire event again
}

void ShipLoadDialog::OnRecentDirectorySelected(wxCommandEvent & /*event*/)
{
    mDirCtrl->SetPath(mRecentDirectoriesComboBox->GetValue()); // Will send its own event
}

void ShipLoadDialog::OnShipSearchCtrlText(wxCommandEvent & /*event*/)
{
    StartShipSearch();
}

void ShipLoadDialog::OnShipSearchCtrlSearchBtn(wxCommandEvent & /*event*/)
{
    mShipPreviewWindow->ChooseSelectedIfAny();
}

void ShipLoadDialog::OnShipSearchCtrlCancelBtn(wxCommandEvent & /*event*/)
{
    mShipSearchCtrl->Clear();
    mSearchNextButton->Enable(false);
}

void ShipLoadDialog::OnSearchNextButtonClicked(wxCommandEvent & /*event*/)
{
    auto searchString = mShipSearchCtrl->GetValue();
    assert(!searchString.IsEmpty());

    mShipPreviewWindow->Search(searchString.ToStdString());
}

void ShipLoadDialog::OnHomeDirButtonClicked(wxCommandEvent & /*event*/)
{
    assert(!mRecentDirectoriesComboBox->IsListEmpty());

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
            mResourceLocator);

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
    EndModal(wxID_CANCEL);
}

void ShipLoadDialog::OnCloseWindow(wxCloseEvent & /*event*/)
{
    // Invoked when the user has tried to close a frame or dialog box
    // using the window manager (X) or system menu (Windows); it can also be invoked by the application itself
    EndModal(wxID_CANCEL);
}

void ShipLoadDialog::OnDirectorySelected(std::filesystem::path directoryPath)
{
    // Reset our current selection
    mSelectedShipMetadata.reset();
    mSelectedShipFilepath.reset();

    // Disable controls
    mInfoButton->Enable(false);
    mLoadButton->Enable(false);

    // Clear search
    mShipSearchCtrl->Clear();
    mSearchNextButton->Enable(false);

    // Propagate to preview panel
    mShipPreviewWindow->SetDirectory(directoryPath);
}

void ShipLoadDialog::OnShipFileChosen(std::filesystem::path shipFilepath)
{
    LogMessage("ShipLoadDialog::OnShipFileChosen: ", shipFilepath);

    // Re-populate combo box
    RepopulateRecentDirectoriesComboBox();

    // Select this directory in the combo box
    mRecentDirectoriesComboBox->SetValue(shipFilepath.parent_path().string());

    // Store path
    mChosenShipFilepath = shipFilepath;

    // End modal dialog
    EndModal(wxID_OK);
}

////////////////////////////////////////////////////////////////////////////

void ShipLoadDialog::EndModal(int retCode)
{
    LogMessage("ShipLoadDialog::EndModal(", retCode, ")");

    mShipPreviewWindow->OnClose();

    wxDialog::EndModal(retCode);
}

void ShipLoadDialog::StartShipSearch()
{
    bool found = false;

    auto searchString = mShipSearchCtrl->GetValue();
    if (!searchString.IsEmpty())
    {
        found = mShipPreviewWindow->Search(searchString.ToStdString());
    }

    mSearchNextButton->Enable(found);
}

void ShipLoadDialog::RepopulateRecentDirectoriesComboBox()
{
    assert(!mShipLoadDirectories.empty());

    mRecentDirectoriesComboBox->Clear();
    for (auto const & dir : mShipLoadDirectories)
    {
        if (std::filesystem::exists(dir))
        {
            mRecentDirectoriesComboBox->Append(dir.string());
        }
    }
}