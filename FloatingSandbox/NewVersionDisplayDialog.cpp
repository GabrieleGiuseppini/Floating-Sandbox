/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-06-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "NewVersionDisplayDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>

#include <sstream>

NewVersionDisplayDialog::NewVersionDisplayDialog(
    wxWindow* parent,
    Version const & version,
    std::vector<std::vector<std::string>> const & features,
    UIPreferencesManager * uiPreferencesManager)
    : wxDialog(parent, wxID_ANY, wxString(_("A New Version Is Available!")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
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
        html->SetPage(MakeHtml(version, features));

        html->Bind(wxEVT_HTML_LINK_CLICKED, &NewVersionDisplayDialog::OnHtmlLinkClicked, this);

        topSizer->Add(html, 1, wxALL, 10);
    }

#if wxUSE_STATLINE
    topSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
#endif // wxUSE_STATLINE

    if (nullptr != uiPreferencesManager)
    {
        {
            wxCheckBox * dontNotifyChk = new wxCheckBox(this, wxID_ANY, "Don't notify about this version again");
            dontNotifyChk->SetToolTip("Prevents the automatic update check from notifying about version " + version.ToString() + " again.");
            dontNotifyChk->SetValue(false);
            dontNotifyChk->Bind(wxEVT_CHECKBOX, &NewVersionDisplayDialog::OnDoNotNotifyAboutThisVersionAgainCheckboxChanged, this);

            topSizer->Add(dontNotifyChk, 0, wxALL | wxALIGN_LEFT, 6);
        }

        {
            wxCheckBox * dontCheckChk = new wxCheckBox(this, wxID_ANY, "Don't check for updates at startup");
            dontCheckChk->SetToolTip("Prevents the automatic update check from running at startup.");
            dontCheckChk->SetValue(false);
            dontCheckChk->Bind(wxEVT_CHECKBOX, &NewVersionDisplayDialog::OnDoNotCheckForUpdatesAtStartupCheckboxChanged, this);

            topSizer->Add(dontCheckChk, 0, wxALL | wxALIGN_LEFT, 6);
        }
    }

    {
        wxButton * okButton = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition, wxSize(100, -1));
        okButton->SetDefault();

        topSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    }

    this->SetSizerAndFit(topSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

NewVersionDisplayDialog::~NewVersionDisplayDialog()
{
}

std::string NewVersionDisplayDialog::MakeHtml(
    Version const & version,
    std::vector<std::vector<std::string>> const & features)
{
    std::stringstream ss;

    ss << R"(
<html><body>
<table cellpadding="3" cellspacing="0" width="100%">
<tr>
    <td align="center">
    <font size=+1><b>Version )";

    ss << version.ToString();

    ss << R"( is now available!</b></font>
    </td>
</tr>)";


    ss << "<tr><td><ul>";

    for (auto const & feature : features)
    {
        ss << "<li>";

        auto sfIt = feature.begin();
        assert(sfIt != feature.end());
        ss << *(sfIt++);

        if (sfIt != feature.end())
        {
            ss << "<ul>";

            for (; sfIt != feature.end(); ++sfIt)
            {
                ss << "<li>" << *sfIt << "</li>";
            }

            ss << "</ul>";
        }

        ss << "</li>";
    }

    ss << "</ul></td></tr>";


    //
    // Download link
    //

    ss << R"(
<tr>
    <td align="center">
        <font size=+1><a href=")";

    ss << APPLICATION_DOWNLOAD_PAGE;

    ss
        << R"(">Click here to download )"
        << version.ToString()
        << R"(!</a></b></font>
    </td>
</tr>)";


    ss << R"(</table></body></html>)";

    return ss.str();
}

void NewVersionDisplayDialog::OnHtmlLinkClicked(wxHtmlLinkEvent & event)
{
    wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
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