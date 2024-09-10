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
#include <wx/sizer.h>
#include <wx/statline.h>

#include <cassert>
#include <sstream>

namespace ShipBuilder {

#define TrimLabelMask "---"
#define IsFloatingLabelMask "---"

WaterlineAnalyzerDialog::WaterlineAnalyzerDialog(
    wxWindow * parent,
    wxPoint const & centerScreen,
    IModelObservable const & model,
    View & view,
    IUserInterface & userInterface,
    bool isWaterMarkerDisplayed,
    UnitsSystem displayUnitsSystem,
    ResourceLocator const & resourceLocator)
    : wxDialog(
        parent,
        wxID_ANY,
        _("Waterline Analysis"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCLOSE_BOX | wxCAPTION)
    , mModel(model)
    , mView(view)
    , mUserInterface(userInterface)
    , mOwnsCenterOfMassMarker(!isWaterMarkerDisplayed)
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
                    InitializeAnalysis(StateType::Paused);
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

        // Outcome labels
        {
            int constexpr OutcomeLabelWidth = 50;

            auto * gridSizer = new wxFlexGridSizer(2, 2, 0, InterButtonMargin);

            gridSizer->AddGrowableRow(0, 1);
            gridSizer->AddGrowableRow(1, 1);

            {
                auto * label = new wxStaticText(this, wxID_ANY, _("Trim:"));

                gridSizer->Add(
                    label,
                    0,
                    wxALIGN_CENTRE_VERTICAL,
                    0);

            }

            {
                mTrimLabel = new wxStaticText(this, wxID_ANY, TrimLabelMask, wxDefaultPosition, wxSize(OutcomeLabelWidth, -1), wxALIGN_CENTER_HORIZONTAL | wxBORDER_SIMPLE);

                {
                    auto font = GetFont();
                    font.SetFamily(wxFONTFAMILY_TELETYPE);
                    mTrimLabel->SetFont(font);
                }

                gridSizer->Add(
                    mTrimLabel,
                    0,
                    wxALIGN_CENTRE_VERTICAL,
                    0);

            }

            {
                auto * label = new wxStaticText(this, wxID_ANY, _("Floats:"));

                gridSizer->Add(
                    label,
                    0,
                    wxALIGN_CENTRE_VERTICAL,
                    0);

            }

            {
                mIsFloatingLabel = new wxStaticText(this, wxID_ANY, IsFloatingLabelMask, wxDefaultPosition, wxSize(OutcomeLabelWidth, -1), wxALIGN_CENTER_HORIZONTAL | wxBORDER_SIMPLE);

                {
                    auto font = GetFont();
                    font.SetFamily(wxFONTFAMILY_TELETYPE);
                    mIsFloatingLabel->SetFont(font);
                }

                gridSizer->Add(
                    mIsFloatingLabel,
                    0,
                    wxALIGN_CENTRE_VERTICAL,
                    0);

            }

            mainHSizer->Add(
                gridSizer,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                InterButtonMargin);
        }

        // Outcome control
        {
            mOutcomeControl = new WaterlineAnalysisOutcomeVisualizationControl(this, resourceLocator);

            mainHSizer->Add(
                mOutcomeControl,
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
    // Setup timer
    //

    mRefreshTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mRefreshTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&WaterlineAnalyzerDialog::OnRefreshTimer);

    //
    // Initialize analysis
    //

    InitializeAnalysis(StateType::Playing);

    ReconcileUIWithState();
}

void WaterlineAnalyzerDialog::OnRefreshTimer(wxTimerEvent & /*event*/)
{
    assert(mCurrentState == StateType::Playing);

    DoStep();
}

void WaterlineAnalyzerDialog::OnClose(wxCloseEvent & event)
{
    if (mOwnsCenterOfMassMarker)
    {
        mView.RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfMass);
    }

    mView.RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfBuoyancy);

    mView.RemoveWaterline();

    mUserInterface.RefreshView();

    event.Skip();
}

void WaterlineAnalyzerDialog::InitializeAnalysis(StateType initialState)
{
    mWaterlineAnalyzer = std::make_unique<WaterlineAnalyzer>(mModel);

    mCurrentState = initialState;
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
            mRefreshTimer->Stop();

            mPlayContinuouslyButton->Enable(true);
            mPlayStepByStepButton->Enable(true);
            mRewindButton->Enable(true);

            break;
        }

        case StateType::Playing:
        {
            mRefreshTimer->Start(25, false);

            mPlayContinuouslyButton->Enable(false);
            mPlayStepByStepButton->Enable(false);
            mRewindButton->Enable(true);

            break;
        }
    }

    assert(mWaterlineAnalyzer);

    //
    // Visualizations
    //

    if (mCurrentState == StateType::Completed && mWaterlineAnalyzer->GetModelMacroProperties().MassParticleCount != 0)
    {
        assert(mWaterlineAnalyzer->GetTotalBuoyantForceWhenFullySubmerged().has_value());
        assert(mWaterlineAnalyzer->GetWaterline().has_value());

        wxColor const Green = wxColor(0, 166, 81);
        wxColor const Red = wxColor(237, 28, 36);

        float const trim = -vec2f(0.0, -1.0f).angleCw(mWaterlineAnalyzer->GetWaterline()->WaterDirection);
        float visualizationControlExaggeratedTrim = trim;
        bool const isFloating = mWaterlineAnalyzer->GetTotalBuoyantForceWhenFullySubmerged() > mWaterlineAnalyzer->GetModelMacroProperties().TotalMass * 1.01f;

        // Trim
        {
            int const trimDegrees = static_cast<int>(std::abs(std::round(Conversions::RadiansCWToDegrees(trim))));

            wxString trimString;

            if (trimDegrees < 1)
            {
                trimString = L"~0°";
                mTrimLabel->SetBackgroundColour(Green);
                visualizationControlExaggeratedTrim = 0.0f;
            }
            else
            {
                trimString = wxString(std::to_string(trimDegrees)) + wxString(L"°");
                mTrimLabel->SetBackgroundColour(Red);

                float constexpr MinDegrees = 15.0f;
                if (trimDegrees < MinDegrees)
                {
                    if (trim >= 0.0f)
                    {
                        visualizationControlExaggeratedTrim = 0.0174533f * MinDegrees;
                    }
                    else
                    {
                        visualizationControlExaggeratedTrim = -0.0174533f * MinDegrees;
                    }
                }
            }

            mTrimLabel->SetForegroundColour(*wxWHITE);
            mTrimLabel->SetLabel(trimString);
        }

        // IsFloating
        {
            std::stringstream ss;

            if (isFloating)
            {
                ss << _("Yes");
                mIsFloatingLabel->SetBackgroundColour(Green);
            }
            else
            {
                ss << _("No");
                mIsFloatingLabel->SetBackgroundColour(Red);
            }

            mIsFloatingLabel->SetForegroundColour(*wxWHITE);
            mIsFloatingLabel->SetLabel(ss.str());
        }

        // Outcome
        {
            mOutcomeControl->SetValue(
                visualizationControlExaggeratedTrim,
                isFloating);
        }
    }
    else
    {
        mTrimLabel->SetBackgroundColour(GetBackgroundColour());
        mTrimLabel->SetForegroundColour(*wxBLACK);
        mTrimLabel->SetLabel(TrimLabelMask);

        mIsFloatingLabel->SetBackgroundColour(GetBackgroundColour());
        mIsFloatingLabel->SetForegroundColour(*wxBLACK);
        mIsFloatingLabel->SetLabel(IsFloatingLabelMask);

        mOutcomeControl->Clear();
    }

    // Center of mass marker
    if (mOwnsCenterOfMassMarker)
    {
        if (mWaterlineAnalyzer->GetModelMacroProperties().CenterOfMass.has_value())
        {
            mView.UploadWaterlineMarker(
                *(mWaterlineAnalyzer->GetModelMacroProperties().CenterOfMass),
                View::WaterlineMarkerType::CenterOfMass);
        }
        else
        {
            mView.RemoveWaterlineMarker(View::WaterlineMarkerType::CenterOfMass);
        }
    }

    // Center of buoyancy marker
    if (mWaterlineAnalyzer->GetCenterOfBuoyancy().has_value())
    {
        mView.UploadWaterlineMarker(
            *(mWaterlineAnalyzer->GetCenterOfBuoyancy()),
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