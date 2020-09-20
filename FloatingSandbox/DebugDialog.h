/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-09-14
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/IGameController.h>

#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>

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

private:

    wxSpinCtrl * mTriangleIndexSpinCtrl;

private:

    wxWindow * const mParent;
    std::shared_ptr<IGameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
};
