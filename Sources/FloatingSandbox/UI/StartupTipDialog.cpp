/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StartupTipDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/wxhtml.h>

StartupTipDialog::StartupTipDialog(
    wxWindow * parent,
    UIPreferencesManager & uiPreferencesManager,
    GameAssetManager const & gameAssetManager,
    LocalizationManager const & localizationManager)
    : wxDialog(parent, wxID_ANY, _("Welcome!"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
    , mUIPreferencesManager(uiPreferencesManager)
{
    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    {
        wxHtmlWindow *html = new wxHtmlWindow(
            this,
            wxID_ANY,
            wxDefaultPosition,
            wxSize(480, 270),
            wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION);

        html->SetBorders(0);
        html->LoadPage(
            gameAssetManager.GetStartupTipFilePath(
                localizationManager.GetEnforcedLanguageIdentifier(),
                localizationManager.GetDefaultLanguageIdentifier()).string());
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
            wxCheckBox * dontChk = new wxCheckBox(this, wxID_ANY, _("Don't show this tip again"));
            dontChk->SetToolTip(_("Prevents these tips from being shown each time the game starts. You can always change this setting later from the \"Game Preferences\" window."));
            dontChk->SetValue(false);
            dontChk->Bind(wxEVT_CHECKBOX, &StartupTipDialog::OnDontShowAgainCheckboxChanged, this);

            rowSizer->Add(dontChk, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        }

        rowSizer->AddStretchSpacer(1);

        {
            wxButton * okButton = new wxButton(this, wxID_OK, _("OK"));
            okButton->SetDefault();

            rowSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        }

        topSizer->Add(rowSizer, 0, wxEXPAND);
    }

    this->SetSizerAndFit(topSizer);

    Centre(wxBOTH);
}

StartupTipDialog::~StartupTipDialog()
{
}

void StartupTipDialog::OnDontShowAgainCheckboxChanged(wxCommandEvent & event)
{
    mUIPreferencesManager.SetShowStartupTip(!event.IsChecked());
}