/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../Controller.h"
#include "../ModelValidationResults.h"
#include "../ModelValidationSession.h"

#include <Game/GameAssetManager.h>

#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class ModelValidationDialog : public wxDialog
{
public:

    ModelValidationDialog(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager);

    void ShowModalForStandAloneValidation(Controller & controller);

    bool ShowModalForSaveShipValidation(Controller & controller);

private:

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void PrepareUIForValidationRun();
    void StartValidation();
    void OnValidationTimer(wxTimerEvent & event);
    void ShowResults(ModelValidationResults const & results);

private:

    GameAssetManager const & mGameAssetManager;

    wxSizer * mMainVSizer;

    std::unique_ptr<wxTimer> mValidationTimer;

    // Validation panel
    wxPanel * mValidationPanel;
    wxGauge * mValidationWaitGauge;

    // Results panel
    wxScrolledWindow * mResultsPanel;
    wxSizer * mResultsPanelVSizer;

    // Buttons panel
    wxPanel * mButtonsPanel;
    wxSizer * mButtonsPanelVSizer;

    //
    // State
    //

    struct SessionData
    {
        std::optional<ModelValidationSession> ValidationSession;
        Controller & BuilderController;
        bool const IsForSave;
        bool IsInValidationWorkflow;
        std::optional<ModelValidationResults> ValidationResults;

        SessionData(
            Controller & builderController,
            bool isForSave)
            : ValidationSession()
            , BuilderController(builderController)
            , IsForSave(isForSave)
            , IsInValidationWorkflow(false)
            , ValidationResults()
        {}
    };

    std::optional<SessionData> mSessionData;
};

}