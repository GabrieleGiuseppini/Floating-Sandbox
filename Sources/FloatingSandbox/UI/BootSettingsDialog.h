/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-01-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "BootSettings.h"

#include <Game/GameAssetManager.h>

#include <wx/radiobut.h>
#include <wx/dialog.h>

class BootSettingsDialog : public wxDialog
{
public:

    BootSettingsDialog(
        wxWindow* parent,
        GameAssetManager const & gameAssetManager);

    virtual ~BootSettingsDialog();

private:

    void PopulateCheckboxes(BootSettings const & settings);

    void OnRevertToDefaultsButton(wxCommandEvent & event);
    void OnSaveAndQuitButton(wxCommandEvent & event);

    wxRadioButton * mDoForceNoGlFinish_UnsetRadioButton;
    wxRadioButton * mDoForceNoGlFinish_TrueRadioButton;
    wxRadioButton * mDoForceNoGlFinish_FalseRadioButton;
    wxRadioButton * mDoForceNoMultithreadedRendering_UnsetRadioButton;
    wxRadioButton * mDoForceNoMultithreadedRendering_TrueRadioButton;
    wxRadioButton * mDoForceNoMultithreadedRendering_FalseRadioButton;

private:

    GameAssetManager const & mGameAssetManager;
};
