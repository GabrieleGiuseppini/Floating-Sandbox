/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIPreferences.h"

#include <Game/ResourceLoader.h>

#include <wx/dialog.h>

#include <memory>

class StartupTipDialog : public wxDialog
{
public:

    StartupTipDialog(
        wxWindow* parent,
        std::shared_ptr<UIPreferences> uiPreferences,
        ResourceLoader const & resourceLoader);

    virtual ~StartupTipDialog();

private:

    void OnDontShowAgainCheckboxChanged(wxCommandEvent & event);

private:

    std::shared_ptr<UIPreferences> mUIPreferences;
};
