/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-16
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "NewPasswordDialog.h"

#include <GameCore/Log.h>

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

NewPasswordDialog::NewPasswordDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Type New Password"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // Password fields
    {
        int constexpr PasswordFieldWidth = 180;

        // Password 1
        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Type your password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
                hSizer->Add(label, 1, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(5);

            // Text Ctrl
            {
                mPassword1TextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PasswordFieldWidth, -1), wxTE_PASSWORD);
                mPassword1TextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent &)
                    {
                        OnPasswordKey();
                    });

                hSizer->Add(mPassword1TextCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            dialogVSizer->Add(hSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        }

        dialogVSizer->AddSpacer(10);

        // Password 2
        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Re-type your password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
                hSizer->Add(label, 1, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(5);

            // Text Ctrl
            {
                mPassword2TextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PasswordFieldWidth, -1), wxTE_PASSWORD);
                mPassword2TextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent &)
                    {
                        OnPasswordKey();
                    });

                hSizer->Add(mPassword2TextCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            dialogVSizer->Add(hSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        }
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            mOkButton = new wxButton(this, wxID_OK, _("OK"));
            buttonsSizer->Add(mOkButton, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_CANCEL, _("Cancel"));
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

int NewPasswordDialog::ShowModal()
{
    // TODO: clear fields
    mOkButton->Enable(false);

    auto const result = wxDialog::ShowModal();

    // TODOTEST
    LogMessage("Result: ", result);

    // TODO: if OK, populate password
    return result;
}

void NewPasswordDialog::OnPasswordKey()
{
    if (mPassword1TextCtrl->GetValue() == mPassword2TextCtrl->GetValue())
    {
        mPassword2TextCtrl->SetForegroundColour(wxNullColour);
    }
    else
    {
        mPassword2TextCtrl->SetForegroundColour(*wxRED);
    }

    mPassword2TextCtrl->Refresh();

    bool const mayClose =
        IsPasswordGood(mPassword1TextCtrl->GetValue().ToStdString())
        && mPassword1TextCtrl->GetValue() == mPassword2TextCtrl->GetValue();

    mOkButton->Enable(mayClose);
}

bool NewPasswordDialog::IsPasswordGood(std::string const & password)
{
    return password.length() >= 3;
}

}