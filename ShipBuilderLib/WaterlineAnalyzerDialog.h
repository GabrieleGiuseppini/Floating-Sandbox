/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-03
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "WaterlineAnalyzer.h"

#include <Game/ResourceLocator.h>

#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/timer.h>

#include <memory>

namespace ShipBuilder {

class WaterlineAnalyzerDialog : public wxDialog
{
public:

    WaterlineAnalyzerDialog(
        wxWindow * parent,
        wxPoint const & centerScreen,
        ResourceLocator const & resourceLocator);

private:

    void OnRefreshTimer(wxTimerEvent & event);

    void InitializeAnalysis();
    void ReconcileUIWithState();

    void DoStep();

private:

    //
    // UI
    //

    wxBitmapButton * mPlayContinuouslyButton;
    wxBitmapButton * mPlayStepByStepButton;
    wxBitmapButton * mRewindButton;
    std::unique_ptr<wxTimer> mRefreshTimer;

    //
    // State
    //

    std::unique_ptr<WaterlineAnalyzer> mWaterAnalyzer;

    enum class StateType
    {
        Paused,
        Playing,
        Completed
    };

    StateType mCurrentState;
};

}