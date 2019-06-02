/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <GameCore/Version.h>

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
        std::vector<std::vector<std::string>> const & features,
        bool isAtStartup,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager);

    virtual ~NewVersionDisplayDialog();

private:

    static std::string MakeHtml(
        Version const & version,
        std::vector<std::vector<std::string>> const & features);

    void OnHtmlLinkClicked(wxHtmlLinkEvent & event);

    void OnDoNotNotifyAboutThisVersionAgainCheckboxChanged(wxCommandEvent & event);
    void OnDoNotCheckForUpdatesAtStartupCheckboxChanged(wxCommandEvent & event);

private:

    Version const mVersion;

    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
};
