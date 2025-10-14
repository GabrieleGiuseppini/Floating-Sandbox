/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ModelValidationDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/wupdlock.h>

#include <cassert>

namespace ShipBuilder {

wxSize const MinDialogSizeForValidationResults = wxSize(680, 600);

int constexpr ValidationTimerPeriodMsec = 100;

ModelValidationDialog::ModelValidationDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
    : mGameAssetManager(gameAssetManager)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Issues"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    mMainVSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Validation panel
    //

    {
        mValidationPanel = new wxPanel(this);

        auto validationPanelVSizer = new wxBoxSizer(wxVERTICAL);

        validationPanelVSizer->AddStretchSpacer(1);

        validationPanelVSizer->AddSpacer(10);

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
                wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                20);
        }

        validationPanelVSizer->AddSpacer(20);

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
        mResultsPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SUNKEN);
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
            0,          // Retain own vertical size
            wxEXPAND,   // Expand panel horizontally
            0);
    }

    //
    // Finalize dialog
    //

    SetSizer(mMainVSizer);

    CentreOnParent(wxBOTH);

    //
    // Timer
    //

    mValidationTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mValidationTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&ModelValidationDialog::OnValidationTimer);
}

void ModelValidationDialog::ShowModalForStandAloneValidation(Controller & controller)
{
    mSessionData.emplace(
        controller,
        false);

    PrepareUIForValidationRun();
    this->SetMinSize(wxSize(-1, -1));
    this->Fit();
    Layout();
    CentreOnParent(wxBOTH);

    // Start validation
    StartValidation();

    wxDialog::ShowModal();
}

bool ModelValidationDialog::ShowModalForSaveShipValidation(Controller & controller)
{
    mSessionData.emplace(
        controller,
        true);

    PrepareUIForValidationRun();
    this->SetMinSize(wxSize(-1, -1));
    this->Fit();
    Layout();
    CentreOnParent(wxBOTH);

    // Start validation
    StartValidation();

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

void ModelValidationDialog::PrepareUIForValidationRun()
{
    // Toggle validation panel
    mMainVSizer->Show(mValidationPanel);
    mMainVSizer->Hide(mResultsPanel);
    mMainVSizer->Hide(mButtonsPanel);
}

void ModelValidationDialog::StartValidation()
{
    assert(mSessionData);

    // Clear results
    mSessionData->ValidationResults.reset();

    // Start validation session
    assert(!mSessionData->ValidationSession);
    mSessionData->ValidationSession.emplace(mSessionData->BuilderController.StartValidation());

    // Setup gauge
    mValidationWaitGauge->SetValue(0);
    mValidationWaitGauge->SetRange(mSessionData->ValidationSession->GetNumberOfSteps());

    // Start timer
    mValidationTimer->Start(ValidationTimerPeriodMsec, true);
}

void ModelValidationDialog::OnValidationTimer(wxTimerEvent & /*event*/)
{
    assert(mSessionData);
    assert(mSessionData->ValidationSession);

    // Check whether we're done
    if (mSessionData->ValidationResults)
    {
        //
        // We're done!
        //

        // Nuke validation session (resumes tool)
        mSessionData->ValidationSession.reset();

        // Check whether we need to show validation results
        assert(mSessionData);
        if (mSessionData->IsForSave
            && !mSessionData->ValidationResults->HasErrorsOrWarnings()
            && !mSessionData->IsInValidationWorkflow)
        {
            // Nothing more to do
            EndModal(0);
        }
        else
        {
            // Make it workflow, so we won't shrink anymore
            mSessionData->IsInValidationWorkflow = true;

            // Show results
            ShowResults(*mSessionData->ValidationResults);
        }
    }
    else
    {
        // Do next validation
        mSessionData->ValidationResults = mSessionData->ValidationSession->DoNext();

        // Advance gauge
        mValidationWaitGauge->SetValue(mValidationWaitGauge->GetValue() + 1);

        // Schedule next timer step
        mValidationTimer->Start(ValidationTimerPeriodMsec, true);
    }
}

void ModelValidationDialog::ShowResults(ModelValidationResults const & results)
{
    assert(mSessionData);
    assert(mSessionData->ValidationResults);

    wxWindowUpdateLocker scopedUpdateFreezer(this);

    //
    // Populate results panel
    //

    mResultsPanelVSizer->Clear(true);

    if (mSessionData->IsForSave
        && !mSessionData->ValidationResults->HasErrorsOrWarnings())
    {
        //
        // Single "success" panel
        //

        wxStaticBoxSizer * successBoxHSizer = new wxStaticBoxSizer(wxHORIZONTAL, mResultsPanel, _("Success"));

        // Icon
        {
            auto bmp = new wxStaticBitmap(mResultsPanel, wxID_ANY, WxHelpers::LoadBitmap("success_medium", mGameAssetManager));

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

        size_t errorInsertIndex = 0;
        size_t warningInsertIndex = 0;

        // If in Save mode, display titles
        if (mSessionData->IsForSave)
        {
            if (results.HasErrors())
            {
                auto label = new wxStaticText(mResultsPanel, wxID_ANY, _("The ship may not be saved unless the following error(s) are resolved:"),
                    wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

                wxFont font = mResultsPanel->GetFont();
                font.SetPointSize(font.GetPointSize() + 2);
                label->SetFont(font);

                mResultsPanelVSizer->Insert(
                    errorInsertIndex,
                    label,
                    0, // Retain V size
                    wxEXPAND | wxTOP | wxLEFT | wxRIGHT, // Occupy all available H space (to get uniform width)
                    10);

                ++errorInsertIndex;
                ++warningInsertIndex;
            }

            if (results.HasWarnings())
            {
                auto label = new wxStaticText(mResultsPanel, wxID_ANY, _("Resolving the following warning(s) would improve this ship:"),
                    wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

                wxFont font = mResultsPanel->GetFont();
                font.SetPointSize(font.GetPointSize() + 2);
                label->SetFont(font);

                mResultsPanelVSizer->Insert(
                    warningInsertIndex,
                    label,
                    0, // Retain V size
                    wxEXPAND | wxTOP | wxLEFT | wxRIGHT, // Occupy all available H space (to get uniform width)
                    10);

                ++warningInsertIndex;
            }
        }

        // Render all issues now
        for (ModelValidationIssue const & issue : results.GetIssues())
        {
            if (!mSessionData->IsForSave
                || issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
            {
                wxStaticBoxSizer * issueBoxHSizer = new wxStaticBoxSizer(wxHORIZONTAL, mResultsPanel, wxEmptyString);

                // Icon
                {
                    wxBitmap iconBitmap;
                    switch (issue.GetSeverity())
                    {
                        case ModelValidationIssue::SeverityType::Error:
                        {
                            iconBitmap = WxHelpers::LoadBitmap("error_medium", mGameAssetManager);
                            break;
                        }

                        case ModelValidationIssue::SeverityType::Success:
                        {
                            iconBitmap = WxHelpers::LoadBitmap("success_medium", mGameAssetManager);
                            break;
                        }

                        case ModelValidationIssue::SeverityType::Warning:
                        {
                            iconBitmap = WxHelpers::LoadBitmap("warning_medium", mGameAssetManager);
                            break;
                        }
                    }

                    auto staticBitmap = new wxStaticBitmap(issueBoxHSizer->GetStaticBox(), wxID_ANY, iconBitmap);

                    issueBoxHSizer->Add(
                        staticBitmap,
                        0, // Retain H size
                        wxLEFT | wxRIGHT, // Retain V size
                        10);
                }

                // Content
                {
                    wxWindow * contentWindow = new wxPanel(issueBoxHSizer->GetStaticBox());

                    auto vSizer = new wxBoxSizer(wxVERTICAL);

                    vSizer->AddStretchSpacer();

                    //
                    // Get issue content
                    //

                    wxString labelText;
                    std::function<void()> fixAction;
                    wxString fixActionTooltip;

                    switch (issue.GetCheckClass())
                    {
                        case ModelValidationIssue::CheckClassType::EmptyStructuralLayer:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The structural layer is empty. Place at least one particle in it.");
                            }
                            else
                            {
                                labelText = _("The structural layer contains at least one particle.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::StructureTooLarge:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The structural layer contains too many particles, possibly causing the simulation to lag on low-end computers. It is advisable to reduce the number of structural particles.");
                            }
                            else
                            {
                                labelText = _("The structural layer does not contain too many particles.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::MissingElectricalSubstratum:
                        {
                            if (issue.GetSeverity() == ModelValidationIssue::SeverityType::Error)
                            {
                                labelText = _("One or more particles in the electrical layer have no particles in the structural layer beneath them. Particles in the electrical layer must always be above particles in the structural layer.");
                            }
                            else
                            {
                                labelText = _("All particles in the electrical layer have a particle in the structural layer beneath them. Particles in the electrical layer must always be above particles in the structural layer.");
                            }

                            if (issue.GetSeverity() == ModelValidationIssue::SeverityType::Error)
                            {
                                fixAction = [this]()
                                {
                                    mSessionData->BuilderController.TrimElectricalParticlesWithoutSubstratum();
                                };

                                fixActionTooltip = _("Fix this error by removing the offending electrical particles.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::TooManyLights:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical layer contains too many light-emitting particles, possibly causing the simulation to lag on low-end computers. It is advisable to reduce the number of light-emitting electrical particles.");
                            }
                            else
                            {
                                labelText = _("The electrical layer does not contain too many light-emitting particles.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::TooManyVisibleElectricalPanelElements:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical panel contains too many elements, cluttering the panel and making it difficult to use. It is advised to hide some elements and not exceed a maximum of about 20 elements.");
                            }
                            else
                            {
                                labelText = _("The electrical panel does not contain too many elements.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::UnpoweredElectricalComponent:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical layer contains components that are not connected to a power source. Review your circuits to ensure all components are connected to at least one power source, such as a generator.");
                            }
                            else
                            {
                                labelText = _("All electrical components in the electrical layer are connected to at least one power source.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::UnconsumedElectricalSource:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical layer contains power sources that are not connected to any passive elements. Review your circuits to ensure all power sources are connected to at least one passive electrical element, such as a lamp, a horn, or an engine controller.");
                            }
                            else
                            {
                                labelText = _("All power sources in the electrical layer are connected to at least one electrical element.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::UnpoweredEngineComponent:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical layer contains engine components that are not connected to an engine controller. Review your circuits to ensure all engine components (engines and engine cables) are connected to at least one engine controller.");
                            }
                            else
                            {
                                labelText = _("All engine components in the electrical layer are connected to at least one engine controller.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::UnconsumedEngineSource:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The electrical layer contains engine controllers that are not connected to any engines. Review your circuits to ensure all engine controllers are connected to at least one engine.");
                            }
                            else
                            {
                                labelText = _("All engine controllers in the electrical layer are connected to at least one engine.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::ExteriorLayerTextureTooLarge:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The exterior layer texture might be too large for some computers. Resize the texture in the exterior layer so that no dimension is larger than 4096 pixels.");
                            }
                            else
                            {
                                labelText = _("The exterior layer texture is not too large.");
                            }

                            break;
                        }

                        case ModelValidationIssue::CheckClassType::InteriorLayerTextureTooLarge:
                        {
                            if (issue.GetSeverity() != ModelValidationIssue::SeverityType::Success)
                            {
                                labelText = _("The interior layer texture might be too large for some computers. Resize the texture in the interior layer so that no dimension is larger than 4096 pixels.");
                            }
                            else
                            {
                                labelText = _("The interior layer texture is not too large.");
                            }

                            break;
                        }
                    }

                    // Label
                    {
                        auto label = new wxStaticText(contentWindow, wxID_ANY, labelText,
                            wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);

                        // Semaphore to prevent Wrap() calls re-entering the SIZE event handler
                        // (as they do on GTK) from running infinite loops
                        bool isWrapping = false;

                        label->Bind(
                            wxEVT_SIZE,
                            [label, contentWindow, labelText, isWrapping](wxSizeEvent & event) mutable
                            {
                                if (!isWrapping)
                                {
                                    isWrapping = true;
                                    label->SetLabel(labelText);
                                    label->Wrap(contentWindow->GetClientSize().GetWidth() - 10);
                                    isWrapping = false;
                                }

                                event.Skip();
                            });

                        vSizer->Add(
                            label,
                            0,  // Retain own height
                            wxEXPAND, // Use all H space
                            0);
                    }

                    // Button
                    if (fixAction)
                    {
                        vSizer->AddSpacer(10);

                        auto button = new wxButton(contentWindow, wxID_ANY, _("Fix This Error"));
                        button->SetToolTip(fixActionTooltip);
                        button->Bind(
                            wxEVT_BUTTON,
                            [this, fixAction](wxCommandEvent & /*event*/)
                            {
                                // Fix
                                fixAction();

                                // Prepare for validation
                                PrepareUIForValidationRun();
                                Layout();

                                // Re-start validation
                                StartValidation();
                            });

                        vSizer->Add(
                            button,
                            0,  // Retain own height
                            wxALIGN_LEFT | wxBOTTOM, // Do not expand H
                            4);
                    }

                    vSizer->AddStretchSpacer();

                    contentWindow->SetSizer(vSizer);

                    issueBoxHSizer->Add(
                        contentWindow,
                        1, // Use remaining H space
                        wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, // Retain V size
                        10);
                }

                // Calculate insert index
                size_t insertIndex;
                if (mSessionData->IsForSave)
                {
                    if (issue.GetSeverity() == ModelValidationIssue::SeverityType::Error)
                    {
                        insertIndex = errorInsertIndex;
                        ++errorInsertIndex;
                        ++warningInsertIndex;
                    }
                    else
                    {
                        assert(issue.GetSeverity() == ModelValidationIssue::SeverityType::Warning);
                        insertIndex = warningInsertIndex;
                        ++warningInsertIndex;
                    }
                }
                else
                {
                    insertIndex = mResultsPanelVSizer->GetItemCount();
                }

                mResultsPanelVSizer->Insert(
                    insertIndex,
                    issueBoxHSizer,
                    0, // Retain V size
                    wxEXPAND | wxLEFT | wxRIGHT, // Occupy all available H space (to get uniform width)
                    10);
            }
        }

        mResultsPanelVSizer->AddSpacer(10);
    }

    //
    // Populate buttons panel
    //

    mButtonsPanelVSizer->Clear(true);

    {
        mButtonsPanelVSizer->AddSpacer(20);

        auto hSizer = new wxBoxSizer(wxHORIZONTAL);

        if (!mSessionData->IsForSave)
        {
            auto okButton = new wxButton(mButtonsPanel, wxID_ANY, _("OK"));
            okButton->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnOkButton, this);
            okButton->Enable(!mSessionData->IsForSave || !mSessionData->ValidationResults->HasErrors());
            hSizer->Add(okButton, 0);
        }
        else
        {
            auto okButton = new wxButton(mButtonsPanel, wxID_ANY, _("Save"));
            okButton->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnOkButton, this);
            okButton->Enable(!mSessionData->ValidationResults->HasErrors());
            hSizer->Add(okButton, 0);

            hSizer->AddSpacer(20);

            auto cancelButton = new wxButton(mButtonsPanel, wxID_ANY, _("Cancel"));
            cancelButton->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnCancelButton, this);
            hSizer->Add(cancelButton, 0);
        }

        hSizer->AddSpacer(20);

        mButtonsPanelVSizer->Add(hSizer, 0, wxALIGN_RIGHT, 0);

        mButtonsPanelVSizer->AddSpacer(20);
    }

    //
    // Toggle results panel
    //

    mMainVSizer->Hide(mValidationPanel);
    mMainVSizer->Show(mResultsPanel);
    mMainVSizer->Show(mButtonsPanel);

    //
    // Show
    //

    this->SetMinSize(MinDialogSizeForValidationResults);
    this->SetSize(MinDialogSizeForValidationResults);
    Layout();
    CentreOnParent(wxBOTH);
}

}