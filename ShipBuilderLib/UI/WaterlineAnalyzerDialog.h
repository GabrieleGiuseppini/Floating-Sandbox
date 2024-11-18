/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-03
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "WaterlineAnalysisOutcomeVisualizationControl.h"

#include "../IModelObservable.h"
#include "../IUserInterface.h"
#include "../View.h"
#include "WaterlineAnalyzer.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/timer.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class WaterlineAnalyzerDialog : public wxDialog
{
public:

    WaterlineAnalyzerDialog(
        wxWindow * parent,
        wxPoint const & centerScreen,
        IModelObservable const & model,
        View & view,
        IUserInterface & userInterface,
        bool isWaterMarkerDisplayed,
        UnitsSystem displayUnitsSystem,
        ResourceLocator const & resourceLocator);

private:

    enum class StateType;

    void OnRefreshTimer(wxTimerEvent & event);
    void OnClose(wxCloseEvent & event);
    void InitializeAnalysis(StateType initialState);
    void ReconcileUIWithState();

    void DoStep();

private:

    IModelObservable const & mModel;
    View & mView;
    IUserInterface & mUserInterface;

    bool const mOwnsCenterOfMassMarker;
    UnitsSystem const mDisplayUnitsSystem;

    //
    // UI
    //

    wxBitmapButton * mPlayContinuouslyButton;
    wxBitmapButton * mPlayStepByStepButton;
    wxBitmapButton * mRewindButton;
    wxStaticText * mTrimLabel;
    wxStaticText * mIsFloatingLabel;
    WaterlineAnalysisOutcomeVisualizationControl * mOutcomeControl;
    std::unique_ptr<wxTimer> mRefreshTimer;

    //
    // State
    //

    std::unique_ptr<WaterlineAnalyzer> mWaterlineAnalyzer;

    enum class StateType
    {
        Paused,
        Playing,
        Completed
    };

    StateType mCurrentState;
};

}