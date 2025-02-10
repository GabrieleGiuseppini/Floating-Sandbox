/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-16
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "NewPasswordDialog.h"

#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cassert>
#include <cctype>

namespace ShipBuilder {

NewPasswordDialog::NewPasswordDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
    : mGameAssetManager(gameAssetManager)
{
    Create(
        parent,
        wxID_ANY,
        _("Type New Password"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    auto explanationFont = GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // Password fields
    {
        int constexpr PasswordFieldWidth = 180;

        wxGridBagSizer * gSizer = new wxGridBagSizer(0, 5);

        // Password 1
        {
            // Label 1
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Type your new password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);

                gSizer->Add(label, wxGBPosition(0, 0), wxGBSpan(1, 1), wxEXPAND | wxALIGN_CENTER_VERTICAL);
            }

            // Label 2
            {
                auto label = new wxStaticText(this, wxID_ANY, _("(Min 5 characters, at least one digit or punctuation character)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
                label->SetFont(explanationFont);

                gSizer->Add(label, wxGBPosition(1, 0), wxGBSpan(1, 3), wxEXPAND);
            }

            // Text Ctrl
            {
                mPassword1TextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PasswordFieldWidth, -1), wxTE_PASSWORD | wxTE_PROCESS_ENTER);

                mPassword1TextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent & event)
                    {
                        OnPasswordKey();
                        event.Skip();
                    });

                mPassword1TextCtrl->Bind(
                    wxEVT_TEXT_ENTER,
                    [this](wxCommandEvent &)
                    {
                        mPassword1TextCtrl->Navigate();
                    });

                gSizer->Add(mPassword1TextCtrl, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
            }

            // Password strength
            {
                mPasswordStrengthPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(60, mPassword1TextCtrl->GetSize().GetHeight()), wxBORDER_SUNKEN);

                gSizer->Add(mPasswordStrengthPanel, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
            }
        }

        gSizer->Add(-1, 10, wxGBPosition(2, 0), wxGBSpan(1, 3));

        // Password 2
        {
            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Confirm password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);

                gSizer->Add(label, wxGBPosition(3, 0), wxGBSpan(1, 1), wxEXPAND | wxALIGN_CENTER_VERTICAL);
            }

            // Text Ctrl
            {
                mPassword2TextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PasswordFieldWidth, -1), wxTE_PASSWORD | wxTE_PROCESS_ENTER);

                mPassword2TextCtrl->Bind(
                    wxEVT_TEXT,
                    [this](wxCommandEvent & event)
                    {
                        OnPasswordKey();
                        event.Skip();
                    });

                mPassword2TextCtrl->Bind(
                    wxEVT_TEXT_ENTER,
                    [this](wxCommandEvent &)
                    {
                        if (mOkButton->IsEnabled())
                        {
                            EndModal(wxID_OK);
                        }
                    });

                gSizer->Add(mPassword2TextCtrl, wxGBPosition(3, 1), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
            }            
        }

        dialogVSizer->Add(gSizer, 0, wxLEFT | wxRIGHT, 10);
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

    mPasswordStrengthPanel->MoveAfterInTabOrder(mPassword2TextCtrl);
}

int NewPasswordDialog::ShowModal()
{
    mPassword1TextCtrl->Clear();
    mPassword2TextCtrl->Clear();
    OnPasswordKey();

    auto const result = wxDialog::ShowModal();
    if (result == wxID_OK)
    {
        // Store password
        mPassword = mPassword1TextCtrl->GetValue().ToStdString();
    }
    else
    {
        mPassword = "";
    }
    
    return result;
}

void NewPasswordDialog::OnPasswordKey()
{
    auto const passwordValue = mPassword1TextCtrl->GetValue();

    // Calculate password strength

    int passwordStrength;
    if (passwordValue.Length() < 5)
    {
        passwordStrength = 0;
    }
    else
    {
        passwordStrength = 1;

        bool hasDigits = false;
        bool hasPunct = false;
        for (size_t c = 0; c < passwordValue.Length(); ++c)
        {
            if (std::isdigit(passwordValue[c]))
                hasDigits = true;

            if (std::ispunct(passwordValue[c]))
                hasPunct = true;
        }

        if (hasDigits)
            ++passwordStrength;

        if (hasPunct)
            ++passwordStrength;
    }

    // Set value of strength meter

    if (passwordStrength == 0)
        mPasswordStrengthPanel->SetBackgroundColour(wxColor(181, 46, 5));
    else if (passwordStrength == 1)
        mPasswordStrengthPanel->SetBackgroundColour(wxColor(196, 184, 6));
    else
        mPasswordStrengthPanel->SetBackgroundColour(wxColor(5, 140, 0));
    mPasswordStrengthPanel->Refresh();

    // Set second field color

    if (passwordValue == mPassword2TextCtrl->GetValue())
    {
        mPassword2TextCtrl->SetForegroundColour(wxNullColour);
    }
    else
    {
        mPassword2TextCtrl->SetForegroundColour(*wxRED);
    }

    mPassword2TextCtrl->Refresh();

    // Enable OK button

    bool const mayClose =
        passwordStrength > 0 // Allow weak passwords
        && passwordValue == mPassword2TextCtrl->GetValue();

    mOkButton->Enable(mayClose);
}

}