/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipDefinition.h>

#include <GameCore/GameTypes.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>

namespace ShipBuilder {

class AskPasswordDialog : private wxDialog
{
public:

    static bool CheckPasswordProtectedEdit(
        ShipDefinition const & shipDefinition,
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

private:

    AskPasswordDialog(
        wxWindow * parent,
        PasswordHash const & passwordHash,
        ResourceLocator const & resourceLocator);

    void OnPasswordKey();
    void OnOkButton();

private:

    PasswordHash const mPasswordHash;

    wxTextCtrl * mPasswordTextCtrl;
    wxButton * mOkButton;
};

}