/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-07-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "HelpDialog.h"

#include <wx/button.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/sizer.h>
#include <wx/wxhtml.h>

HelpDialog::HelpDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager,
    LocalizationManager const & localizationManager)
    : wxDialog(parent, wxID_ANY, _("Help"))
{
    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    wxHtmlWindow *html = new wxHtmlWindow(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(640, 800),
        wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION);

    html->SetBorders(0);
    html->LoadPage(
        gameAssetManager.GetHelpFilePath(
            localizationManager.GetEnforcedLanguageIdentifier(),
            localizationManager.GetDefaultLanguageIdentifier()).string());
    html->SetSize(
        html->GetInternalRepresentation()->GetWidth(),
        html->GetInternalRepresentation()->GetHeight());

    topSizer->Add(html, 1, wxALL, 10);

    this->SetSizerAndFit(topSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

HelpDialog::~HelpDialog()
{
}