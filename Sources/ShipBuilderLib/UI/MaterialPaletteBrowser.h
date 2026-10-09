/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-08-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include "IMaterialPalettesController.h"
#include "MaterialPalettePanel.h"

#include <Game/GameAssetManager.h>
#include <Game/ISoundController.h>

#include <Simulation/DefaultMaterialDatabase.h>
#include <Simulation/Layers.h>
#include <Simulation/Materials.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/GameTypes.h>
#include <Core/ProgressCallback.h>

#include <wx/wx.h>
#include <wx/popupwin.h>
#include <wx/propgrid/propgrid.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h>

#include <array>
#include <optional>
#include <vector>

namespace ShipBuilder {


struct IMaterialPalette
{
public:

    virtual ~IMaterialPalette() = default;

    virtual bool IsOpen() const = 0;
};

template<LayerType TLayer>
class MaterialPaletteBrowser final :
    public wxPopupTransientWindow,
    public IMaterialPalette
{
public:

    using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

    MaterialPaletteBrowser(
        wxWindow * parent,
        IMaterialPalettesController & materialPalettesController,
        DefaultMaterialDatabase::Palette<TMaterial> const & materialPalette,
        ShipTexturizer const & shipTexturizer,
        ISoundController * soundController,
        GameAssetManager const & gameAssetManager,
        ProgressCallback const & progressCallback);

    void Open(
        wxRect const & referenceArea,
        MaterialPlaneType planeType,
        TMaterial const * initialMaterial);

    void Close();

    bool IsOpen() const override
    {
        return IsShown();
    }

private:

    MaterialPalettePanel<TLayer> * CreateCategoryPanel(
        wxWindow * parent,
        typename DefaultMaterialDatabase::Palette<TMaterial>::Category const & defaultMaterialCategory,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & gameAssetManager);

    std::array<wxPropertyGrid *, 2> CreateStructuralMaterialPropertyGrids(wxWindow * parent);

    std::array<wxPropertyGrid *, 2> CreateElectricalMaterialPropertyGrids(wxWindow * parent);

    void PopulateMaterialProperties(TMaterial const * material);

    void SetMaterialSelected(TMaterial const * material);

    void OnMaterialClicked(TMaterial const * material);
    void OnMaterialHoveredIn(TMaterial const * material);
    void OnMaterialHoveredOut();

private:

    IMaterialPalettesController & mMaterialPalettesController;
    DefaultMaterialDatabase::Palette<TMaterial> const & mMaterialPalette;
    ISoundController * const mSoundController;

    wxSizer * mRootHSizer;

    //
    // Category list
    //

    // The category list panel and its sizer
    wxScrolledWindow * mCategoryListPanel;
    wxSizer * mCategoryListPanelSizer;

    // Category buttons in the category list; one for each category + 1 ("clear")
    std::vector<wxToggleButton *> mCategoryButtons;

    //
    // Category panels
    //

    // All category panels are in this container
    wxScrolledWindow * mCategoryPanelsContainer;
    wxSizer * mCategoryPanelsContainerSizer;

    // Category panels; one for each category
    std::vector<MaterialPalettePanel<TLayer> *> mCategoryPanels;

    //
    // Material properties
    //

    // Material properties
    std::array<wxPropertyGrid *, 2> mStructuralMaterialPropertyGrids;
    std::array<wxPropertyGrid *, 2> mElectricalMaterialPropertyGrids;

    //
    // State
    //

    std::optional<MaterialPlaneType> mCurrentPlane;
};

}