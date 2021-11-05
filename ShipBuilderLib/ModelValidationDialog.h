/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>
#include <wx/panel.h>

#include <optional>

namespace ShipBuilder {

class ModelValidationDialog : public wxDialog
{
public:

    ModelValidationDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void ShowModal(Controller & controller);

private:

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

private:

    ResourceLocator const & mResourceLocator;

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