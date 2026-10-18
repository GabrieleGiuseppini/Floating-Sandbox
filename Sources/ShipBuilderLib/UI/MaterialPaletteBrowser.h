/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-08-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include "IMaterialPalettesController.h"
#include "ElectricalMaterialEditDialog.h"
#include "MaterialPalettePanel.h"
#include "StructuralMaterialEditDialog.h"

#include <UILib/BitmappedCheckbox.h>
#include <Game/GameAssetManager.h>
#include <Game/ISoundController.h>

#include <Simulation/DefaultMaterialDatabase.h>
#include <Simulation/Layers.h>
#include <Simulation/Materials.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/GameTypes.h>
#include <Core/ProgressCallback.h>

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/propgrid/propgrid.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h>

#include <array>
#include <map>
#include <memory>
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
    public wxFrame,
    public IMaterialPalette
{
public:

    using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

    MaterialPaletteBrowser(
        wxWindow * parent,
        wxIcon const & icon,
        IMaterialPalettesController & materialPalettesController,
        DefaultMaterialDatabase::Palette<TMaterial> const & defaultMaterialPalette,
        std::vector<std::unique_ptr<TMaterial const>> const & initialCustomUserMaterials,
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

    bool GetIsDocked() const
    {
        return mIsDocked;
    }

    std::optional<typename TMaterial::VariantOverridesType> RunNewCustomMaterial(typename TMaterial::VariantOverridesType const & startingOverrides, TMaterial const * baseMaterial);
    std::optional<typename TMaterial::VariantOverridesType> RunEditCustomMaterial(typename TMaterial::VariantOverridesType const & startingOverrides, TMaterial const * baseMaterial);

    //
    // Operations
    //

    void AddUserCustomMaterials(std::vector<TMaterial const *> const & customMaterials);

    void SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> const & shipCustomMaterials);

private:

    MaterialPalettePanel<TLayer> * CreateCategoryPanel(
        wxWindow * parent,
        typename DefaultMaterialDatabase::Palette<TMaterial>::Category const & defaultMaterialCategory,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & gameAssetManager);

    std::array<wxPropertyGrid *, 2> CreateStructuralMaterialPropertyGrids(wxWindow * parent);

    std::array<wxPropertyGrid *, 2> CreateElectricalMaterialPropertyGrids(wxWindow * parent);

    void PopulateMaterialProperties(TMaterial const * material);

    StructuralMaterialEditDialog & GetStructuralMaterialEditDialog();
    ElectricalMaterialEditDialog & GetElectricalMaterialEditDialog();

    void SetMaterialSelected(TMaterial const * material);

    void OnMaterialClicked(TMaterial const * material);
    void OnMaterialHoveredIn(TMaterial const * material);
    void OnMaterialHoveredOut();

    void ReconciliateUIWithCurrentPanelCheckedState();
    void ReconciliateUIWithPanelCheckedState(MaterialPalettePanel<TLayer> * const panel);

    // After this call, requires Refresh() and syncing button checkbox state
    void SetDocked(bool isDocked);

    MaterialPalettePanel<TLayer> * GetCurrentlyShownPanel();

    static bool IsOverridableMaterial(TMaterial const * material);

private:

    IMaterialPalettesController & mMaterialPalettesController;
    DefaultMaterialDatabase::Palette<TMaterial> const & mDefaultMaterialPalette;
    ISoundController * const mSoundController;

    wxSizer * mRootHSizer;

    //
    // Category list
    //

    // The category list panel and its sizer
    wxScrolledWindow * mCategoryListPanel;
    wxSizer * mCategoryListPanelSizer;

    // Category buttons in the category list; one for each category + 1 ("clear", category is "")
    std::map<std::string, wxToggleButton *> mCategoryButtonsByCategory;

    //
    // Category panels
    //

    // All category panels are in this container
    wxScrolledWindow * mCategoryPanelsContainer;
    wxSizer * mCategoryPanelsContainerSizer;

    // Category panels; one for each category
    std::map<std::string, MaterialPalettePanel<TLayer> *> mCategoryPanelsByCategory;

    //
    // Own UI
    //

    // Docking
    BitmappedCheckbox * mDockCheckbox;

    // Material properties
    std::array<wxPropertyGrid *, 2> mStructuralMaterialPropertyGrids;
    std::array<wxPropertyGrid *, 2> mElectricalMaterialPropertyGrids;

    // Controls
    wxButton * mSelectAllButton;
    wxButton * mClearSelectionButton;
    wxButton * mDeleteSelectedButton;

    //
    // State
    //

    std::optional<MaterialPlaneType> mCurrentPlane;
    wxRect mDockingRect;
    bool mIsDocked;

    bool mStickAroundOnFocusLoss;

    //
    // Children
    //

    std::unique_ptr<StructuralMaterialEditDialog> mStructuralMaterialEditDialog;
    std::unique_ptr<ElectricalMaterialEditDialog> mElectricalMaterialEditDialog;
};

}