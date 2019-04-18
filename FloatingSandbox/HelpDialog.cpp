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
#include <wx/statline.h>
#include <wx/wxhtml.h>

HelpDialog::HelpDialog(
    wxWindow * parent,
    ResourceLoader const & resourceLoader)
    : wxDialog(parent, wxID_ANY, wxString(_("Help")))
{
    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    wxHtmlWindow *html = new wxHtmlWindow(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(640, 800),
        wxHW_SCROLLBAR_AUTO | wxHW_NO_SELECTION);

    html->SetBorders(0);
    html->LoadPage(resourceLoader.GetHelpFilepath().string());
    html->SetSize(
        html->GetInternalRepresentation()->GetWidth(),
        html->GetInternalRepresentation()->GetHeight());

    topSizer->Add(html, 1, wxALL, 10);

#if wxUSE_STATLINE
    topSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
#endif // wxUSE_STATLINE

    wxButton * okButton = new wxButton(this, wxID_OK, _("OK"));
    okButton->SetDefault();

    topSizer->Add(okButton, 0, wxALL | wxALIGN_RIGHT, 15);

    this->SetSizer(topSizer);
    topSizer->Fit(this);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

HelpDialog::~HelpDialog()
{
}
