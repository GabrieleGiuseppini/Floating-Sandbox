/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"

#include <Game/ShipMetadata.h>
#include <Game/ShipPhysicsData.h>
#include <Game/ShipAutoTexturizationSettings.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class ShipPropertiesEditDialog : public wxDialog
{
public:

    ShipPropertiesEditDialog(wxWindow * parent);

    void ShowModal(
        Controller & controller,
        ShipMetadata const & shipMetadata,
        ShipPhysicsData const & shipPhysicsData,
        std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings);

private:

    void PopulateMetadataPanel(wxPanel * panel);
    void PopulatePhysicsDataPanel(wxPanel * panel);
    void PopulateAutoTexturizationPanel(wxPanel * panel);

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void Initialize(
        ShipMetadata const & shipMetadata,
        ShipPhysicsData const & shipPhysicsData,
        std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings);

    void OnDirty();

private:

    wxTextCtrl * mShipNameTextCtrl;

    wxButton * mOkButton;

};

}