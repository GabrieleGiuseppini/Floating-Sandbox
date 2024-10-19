/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-09-14
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "DebugDialog.h"

#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/settings.h>
#include <wx/statbox.h>

#include <cassert>

static constexpr int Border = 10;
static int constexpr CellBorder = 8;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr StaticBoxTopMargin = 7;

DebugDialog::DebugDialog(
    wxWindow* parent,
    IGameController & gameController,
    SoundController & soundController)
    : mParent(parent)
    , mGameController(gameController)
    , mSoundController(soundController)
    , mRecordedEvents()
    , mCurrentRecordedEventIndex(0)
{
    Create(
        mParent,
        wxID_ANY,
        _("Debug"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);


    //
    // Triangles
    //

    {
        wxPanel * trianglesPanel = new wxPanel(notebook);

        PopulateTrianglesPanel(trianglesPanel);

        notebook->AddPage(trianglesPanel, _("Triangles"));
    }


    //
    // Event Recording
    //

    {
        wxPanel * eventRecordingPanel = new wxPanel(notebook);

        PopulateEventRecordingPanel(eventRecordingPanel);

        notebook->AddPage(eventRecordingPanel, _("Event Recording"));
    }


    //
    // Finalize dialog
    //

    dialogVSizer->Add(notebook, 1, wxEXPAND);

    SetSizer(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void DebugDialog::Open()
{
    this->Show();
}

void DebugDialog::PopulateTrianglesPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Destroy
    //

    {
        wxStaticBox * destroyBox = new wxStaticBox(panel, wxID_ANY, _("Destroy"));

        wxBoxSizer * destroyBoxSizer = new wxBoxSizer(wxVERTICAL);
        destroyBoxSizer->AddSpacer(StaticBoxTopMargin + StaticBoxInsetMargin);

        // Triangle Index
        {
            mDestroyTriangleIndexSpinCtrl = new wxSpinCtrl(destroyBox, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
            mDestroyTriangleIndexSpinCtrl->SetRange(0, std::numeric_limits<int>::max());

            destroyBoxSizer->Add(
                mDestroyTriangleIndexSpinCtrl,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        // Button
        {
            auto destroyButton = new wxButton(destroyBox, wxID_ANY, _("Destroy!"));
            destroyButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    bool bRes = mGameController.DestroyTriangle(
                        GlobalElementId(
                            0, // TODO: ship ID
                            static_cast<ElementIndex>(mDestroyTriangleIndexSpinCtrl->GetValue())));

                    if (!bRes)
                    {
                        mSoundController.PlayErrorSound();
                    }
                });

            destroyBoxSizer->Add(
                destroyButton,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        destroyBox->SetSizerAndFit(destroyBoxSizer);

        gridSizer->Add(
            destroyBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Restore
    //

    {
        wxStaticBox * restoreBox = new wxStaticBox(panel, wxID_ANY, _("Restore"));

        wxBoxSizer * restoreBoxSizer = new wxBoxSizer(wxVERTICAL);
        restoreBoxSizer->AddSpacer(StaticBoxTopMargin + StaticBoxInsetMargin);

        // Triangle Index
        {
            mRestoreTriangleIndexSpinCtrl = new wxSpinCtrl(restoreBox, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
            mRestoreTriangleIndexSpinCtrl->SetRange(0, std::numeric_limits<int>::max());

            restoreBoxSizer->Add(
                mRestoreTriangleIndexSpinCtrl,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        // Button
        {
            auto restoreButton = new wxButton(restoreBox, wxID_ANY, _("Restore!"));
            restoreButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    bool bRes = mGameController.RestoreTriangle(
                        GlobalElementId(
                            0, // TODO: ship ID
                            static_cast<ElementIndex>(mRestoreTriangleIndexSpinCtrl->GetValue())));

                    if (!bRes)
                    {
                        mSoundController.PlayErrorSound();
                    }
                });

            restoreBoxSizer->Add(
                restoreButton,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        restoreBox->SetSizerAndFit(restoreBoxSizer);

        gridSizer->Add(
            restoreBox,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}

void DebugDialog::PopulateEventRecordingPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Control
    //

    {
        mRecordEventPlayButton = new wxButton(panel, wxID_ANY, _("Start"));

        mRecordEventPlayButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                mRecordEventPlayButton->Enable(false);
                mRecordEventStopButton->Enable(true);
                mRecordEventStepButton->Enable(false);
                mRecordEventRewindButton->Enable(false);

                mRecordedEventTextCtrl->Clear();

                mGameController.StartRecordingEvents(
                    [this](uint32_t eventIndex, RecordedEvent const & recordedEvent)
                    {
                        SetRecordedEventText(eventIndex, recordedEvent);
                    }
                );
            });

        gridSizer->Add(
            mRecordEventPlayButton,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    {
        mRecordEventStopButton = new wxButton(panel, wxID_ANY, _("Stop"));

        mRecordEventStopButton->Enable(false);

        mRecordEventStopButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                mRecordEventPlayButton->Enable(true);
                mRecordEventStopButton->Enable(false);

                mRecordedEvents = std::make_shared<RecordedEvents>(
                    mGameController.StopRecordingEvents());

                mCurrentRecordedEventIndex = 0;

                if (mRecordedEvents->GetSize() > 0)
                {
                    mRecordEventStepButton->Enable(true);
                    mRecordEventRewindButton->Enable(true);
                    SetRecordedEventText(0, mRecordedEvents->GetEvent(0));
                }
                else
                {
                    mRecordEventStepButton->Enable(false);
                    mRecordEventRewindButton->Enable(false);
                    mRecordedEventTextCtrl->Clear();
                }
            });

        gridSizer->Add(
            mRecordEventStopButton,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    //
    // Playback
    //

    {
        mRecordedEventTextCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 40),
            wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);

        gridSizer->Add(
            mRecordedEventTextCtrl,
            wxGBPosition(1, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    {
        mRecordEventStepButton = new wxButton(panel, wxID_ANY, _("Step"));

        mRecordEventStepButton->Enable(false);

        mRecordEventStepButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                assert(!!mRecordedEvents);
                assert(mCurrentRecordedEventIndex < mRecordedEvents->GetSize());

                mGameController.ReplayRecordedEvent(
                    mRecordedEvents->GetEvent(mCurrentRecordedEventIndex));

                ++mCurrentRecordedEventIndex;
                if (mCurrentRecordedEventIndex >= mRecordedEvents->GetSize())
                {
                    mRecordEventStepButton->Enable(false);
                }
                else
                {
                    SetRecordedEventText(mCurrentRecordedEventIndex, mRecordedEvents->GetEvent(mCurrentRecordedEventIndex));
                }
            });

        gridSizer->Add(
            mRecordEventStepButton,
            wxGBPosition(2, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    {
        mRecordEventRewindButton = new wxButton(panel, wxID_ANY, _("Rewind"));

        mRecordEventRewindButton->Enable(false);

        mRecordEventRewindButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                assert(!!mRecordedEvents);
                mCurrentRecordedEventIndex = 0;
                SetRecordedEventText(mCurrentRecordedEventIndex, mRecordedEvents->GetEvent(mCurrentRecordedEventIndex));
            });

        gridSizer->Add(
            mRecordEventRewindButton,
            wxGBPosition(2, 1),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}