/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-03
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzerDialog.h"

#include <UILib/WxHelpers.h>

#include <GameCore/Conversions.h>
#include <GameCore/SysSpecifics.h>

#include <wx/sizer.h>
#include <wx/statline.h>

#include <cassert>
#include <sstream>

namespace ShipBuilder {

WaterlineAnalyzerDialog::WaterlineAnalyzerDialog(
    wxWindow * parent,
    wxPoint const & centerScreen,
    Model const & model,
    View & view,
    IUserInterface & userInterface,
    UnitsSystem displayUnitsSystem,
    ResourceLocator const & resourceLocator)
    : wxDialog(
        parent,
        wxID_ANY,
        _("Waterline Analysis"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCLOSE_BOX | wxCAPTION | wxSTAY_ON_TOP)
    , mModel(model)
    , mView(view)
    , mUserInterface(userInterface)
    , mDisplayUnitsSystem(displayUnitsSystem)
{
    Bind(wxEVT_CLOSE_WINDOW, &WaterlineAnalyzerDialog::OnClose, this);

    //
    // Layout controls
    //

    wxSizer * mainHSizer = new wxBoxSizer(wxHORIZONTAL);

    {
        int constexpr InterButtonMargin = 5;

        // Play continuously button
        {
            mPlayContinuouslyButton = new wxBitmapButton(
                this,
                wxID_ANY,
                WxHelpers::LoadBitmap("play_icon_medium", resourceLocator));

            mPlayContinuouslyButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    mCurrentState = StateType::Playing;
                    ReconcileUIWithState();
                });

            mainHSizer->Add(
                mPlayContinuouslyButton,
                0,
                wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                InterButtonMargin);
        }

        // Separator
        {
            wxStaticLine * line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

            mainHSizer->Add(
                line,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                8);
        }

        // Play step-by-step button
        {
            mPlayStepByStepButton = new wxBitmapButton(
                this,
                wxID_ANY,
                WxHelpers::LoadBitmap("play_step_icon_medium", resourceLocator));

            mPlayStepByStepButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    DoStep();
                    ReconcileUIWithState();
                });

            mainHSizer->Add(
                mPlayStepByStepButton,
                0,
                wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                InterButtonMargin);
        }

        // Rewind button
        {
            mRewindButton = new wxBitmapButton(
                this,
                wxID_ANY,
                WxHelpers::LoadBitmap("rewind_icon_medium", resourceLocator));

            mRewindButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    InitializeAnalysis();
                    ReconcileUIWithState();
                });

            mainHSizer->Add(
                mRewindButton,
                0,
                wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                InterButtonMargin);
        }

        // Separator
        {
            wxStaticLine * line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

            mainHSizer->Add(
                line,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                8);
        }

        // Static analysis
        {
            mStaticAnalysisTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300, -1), wxTE_READONLY | wxTE_MULTILINE | wxTE_LEFT | wxTE_RICH);

            {
                auto font = GetFont();
                font.SetFamily(wxFONTFAMILY_TELETYPE);
                mStaticAnalysisTextCtrl->SetFont(font);
            }

            mainHSizer->Add(
                mStaticAnalysisTextCtrl,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                InterButtonMargin);
        }
    }

    // Wrap for margins
    {
        wxSizer * marginSizer = new wxBoxSizer(wxHORIZONTAL);

        marginSizer->Add(mainHSizer, 0, wxALL, 20);

        SetSizerAndFit(marginSizer);
    }

    // Center in parent rect
    Layout();
    SetPosition(
        wxPoint(
            centerScreen.x - GetSize().x / 2,
            centerScreen.y - GetSize().y / 2));

    //
    // Initialize analysis
    //

    InitializeAnalysis();

    ReconcileUIWithState();

    //
    // Setup timer
    //

    mRefreshTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mRefreshTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&WaterlineAnalyzerDialog::OnRefreshTimer);
}

void WaterlineAnalyzerDialog::OnRefreshTimer(wxTimerEvent & /*event*/)
{
    assert(mCurrentState == StateType::Playing);

    DoStep();
}

void WaterlineAnalyzerDialog::OnClose(wxCloseEvent & event)
{
    mView.RemoveWaterlineMarkers();
    mView.RemoveWaterline();
    mUserInterface.RefreshView();

    event.Skip();
}

void WaterlineAnalyzerDialog::InitializeAnalysis()
{
    mWaterlineAnalyzer = std::make_unique<WaterlineAnalyzer>(mModel);

    mCurrentState = StateType::Paused;
}

void WaterlineAnalyzerDialog::ReconcileUIWithState()
{
    // Buttons
    switch (mCurrentState)
    {
        case StateType::Completed:
        {
            mRefreshTimer->Stop();

            mPlayContinuouslyButton->Enable(false);
            mPlayStepByStepButton->Enable(false);
            mRewindButton->Enable(true);

            break;
        }

        case StateType::Paused:
        {
            mPlayContinuouslyButton->Enable(true);
            mPlayStepByStepButton->Enable(true);
            mRewindButton->Enable(false);

            break;
        }

        case StateType::Playing:
        {
            mRefreshTimer->Start(100, false);

            mPlayContinuouslyButton->Enable(false);
            mPlayStepByStepButton->Enable(false);
            mRewindButton->Enable(false);

            break;
        }
    }

    assert(mWaterlineAnalyzer);

    //
    // Visualizations
    //

    // Static analysis
    PopulateStaticAnalysisText(mWaterlineAnalyzer->GetStaticResults());

    // Center of mass
    if (mWaterlineAnalyzer->GetStaticResults().has_value() && mWaterlineAnalyzer->GetStaticResults()->TotalMass != 0.0f)
    {
        mView.UploadWaterlineMarker(
            mWaterlineAnalyzer->GetStaticResults()->CenterOfMass,
            View::WaterlineMarkerType::CenterOfMass);
    }
    else
    {
        mView.RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfMass);
    }

    // Center of buoyancy
    if (mWaterlineAnalyzer->GetCenterOfBuoyancy().has_value())
    {
        mView.UploadWaterlineMarker(
            *mWaterlineAnalyzer->GetCenterOfBuoyancy(),
            View::WaterlineMarkerType::CenterOfBuoyancy);
    }
    else
    {
        mView.RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfBuoyancy);
    }

    // Waterline
    if (mWaterlineAnalyzer->GetWaterline().has_value())
    {
        mView.UploadWaterline(
            mWaterlineAnalyzer->GetWaterline()->Center,
            mWaterlineAnalyzer->GetWaterline()->WaterDirection);
    }
    else
    {
        mView.RemoveWaterline();
    }

    mUserInterface.RefreshView();
}

void WaterlineAnalyzerDialog::PopulateStaticAnalysisText(std::optional<WaterlineAnalyzer::StaticResults> const & staticResults)
{
    std::stringstream ss;

    if (staticResults.has_value())
    {
        if (staticResults->TotalMass != 0.0f)
        {
            ss << _("Total mass: ");

            switch (mDisplayUnitsSystem)
            {
                case UnitsSystem::SI_Celsius:
                case UnitsSystem::SI_Kelvin:
                {
                    ss << KilogramToMetricTon(staticResults->TotalMass) << _(" tons");
                    break;
                }

                case UnitsSystem::USCS:
                {
                    ss << KilogramToUscsTon(staticResults->TotalMass) << _(" tons");
                    break;
                }
            }
        }
        else
        {
            ss << _("No particles");
        }
    }

    mStaticAnalysisTextCtrl->SetValue(ss.str());

    // Move focus away
    mPlayContinuouslyButton->SetFocus();

#if FS_IS_OS_WINDOWS()
    mStaticAnalysisTextCtrl->HideNativeCaret();
#endif
}

void WaterlineAnalyzerDialog::DoStep()
{
    assert(mWaterlineAnalyzer);

    auto const isCompleted = mWaterlineAnalyzer->Update();

    // Check if we need to change state
    if (isCompleted)
    {
        // We're done
        mCurrentState = StateType::Completed;
    }

    ReconcileUIWithState();
}

}