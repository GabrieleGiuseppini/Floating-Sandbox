/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-09-14
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/IGameController.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

#include <memory>

class DebugDialog : public wxDialog
{
public:

    DebugDialog(
        wxWindow * parent,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController);

    void Open();

private:

    void PopulateTrianglesPanel(wxPanel * panel);
    void PopulateEventRecordingPanel(wxPanel * panel);

    inline void SetRecordedEventText(
        uint32_t eventIndex,
        RecordedEvent const & recordedEvent)
    {
        mRecordedEventTextCtrl->SetValue(
            std::to_string(eventIndex)
            + ": "
            + recordedEvent.ToString());
    }

private:

    wxSpinCtrl * mTriangleIndexSpinCtrl;
    wxTextCtrl * mRecordedEventTextCtrl;
    wxButton * mRecordEventPlayButton;
    wxButton * mRecordEventStopButton;
    wxButton * mRecordEventStepButton;
    wxButton * mRecordEventRewindButton;

private:

    wxWindow * const mParent;
    std::shared_ptr<IGameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;

    std::shared_ptr<RecordedEvents> mRecordedEvents;
    uint32_t mCurrentRecordedEventIndex;
};
