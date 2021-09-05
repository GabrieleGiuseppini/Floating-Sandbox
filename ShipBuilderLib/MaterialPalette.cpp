/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

#include <GameCore/Log.h>

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/statline.h>

#include <cassert>
#include <sstream>

namespace ShipBuilder {

wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_SELECTED, fsStructuralMaterialSelectedEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_SELECTED, fsElectricalMaterialSelectedEvent);

ImageSize constexpr CategoryButtonSize(80, 60);
ImageSize constexpr PaletteButtonSize(80, 60);

template<typename TMaterial>
MaterialPalette<TMaterial>::MaterialPalette(
    wxWindow * parent,
    MaterialDatabase::Palette<TMaterial> const & materialPalette,
    ShipTexturizer const & shipTexturizer,
    ResourceLocator const & resourceLocator)
    : wxPopupTransientWindow(parent, wxPU_CONTAINS_CONTROLS | wxBORDER_SIMPLE)
    , mMaterialPalette(materialPalette)
    , mCurrentMaterialInPropertyGrid(nullptr)
    , mCurrentPlane()
{
    SetBackgroundColour(wxColour("WHITE"));

    {
        auto font = GetFont();
        font.SetPointSize(font.GetPointSize() - 2);

        SetFont(font);
    }

    //
    // Build UI
    //
    //               |
    // Category List |   Category Panels Container
    //               |     Material Properties
    //

    mRootSizer = new wxBoxSizer(wxHORIZONTAL);

    // Category list
    {
        mCategoryListPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        mCategoryListPanel->SetScrollRate(0, 5);

        mCategoryListPanelSizer = new wxBoxSizer(wxVERTICAL);

        // List
        {
            mCategoryListPanelSizer->AddSpacer(4);

            // All material categories
            int TODO = 0;
            for (auto const & category : materialPalette.Categories)
            {
                ////if (TODO++ > 5)
                ////    break;
                // Take first material
                assert(category.SubCategories.size() > 0 && category.SubCategories[0].Materials.size() > 0);
                TMaterial const & categoryHeadMaterial = category.SubCategories[0].Materials[0];

                // Create category button
                {
                    wxToggleButton * categoryButton = CreateMaterialButton(mCategoryListPanel, CategoryButtonSize, categoryHeadMaterial, shipTexturizer);

                    categoryButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this, categoryHeadMaterial](wxMouseEvent & /*event*/)
                        {
                            // Select head material
                            SetMaterialSelected(&categoryHeadMaterial);
                        });

                    mCategoryListPanelSizer->Add(
                        categoryButton,
                        0,
                        wxALIGN_CENTER_HORIZONTAL,
                        0);

                    mCategoryButtons.push_back(categoryButton);
                }

                // Create label
                {
                    wxStaticText * label = new wxStaticText(mCategoryListPanel, wxID_ANY, category.Name,
                        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

                    mCategoryListPanelSizer->Add(
                        label,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        3);
                }

                mCategoryListPanelSizer->AddSpacer(10);
            }

            // "Clear" category
            {
                static std::string const ClearMaterialName = "Clear";

                // Create category button
                {
                    wxToggleButton * categoryButton = new wxToggleButton(mCategoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

                    categoryButton->SetBitmap(
                        WxHelpers::LoadBitmap(
                            "null_material",
                            CategoryButtonSize,
                            resourceLocator));

                    categoryButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this](wxMouseEvent & /*event*/)
                        {
                            OnMaterialClicked(nullptr);
                        });

                    mCategoryListPanelSizer->Add(
                        categoryButton,
                        0,
                        wxALIGN_CENTER_HORIZONTAL,
                        0);

                    mCategoryButtons.push_back(categoryButton);
                }

                // Create label
                {
                    wxStaticText * label = new wxStaticText(mCategoryListPanel, wxID_ANY, ClearMaterialName);

                    mCategoryListPanelSizer->Add(
                        label,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        3);
                }
            }
        }

        mCategoryListPanel->SetSizerAndFit(mCategoryListPanelSizer);

        mRootSizer->Add(
            mCategoryListPanel,
            0,
            wxEXPAND,
            0);
    }

    // Category panel container and material properties
    {
        wxSizer * rVSizer = new wxBoxSizer(wxVERTICAL);

        // Category panel container
        {
            mCategoryPanelsContainer = new wxScrolledWindow(this);
            mCategoryPanelsContainer->SetScrollRate(5, 5);

            mCategoryPanelsContainerSizer = new wxBoxSizer(wxHORIZONTAL);

            for (auto const & category : materialPalette.Categories)
            {
                wxPanel * categoryPanel = CreateCategoryPanel(
                    mCategoryPanelsContainer,
                    category,
                    shipTexturizer);

                mCategoryPanelsContainerSizer->Add(
                    categoryPanel,
                    0,
                    0,
                    0);

                mCategoryPanels.push_back(categoryPanel);
            }

            mCategoryPanelsContainer->SetSizer(mCategoryPanelsContainerSizer);

            rVSizer->Add(
                mCategoryPanelsContainer,
                1, // Take all V space available
                wxEXPAND, // Also expand horizontally
                0);
        }

        // Material property grid(s)
        {
            if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
            {
                wxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

                {
                    mStructuralMaterialPropertyGrids = CreateStructuralMaterialPropertyGrids(this);

                    hSizer->Add(mStructuralMaterialPropertyGrids[0], 0, 0, 0);
                    hSizer->Add(mStructuralMaterialPropertyGrids[1], 0, 0, 0);
                }

                rVSizer->Add(
                    hSizer,
                    0, // Retain vertical size
                    wxEXPAND, // Expand horizontally
                    0);
            }
            else
            {
                assert(TMaterial::Layer == MaterialLayerType::Electrical);

                mElectricalMaterialPropertyGrid = CreateElectricalMaterialPropertyGrid(this);

                rVSizer->Add(
                    mElectricalMaterialPropertyGrid,
                    0, // Retain vertical size
                    wxEXPAND, // Expand horizontally
                    0);
            }
        }

        mRootSizer->Add(
            rVSizer,
            1,
            wxEXPAND | wxALIGN_LEFT,
            0);
    }

    SetSizerAndFit(mRootSizer);
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::Open(
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    TMaterial const * initialMaterial)
{
    // Remember current plane for this session
    mCurrentPlane = planeType;

    // Position and dimension
    SetPosition(referenceArea.GetLeftTop());
    SetMaxSize(referenceArea.GetSize());

    // Clear material properties
    PopulateMaterialProperties(nullptr);

    // Select material - showing its category panel
    SetMaterialSelected(initialMaterial);

    // Take care of appearing vertical scrollbar in the category list
    mCategoryListPanelSizer->SetSizeHints(mCategoryListPanel);
    Layout(); // Given that the category list has resized, re-layout from the root

    // Resize ourselves now to take into account category list resize
    mRootSizer->SetSizeHints(this);

    // Open
    Popup();
}

template<typename TMaterial>
wxPanel * MaterialPalette<TMaterial>::CreateCategoryPanel(
    wxWindow * parent,
    typename MaterialDatabase::Palette<TMaterial>::Category const & materialCategory,
    ShipTexturizer const & shipTexturizer)
{
    // Make sure we have room for this category in the list of material buttons
    mMaterialButtons.resize(mMaterialButtons.size() + 1);

    // Make data font
    auto dataFont = GetFont();
    dataFont.SetPointSize(dataFont.GetPointSize() - 1);

    //
    // Create panel
    //

    // Create panel
    wxPanel * categoryPanel = new wxPanel(parent);

    int constexpr RowsPerSubcategory = (TMaterial::Layer == MaterialLayerType::Structural ? 3 : 2);

    wxSizer * sizer = new wxBoxSizer(wxHORIZONTAL); // Just to add a margin

    {
        wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

        gridSizer->SetFlexibleDirection(wxVERTICAL);
        gridSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_ALL);

        for (size_t iSubCategory = 0; iSubCategory < materialCategory.SubCategories.size(); ++iSubCategory)
        {
            auto const & subCategory = materialCategory.SubCategories[iSubCategory];

            // Sub-category Name
            /*
            {
                wxStaticText * subCategoryNameLabel = new wxStaticText(categoryPanel, wxID_ANY, subCategory.Name);

                subCategoryNameLabel->Wrap(PaletteButtonSize.Width);

                gridSizer->Add(
                    subCategoryNameLabel,
                    wxGBPosition(iSubCategory * RowsPerSubcategory, 0),
                    wxGBSpan(RowsPerSubcategory, 1),
                    wxEXPAND | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL,
                    0);
            }
            */

            // Materials
            for (size_t iMaterial = 0; iMaterial < subCategory.Materials.size(); ++iMaterial)
            {
                TMaterial const * material = (&subCategory.Materials[iMaterial].get());

                // Button
                {
                    wxToggleButton * materialButton = CreateMaterialButton(categoryPanel, PaletteButtonSize, *material, shipTexturizer);

                    // Bind mouse click
                    materialButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this, material](wxMouseEvent & /*event*/)
                        {
                            OnMaterialClicked(material);
                        });

                    // Bind mouse enter
                    materialButton->Bind(
                        wxEVT_ENTER_WINDOW,
                        [this, material, materialButton](wxMouseEvent & event)
                        {
                            PopulateMaterialProperties(material);
                            mCurrentMaterialInPropertyGrid = material;
                        });

                    // Bind mouse leave
                    materialButton->Bind(
                        wxEVT_LEAVE_WINDOW,
                        [this, material](wxMouseEvent & /*event*/)
                        {
                            if (material == mCurrentMaterialInPropertyGrid)
                            {
                                mCurrentMaterialInPropertyGrid = nullptr;
                                PopulateMaterialProperties(nullptr);
                            }
                        });

                    gridSizer->Add(
                        materialButton,
                        wxGBPosition(iSubCategory * RowsPerSubcategory, iMaterial),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
                        0);

                    // Remember button
                    materialButton->SetClientData(const_cast<void *>(reinterpret_cast<void const *>(material)));
                    mMaterialButtons.back().push_back(materialButton);
                }

                // Name
                {
                    wxStaticText * nameLabel = new wxStaticText(categoryPanel, wxID_ANY, material->Name,
                        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
                    nameLabel->Wrap(PaletteButtonSize.Width);

                    gridSizer->Add(
                        nameLabel,
                        wxGBPosition(iSubCategory * RowsPerSubcategory + 1, iMaterial),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP,
                        0);
                }

                // Data
                if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
                {
                    std::stringstream ss;

                    ss << std::fixed << std::setprecision(2)
                        << "M:" << material->GetMass()
                        << "    "
                        << "S:" << material->Strength;

                    wxStaticText * dataLabel = new wxStaticText(categoryPanel, wxID_ANY, ss.str(),
                        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
                    dataLabel->SetFont(dataFont);

                    gridSizer->Add(
                        dataLabel,
                        wxGBPosition(iSubCategory * RowsPerSubcategory + 2, iMaterial),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP | wxBOTTOM,
                        8);
                }
            }
        }

        sizer->Add(gridSizer, 0, wxALL, 4);
    }

    categoryPanel->SetSizerAndFit(sizer);

    return categoryPanel;
}

template<typename TMaterial>
wxToggleButton * MaterialPalette<TMaterial>::CreateMaterialButton(
    wxWindow * parent,
    ImageSize const & size,
    TMaterial const & material,
    ShipTexturizer const & shipTexturizer)
{
    wxToggleButton * categoryButton = new wxToggleButton(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
    {
        categoryButton->SetBitmap(
            WxHelpers::MakeBitmap(
                shipTexturizer.MakeTextureSample(
                    std::nullopt, // Use shared settings
                    size,
                    material)));
    }
    else
    {
        static_assert(TMaterial::Layer == MaterialLayerType::Electrical);

        categoryButton->SetBitmap(
            WxHelpers::MakeMatteBitmap(
                rgbaColor(material.RenderColor),
                size));
    }

    return categoryButton;
}

namespace /* anonymous */ {

class fsPropertyGridWithoutFooter : public wxPropertyGrid
{
public:

    fsPropertyGridWithoutFooter(
        wxWindow * parent,
        wxSize const & size,
        long style)
        : wxPropertyGrid(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            size,
            style)
    {}

    wxSize DoGetBestSize() const override
    {
        auto size = wxPropertyGrid::DoGetBestSize();
        size.y -= 36;
        return size;
    }
};

wxPropertyGrid * CreatePropertyGrid(wxWindow * parent)
{
    return new fsPropertyGridWithoutFooter(parent, wxSize(300, -1), wxPG_DEFAULT_STYLE | wxPG_STATIC_LAYOUT);
}

wxPGProperty * AddFloatProperty(
    wxPropertyGrid * pg,
    wxString const & name,
    wxString const & label)
{
    wxPGProperty * property = pg->Append(new wxFloatProperty(label, name));

    property->SetAttribute("Precision", 2);
    property->ChangeFlag(wxPG_PROP_NOEDITOR, true);
    pg->SetPropertyReadOnly(property);

    return property;
}

wxPGProperty * AddBoolProperty(
    wxPropertyGrid * pg,
    wxString const & name,
    wxString const & label)
{
    wxPGProperty * property = pg->Append(new wxBoolProperty(label, name));

    property->ChangeFlag(wxPG_PROP_NOEDITOR, true);
    pg->SetPropertyReadOnly(property);

    return property;
}

wxPGProperty * AddStringProperty(
    wxPropertyGrid * pg,
    wxString const & name,
    wxString const & label)
{
    wxPGProperty * property = pg->Append(new wxStringProperty(label, name));

    property->ChangeFlag(wxPG_PROP_NOEDITOR, true);
    pg->SetPropertyReadOnly(property);

    return property;
}

}

template<typename TMaterial>
std::array<wxPropertyGrid *, 2> MaterialPalette<TMaterial>::CreateStructuralMaterialPropertyGrids(wxWindow * parent)
{
    std::array<wxPropertyGrid *, 2> pgs;

    pgs[0] = CreatePropertyGrid(parent);
    pgs[1] = CreatePropertyGrid(parent);

    AddFloatProperty(pgs[0], "Mass", _("Mass (Kg)"));
    AddFloatProperty(pgs[0], "Strength", _("Strength"));
    AddFloatProperty(pgs[0], "Stiffness", _("Stiffness"));
    AddBoolProperty(pgs[0], "IsHull", _("Hull"));
    AddFloatProperty(pgs[0], "BuoyancyVolumeFill", _("Buoyant Volume"));
    AddFloatProperty(pgs[0], "RustReceptivity", _("Rust Receptivity"));

    pgs[0]->FitColumns();

    AddStringProperty(pgs[1], "CombustionType", _("Combustion Type"));
    AddFloatProperty(pgs[1], "IgnitionTemperature", _("Ignition Temperature (K)"));
    AddFloatProperty(pgs[1], "MeltingTemperature", _("Melting Temperature (K)"));
    AddFloatProperty(pgs[1], "SpecificHeat", _("Specific Heat (J/(Kg*K))"));
    AddFloatProperty(pgs[1], "ThermalConductivity", _("Thermal Conductivity (W/(m*K))"));
    AddFloatProperty(pgs[1], "ThermalExpansionCoefficient", _("Thermal Expansion Coefficient (1/K)"));

    pgs[1]->FitColumns();

    return pgs;
}

template<typename TMaterial>
wxPropertyGrid * MaterialPalette<TMaterial>::CreateElectricalMaterialPropertyGrid(wxWindow * parent)
{
    return CreatePropertyGrid(parent);
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::PopulateMaterialProperties(TMaterial const * material)
{
    if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
    {
        mStructuralMaterialPropertyGrids[0]->Freeze();
        mStructuralMaterialPropertyGrids[1]->Freeze();

        if (material == nullptr)
        {
            for (int iGrid = 0; iGrid < 2; ++iGrid)
            {
                for (auto it = mStructuralMaterialPropertyGrids[iGrid]->GetIterator(); !it.AtEnd(); ++it)
                {
                    it.GetProperty()->SetValueToUnspecified();
                }
            }
        }
        else
        {
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Mass", material->GetMass());
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Strength", material->Strength);
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Stiffness", material->Stiffness);
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("IsHull", material->IsHull);
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("BuoyancyVolumeFill", material->BuoyancyVolumeFill);
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("RustReceptivity", material->RustReceptivity);

            switch (material->CombustionType)
            {
                case StructuralMaterial::MaterialCombustionType::Combustion:
                {
                    mStructuralMaterialPropertyGrids[1]->SetPropertyValue("CombustionType", _("Combustion"));
                    break;
                }

                case StructuralMaterial::MaterialCombustionType::Explosion:
                {
                    mStructuralMaterialPropertyGrids[1]->SetPropertyValue("CombustionType", _("Explosion"));
                    break;
                }
            }

            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("IgnitionTemperature", material->IgnitionTemperature);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("MeltingTemperature", material->MeltingTemperature);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("SpecificHeat", material->SpecificHeat);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("ThermalConductivity", material->ThermalConductivity);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("ThermalExpansionCoefficient", material->ThermalExpansionCoefficient);
        }

        mStructuralMaterialPropertyGrids[0]->Thaw();
        mStructuralMaterialPropertyGrids[1]->Thaw();
    }
    else
    {
        assert(TMaterial::Layer == MaterialLayerType::Electrical);

        mElectricalMaterialPropertyGrid->Freeze();

        mElectricalMaterialPropertyGrid->Clear();

        switch (material->ElectricalType)
        {
            case ElectricalMaterial::ElectricalElementType::Engine:
            {
                // TODO
                break;
            }

            case ElectricalMaterial::ElectricalElementType::Generator:
            {
                // TODO
                break;
            }

            case ElectricalMaterial::ElectricalElementType::Lamp:
            {
                // TODO
                break;
            }

            case ElectricalMaterial::ElectricalElementType::WaterPump:
            {
                // TODO
                break;
            }

            default:
            {
                break;
            }
        }

        {
            auto prop = AddBoolProperty(mElectricalMaterialPropertyGrid, "IsSelfPowered", _("Self-Powered"));
            mElectricalMaterialPropertyGrid->SetPropertyValue(prop, material->IsSelfPowered);
        }


        {
            auto prop = AddFloatProperty(mElectricalMaterialPropertyGrid, "HeatGenerated", _("Heat Generated (KJ/s)"));
            mElectricalMaterialPropertyGrid->SetPropertyValue(prop, material->HeatGenerated);
        }

        mElectricalMaterialPropertyGrid->FitColumns();

        mElectricalMaterialPropertyGrid->Thaw();
    }
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::SetMaterialSelected(TMaterial const * material)
{
    Freeze();

    //
    // Select category button and unselect all others
    //

    size_t iCategorySelected = 0;

    if (material != nullptr)
    {
        assert(material->PaletteCoordinates.has_value());

        // Find category index
        for (size_t iCategory = 0; iCategory < mMaterialPalette.Categories.size(); ++iCategory)
        {
            auto const & category = mMaterialPalette.Categories[iCategory];
            if (category.Name == material->PaletteCoordinates->Category)
            {
                iCategorySelected = iCategory;
                break;
            }
        }
    }
    else
    {
        // Use "Clear" category
        iCategorySelected = mMaterialPalette.Categories.size();
    }

    // Select category button and deselect others
    for (size_t i = 0; i < mCategoryButtons.size(); ++i)
    {
        mCategoryButtons[i]->SetValue(i == iCategorySelected);
    }

    //
    // Select category panel, its material, and unselect all other materials
    //

    for (size_t i = 0; i < mCategoryPanels.size(); ++i)
    {
        if (i == iCategorySelected)
        {
            // This is the panel we want to be shown

            // Make it visible
            mCategoryPanelsContainerSizer->Show(mCategoryPanels[i], true);

            // Deselect all the material buttons of this panel, except
            // for the selected material's
            for (auto * button : mMaterialButtons[iCategorySelected])
            {
                button->SetValue(button->GetClientData() == material);
            }
        }
        else
        {
            mCategoryPanelsContainerSizer->Show(mCategoryPanels[i], false);
        }
    }

    // Make our container as wide as the visible panel - plus some slack for the scrollbars,
    // will eventually shrink
    auto const visiblePanelWidth = mCategoryPanels[iCategorySelected]->GetSize().x;
    mCategoryPanelsContainer->SetMinSize(wxSize(visiblePanelWidth, -1));

    // Make visibility changes in the container effective
    mCategoryPanelsContainerSizer->Layout();

    // Resize whole popup now that category panel has changed its size
    Layout();
    mRootSizer->SetSizeHints(this); // this->Fit() and this->SetSizeHints

    if (mCategoryPanelsContainer->HasScrollbar(wxVERTICAL))
    {
        // Take scrollbars into account
        auto const scrollbarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, mCategoryPanelsContainer);
        mCategoryPanelsContainer->SetMinSize(wxSize(visiblePanelWidth + scrollbarWidth, -1));

        // Resize whole popup now that category panel has changed its size
        Layout();
        mRootSizer->SetSizeHints(this); // this->Fit() and this->SetSizeHints
    }

    Thaw();
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::OnMaterialClicked(TMaterial const * material)
{
    assert(mCurrentPlane.has_value());

    // Fire event
    if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
    {
        auto event = fsStructuralMaterialSelectedEvent(
            fsEVT_STRUCTURAL_MATERIAL_SELECTED,
            this->GetId(),
            material,
            *mCurrentPlane);

        ProcessWindowEvent(event);
    }
    else
    {
        assert(TMaterial::Layer == MaterialLayerType::Electrical);

        auto event = fsElectricalMaterialSelectedEvent(
            fsEVT_ELECTRICAL_MATERIAL_SELECTED,
            this->GetId(),
            material,
            *mCurrentPlane);

        ProcessWindowEvent(event);
    }

    // Close ourselves
    Dismiss();
}

//
// Explicit specializations for all material layers
//

template class MaterialPalette<StructuralMaterial>;
template class MaterialPalette<ElectricalMaterial>;

}