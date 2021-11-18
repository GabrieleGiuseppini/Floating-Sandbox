/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipDefinition.h>

#include <GameCore/GameTypes.h>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include <memory>

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

    void OnSuccessTimer();
    void OnFailureTimer();

private:

    PasswordHash const mPasswordHash;

    wxStaticBitmap * mIconBitmap;
    wxBitmap const mLockedBitmap;
    wxBitmap const mUnlockedBitmap;
    wxTextCtrl * mPasswordTextCtrl;
    wxButton * mOkButton;

    std::unique_ptr<wxTimer> mTimer;

    class WaitDialog : public wxDialog
    {
    public:

        WaitDialog(wxWindow * parent);

    private:

        void SetLabel();

        int mCounter;

        wxStaticText * mLabel;
        std::unique_ptr<wxTimer> mTimer;
    };
};

}