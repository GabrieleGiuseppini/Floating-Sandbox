/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"
#include "ShipOffsetVisualizationControl.h"

#include <UILib/BitmapToggleButton.h>
#include <UILib/SliderControl.h>

#include <Game/ResourceLocator.h>
#include <Game/ShipMetadata.h>
#include <Game/ShipPhysicsData.h>
#include <Game/ShipAutoTexturizationSettings.h>

#include <GameCore/GameTypes.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
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
        RgbaImageData const & shipVisualization,
        bool hasTexture);

private:

    void PopulateMetadataPanel(wxPanel * panel);
    void PopulateDescriptionPanel(wxPanel * panel);
    void PopulatePhysicsDataPanel(wxPanel * panel);
    void PopulateAutoTexturizationPanel(wxPanel * panel);
    void PopulatePasswordProtectionPanel(wxPanel * panel);

    void OnSetPassword();
    void OnClearPassword();
    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void OnDirty();

    void ReconciliateUI();
    void ReconciliateUIWithPassword();

    bool IsMetadataDirty() const;
    bool IsPhysicsDataDirty() const;
    bool IsAutoTexturizationSettingsDirty() const;

    std::optional<std::string> MakeString(wxString && value);

private:

    ResourceLocator const & mResourceLocator;

    //
    // Fields/storage
    //

    wxTextCtrl * mShipNameTextCtrl;
    wxTextCtrl * mShipAuthorTextCtrl;
    wxTextCtrl * mArtCreditsTextCtrl;
    wxTextCtrl * mYearBuiltTextCtrl;

    wxRadioButton * mFlatStructureAutoTexturizationModeRadioButton;
    wxRadioButton * mMaterialTexturesAutoTexturizationModeRadioButton;
    SliderControl<float> * mMaterialTextureMagnificationSlider;
    SliderControl<float> * mMaterialTextureTransparencySlider;

    std::optional<PasswordHash> mPasswordHash;
    bool mIsPasswordHashModified;

    //
    // UI
    //

    ShipOffsetVisualizationControl * mShipOffsetVisualizationControl;

    BitmapToggleButton * mPasswordOnButton;
    BitmapToggleButton * mPasswordOffButton;

    wxButton * mOkButton;

    struct SessionData
    {
        Controller & BuilderController;
        ShipMetadata const & Metadata;
        ShipPhysicsData const & PhysicsData;
        std::optional<ShipAutoTexturizationSettings> const & AutoTexturizationSettings;
        RgbaImageData const & ShipVisualization;
        bool HasTexture;

        SessionData(
            Controller & controller,
            ShipMetadata const & shipMetadata,
            ShipPhysicsData const & shipPhysicsData,
            std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings,
            RgbaImageData const & shipVisualization,
            bool hasTexture)
            : BuilderController(controller)
            , Metadata(shipMetadata)
            , PhysicsData(shipPhysicsData)
            , AutoTexturizationSettings(shipAutoTexturizationSettings)
            , ShipVisualization(shipVisualization)
            , HasTexture(hasTexture)
        {}
    };

    std::optional<SessionData const> mSessionData;
};

}