/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>

#include <optional>

namespace ShipBuilder {

class ResizeDialog : public wxDialog
{
public:

    ResizeDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    int ShowModalForResize();

    int ShowModalForTexture();

private:

    using wxDialog::ShowModal;

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void InitializeUI();

private:

    ResourceLocator const & mResourceLocator;

    enum class ModeType
    {
        ForResize,
        ForTexture
    };

    struct SessionData
    {
        ModeType Mode;

        explicit SessionData(ModeType mode)
            : Mode(mode)
        {}
    };

    std::optional<SessionData const> mSessionData;
};

}