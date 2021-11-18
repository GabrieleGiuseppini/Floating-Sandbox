/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-16
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

#include <string>

namespace ShipBuilder {

class NewPasswordDialog : public wxDialog
{
public:

    NewPasswordDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    int ShowModal() override;

    std::string const & GetPassword() const
    {
        return mPassword;
    }

private:

    void OnPasswordKey();

private:

    ResourceLocator const & mResourceLocator;

    wxTextCtrl * mPassword1TextCtrl;
    wxTextCtrl * mPassword2TextCtrl;
    wxPanel * mPasswordStrengthPanel;
    wxButton * mOkButton;

    std::string mPassword;
};

}