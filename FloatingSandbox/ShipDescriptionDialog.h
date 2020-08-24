/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIPreferencesManager.h"

#include <Game/ShipMetadata.h>

#include <wx/custombgwin.h>
#include <wx/dialog.h>

#include <memory>

class ShipDescriptionDialog : public wxCustomBackgroundWindow<wxDialog>
{
public:

    ShipDescriptionDialog(
        wxWindow* parent,
        ShipMetadata const & shipMetadata,
        bool isAutomatic,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        ResourceLocator & resourceLocator);

    virtual ~ShipDescriptionDialog();

private:

    void OnDontShowOnShipLoadheckboxChanged(wxCommandEvent & event);

private:

    static std::string MakeHtml(ShipMetadata const & shipMetadata);

private:

    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
};
