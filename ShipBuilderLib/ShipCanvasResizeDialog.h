/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>
#include <wx/panel.h>

#include <optional>

namespace ShipBuilder {

class ShipCanvasResizeDialog : public wxDialog
{
public:

    ShipCanvasResizeDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void ShowModal(Controller & controller);

private:

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void InitializeUI();

    void OnDirty();

private:

    ResourceLocator const & mResourceLocator;

    wxButton * mOkButton;

    struct SessionData
    {
        Controller & BuilderController;

        explicit SessionData(Controller & controller)
            : BuilderController(controller)
        {}
    };

    std::optional<SessionData const> mSessionData;
};

}