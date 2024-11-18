/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-01
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../ShipNameNormalizer.h"

#include <Game/ResourceLocator.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/string.h>
#include <wx/textctrl.h>

#include <optional>
#include <string>

namespace ShipBuilder {

class NewShipNameDialog : public wxDialog
{
public:

    NewShipNameDialog(
        wxWindow * parent,
        ShipNameNormalizer const & shipNameNormalizer,
        ResourceLocator const & resourceLocator);

    std::string AskName();

private:

    void OnDirty();
    std::optional<std::string> MakeString(wxString && value);

private:

    wxWindow * const mParent;
    ShipNameNormalizer const & mShipNameNormalizer;
    ResourceLocator const & mResourceLocator;

    wxTextCtrl * mShipNameTextCtrl;
    wxButton * mOkButton;
};

}