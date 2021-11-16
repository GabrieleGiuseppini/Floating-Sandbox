/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-16
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>
#include <wx/textctrl.h>

#include <optional>
#include <string>

namespace ShipBuilder {

class NewPasswordDialog : public wxDialog
{
public:

    NewPasswordDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    int ShowModal() override;

private:

    void OnPasswordKey();

    bool IsPasswordGood(std::string const & password);

private:

    ResourceLocator const & mResourceLocator;

    wxTextCtrl * mPassword1TextCtrl;
    wxTextCtrl * mPassword2TextCtrl;
    wxButton * mOkButton;
};

}