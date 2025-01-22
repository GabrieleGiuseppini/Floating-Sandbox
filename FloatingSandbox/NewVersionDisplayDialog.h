/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <Game/Version.h>

#include <wx/dialog.h>
#include <wx/wxhtml.h>

#include <memory>
#include <string>
#include <vector>

class NewVersionDisplayDialog : public wxDialog
{
public:

    NewVersionDisplayDialog(
        wxWindow* parent,
        Version const & version,
        std::string const & htmlFeatures,
        UIPreferencesManager * uiPreferencesManager); // Only if at startup

    virtual ~NewVersionDisplayDialog();

private:

    static std::string MakeHtml(
        Version const & version,
        std::string const & htmlFeatures);

    void OnGoToDownloadPageButtonClicked(wxCommandEvent & event);

    void OnDoNotNotifyAboutThisVersionAgainCheckboxChanged(wxCommandEvent & event);
    void OnDoNotCheckForUpdatesAtStartupCheckboxChanged(wxCommandEvent & event);

private:

    Version const mVersion;

    UIPreferencesManager * const mUIPreferencesManager;
};
