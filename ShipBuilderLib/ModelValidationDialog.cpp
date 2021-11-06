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

    SetMinSize(wxSize(400, 400));

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
            1,          // Expand vertically
            wxEXPAND,   // Expand horizontally
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
            1,          // Expand vertically
            wxEXPAND,   // Expand horizontally
            0);
    }

    //
    // Buttons panel
    //

    {
        mButtonsPanel = new wxPanel(this);

        mButtonsPanelVSizer = new wxBoxSizer(wxVERTICAL);

        // No buttons now

        mButtonsPanel->SetSizer(mButtonsPanelVSizer);

        mMainVSizer->Add(
            mButtonsPanel,
            0,                           // Retain own vertical size
            wxALIGN_CENTER_HORIZONTAL,   // Do not expand panel horizontally
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

void ModelValidationDialog::ShowModalForStandAloneValidation(Controller & controller)
{
    mSessionData.emplace(controller, false);

    StartValidation();

    wxDialog::ShowModal();
}

void ModelValidationDialog::ShowModalForSaveShipValidation(Controller & controller)
{
    mSessionData.emplace(controller, false);

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

    // Get validation
    assert(mSessionData.has_value());
    auto validationResults = mSessionData->BuilderController.ValidateModel();

    // Store - signaling that we're done
    mValidationResults.emplace(validationResults);
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
    //
    // Populate results panel
    //

    mResultsPanelVSizer->Clear(true);

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

    //
    // Populate buttons panel
    //

    mButtonsPanelVSizer->Clear(true);

    {
        mButtonsPanelVSizer->AddSpacer(20);

        auto hSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            auto button = new wxButton(mButtonsPanel, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnOkButton, this);
            hSizer->Add(button, 0);
        }

        if (mSessionData->IsForSave)
        {
            hSizer->AddSpacer(20);

            auto button = new wxButton(mButtonsPanel, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnCancelButton, this);
            hSizer->Add(button, 0);
        }

        mButtonsPanelVSizer->Add(hSizer);

        mButtonsPanelVSizer->AddSpacer(20);
    }

    //
    // Toggle results panel
    //

    mMainVSizer->Hide(mValidationPanel);
    mMainVSizer->Show(mResultsPanel);
    mMainVSizer->Show(mButtonsPanel);

    Layout();
}

}