/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-17
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <Simulation/ShipDefinition.h>

#include <Core/GameTypes.h>

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
        GameAssetManager const & gameAssetManager);

private:

    AskPasswordDialog(
        wxWindow * parent,
        PasswordHash const & passwordHash,
        GameAssetManager const & gameAssetManager);

    void OnPasswordKey();
    void OnOkButton();

    void OnSuccessTimer();
    void OnFailureTimer();

private:

    PasswordHash const mPasswordHash;

    int mWrongAttemptCounter;

    wxStaticBitmap * mIconBitmap;
    wxBitmap const mLockedBitmap;
    wxBitmap const mUnlockedBitmap;
    wxTextCtrl * mPasswordTextCtrl;
    wxButton * mOkButton;

    std::unique_ptr<wxTimer> mTimer;

    class WaitDialog : public wxDialog
    {
    public:

        WaitDialog(
            wxWindow * parent,
            bool isForFinal);

    private:

        void SetLabel(bool isForFinal);

    private:

        int mCounter;

        wxStaticText * mLabel;
        std::unique_ptr<wxTimer> mTimer;
    };
};

}