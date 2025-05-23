/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../UIPreferencesManager.h"

#include <UILib/LocalizationManager.h>

#include <Game/GameAssetManager.h>

#include <wx/dialog.h>

#include <memory>

class StartupTipDialog : public wxDialog
{
public:

    StartupTipDialog(
        wxWindow * parent,
        UIPreferencesManager & uiPreferencesManager,
        GameAssetManager const & gameAssetManager,
        LocalizationManager const & localizationManager);

    virtual ~StartupTipDialog();

private:

    void OnDontShowAgainCheckboxChanged(wxCommandEvent & event);

private:

    UIPreferencesManager & mUIPreferencesManager;
};
