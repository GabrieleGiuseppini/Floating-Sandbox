/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ModelValidationDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

ModelValidationDialog::ModelValidationDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Issues"),
        wxDefaultPosition,
        wxSize(400, 400),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    mMainVSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Validation panel
    //

    {
        mValidationPanel = new wxPanel(this);

        auto validationPanelVSizer = new wxBoxSizer(wxVERTICAL);

        validationPanelVSizer->AddStretchSpacer(1);

        // Label
        {
            auto label = new wxStaticText(mValidationPanel, wxID_ANY, _("Checking ship..."));

            validationPanelVSizer->Add(
                label,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        // Gauge
        {
            mValidationWaitGauge = new wxGauge(mValidationPanel, wxID_ANY, 1);

            validationPanelVSizer->Add(
                mValidationWaitGauge,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        validationPanelVSizer->AddStretchSpacer(1);

        mValidationPanel->SetSizer(validationPanelVSizer);

        mMainVSizer->Add(
            mValidationPanel,
            0,
            wxEXPAND,
            0);
    }

    //
    // Results panel
    //

    {
        mResultsPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);

        mResultsPanelVSizer = new wxBoxSizer(wxVERTICAL);

        // No results now

        mResultsPanel->SetSizer(mResultsPanelVSizer);

        mMainVSizer->Add(
            mResultsPanel,
            0,
            wxEXPAND,
            0);
    }

    //
    // Buttons panel
    //

    {
        mButtonsPanel = new wxPanel(this);

        wxBoxSizer * buttonsHSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsHSizer->AddSpacer(20);

        {
            auto button = new wxButton(mButtonsPanel, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnOkButton, this);
            buttonsHSizer->Add(button, 0);
        }

        buttonsHSizer->AddSpacer(20);

        {
            auto button = new wxButton(mButtonsPanel, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnCancelButton, this);
            buttonsHSizer->Add(button, 0);
        }

        buttonsHSizer->AddSpacer(20);

        mButtonsPanel->SetSizer(buttonsHSizer);

        mMainVSizer->Add(
            mButtonsPanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    //
    // Finalize dialog
    //

    SetSizer(mMainVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);

    //
    // Timer
    //

    mValidationTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mValidationTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&ModelValidationDialog::OnValidationTimer);
}

void ModelValidationDialog::ShowModal(
    Controller & controller)
{
    mController = &controller;

    StartValidation();

    wxDialog::ShowModal();
}

void ModelValidationDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    EndModal(0);
}

void ModelValidationDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    EndModal(-1);
}

void ModelValidationDialog::StartValidation()
{
    // Toggle validation panel
    mMainVSizer->Show(mValidationPanel);
    mMainVSizer->Hide(mResultsPanel);
    mMainVSizer->Hide(mButtonsPanel);
    Layout();

    // Start validation
    mValidationResults.reset();
    assert(!mValidationThread.joinable());
    mValidationThread = std::thread(&ModelValidationDialog::ValidationThreadLoop, this);

    // Start validation timer
    mValidationTimer->Start(200);
}

void ModelValidationDialog::ValidationThreadLoop()
{
    //
    // Runs on separate thread
    //

    assert(mController);
    mValidationResults.emplace(mController->ValidateModel());
}

void ModelValidationDialog::OnValidationTimer(wxTimerEvent & /*event*/)
{
    // Check if done
    if (mValidationResults.has_value())
    {
        // Wait until thread is done
        mValidationThread.join();

        // Stop timer
        mValidationTimer->Stop();

        // Show results
        ShowResults(*mValidationResults);
    }
    else
    {
        // Pulse gauge
        mValidationWaitGauge->Pulse();
    }
}

void ModelValidationDialog::ShowResults(ModelValidationResults const & results)
{
    // Toggle results panel
    mMainVSizer->Hide(mValidationPanel);
    mMainVSizer->Show(mResultsPanel);
    mMainVSizer->Show(mButtonsPanel);

    // Cleanup results panel
    mResultsPanelVSizer->Clear();

    // Populate results panel
    if (results.IsEmpty())
    {
        // Success

        mResultsPanelVSizer->AddStretchSpacer(1);

        auto bmp = new wxStaticBitmap(mResultsPanel, wxID_ANY, WxHelpers::LoadBitmap("success_medium", mResourceLocator));
        mResultsPanelVSizer->Add(
            bmp,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mResultsPanelVSizer->AddStretchSpacer(1);
    }
    else
    {
        // TODO: one section per issue? If so, create all sections at cctor?
        for (ModelValidationIssue const & issue : results.GetIssues())
        {
            wxStaticBoxSizer * issueBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL, mResultsPanel, wxEmptyString);
            // TODOHERE

            mResultsPanelVSizer->Add(
                issueBoxSizer,
                1,
                wxEXPAND,
                0);
        }
    }

    Layout();
}

}