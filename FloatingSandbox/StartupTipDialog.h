/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>

#include <memory>

class StartupTipDialog : public wxDialog
{
public:

    StartupTipDialog(
        wxWindow* parent,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        ResourceLocator const & resourceLocator);

    virtual ~StartupTipDialog();

private:

    void OnDontShowAgainCheckboxChanged(wxCommandEvent & event);

private:

    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
};
