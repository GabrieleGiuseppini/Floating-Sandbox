/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-03
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzerDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/sizer.h>
#include <wx/statline.h>

namespace ShipBuilder {

WaterlineAnalyzerDialog::WaterlineAnalyzerDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : wxDialog(
        parent,
        wxID_ANY,
        _("Waterline Analysis"),
        wxDefaultPosition,
        wxDefaultSize,
        wxSTAY_ON_TOP)
    , mIsPlayingContinuously(false)
{
    //
    // Layout controls
    //

    wxSizer * mainHSizer = new wxBoxSizer(wxHORIZONTAL);

    {
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
                    // TODOHERE
                });

            mainHSizer->Add(
                mPlayContinuouslyButton,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
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
                    // TODOHERE
                });

            mainHSizer->Add(
                mPlayStepByStepButton,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
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
                    // TODOHERE
                });

            mainHSizer->Add(
                mRewindButton,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
        }
    }

    SetSizerAndFit(mainHSizer);

    //
    // Initialize analysis
    //

    InitializeAnalysis();

    //
    // Setup timer
    //

    mRefreshTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mRefreshTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&WaterlineAnalyzerDialog::OnRefreshTimer);
    mRefreshTimer->Start(
        100,
        false); // Continuous
}

void WaterlineAnalyzerDialog::OnRefreshTimer(wxTimerEvent & /*event*/)
{
    // TODOHERE
}

void WaterlineAnalyzerDialog::InitializeAnalysis()
{
    // TODOHERE
}

}