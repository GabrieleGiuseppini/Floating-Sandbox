/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-06-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "NewVersionDisplayDialog.h"

#include <Game/GameVersion.h>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>

#include <sstream>

NewVersionDisplayDialog::NewVersionDisplayDialog(
    wxWindow* parent,
    Version const & version,
    std::string const & htmlFeatures,
    UIPreferencesManager * uiPreferencesManager)
    : wxDialog(parent, wxID_ANY, _("A New Version Is Available!"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
    , mVersion(version)
    , mUIPreferencesManager(uiPreferencesManager)
{
    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    {
        wxHtmlWindow *html = new wxHtmlWindow(
            this,
            wxID_ANY,
            wxDefaultPosition,
            wxSize(800, 240),
            wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION);

        html->SetBorders(0);
        html->SetPage(MakeHtml(version, htmlFeatures));

        topSizer->Add(html, 1, wxALL, 10);
    }

#if wxUSE_STATLINE
    topSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
#endif // wxUSE_STATLINE

    {
        wxButton * goToDownloadPageButton = new wxButton(this, wxID_ANY, _("Go to the Download Page!"), wxDefaultPosition, wxDefaultSize);
        goToDownloadPageButton->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&NewVersionDisplayDialog::OnGoToDownloadPageButtonClicked, this);

        topSizer->Add(goToDownloadPageButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    }

    if (nullptr != uiPreferencesManager)
    {
        {
            wxCheckBox * dontNotifyChk = new wxCheckBox(this, wxID_ANY, _("Don't notify about this version again"));
            wxString message;
            message.Printf(_("Prevents the automatic update check from notifying about version %s again."), version.ToString());
            dontNotifyChk->SetToolTip(message);
            dontNotifyChk->SetValue(false);
            dontNotifyChk->Bind(wxEVT_CHECKBOX, &NewVersionDisplayDialog::OnDoNotNotifyAboutThisVersionAgainCheckboxChanged, this);

            topSizer->Add(dontNotifyChk, 0, wxALL | wxALIGN_LEFT, 6);
        }

        {
            wxCheckBox * dontCheckChk = new wxCheckBox(this, wxID_ANY, _("Don't check for updates at startup"));
            dontCheckChk->SetToolTip(_("Prevents the automatic update check from running at startup."));
            dontCheckChk->SetValue(false);
            dontCheckChk->Bind(wxEVT_CHECKBOX, &NewVersionDisplayDialog::OnDoNotCheckForUpdatesAtStartupCheckboxChanged, this);

            topSizer->Add(dontCheckChk, 0, wxALL | wxALIGN_LEFT, 6);
        }
    }

    this->SetSizerAndFit(topSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

NewVersionDisplayDialog::~NewVersionDisplayDialog()
{
}

std::string NewVersionDisplayDialog::MakeHtml(
    Version const & version,
    std::string const & htmlFeatures)
{
    std::stringstream ss;

    ss << R"(
<html><body>
<table cellpadding="3" cellspacing="0" width="100%">
<tr>
    <td align="center">
    <font size=+1><b>Version )";

    ss << version.ToMajorMinorPatchString();

    ss << R"( is now available!</b></font>
    </td>
</tr>)";


    ss  << "<tr><td>"
        << htmlFeatures
        << "</td></tr>";


    ss << R"(</table></body></html>)";

    return ss.str();
}

void NewVersionDisplayDialog::OnGoToDownloadPageButtonClicked(wxCommandEvent & /*event*/)
{
    wxLaunchDefaultBrowser(APPLICATION_DOWNLOAD_URL);
}

void NewVersionDisplayDialog::OnDoNotNotifyAboutThisVersionAgainCheckboxChanged(wxCommandEvent & event)
{
    assert(nullptr != mUIPreferencesManager);

    if (event.IsChecked())
    {
        mUIPreferencesManager->AddUpdateToBlacklist(mVersion);
    }
    else
    {
        mUIPreferencesManager->RemoveUpdateFromBlacklist(mVersion);
    }
}

void NewVersionDisplayDialog::OnDoNotCheckForUpdatesAtStartupCheckboxChanged(wxCommandEvent & event)
{
    assert(nullptr != mUIPreferencesManager);

    mUIPreferencesManager->SetCheckUpdatesAtStartup(!event.IsChecked());
}