/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPaletteBrowser_NEW.h"

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/wupdlock.h>

#include <cassert>
#include <iomanip>
#include <sstream>

namespace ShipBuilder {

wxDEFINE_EVENT(fsEVT_STRUCTURAL_MATERIAL_SELECTED, fsStructuralMaterialSelectedEvent);
wxDEFINE_EVENT(fsEVT_ELECTRICAL_MATERIAL_SELECTED, fsElectricalMaterialSelectedEvent);

int constexpr MinCategoryPanelsContainerHeight = 400; // Min height of the scrollable panel that contains the swaths; without a min height, a palette that only has a few categories would be too short
ImageSize constexpr CategoryButtonSize(80, 60);
ImageSize constexpr PaletteButtonSize(80, 60);

template<LayerType TLayer>
MaterialPaletteBrowser_NEW<TLayer>::MaterialPaletteBrowser_NEW(
    wxWindow * parent,
    MaterialDatabase::Palette<TMaterial> const & materialPalette,
    ShipTexturizer const & shipTexturizer,
    ISoundController * soundController,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : wxPopupTransientWindow(parent, wxPU_CONTAINS_CONTROLS | wxBORDER_SIMPLE)
    , mMaterialPalette(materialPalette)
    , mSoundController(soundController)
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
    // Category List | Category Panels Container
    //               |   Material Properties
    //

    mRootHSizer = new wxBoxSizer(wxHORIZONTAL);

    // Category list
    {
        int constexpr Margin = 4;

        mCategoryListPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SIMPLE);
        mCategoryListPanel->SetScrollRate(0, 5);

        mCategoryListPanelSizer = new wxBoxSizer(wxVERTICAL);

        // List
        {
            mCategoryListPanelSizer->AddSpacer(Margin);

            // All material categories
            for (auto const & category : materialPalette.Categories)
            {
                assert(category.SubCategories.size() > 0 && category.SubCategories[0].Materials.size() > 0);
                TMaterial const & categoryHeadMaterial = category.SubCategories[0].Materials[0];

                // Create category button
                {
                    wxToggleButton * categoryButton = new wxToggleButton(mCategoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

                    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                    {
                        ShipAutoTexturizationSettings texturizationSettings;
                        texturizationSettings.MaterialTextureMagnification = 0.5f;

                        categoryButton->SetBitmap(
                            WxHelpers::MakeBitmap(
                                shipTexturizer.MakeMaterialTextureSample(
                                    texturizationSettings,
                                    CategoryButtonSize,
                                    categoryHeadMaterial,
                                    gameAssetManager)));
                    }
                    else
                    {
                        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

                        categoryButton->SetBitmap(
                            WxHelpers::MakeMatteBitmap(
                                rgbaColor(categoryHeadMaterial.RenderColor, 255),
                                CategoryButtonSize));
                    }

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
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        Margin);

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
                        Margin);
                }

                mCategoryListPanelSizer->AddSpacer(10);
            }

            // "Clear" category
            if constexpr (TLayer != LayerType::Ropes)
            {
                static std::string const ClearMaterialName = "Clear";

                // Create category button
                {
                    wxToggleButton * categoryButton = new wxToggleButton(mCategoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

                    categoryButton->SetBitmap(
                        WxHelpers::LoadBitmap(
                            "null_material",
                            CategoryButtonSize,
                            gameAssetManager));

                    categoryButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this](wxMouseEvent & /*event*/)
                        {
                            OnMaterialClicked(nullptr);
                        });

                    mCategoryListPanelSizer->Add(
                        categoryButton,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        Margin);

                    mCategoryButtons.push_back(categoryButton);
                }

                // Create label
                {
                    wxStaticText * label = new wxStaticText(mCategoryListPanel, wxID_ANY, ClearMaterialName);

                    mCategoryListPanelSizer->Add(
                        label,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        Margin);
                }
            }
        }

        mCategoryListPanel->SetSizerAndFit(mCategoryListPanelSizer);

        mRootHSizer->Add(
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

            size_t const categoriesCount = materialPalette.Categories.size();
            for (size_t c = 0; c < categoriesCount; ++c)
            {
                auto const & category = materialPalette.Categories[c];

                auto * categoryPanel = CreateCategoryPanel(
                    mCategoryPanelsContainer,
                    category,
                    shipTexturizer,
                    gameAssetManager);

                // Bind click
                if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                {
                    categoryPanel->Bind(
                        fsEVT_STRUCTURAL_MATERIAL_PALETTE_CLICKED,
                        [this](fsStructuralMaterialPaletteEvent & event)
                        {
                            OnMaterialClicked(event.GetMaterial());
                        });
                }
                else
                {
                    categoryPanel->Bind(
                        fsEVT_ELECTRICAL_MATERIAL_PALETTE_CLICKED,
                        [this](fsElectricalMaterialPaletteEvent & event)
                        {
                            OnMaterialClicked(event.GetMaterial());
                        });
                }

                // Bind hover-in
                if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                {
                    categoryPanel->Bind(
                        fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_IN,
                        [this](fsStructuralMaterialPaletteEvent & event)
                        {
                            OnMaterialHoveredIn(event.GetMaterial());
                        });
                }
                else
                {
                    categoryPanel->Bind(
                        fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_IN,
                        [this](fsElectricalMaterialPaletteEvent & event)
                        {
                            OnMaterialHoveredIn(event.GetMaterial());
                        });
                }

                // Bind hover-out
                if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                {
                    categoryPanel->Bind(
                        fsEVT_STRUCTURAL_MATERIAL_PALETTE_HOVERED_OUT,
                        [this](fsStructuralMaterialPaletteEvent & /*event*/)
                        {
                            OnMaterialHoveredOut();
                        });
                }
                else
                {
                    categoryPanel->Bind(
                        fsEVT_ELECTRICAL_MATERIAL_PALETTE_HOVERED_OUT,
                        [this](fsElectricalMaterialPaletteEvent & /*event*/)
                        {
                            OnMaterialHoveredOut();
                        });
                }

                mCategoryPanelsContainerSizer->Add(
                    categoryPanel,
                    0,
                    0,
                    0);

                mCategoryPanels.push_back(categoryPanel);

                progressCallback(static_cast<float>(c + 1) / static_cast<float>(categoriesCount), ProgressMessageType::LoadingMaterialPalette);
            }

            mCategoryPanelsContainer->SetSizer(mCategoryPanelsContainerSizer);

            rVSizer->Add(
                mCategoryPanelsContainer,
                1, // Take all V space available
                wxEXPAND, // Also expand horizontally
                0);
        }

        rVSizer->AddSpacer(5);

        // Material property grid(s)
        {
            wxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            {
                hSizer->AddStretchSpacer(1);
                if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
                {
                    mStructuralMaterialPropertyGrids = CreateStructuralMaterialPropertyGrids(this);
                    hSizer->Add(mStructuralMaterialPropertyGrids[0], 0, wxEXPAND, 0);
                    hSizer->Add(mStructuralMaterialPropertyGrids[1], 0, wxEXPAND, 0);
                }
                else
                {
                    assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

                    mElectricalMaterialPropertyGrids = CreateElectricalMaterialPropertyGrids(this);
                    hSizer->Add(mElectricalMaterialPropertyGrids[0], 0, wxEXPAND, 0);
                    hSizer->Add(mElectricalMaterialPropertyGrids[1], 0, wxEXPAND, 0);
                }

                hSizer->AddStretchSpacer(1);
            }

            rVSizer->Add(
                hSizer,
                0, // Retain vertical size
                wxEXPAND, // Expand horizontally
                0);
        }

        mRootHSizer->Add(
            rVSizer,
            1,
            wxEXPAND | wxALIGN_LEFT,
            0);
    }

    SetSizerAndFit(mRootHSizer);
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::Open(
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
    mRootHSizer->SetSizeHints(this);

    // Open
    Popup();
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::Close()
{
    Dismiss();
}

template<LayerType TLayer>
MaterialPalettePanel<TLayer> * MaterialPaletteBrowser_NEW<TLayer>::CreateCategoryPanel(
    wxWindow * parent,
    typename MaterialDatabase::Palette<TMaterial>::Category const & materialCategory,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
{
    //
    // Create and build panel
    //

    MaterialPalettePanel<TLayer> * categoryPanel = new MaterialPalettePanel<TLayer>(
        parent,
        shipTexturizer,
        gameAssetManager);

    categoryPanel->StartBuild();

    std::optional<typename MaterialDatabase::Palette<TMaterial>::Category::SubCategory::Group> currentGroup;
    for (size_t iSubCategory = 0; iSubCategory < materialCategory.SubCategories.size(); ++iSubCategory) // Rows
    {
        auto const & subCategory = materialCategory.SubCategories[iSubCategory];

        // Check if a group change
        if (currentGroup.has_value() && subCategory.ParentGroup.UniqueId != currentGroup->UniqueId)
        {
            categoryPanel->AddSeparator();
        }

        // Remember this group
        currentGroup = subCategory.ParentGroup;

        // Materials
        for (size_t iMaterial = 0; iMaterial < subCategory.Materials.size(); ++iMaterial) // Cols
        {
            TMaterial const * material = (&subCategory.Materials[iMaterial].get());
            categoryPanel->Add(material, iMaterial == 0);
        }
    }

    categoryPanel->EndBuild();

    return categoryPanel;
}

namespace /* anonymous */ {

class MaterialPropertyGrid : public wxPropertyGrid
{
public:

    MaterialPropertyGrid(
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
        size.y -= 36; // wxWidgets up to 3.1.4 has a harcoded "+40" here, which generates an ugly footer
        return size;
    }
};

wxPropertyGrid * CreatePropertyGrid(wxWindow * parent)
{
    return new MaterialPropertyGrid(parent, wxSize(300, -1), wxPG_DEFAULT_STYLE | wxPG_STATIC_LAYOUT);
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

template<LayerType TLayer>
std::array<wxPropertyGrid *, 2> MaterialPaletteBrowser_NEW<TLayer>::CreateStructuralMaterialPropertyGrids(wxWindow * parent)
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
    AddFloatProperty(pgs[1], "ThermalExpansionCoefficient", _("Thermal Expansion Coefficient (1/MK)"));

    pgs[1]->FitColumns();

    return pgs;
}

template<LayerType TLayer>
std::array<wxPropertyGrid *, 2> MaterialPaletteBrowser_NEW<TLayer>::CreateElectricalMaterialPropertyGrids(wxWindow * parent)
{
    std::array<wxPropertyGrid *, 2> pgs;

    pgs[0] = CreatePropertyGrid(parent);
    pgs[1] = CreatePropertyGrid(parent);

    AddBoolProperty(pgs[0], "IsSelfPowered", _("Self-Powered"));
    AddBoolProperty(pgs[0], "ConductsElectricity", _("Conductive"));
    AddFloatProperty(pgs[0], "HeatGenerated", _("Heat Generated (KJ/s)"));
    AddBoolProperty(pgs[0], "IsInstanced", _("Instanced"));

    pgs[0]->FitColumns();

    // Leave second one empty

    pgs[1]->FitColumns();

    return pgs;
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::PopulateMaterialProperties(TMaterial const * material)
{
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
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

                case StructuralMaterial::MaterialCombustionType::FireExtinguishingExplosion:
                {
                    mStructuralMaterialPropertyGrids[1]->SetPropertyValue("CombustionType", _("Fire-Extinguishing"));
                    break;
                }
            }

            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("IgnitionTemperature", material->IgnitionTemperature);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("MeltingTemperature", material->MeltingTemperature);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("SpecificHeat", material->SpecificHeat);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("ThermalConductivity", material->ThermalConductivity);
            mStructuralMaterialPropertyGrids[1]->SetPropertyValue("ThermalExpansionCoefficient", material->ThermalExpansionCoefficient * 1000'000.0f);
        }

        mStructuralMaterialPropertyGrids[0]->Thaw();
        mStructuralMaterialPropertyGrids[1]->Thaw();
    }
    else
    {
        assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        mElectricalMaterialPropertyGrids[0]->Freeze();
        mElectricalMaterialPropertyGrids[1]->Freeze();

        if (material == nullptr)
        {
            for (int iGrid = 0; iGrid < 2; ++iGrid)
            {
                for (auto it = mElectricalMaterialPropertyGrids[iGrid]->GetIterator(); !it.AtEnd(); ++it)
                {
                    it.GetProperty()->SetValueToUnspecified();
                }
            }
        }
        else
        {
            mElectricalMaterialPropertyGrids[0]->SetPropertyValue("IsSelfPowered", material->IsSelfPowered);
            mElectricalMaterialPropertyGrids[0]->SetPropertyValue("ConductsElectricity", material->ConductsElectricity);
            mElectricalMaterialPropertyGrids[0]->SetPropertyValue("HeatGenerated", material->HeatGenerated);
            mElectricalMaterialPropertyGrids[0]->SetPropertyValue("IsInstanced", material->IsInstanced);

            mElectricalMaterialPropertyGrids[1]->Clear();

            auto typeProp = AddStringProperty(mElectricalMaterialPropertyGrids[1], "Type", _("Type"));
            int grid1PropertyCount = 1;

            // Type-specific
            switch (material->ElectricalType)
            {
                case ElectricalMaterial::ElectricalElementType::Cable:
                {
                    typeProp->SetValue(_("Cable"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::Engine:
                {
                    typeProp->SetValue(_("Engine"));

                    {
                        auto prop = AddStringProperty(mElectricalMaterialPropertyGrids[1], "EngineType", _("Engine Type"));
                        ++grid1PropertyCount;
                        switch (material->EngineType)
                        {
                            case ElectricalMaterial::EngineElementType::Diesel:
                            {
                                prop->SetValue(_("Diesel"));
                                break;
                            }

                            case ElectricalMaterial::EngineElementType::Jet:
                            {
                                prop->SetValue(_("Jet"));
                                break;
                            }

                            case ElectricalMaterial::EngineElementType::Outboard:
                            {
                                prop->SetValue(_("Outboard"));
                                break;
                            }

                            case ElectricalMaterial::EngineElementType::Steam:
                            {
                                prop->SetValue(_("Steam"));
                                break;
                            }
                        }
                    }

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "EnginePower", _("Power (HP)"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->EnginePower);
                    }

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "EngineDirection", _("Direction (rad)"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->EngineCCWDirection);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::EngineController:
                {
                    typeProp->SetValue(_("Engine Controller"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::EngineTransmission:
                {
                    typeProp->SetValue(_("Engine Transmission"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::Generator:
                {
                    typeProp->SetValue(_("Generator"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
                {
                    typeProp->SetValue(_("Interactive Switch"));

                    {
                        auto prop = AddStringProperty(mElectricalMaterialPropertyGrids[1], "SwitchType", _("Switch Type"));
                        ++grid1PropertyCount;
                        switch (material->InteractiveSwitchType)
                        {
                            case ElectricalMaterial::InteractiveSwitchElementType::Push:
                            {
                                prop->SetValue(_("Push Button"));
                                break;
                            }

                            case ElectricalMaterial::InteractiveSwitchElementType::Toggle:
                            {
                                prop->SetValue(_("Toggle Switch"));
                                break;
                            }
                        }
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::Lamp:
                {
                    typeProp->SetValue(_("Lamp"));

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "Luminiscence", _("Luminescence"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->Luminiscence);
                    }

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "LightSpread", _("Spread"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->LightSpread);
                    }

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "WetFailureRate", _("Watertight Factor"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->WetFailureRate);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::OtherSink:
                {
                    typeProp->SetValue(_("Other Sink"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::PowerMonitor:
                {
                    typeProp->SetValue(_("Power Monitor"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::ShipSound:
                {
                    typeProp->SetValue(_("Sound Emitter"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
                {
                    typeProp->SetValue(_("Smoke Emitter"));

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "ParticleEmissionRate", _("Emission Rate"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->ParticleEmissionRate);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::ThermalSwitch:
                {
                    typeProp->SetValue(_("Thermal Switch"));

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "ThermalSwitchTransitionTemperature", _("Threshold Temperature (K)"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->ThermalSwitchTransitionTemperature);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::TimerSwitch:
                {
                    typeProp->SetValue(_("Timed Switch"));

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "TimerDurationSeconds", _("Timer Duration (s)"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->TimerDurationSeconds);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::WaterPump:
                {
                    typeProp->SetValue(_("Water Pump"));

                    {
                        auto prop = AddFloatProperty(mElectricalMaterialPropertyGrids[1], "WaterPumpNominalForce", _("Nominal Force"));
                        ++grid1PropertyCount;
                        prop->SetValue(material->WaterPumpNominalForce);
                    }

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
                {
                    typeProp->SetValue(_("Water Switch"));
                    break;
                }

                case ElectricalMaterial::ElectricalElementType::WatertightDoor:
                {
                    typeProp->SetValue(_("Watertight Door"));
                    break;
                }
            }

            // Fill-in second grid with dummy properties
            for (; grid1PropertyCount < 4; ++grid1PropertyCount)
            {
                AddStringProperty(mElectricalMaterialPropertyGrids[1], std::string("Dummy") + std::to_string(grid1PropertyCount), _(""));
            }

            mElectricalMaterialPropertyGrids[1]->FitColumns();
            mElectricalMaterialPropertyGrids[1]->GetParent()->Layout();
        }

        mElectricalMaterialPropertyGrids[0]->Thaw();
        mElectricalMaterialPropertyGrids[1]->Thaw();
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::SetMaterialSelected(TMaterial const * material)
{
    wxWindowUpdateLocker locker(this);

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
        assert(TLayer != LayerType::Ropes);
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

    MaterialPalettePanel<TLayer> * selectedCategoryPanel = nullptr;

    for (size_t i = 0; i < mCategoryPanels.size(); ++i)
    {
        if (i == iCategorySelected)
        {
            // This is the panel we want to be shown

            selectedCategoryPanel = mCategoryPanels[i];

            // Make it visible
            mCategoryPanelsContainerSizer->Show(selectedCategoryPanel, true);

            // Set selection on the material
            selectedCategoryPanel->SetSelected(material);
        }
        else
        {
            mCategoryPanelsContainerSizer->Show(mCategoryPanels[i], false);
        }
    }

    // Make our container as wide as the visible panel - plus some slack for the scrollbars,
    // will eventually shrink
    if (selectedCategoryPanel != nullptr)
    {
        auto const visiblePanelWidth = selectedCategoryPanel->GetSize().x;
        mCategoryPanelsContainer->SetMinSize(wxSize(visiblePanelWidth, MinCategoryPanelsContainerHeight));
    }

    // Resize whole popup now that category panel has changed its size
    Layout(); // Will make visibility changes in the container effective
    mRootHSizer->SetSizeHints(this); // this->Fit() and this->SetSizeHints

    if (mCategoryPanelsContainer->HasScrollbar(wxVERTICAL))
    {
        // Take scrollbars into account
        if (selectedCategoryPanel != nullptr)
        {
            auto const scrollbarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, mCategoryPanelsContainer);
            mCategoryPanelsContainer->SetMinSize(wxSize(selectedCategoryPanel->GetSize().x + scrollbarWidth, MinCategoryPanelsContainerHeight));
        }

        // Resize whole popup now that category panel has changed its size
        Layout();
        mRootHSizer->SetSizeHints(this); // this->Fit() and this->SetSizeHints
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::OnMaterialClicked(TMaterial const * material)
{
    assert(mCurrentPlane.has_value());

    // Fire event
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
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
        assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

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

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::OnMaterialHoveredIn(TMaterial const * material)
{
    PopulateMaterialProperties(material);

    if constexpr (TLayer == LayerType::Electrical)
    {
        if (material != nullptr
            && material->ElectricalType == ElectricalMaterial::ElectricalElementType::ShipSound
            && mSoundController != nullptr)
        {
            mSoundController->PlayOneShotShipSound(material->ShipSoundType);
        }
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser_NEW<TLayer>::OnMaterialHoveredOut()
{
    PopulateMaterialProperties(nullptr);

    if constexpr (TLayer == LayerType::Electrical)
    {
        if (mSoundController != nullptr)
        {
            mSoundController->PlayOneShotShipSound(std::nullopt);
        }
    }
}

//
// Explicit specializations for all material layers
//

template class MaterialPaletteBrowser_NEW<LayerType::Structural>;
template class MaterialPaletteBrowser_NEW<LayerType::Electrical>;
template class MaterialPaletteBrowser_NEW<LayerType::Ropes>;

}