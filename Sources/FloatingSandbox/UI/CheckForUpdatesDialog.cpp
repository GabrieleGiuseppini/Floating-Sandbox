/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-06-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "CheckForUpdatesDialog.h"

#include <Game/GameVersion.h>

#include <Core/Log.h>

#include <wx/gbsizer.h>
#include <wx/stattext.h>

const long ID_CHECK_COMPLETION_TIMER = wxNewId();

CheckForUpdatesDialog::CheckForUpdatesDialog(
    wxWindow* parent)
    : wxDialog(
        parent,
        wxID_ANY,
        _("Checking for Updates..."),
        wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxFRAME_SHAPED | wxSTAY_ON_TOP)
{
    int constexpr Margin = 12;

    SetMinSize(wxSize(300, -1));

    mPanelSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Checking panel
    //

    {
        mCheckingPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        {
            wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            vSizer->AddStretchSpacer(1);

            {
                auto label = new wxStaticText(mCheckingPanel, wxID_ANY, _("Checking for updates..."), wxDefaultPosition,
                    wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

                vSizer->Add(label, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, Margin);
            }

            {
                mCheckingGauge = new wxGauge(mCheckingPanel, wxID_ANY, 20, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);

                vSizer->Add(mCheckingGauge, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, Margin);
            }

            vSizer->AddStretchSpacer(1);

            mCheckingPanel->SetSizer(vSizer);
        }

        mPanelSizer->Add(mCheckingPanel, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    //
    // No update panel
    //

    {
        mNoUpdatePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        {
            wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            vSizer->AddStretchSpacer(1);

            {
                mNoUpdateMessage = new wxStaticText(mNoUpdatePanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                    wxSize(-1, 30), wxALIGN_CENTER_HORIZONTAL);

                vSizer->Add(mNoUpdateMessage, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, Margin);
            }

            {
                wxButton * okButton = new wxButton(mNoUpdatePanel, wxID_CANCEL, _("OK"));
                okButton->SetDefault();

                vSizer->Add(okButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, Margin);
            }

            vSizer->AddStretchSpacer(1);

            mNoUpdatePanel->SetSizer(vSizer);
        }

        mPanelSizer->Add(mNoUpdatePanel, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    mPanelSizer->Hide(mNoUpdatePanel);
    mPanelSizer->Layout();

    SetSizerAndFit(mPanelSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);


    //
    // Start update checker
    //

    mUpdateChecker = std::make_unique<UpdateChecker>();


    //
    // Start check timer
    //

    mCheckCompletionTimer = std::make_unique<wxTimer>(this, ID_CHECK_COMPLETION_TIMER);
    mCheckCompletionTimer->Start(50, false);
    Connect(ID_CHECK_COMPLETION_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&CheckForUpdatesDialog::OnCheckCompletionTimer);
}

CheckForUpdatesDialog::~CheckForUpdatesDialog()
{
}

void CheckForUpdatesDialog::OnCheckCompletionTimer(wxTimerEvent & /*event*/)
{
    assert(!!mUpdateChecker);

    if (!!mUpdateChecker)
    {
        auto outcome = mUpdateChecker->GetOutcome();
        if (!!outcome)
        {
            // Stop timer
            mCheckCompletionTimer->Stop();
            mCheckCompletionTimer.reset();

            // Check if we got a version
            switch (outcome->OutcomeType)
            {
                case UpdateChecker::UpdateCheckOutcomeType::HasVersion:
                {
                    if (*(outcome->LatestVersion) > CurrentGameVersion)
                    {
                        // Tell the caller to display the new version
                        mHasVersionOutcome = outcome;
                        this->EndModal(wxID_OK);
                    }
                    else
                    {
                        //
                        // Notify user of no new version
                        //

                        this->SetTitle(_("No New Updates"));

                        wxString message;
                        message.Printf(_(
                            "The latest available version is %s, and you are running version %s;"
                            " there are no new updates..."),
                            outcome->LatestVersion->ToMajorMinorPatchString().c_str(),
                            CurrentGameVersion.ToMajorMinorPatchString().c_str());

                        ShowNoUpdateMessage(message);
                    }

                    break;
                }

                case UpdateChecker::UpdateCheckOutcomeType::Error:
                {
                    //
                    // Notify user of error
                    //

                    this->SetTitle(_("Cannot Check for Updates at This Moment"));

                    ShowNoUpdateMessage(
                        _("At this moment it is not possible to check for updates; please try again later..."));

                    break;
                }
            }
        }
        else
        {
            mCheckingGauge->Pulse();
        }
    }
}

void CheckForUpdatesDialog::ShowNoUpdateMessage(wxString const & message)
{
    mNoUpdateMessage->SetLabelText(message);
    mNoUpdateMessage->Fit();

    Freeze();

    mPanelSizer->Hide(mCheckingPanel);
    mPanelSizer->Show(mNoUpdatePanel);
    Fit();
    Centre();

    Thaw();
}