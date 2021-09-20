/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <GameCore/GameTypes.h>

#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

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

/*
 * Event fired when a structural|electrical material has been selected.
 */
template<typename TMaterial>
class _fsMaterialSelectedEvent : public wxEvent
{
public:

    _fsMaterialSelectedEvent(
        wxEventType eventType,
        int winid,
        TMaterial const * material,
        MaterialPlaneType materialPlane)
        : wxEvent(winid, eventType)
        , mMaterial(material)
        , mMaterialPlane(materialPlane)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    _fsMaterialSelectedEvent(_fsMaterialSelectedEvent const & other)
        : wxEvent(other)
        , mMaterial(other.mMaterial)
        , mMaterialPlane(other.mMaterialPlane)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent * Clone() const override
    {
        return new _fsMaterialSelectedEvent(*this);
    }

    TMaterial const * GetMaterial() const
    {
        return mMaterial;
    }

    MaterialPlaneType GetMaterialPlane() const
    {
        return mMaterialPlane;
    }

private:

    TMaterial const * const mMaterial;
    MaterialPlaneType const mMaterialPlane;
};

using fsStructuralMaterialSelectedEvent = _fsMaterialSelectedEvent<StructuralMaterial>;
using fsElectricalMaterialSelectedEvent = _fsMaterialSelectedEvent<ElectricalMaterial>;

wxDECLARE_EVENT(fsEVT_STRUCTURAL_MATERIAL_SELECTED, fsStructuralMaterialSelectedEvent);
wxDECLARE_EVENT(fsEVT_ELECTRICAL_MATERIAL_SELECTED, fsElectricalMaterialSelectedEvent);

template<typename TMaterial>
class MaterialPalette : public wxPopupTransientWindow
{
public:

    MaterialPalette(
        wxWindow * parent,
        MaterialDatabase::Palette<TMaterial> const & materialPalette,
        ShipTexturizer const & shipTexturizer,
        ResourceLocator const & resourceLocator);

    void Open(
        wxRect const & referenceArea,
        MaterialPlaneType planeType,
        TMaterial const * initialMaterial);

    void Close();

private:

    wxPanel * CreateCategoryPanel(
        wxWindow * parent,
        typename MaterialDatabase::Palette<TMaterial>::Category const & materialCategory,
        ShipTexturizer const & shipTexturizer);

    wxToggleButton * CreateMaterialButton(
        wxWindow * parent,
        ImageSize const & size,
        TMaterial const & material,
        ShipTexturizer const & shipTexturizer);

    std::array<wxPropertyGrid *, 2> CreateStructuralMaterialPropertyGrids(wxWindow * parent);

    std::array<wxPropertyGrid *, 2> CreateElectricalMaterialPropertyGrids(wxWindow * parent);

    void PopulateMaterialProperties(TMaterial const * material);

    void SetMaterialSelected(TMaterial const * material);

    void OnMaterialClicked(TMaterial const * material);

private:

    MaterialDatabase::Palette<TMaterial> const & mMaterialPalette;

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
    std::vector<wxPanel *> mCategoryPanels;

    // Material buttons for each category panel
    std::vector<std::vector<wxToggleButton *>> mMaterialButtons;

    //
    // Material properties
    //

    // Material properties
    std::array<wxPropertyGrid *, 2> mStructuralMaterialPropertyGrids;
    std::array<wxPropertyGrid *, 2> mElectricalMaterialPropertyGrids;
    TMaterial const * mCurrentMaterialInPropertyGrid;

    //
    // State
    //

    std::optional<MaterialPlaneType> mCurrentPlane;
};

}