/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-03-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipDescriptionDialog.h"

#include <wx/button.h>
#include <wx/image.h>
#include <wx/sizer.h>
#include <wx/wxhtml.h>

#include <sstream>

ShipDescriptionDialog::ShipDescriptionDialog(
    wxWindow* parent,
    ShipMetadata const & shipMetadata,
    bool isAutomatic,
    GameAssetManager const & gameAssetManager)
    : mShowDescriptionsUserPreference(std::nullopt)
{
    Create(
        parent,
        wxID_ANY,
        shipMetadata.ShipName,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SUNKEN | wxSTAY_ON_TOP);

    auto const backgroundBitmap = wxBitmap(
        gameAssetManager.GetBitmapFilePath("ship_description_background").string(),
        wxBITMAP_TYPE_PNG);

    SetBackgroundBitmap(backgroundBitmap);

    //

    wxBoxSizer * topSizer = new wxBoxSizer(wxVERTICAL);

    topSizer->AddSpacer(75);

    {
        wxHtmlWindow *html = new wxHtmlWindow(
            this,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxHW_SCROLLBAR_AUTO | wxBORDER_SUNKEN);

        html->SetBorders(5);

        html->SetPage(MakeHtml(shipMetadata));
        html->SetFonts("Georgia", "");
        html->SetBackgroundColour(wxColour(158, 141, 121));

        topSizer->Add(html, 1, wxALL | wxEXPAND, 10);
    }

    if (isAutomatic)
    {
        wxCheckBox * dontChk = new wxCheckBox(this, wxID_ANY, _("Don't show descriptions when ships are loaded"));
        dontChk->SetForegroundColour(wxColour(79, 63, 49));
        dontChk->SetToolTip(_("Prevents ship descriptions from being shown each time a ship is loaded. You can always change this setting later from the \"Game Preferences\" window."));
        dontChk->SetValue(false);
        dontChk->Bind(
            wxEVT_CHECKBOX,
            [this](wxCommandEvent & event)
            {
                mShowDescriptionsUserPreference = !event.IsChecked();
            });

        topSizer->Add(dontChk, 0, wxLEFT | wxRIGHT | wxALIGN_LEFT, 10);
    }

    {
        wxButton * okButton = new wxButton(this, wxID_OK, _("OK"));
        okButton->SetDefault();

        topSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    }

    this->SetSizer(topSizer);

    SetMinSize(backgroundBitmap.GetSize());
    SetSize(backgroundBitmap.GetSize());

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

ShipDescriptionDialog::~ShipDescriptionDialog()
{
}

std::string ShipDescriptionDialog::MakeHtml(ShipMetadata const & shipMetadata)
{
    std::stringstream ss;

    ss << "<html> <body>";


    // Title

    ss << "<p/>";

    ss << "<p align=center><font size=\"+2\" color=\"#4f3f31\">";
    ss << shipMetadata.ShipName;
    ss << "</font></p>";


    // Metadata

    ////ss << "<font size=-1>";
    ////ss << "<table width=100%><tr>";
    ////ss << "<td align=left>";
    ////if (!!shipMetadata.YearBuilt)
    ////    ss << "<i>" << *(shipMetadata.YearBuilt) << "</i>";
    ////ss << "</td>";
    ////ss << "<td align=right>";
    ////if (!!shipMetadata.Author)
    ////    ss << "<i>Created by " << *(shipMetadata.Author) << "</i>";
    ////ss << "</td>";
    ////ss << "</tr></table>";
    ////ss << "</font>";



    // Description

    ss << "<p align=center><font size=\"+0\" color=\"#4f3f31\">";

    if (!!shipMetadata.Description)
        ss << *(shipMetadata.Description);
    else
        ss << "This ship does not have a description.";

    ss << "</font></p>";



    ss << "</body> </html>";

    return ss.str();
}