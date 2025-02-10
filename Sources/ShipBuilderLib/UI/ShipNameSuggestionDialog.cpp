/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipNameSuggestionDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ShipBuilder {

ShipNameSuggestionDialog::ShipNameSuggestionDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
    : mGameAssetManager(gameAssetManager)
{
    Create(
        parent,
        wxID_ANY,
        _("Prefer This Ship Name?"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    //
    // Lay out controls
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    {
        {
            auto label = new wxStaticText(this, wxID_ANY, _("It seems that a better name for this ship might be:"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            dialogVSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        dialogVSizer->AddSpacer(20);

        {
            mSuggestedShipNameTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(350, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER | wxTE_READONLY);

            auto font = GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mSuggestedShipNameTextCtrl->SetFont(font);

            dialogVSizer->Add(mSuggestedShipNameTextCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 14);
        }
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        {
            auto * button = new wxButton(this, wxID_OK, _("Yes, I want to use the suggested name"), wxDefaultPosition, wxSize(-1, 40));
            dialogVSizer->Add(button, 0, wxLEFT | wxRIGHT | wxEXPAND, 14);
        }

        dialogVSizer->AddSpacer(5);

        {
            auto * button = new wxButton(this, wxID_CANCEL, _("No, I want to keep my name"), wxDefaultPosition, wxSize(-1, 40));
            dialogVSizer->Add(button, 0, wxLEFT | wxRIGHT | wxEXPAND, 14);
        }
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

bool ShipNameSuggestionDialog::AskUserIfAcceptsSuggestedName(std::string const & newName)
{
    mSuggestedShipNameTextCtrl->SetValue(newName);
    return ShowModal() == wxID_OK;
}

}