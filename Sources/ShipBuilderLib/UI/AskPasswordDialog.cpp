/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AskPasswordDialog.h"

#include <UILib/WxHelpers.h>

#include <Simulation/ShipDefinitionFormatDeSerializer.h>

#include <wx/sizer.h>

#include <cassert>

namespace ShipBuilder {

bool AskPasswordDialog::CheckPasswordProtectedEdit(
    ShipDefinition const & shipDefinition,
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
{
    //// TODOTEST
    //(void)shipDefinition;
    //(void)parent;
    //(void)gameAssetManager;
    //return true;

    if (!shipDefinition.Metadata.Password.has_value())
    {
        // No password to check
        return true;
    }

    AskPasswordDialog dialog(parent, *shipDefinition.Metadata.Password, gameAssetManager);

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
    GameAssetManager const & gameAssetManager)
    : mPasswordHash(passwordHash)
    , mWrongAttemptCounter(0)
    , mLockedBitmap(WxHelpers::LoadBitmap("protected_medium", gameAssetManager))
    , mUnlockedBitmap(WxHelpers::LoadBitmap("unprotected_with_check_medium", gameAssetManager))
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

    {
        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        // Icon
        {
            mIconBitmap = new wxStaticBitmap(this, wxID_ANY, mLockedBitmap);

            hSizer->Add(mIconBitmap, 0, wxRIGHT, 20);
        }

        // Label + Password
        {
            wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("The ship is password-protected, please provide the password to continue:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

                vSizer->Add(label, 0, wxEXPAND);
            }

            vSizer->AddSpacer(5);

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

                vSizer->Add(mPasswordTextCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
            }

            hSizer->Add(vSizer, 0, wxALIGN_CENTER_VERTICAL);
        }

        dialogVSizer->Add(hSizer);
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
    mOkButton->Enable(!mPasswordTextCtrl->GetValue().Trim(true).Trim(false).IsEmpty());
}

void AskPasswordDialog::OnOkButton()
{
    // Check if password hash matches
    std::string const passwordValue = mPasswordTextCtrl->GetValue().Trim(true).Trim(false).ToStdString();
    if (ShipDefinitionFormatDeSerializer::CalculatePasswordHash(passwordValue) == mPasswordHash)
    {
        //
        // Correct password
        //

        // Change icon
        mIconBitmap->SetBitmap(mUnlockedBitmap);

        // Delayed endmodal
        mTimer = std::make_unique<wxTimer>();
        mTimer->Bind(
            wxEVT_TIMER,
            [this](wxTimerEvent &)
            {
                EndModal(wxID_OK);
            });

        mTimer->Start(500, true);
    }
    else
    {
        //
        // Wrong password
        //

        // Increment wrong attempt counter
        ++mWrongAttemptCounter;
        if (mWrongAttemptCounter <= 2)
        {
            // Wait some time
            WaitDialog waitDialog(this, false);
            waitDialog.ShowModal();

            // Clear password
            mPasswordTextCtrl->Clear();

            // Retry
        }
        else
        {
            // Enough

            // Notify
            WaitDialog waitDialog(this, true);
            waitDialog.ShowModal();

            // Close
            EndModal(wxID_CANCEL);
        }
    }
}

AskPasswordDialog::WaitDialog::WaitDialog(
        wxWindow * parent,
        bool isForFinal)
    : wxDialog(
        parent,
        wxID_ANY,
        _("Invalid Password"),
        wxDefaultPosition,
        wxDefaultSize,
        wxSTAY_ON_TOP)
    , mCounter(3)
{
    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    // Label 1
    {
        auto label = new wxStaticText(this, wxID_ANY, _("Invalid password!"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

        dialogVSizer->Add(label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    }

    dialogVSizer->AddSpacer(10);

    // Label 2
    {
        mLabel = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

        dialogVSizer->Add(mLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    }

    //
    // Finalize dialog
    //

    SetLabel(isForFinal);

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);

    //
    // Start timer
    //

    mTimer = std::make_unique<wxTimer>();

    if (!isForFinal)
    {
        mTimer->Bind(
            wxEVT_TIMER,
            [this](wxTimerEvent &)
            {
                --mCounter;
                if (mCounter > 0)
                {
                    SetLabel(false);
                }
                else
                {
                    EndModal(0);
                }
            });

        mTimer->Start(1000, false);
    }
    else
    {
         mTimer->Bind(
            wxEVT_TIMER,
            [this](wxTimerEvent &)
            {
                EndModal(0);
            });

         mTimer->Start(2500, true);
    }


}

void AskPasswordDialog::WaitDialog::SetLabel(bool isForFinal)
{
    if (!isForFinal)
    {
        mLabel->SetLabel(wxString::Format(_("Retry in %d..."), mCounter));
    }
    else
    {
        mLabel->SetLabel(_("Too many attempts, aborting."));
    }

    Layout();
}

}