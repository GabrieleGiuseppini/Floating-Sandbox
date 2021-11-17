/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AskPasswordDialog.h"

#include <Game/ShipDefinitionFormatDeSerializer.h>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

bool AskPasswordDialog::CheckPasswordProtectedEdit(
    ShipDefinition const & shipDefinition,
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
{
    if (!shipDefinition.Metadata.Password.has_value())
    {
        // No password to check
        return true;
    }

    AskPasswordDialog dialog(parent, *shipDefinition.Metadata.Password, resourceLocator);

    if (dialog.ShowModal() == wxID_OK)
    {
        // Password match
        return true;
    }

    // Failure
    return false;
}

AskPasswordDialog::AskPasswordDialog(
    wxWindow * parent,
    PasswordHash const & passwordHash,
    ResourceLocator const & resourceLocator)
    : mPasswordHash(passwordHash)
{
    Create(
        parent,
        wxID_ANY,
        _("Provide Password"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    // Label
    {
        auto label = new wxStaticText(this, wxID_ANY, _("The ship is password-protected, please provide the password to continue:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

        dialogVSizer->Add(label, 0, wxEXPAND);
    }

    dialogVSizer->AddSpacer(5);

    // Password field
    {
        int constexpr PasswordFieldWidth = 180;

        mPasswordTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PasswordFieldWidth, -1), wxTE_PASSWORD | wxTE_PROCESS_ENTER);

        mPasswordTextCtrl->Bind(
            wxEVT_TEXT,
            [this](wxCommandEvent & event)
            {
                OnPasswordKey();
                event.Skip();
            });

        mPasswordTextCtrl->Bind(
            wxEVT_TEXT_ENTER,
            [this](wxCommandEvent &)
            {
                // Simulate OK
                OnOkButton();
            });

        dialogVSizer->Add(mPasswordTextCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            mOkButton = new wxButton(this, wxID_ANY, _("OK"));

            mOkButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    OnOkButton();
                });

            buttonsSizer->Add(mOkButton, 0);

            // Start disabled
            mOkButton->Enable(false);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_CANCEL, _("Cancel"));
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    //
    // Finalize dialog
    //

    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(dialogVSizer, 0, wxEXPAND | wxALL, 20);
    SetSizerAndFit(marginSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void AskPasswordDialog::OnPasswordKey()
{
    mOkButton->Enable(!mPasswordTextCtrl->GetValue().Trim().IsEmpty());
}

void AskPasswordDialog::OnOkButton()
{
    // Check if password hash matches
    std::string const passwordValue = mPasswordTextCtrl->GetValue().Trim().ToStdString();
    if (ShipDefinitionFormatDeSerializer::CalculatePasswordHash(passwordValue) == mPasswordHash)
    {
        // Success!
        EndModal(wxID_OK);
    }

    // TODOHERE
}

}