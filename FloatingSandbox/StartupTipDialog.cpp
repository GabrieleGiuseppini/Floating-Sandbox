/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StartupTipDialog.h"

#include <wx/button.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/wxhtml.h>

StartupTipDialog::StartupTipDialog(
    wxWindow * parent,
    std::shared_ptr<UIPreferences> uiPreferences,
    ResourceLoader const & resourceLoader)
    : wxDialog(parent, wxID_ANY, wxString(_("Welcome!")))
    , mUIPreferences(std::move(uiPreferences))
{
    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    {
        wxHtmlWindow *html = new wxHtmlWindow(
            this,
            wxID_ANY,
            wxDefaultPosition,
            wxSize(480, 240),
            wxHW_SCROLLBAR_AUTO);

        html->SetBorders(0);
        html->LoadPage(resourceLoader.GetStartupTipFilepath().string());
        html->SetSize(
            html->GetInternalRepresentation()->GetWidth(),
            html->GetInternalRepresentation()->GetHeight());

        topSizer->Add(html, 1, wxALL, 10);
    }

#if wxUSE_STATLINE
    topSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
#endif // wxUSE_STATLINE

    {
        wxBoxSizer * rowSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            wxCheckBox * dontChk = new wxCheckBox(this, wxID_ANY, "Don't show this tip on startup again");
            dontChk->SetValue(false);
            dontChk->Bind(wxEVT_CHECKBOX, &StartupTipDialog::OnDontShowAgainCheckboxChanged, this);

            rowSizer->Add(dontChk, 0, wxALL | wxALIGN_CENTER_VERTICAL, 15);
        }

        rowSizer->AddStretchSpacer(1);

        {
            wxButton * okButton = new wxButton(this, wxID_OK, _("OK"));
            okButton->SetDefault();

            rowSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 15);
        }

        topSizer->Add(rowSizer);
    }

    this->SetSizerAndFit(topSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

StartupTipDialog::~StartupTipDialog()
{
}

void StartupTipDialog::OnDontShowAgainCheckboxChanged(wxCommandEvent & event)
{
    mUIPreferences->SetShowStartupTip(!event.IsChecked());
}