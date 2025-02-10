/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "ShipOffsetVisualizationControl.h"

#include "../Controller.h"
#include "../ShipNameNormalizer.h"

#include <UILib/BitmapRadioButton.h>
#include <UILib/EditSpinBox.h>
#include <UILib/SliderControl.h>

#include <Game/GameAssetManager.h>

#include <Simulation/ShipMetadata.h>
#include <Simulation/ShipPhysicsData.h>
#include <Simulation/ShipAutoTexturizationSettings.h>

#include <Core/GameTypes.h>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/textctrl.h>

#include <optional>

namespace ShipBuilder {

class ShipPropertiesEditDialog : public wxDialog
{
public:

    ShipPropertiesEditDialog(
        wxWindow * parent,
        ShipNameNormalizer const & shipNameNormalizer,
        GameAssetManager const & gameAssetManager);

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

    wxWindow * const mParent;
    ShipNameNormalizer const & mShipNameNormalizer;
    GameAssetManager const & mGameAssetManager;

    //
    // Fields/storage
    //

    wxTextCtrl * mShipNameTextCtrl;
    wxTextCtrl * mShipAuthorTextCtrl;
    wxTextCtrl * mArtCreditsTextCtrl;
    wxTextCtrl * mYearBuiltTextCtrl;

    wxTextCtrl * mDescriptionTextCtrl;

    EditSpinBox<float> * mOffsetXEditSpinBox;
    EditSpinBox<float> * mOffsetYEditSpinBox;
    EditSpinBox<float> * mInternalPressureEditSpinBox;

    BitmapRadioButton * mAutoTexturizationSettingsOffButton;
    BitmapRadioButton * mAutoTexturizationSettingsOnButton;
    BitmapRadioButton * mFlatStructureAutoTexturizationModeButton;
    BitmapRadioButton * mMaterialTexturesAutoTexturizationModeButton;
    SliderControl<float> * mMaterialTextureMagnificationSlider;
    SliderControl<float> * mMaterialTextureTransparencySlider;
    bool mIsAutoTexturizationSettingsDirty;

    std::optional<PasswordHash> mPasswordHash;
    bool mIsPasswordHashModified;

    //
    // Misc UI
    //

    ShipOffsetVisualizationControl * mShipOffsetVisualizationControl;
    wxSlider * mOffsetXSlider;
    wxSlider * mOffsetYSlider;

    wxPanel * mAutoTexturizationPanel;
    wxPanel * mAutoTexturizationSettingsPanel;

    BitmapRadioButton * mPasswordOnButton;
    BitmapRadioButton * mPasswordOffButton;

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