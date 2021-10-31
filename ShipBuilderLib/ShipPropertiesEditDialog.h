/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"

#include <Game/ResourceLocator.h>
#include <Game/ShipMetadata.h>
#include <Game/ShipPhysicsData.h>
#include <Game/ShipAutoTexturizationSettings.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

#include <optional>

namespace ShipBuilder {

class ShipPropertiesEditDialog : public wxDialog
{
public:

    ShipPropertiesEditDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void ShowModal(
        Controller & controller,
        ShipMetadata const & shipMetadata,
        ShipPhysicsData const & shipPhysicsData,
        std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings,
        bool hasTexture);

private:

    void PopulateMetadataPanel(wxPanel * panel);
    void PopulateDescriptionPanel(wxPanel * panel);
    void PopulatePhysicsDataPanel(wxPanel * panel);
    void PopulateAutoTexturizationPanel(wxPanel * panel);

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void InitializeUI();

    void OnDirty();

private:

    ResourceLocator const & mResourceLocator;

    wxTextCtrl * mShipNameTextCtrl;
    wxTextCtrl * mShipAuthorTextCtrl;
    wxTextCtrl * mYearBuiltTextCtrl;

    wxButton * mOkButton;

    struct SessionData
    {
        Controller & BuilderController;
        ShipMetadata const & Metadata;
        ShipPhysicsData const & PhysicsData;
        std::optional<ShipAutoTexturizationSettings> const & AutoTexturizationSettings;
        bool HasTexture;

        SessionData(
            Controller & controller,
            ShipMetadata const & shipMetadata,
            ShipPhysicsData const & shipPhysicsData,
            std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings,
            bool hasTexture)
            : BuilderController(controller)
            , Metadata(shipMetadata)
            , PhysicsData(shipPhysicsData)
            , AutoTexturizationSettings(shipAutoTexturizationSettings)
            , HasTexture(hasTexture)
        {}
    };

    std::optional<SessionData const> mSessionData;
};

}