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

class ShipDescriptionDialog : public wxCustomBackgroundWindow<wxDialog>
{
public:

    ShipDescriptionDialog(
        wxWindow* parent,
        ShipMetadata const & shipMetadata,
        bool isAutomatic,
        UIPreferencesManager & uiPreferencesManager,
        ResourceLocator const & resourceLocator);

    virtual ~ShipDescriptionDialog();

private:

    static std::string MakeHtml(ShipMetadata const & shipMetadata);

private:

    UIPreferencesManager & mUIPreferencesManager;
};
