/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-02
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <wx/dialog.h>
#include <wx/string.h>
#include <wx/textctrl.h>

#include <string>

namespace ShipBuilder {

class ShipNameSuggestionDialog : public wxDialog
{
public:

    ShipNameSuggestionDialog(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager);

    bool AskUserIfAcceptsSuggestedName(std::string const & newName);

private:

    using wxDialog::ShowModal;

private:

    GameAssetManager const & mGameAssetManager;

    wxTextCtrl * mSuggestedShipNameTextCtrl;
};

}