/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-09-14
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "DebugDialog.h"

#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/statbox.h>

#include <cassert>

static constexpr int Border = 10;
static int constexpr CellBorder = 8;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr StaticBoxTopMargin = 7;

DebugDialog::DebugDialog(
    wxWindow* parent,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController)
    : mParent(parent)
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
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
            mTriangleIndexSpinCtrl = new wxSpinCtrl(destroyBox, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
            mTriangleIndexSpinCtrl->SetRange(0, std::numeric_limits<int>::max());

            destroyBoxSizer->Add(
                mTriangleIndexSpinCtrl,
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
                    bool bRes = mGameController->DestroyTriangle(
                        ElementId(
                            0, // TODO: ship ID
                            static_cast<ElementIndex>(mTriangleIndexSpinCtrl->GetValue())));

                    if (!bRes)
                    {
                        mSoundController->PlayErrorSound();
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
        wxButton * startButton = new wxButton(panel, wxID_ANY, _T("Start"));
        startButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                mRecordedEventTextCtrl->Clear();
                mStepButton->Enable(false);

                mGameController->StartRecordingEvents(
                    [this](uint32_t eventIndex, RecordedEvent const & recordedEvent)
                    {
                        SetRecordedEventText(eventIndex, recordedEvent);
                    }
                );
            });

        gridSizer->Add(
            startButton,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }

    {
        wxButton * stopButton = new wxButton(panel, wxID_ANY, _T("Stop"));
        stopButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                mRecordedEvents = std::make_shared<RecordedEvents>(
                    mGameController->StopRecordingEvents());

                mCurrentRecordedEventIndex = 0;

                if (mRecordedEvents->GetSize() > 0)
                {
                    mStepButton->Enable(true);
                    SetRecordedEventText(0, mRecordedEvents->GetEvent(0));
                }
                else
                {
                    mRecordedEventTextCtrl->Clear();
                    mStepButton->Enable(false);
                }
            });

        gridSizer->Add(
            stopButton,
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
        mStepButton = new wxButton(panel, wxID_ANY, _T("Step"));

        mStepButton->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent &)
            {
                assert(!!mRecordedEvents);
                assert(mCurrentRecordedEventIndex < mRecordedEvents->GetSize());

                mGameController->ReplayRecordedEvent(
                    mRecordedEvents->GetEvent(mCurrentRecordedEventIndex));

                ++mCurrentRecordedEventIndex;
                if (mCurrentRecordedEventIndex >= mRecordedEvents->GetSize())
                {
                    mStepButton->Enable(false);
                }
                else
                {
                    SetRecordedEventText(mCurrentRecordedEventIndex, mRecordedEvents->GetEvent(mCurrentRecordedEventIndex));
                }
            });

        gridSizer->Add(
            mStepButton,
            wxGBPosition(2, 0),
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}