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
        wxSize(600, 600),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxRESIZE_BORDER);

    SetMinSize(wxSize(400, 600));

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
        mResultsPanel->SetScrollRate(0, 1);

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

bool ModelValidationDialog::ShowModalForSaveShipValidation(
    Controller & controller,
    ModelValidationResults && modelValidationResults)
{
    mSessionData.emplace(controller, false);
    mValidationResults.emplace(std::move(modelValidationResults));

    ShowResults(*mValidationResults);

    if (wxDialog::ShowModal() == 0)
    {
        return true; // May save
    }
    else
    {
        return false;
    }
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
    assert(mSessionData);
    assert(mValidationResults);

    //
    // Populate results panel
    //

    mResultsPanelVSizer->Clear(true);

    if (mSessionData->IsForSave
        && !mValidationResults->HasErrorsOrWarnings())
    {
        //
        // Single "success" panel
        //

        wxStaticBoxSizer * successBoxHSizer = new wxStaticBoxSizer(wxHORIZONTAL, mResultsPanel, _("Success"));

        // Icon
        {
            auto bmp = new wxStaticBitmap(mResultsPanel, wxID_ANY, WxHelpers::LoadBitmap("success_medium", mResourceLocator));

            successBoxHSizer->Add(
                bmp,
                0, // Retain H size
                wxLEFT | wxRIGHT, // Retain V size
                10);
        }

        // Label
        {
            auto label = new wxStaticText(mResultsPanel, wxID_ANY, _("Congratulations! The ship has no issues and it may be saved."),
                wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

            successBoxHSizer->Add(
                label,
                1, // Expand H
                wxLEFT | wxRIGHT, // Retain V size
                10);
        }

        mResultsPanelVSizer->Add(
            successBoxHSizer,
            0, // Retain V size
            wxEXPAND, // Occupy all available H space
            0);
    }
    else
    {
        //
        // Individual panels for each check
        //

        for (ModelValidationIssue const & issue : results.GetIssues())
        {
            wxStaticBoxSizer * issueBoxHSizer = new wxStaticBoxSizer(wxHORIZONTAL, mResultsPanel, wxEmptyString);

            // Icon
            {
                wxBitmap iconBitmap;
                switch (issue.GetSeverity())
                {
                    case ModelValidationIssue::SeverityType::Error:
                    {
                        iconBitmap = WxHelpers::LoadBitmap("error_medium", mResourceLocator);
                        break;
                    }

                    case ModelValidationIssue::SeverityType::Success:
                    {
                        iconBitmap = WxHelpers::LoadBitmap("success_medium", mResourceLocator);
                        break;
                    }

                    case ModelValidationIssue::SeverityType::Warning:
                    {
                        iconBitmap = WxHelpers::LoadBitmap("warning_medium", mResourceLocator);
                        break;
                    }
                }

                auto staticBitmap = new wxStaticBitmap(mResultsPanel, wxID_ANY, iconBitmap);

                issueBoxHSizer->Add(
                    staticBitmap,
                    0, // Retain H size
                    wxLEFT | wxRIGHT, // Retain V size
                    10);
            }

            // Label
            {
                // TODOHERE
                auto label = new wxStaticText(mResultsPanel, wxID_ANY, _("TODOTEST"),
                    wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

                issueBoxHSizer->Add(
                    label,
                    1, // Expand H
                    wxLEFT | wxRIGHT, // Retain V size
                    10);
            }

            mResultsPanelVSizer->Add(
                issueBoxHSizer,
                0, // Retain V size
                wxEXPAND | wxLEFT | wxRIGHT, // Occupy all available H space
                10);
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