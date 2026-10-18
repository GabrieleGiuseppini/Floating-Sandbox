/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2026-08-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPaletteBrowser.h"

#include "ElectricalMaterialEditDialog.h"

#include <UILib/SharedUIResources.h>
#include <UILib/WxHelpers.h>

#include <Core/GameExceptions.h>

#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/wupdlock.h>

#include <algorithm>
#include <cassert>

namespace ShipBuilder {

auto constexpr UndockedFrameStyle = wxCLOSE_BOX | wxRESIZE_BORDER | wxCAPTION | wxFRAME_FLOAT_ON_PARENT;
auto constexpr DockedFrameStyle = wxFRAME_FLOAT_ON_PARENT | wxBORDER_SIMPLE | wxFRAME_NO_TASKBAR;

int constexpr MinCategoryPanelsContainerHeight = 400; // Min height of the scrollable panel that contains the swaths; without a min height, a palette that only has a few categories would be too short
ImageSize constexpr CategoryButtonSize(80, 60);

template<LayerType TLayer>
MaterialPaletteBrowser<TLayer>::MaterialPaletteBrowser(
    wxWindow * parent,
    wxIcon const & icon,
    IMaterialPalettesController & materialPalettesController,
    DefaultMaterialDatabase::Palette<TMaterial> const & defaultMaterialPalette,
    std::vector<std::unique_ptr<TMaterial const>> const & initialCustomUserMaterials,
    ShipTexturizer const & shipTexturizer,
    ISoundController * soundController,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : wxFrame(
        parent,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxDefaultSize,
        DockedFrameStyle)
    , mMaterialPalettesController(materialPalettesController)
    , mDefaultMaterialPalette(defaultMaterialPalette)
    , mSoundController(soundController)
    , mCurrentPlane()
    , mIsDocked(true)
{
    SetIcon(icon);
    SetBackgroundColour(*wxWHITE);

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
            for (auto const & category : defaultMaterialPalette.Categories)
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

                    auto const [_, isInserted] = mCategoryButtonsByCategory.try_emplace(category.Name, categoryButton);
                    if (!isInserted)
                    {
                        throw GameException("Category \"" + category.Name + "\" is found duplicated among default " + LayerTypeToStr_snake(TLayer) + " materials' palette");
                    }
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

                    auto const [_, isInserted] = mCategoryButtonsByCategory.try_emplace("", categoryButton);
                    if (!isInserted)
                    {
                        throw GameException("Category \"\" is found duplicated among default " + LayerTypeToStr_snake(TLayer) + " materials' palette");
                    }
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

            mCategoryPanelsContainerSizer = new wxBoxSizer(wxHORIZONTAL); // Note: only panels added to this, as we search in it visible items by index

            size_t const categoriesCount = defaultMaterialPalette.Categories.size();
            for (size_t c = 0; c < categoriesCount; ++c)
            {
                auto const & category = defaultMaterialPalette.Categories[c];

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

                // Bind checked state change
                categoryPanel->Bind(
                    fsEVT_MATERIAL_PALETTE_CHECKED_STATE_CHANGED,
                    [this](fsCheckedStateChanged & /*event*/)
                    {
                        ReconciliateUIWithCurrentPanelCheckedState();
                    });

                mCategoryPanelsContainerSizer->Add(
                    categoryPanel,
                    0,
                    0,
                    0);

                auto const [_, isInserted] = mCategoryPanelsByCategory.try_emplace(category.Name, categoryPanel);
                if (!isInserted)
                {
                    throw GameException("Category \"" + category.Name + "\" is found duplicated among default " + LayerTypeToStr_snake(TLayer) + " materials' palette");
                }

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

        // Docking, material property grid(s), and controls
        {
            wxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            hSizer->AddStretchSpacer(1);

            // Docking
            {
                mDockCheckbox = new BitmappedCheckbox(
                    this,
                    wxID_ANY,
                    SharedUIResources::GetInstance().GetDockedIconLarge(),
                    SharedUIResources::GetInstance().GetUndockedIconLarge(),
                    "");

                mDockCheckbox->Bind(
                    wxEVT_CHECKBOX,
                    [this](wxCommandEvent &)
                    {
                        SetDocked(mDockCheckbox->IsChecked());
                        Refresh();

                        // Make us regain focus, or else we los focus loss notification
                        this->SetFocus();
                    });

                hSizer->Add(mDockCheckbox, 0, wxALIGN_CENTRE_VERTICAL);
            }

            hSizer->AddStretchSpacer(1);

            // Material property grid(s)
            {
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
            }

            hSizer->AddStretchSpacer(1);

            // Controls
            {
                wxGridBagSizer * gridSizer = new wxGridBagSizer(5, 5);

                {
                    mSelectAllButton = new wxButton(this, wxID_ANY, _T("Select All"), wxDefaultPosition, wxDefaultSize);

                    mSelectAllButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent &)
                        {
                            // Selects all visible ones - i.e. current Layer, current Category

                            auto * panel = GetCurrentlyShownPanel();
                            if (panel != nullptr) // Safety
                            {
                                panel->SetAllCustomMaterialsIsChecked(true);
                            }

                            // Make us regain focus, or else we los focus loss notification
                            this->SetFocus();
                        });

                    gridSizer->Add(
                        mSelectAllButton,
                        wxGBPosition(0, 0),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTRE);
                }

                {
                    mClearSelectionButton = new wxButton(this, wxID_ANY, _T("Clear Selection"), wxDefaultPosition, wxDefaultSize);

                    mClearSelectionButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent &)
                        {
                            // Unselects all visible ones - i.e. current Layer, current Category

                            auto * panel = GetCurrentlyShownPanel();
                            if (panel != nullptr) // Safety
                            {
                                panel->SetAllCustomMaterialsIsChecked(false);
                            }

                            // Make us regain focus, or else we los focus loss notification
                            this->SetFocus();
                        });

                    gridSizer->Add(
                        mClearSelectionButton,
                        wxGBPosition(0, 1),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTRE);
                }

                {
                    mDeleteSelectedButton = new wxButton(this, wxID_ANY, _T("Delete Selected"), wxDefaultPosition, wxDefaultSize);

                    mDeleteSelectedButton->Bind(
                        wxEVT_BUTTON,
                        [this](wxCommandEvent &)
                        {
                            // Deletes only from current Layer, current Category -- otherwise dangerous

                            // TODOHERE
                            // - Get vector<TMaterial const *> from GetCurrentlyShownPanel()->GetCheckedMaterials()
                            // - Call onto controller with that vector

                            // Make us regain focus, or else we los focus loss notification
                            this->SetFocus();
                        });

                    gridSizer->Add(
                        mDeleteSelectedButton,
                        wxGBPosition(1, 0),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTRE);
                }

                WxHelpers::MakeAllRowsExpandable(gridSizer);

                hSizer->Add(gridSizer, 0, wxEXPAND);
            }

            hSizer->AddStretchSpacer(1);

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

    SetSizer(mRootHSizer);

    //
    // Populate with these initial custom user materials
    //

    std::vector<TMaterial const *> initialCustomUserMaterialsPtr;
    std::transform(
        initialCustomUserMaterials.cbegin(),
        initialCustomUserMaterials.cend(),
        std::back_inserter(initialCustomUserMaterialsPtr),
        [](auto const & uq_ptr) -> TMaterial const *
        {
            return uq_ptr.get();
        });

    AddUserCustomMaterials(initialCustomUserMaterialsPtr);

    //
    // Connect events
    //

    Bind(
        wxEVT_KILL_FOCUS,
        [this](wxFocusEvent & focusEvent)
        {
            // Ignore if focus is given to a child
            if (IsDescendant(focusEvent.GetWindow()))
                return;

            if (mIsDocked)
            {
                if (!mStickAroundOnFocusLoss)
                {
                    Hide();
                }
            }
        });

    Bind(
        wxEVT_CLOSE_WINDOW,
        [this](wxCloseEvent & event)
        {
            if (event.CanVeto())
            {
                // User pressed ALT+F4 or clicked CloseBox...
                // ...do not allow close()
                event.Veto();

                if (!mIsDocked)
                {
                    // Closing by hiding
                    Hide();
                }
            }
        });
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::Open(
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    TMaterial const * initialMaterial)
{
    // Remember current plane for this session
    mCurrentPlane = planeType;

    // Calculate docking rect
    mDockingRect = wxRect(
        referenceArea.GetLeftTop(),
        wxSize(
            referenceArea.GetSize().GetWidth() * 2 / 3,
            referenceArea.GetSize().GetHeight()));

    // Start docked
    SetDocked(true);
    mDockCheckbox->SetChecked(mIsDocked);

    // Clear material properties
    PopulateMaterialProperties(nullptr);

    // Select material - showing its category panel
    SetMaterialSelected(initialMaterial);

    // Take care of appearing vertical scrollbar in the category list
    mCategoryListPanelSizer->SetSizeHints(mCategoryListPanel);

    // Re-layout for (possibly) new size
    Layout();

    // From now open, close on focus loss
    mStickAroundOnFocusLoss = false;

    // Open
    Show();
    SetFocus();
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::Close()
{
    Hide();
}

template<LayerType TLayer>
std::optional<typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType> MaterialPaletteBrowser<TLayer>::RunNewCustomMaterial(
    typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType const & startingOverrides,
    TMaterial const * baseMaterial)
{
    assert(IsOpen());

    // Do not close ourselves when we'll lose the focus
    mStickAroundOnFocusLoss = true;

    std::optional<typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType> result;
    if constexpr (TLayer == LayerType::Structural || TLayer == LayerType::Ropes)
    {
        result = GetStructuralMaterialEditDialog().RunForNew(startingOverrides, baseMaterial);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        result = GetElectricalMaterialEditDialog().RunForNew(startingOverrides, baseMaterial);
    }

    // Go back to closing ourselves whenever we lose focus from now on
    mStickAroundOnFocusLoss = false;
    SetFocus();

    return result;
}

template<LayerType TLayer>
std::optional<typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType> MaterialPaletteBrowser<TLayer>::RunEditCustomMaterial(
    typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType const & startingOverrides,
    TMaterial const * baseMaterial)
{
    assert(IsOpen());

    // Do not close ourselves when we'll lose the focus
    mStickAroundOnFocusLoss = true;

    std::optional<typename MaterialPaletteBrowser<TLayer>::TMaterial::VariantOverridesType> result;
    if constexpr (TLayer == LayerType::Structural || TLayer == LayerType::Ropes)
    {
        result = GetStructuralMaterialEditDialog().RunForEdit(startingOverrides, baseMaterial);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        result = GetElectricalMaterialEditDialog().RunForEdit(startingOverrides, baseMaterial);
    }

    // Go back to closing ourselves whenever we lose focus from now on
    mStickAroundOnFocusLoss = false;
    SetFocus();

    return result;
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::AddUserCustomMaterials(std::vector<TMaterial const *> const & customMaterials)
{
    // Organize materials into Category->Materials map
    std::map<std::string, std::vector<TMaterial const *>> customMaterialsByCategory;
    for (TMaterial const * customMaterial : customMaterials)
    {
        assert(customMaterial->PaletteCoordinates.has_value());
        customMaterialsByCategory[customMaterial->PaletteCoordinates->Category].push_back(customMaterial);
    }

    // Visit all these categories
    for (auto const it : customMaterialsByCategory)
    {
        // Get this panel
        auto panel = mCategoryPanelsByCategory.at(it.first);

        // Start layout
        auto layoutSession = panel->StartCustomMaterialsLayout();

        // Add materials
        for (TMaterial const * material : it.second)
        {
            layoutSession = panel->AddCustomMaterial(material, CustomMaterialRegisterKindType::User, layoutSession);
        }

        // Finish layout
        panel->EndCustomMaterialsLayout(layoutSession);
    }

    // Make sure that horizontal scrollbars show/hide for new panel size
    Layout();
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::SetShipCustomMaterials(std::vector<std::unique_ptr<TMaterial const>> const & shipCustomMaterials)
{
    // Organize materials into Category->Materials map
    std::map<std::string, std::vector<TMaterial const *>> newShipCustomMaterialsByCategory;
    for (auto const & shipCustomMaterial : shipCustomMaterials)
    {
        assert(shipCustomMaterial->PaletteCoordinates.has_value());
        newShipCustomMaterialsByCategory[shipCustomMaterial->PaletteCoordinates->Category].push_back(shipCustomMaterial.get());
    }

    //
    // Visit all our categories; for each:
    //  - Remove its ship materials
    //  - Add ship materials
    //

    for (auto & entry : mCategoryPanelsByCategory)
    {
        // Get this panel
        auto panel = entry.second;

        // Start layout
        auto layoutSession = panel->StartCustomMaterialsLayout();

        // Remove its ships
        layoutSession = panel->RemoveAllShipCustomMaterials(layoutSession);

        // Add all ship's, if any
        auto srchIt = newShipCustomMaterialsByCategory.find(entry.first);
        if (srchIt != newShipCustomMaterialsByCategory.end())
        {
            for (TMaterial const * newShipMaterial : srchIt->second)
            {
                layoutSession = panel->AddCustomMaterial(newShipMaterial, CustomMaterialRegisterKindType::Ship, layoutSession);
            }
        }

        // Finish layout
        panel->EndCustomMaterialsLayout(layoutSession);
    }

    // Make sure that horizontal scrollbars show/hide for new panel size
    Layout();
}

template<LayerType TLayer>
MaterialPalettePanel<TLayer> * MaterialPaletteBrowser<TLayer>::CreateCategoryPanel(
    wxWindow * parent,
    typename DefaultMaterialDatabase::Palette<TMaterial>::Category const & defaultMaterialCategory,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager)
{
    //
    // Create and build panel
    //

    MaterialPalettePanel<TLayer> * categoryPanel = new MaterialPalettePanel<TLayer>(
        parent,
        mMaterialPalettesController,
        shipTexturizer,
        gameAssetManager);

    categoryPanel->StartDefaultMaterialsLayout();

    std::optional<typename DefaultMaterialDatabase::Palette<TMaterial>::Category::SubCategory::Group> currentGroup;
    for (size_t iSubCategory = 0; iSubCategory < defaultMaterialCategory.SubCategories.size(); ++iSubCategory) // Rows
    {
        auto const & subCategory = defaultMaterialCategory.SubCategories[iSubCategory];

        // Check if a group change
        if (currentGroup.has_value() && subCategory.ParentGroup.UniqueId != currentGroup->UniqueId)
        {
            categoryPanel->AddSeparatorRow();
        }

        // Remember this group
        currentGroup = subCategory.ParentGroup;

        //
        // Populate row
        //

        categoryPanel->StartNewSubcategoryRow(subCategory.Name);

        // Materials
        unsigned int currentBaseMaterialOrdinal = std::numeric_limits<unsigned int>::max();
        TMaterial const * currentBaseMaterial = nullptr;
        for (size_t iMaterial = 0; iMaterial < subCategory.Materials.size(); ++iMaterial) // Cols
        {
            TMaterial const * material = &(subCategory.Materials[iMaterial].get());

            // Check if first of new stride
            if (material->PaletteSubCategoryBaseMaterialOrdinal != currentBaseMaterialOrdinal)
            {
                // First of new stride

                // Check if we've finished a stride
                if (iMaterial > 0)
                {
                    assert(currentBaseMaterial != nullptr);
                    if (IsOverridableMaterial(currentBaseMaterial))
                    {
                        categoryPanel->AddCreateNewCustomMaterialButton(currentBaseMaterial);
                    }
                }

                // Start new stride
                currentBaseMaterialOrdinal = material->PaletteSubCategoryBaseMaterialOrdinal;
                currentBaseMaterial = &(subCategory.Materials[currentBaseMaterialOrdinal].get());
                categoryPanel->StartNewMaterialStride(currentBaseMaterialOrdinal);
            }

            categoryPanel->AddDefaultMaterial(material);
        }

        // Add last button
        assert(currentBaseMaterial != nullptr); // We've had at least one material
        if (IsOverridableMaterial(currentBaseMaterial))
        {
            categoryPanel->AddCreateNewCustomMaterialButton(currentBaseMaterial);
        }
    }

    categoryPanel->EndDefaultMaterialsLayout();

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
    {
        // If a child control gains focus, give it back to parent - assumed to be this frame (browser)
        // (or else browser loses the ability to notice focus loss)
        Bind(
            wxEVT_CHILD_FOCUS,
            [this](wxChildFocusEvent & /*focusEvent*/)
            {
                GetParent()->SetFocus();
            });
    }

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
std::array<wxPropertyGrid *, 2> MaterialPaletteBrowser<TLayer>::CreateStructuralMaterialPropertyGrids(wxWindow * parent)
{
    std::array<wxPropertyGrid *, 2> pgs;

    pgs[0] = CreatePropertyGrid(parent);
    pgs[1] = CreatePropertyGrid(parent);

    AddStringProperty(pgs[0], "Name", _("Name"));
    AddFloatProperty(pgs[0], "Mass", _("Mass (Kg)"));
    AddFloatProperty(pgs[0], "Strength", _("Strength"));
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
std::array<wxPropertyGrid *, 2> MaterialPaletteBrowser<TLayer>::CreateElectricalMaterialPropertyGrids(wxWindow * parent)
{
    std::array<wxPropertyGrid *, 2> pgs;

    pgs[0] = CreatePropertyGrid(parent);
    pgs[1] = CreatePropertyGrid(parent);

    AddStringProperty(pgs[0], "Name", _("Name"));
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
void MaterialPaletteBrowser<TLayer>::PopulateMaterialProperties(TMaterial const * material)
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
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Name", wxString(material->Name));
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Mass", material->GetMass());
            mStructuralMaterialPropertyGrids[0]->SetPropertyValue("Strength", material->Strength);
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
            mElectricalMaterialPropertyGrids[0]->SetPropertyValue("Name", wxString(material->Name));
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
            for (; grid1PropertyCount < 5; ++grid1PropertyCount)
            {
                AddStringProperty(mElectricalMaterialPropertyGrids[1], std::string("Dummy") + std::to_string(grid1PropertyCount), "");
            }

            mElectricalMaterialPropertyGrids[1]->FitColumns();
            mElectricalMaterialPropertyGrids[1]->GetParent()->Layout();
        }

        mElectricalMaterialPropertyGrids[0]->Thaw();
        mElectricalMaterialPropertyGrids[1]->Thaw();
    }
}

template<LayerType TLayer>
StructuralMaterialEditDialog & MaterialPaletteBrowser<TLayer>::GetStructuralMaterialEditDialog()
{
    // Delayed creation of dialog

    if (!mStructuralMaterialEditDialog)
    {
        mStructuralMaterialEditDialog = std::make_unique<StructuralMaterialEditDialog>(this);
    }

    return *mStructuralMaterialEditDialog;
}

template<LayerType TLayer>
ElectricalMaterialEditDialog & MaterialPaletteBrowser<TLayer>::GetElectricalMaterialEditDialog()
{
    // Delayed creation of dialog

    if (!mElectricalMaterialEditDialog)
    {
        mElectricalMaterialEditDialog = std::make_unique<ElectricalMaterialEditDialog>(this);
    }

    return *mElectricalMaterialEditDialog;
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::SetMaterialSelected(TMaterial const * material)
{
    wxWindowUpdateLocker locker(this);

    std::string categoryName;
    if (material != nullptr)
    {
        categoryName = material->PaletteCoordinates->Category;
    }
    else
    {
        // Clear material
        categoryName = "";
    }

    //
    // Select category button and unselect all others
    //

    for (auto it : mCategoryButtonsByCategory)
    {
        it.second->SetValue(it.first == categoryName);
    }

    //
    // Select category panel, its material, and unselect all other materials
    //

    MaterialPalettePanel<TLayer> * selectedCategoryPanel = nullptr;

    for (auto it : mCategoryPanelsByCategory)
    {
        if (it.first == categoryName)
        {
            // This is the panel we want to be shown

            selectedCategoryPanel = it.second;

            // Make it visible
            mCategoryPanelsContainerSizer->Show(selectedCategoryPanel, true);

            // Set selection on the material
            selectedCategoryPanel->SetSelected(material);

            // Reconciliate UI with _this_ panel
            ReconciliateUIWithPanelCheckedState(selectedCategoryPanel);
        }
        else
        {
            mCategoryPanelsContainerSizer->Show(it.second, false);
        }
    }

    // Resize whole window now that category panel has changed its size
    Layout(); // Will make visibility changes in the container effective

    if (mCategoryPanelsContainer->HasScrollbar(wxVERTICAL))
    {
        // Take scrollbars into account
        if (selectedCategoryPanel != nullptr)
        {
            auto const scrollbarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, mCategoryPanelsContainer);
            mCategoryPanelsContainer->SetMinSize(wxSize(selectedCategoryPanel->GetSize().x + scrollbarWidth, MinCategoryPanelsContainerHeight));
        }

        // Resize whole window now that category panel has changed its size
        Layout();
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::OnMaterialClicked(TMaterial const * material)
{
    assert(mCurrentPlane.has_value());

    // Notify
    if constexpr (TLayer == LayerType::Structural)
    {
        mMaterialPalettesController.OnStructuralMaterialSelected(material, *mCurrentPlane);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        mMaterialPalettesController.OnElectricalMaterialSelected(material, *mCurrentPlane);
    }
    else
    {
        static_assert(TLayer == LayerType::Ropes);

        mMaterialPalettesController.OnRopesMaterialSelected(material, *mCurrentPlane);
    }

    if (mIsDocked)
    {
        // Close ourselves
        Hide();
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::OnMaterialHoveredIn(TMaterial const * material)
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
void MaterialPaletteBrowser<TLayer>::OnMaterialHoveredOut()
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

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::ReconciliateUIWithCurrentPanelCheckedState()
{
    auto * panel = GetCurrentlyShownPanel();
    if (panel != nullptr) // Safety
    {
        ReconciliateUIWithPanelCheckedState(panel);
    }
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::ReconciliateUIWithPanelCheckedState(MaterialPalettePanel<TLayer> * const panel)
{
    assert(panel != nullptr);

    mClearSelectionButton->Enable(panel->IsAnyChecked());
    mDeleteSelectedButton->Enable(panel->IsAnyChecked());
}

template<LayerType TLayer>
void MaterialPaletteBrowser<TLayer>::SetDocked(bool isDocked)
{
    if (isDocked)
    {
        //
        // Set docked
        //

        SetWindowStyle(DockedFrameStyle);

        SetPosition(mDockingRect.GetLeftTop());

        SetMinSize(
            wxSize(
                CategoryButtonSize.width * 10,
                CategoryButtonSize.height * 4));
        SetSize(mDockingRect.GetSize());

        // Get focus, so we may detect focus loss
        SetFocus();
    }
    else
    {
        //
        // Set undocked
        //

        SetWindowStyle(UndockedFrameStyle);
    }

    mIsDocked = isDocked;
}

template<LayerType TLayer>
MaterialPalettePanel<TLayer> * MaterialPaletteBrowser<TLayer>::GetCurrentlyShownPanel()
{
    // Find panel currently visible
    for (size_t i = 0; i < mCategoryPanelsContainerSizer->GetItemCount(); ++i)
    {
        if (mCategoryPanelsContainerSizer->IsShown(i))
        {
            // Assuming only one
            return dynamic_cast<MaterialPalettePanel<TLayer> *>(mCategoryPanelsContainerSizer->GetItem(i)->GetWindow());
        }
    }

    return nullptr;
}

template<LayerType TLayer>
bool MaterialPaletteBrowser<TLayer>::IsOverridableMaterial(TMaterial const * material)
{
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        return true;
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        return ElectricalMaterialEditDialog::IsOverridable(material);
    }
}

//
// Explicit specializations for all material layers
//

template class MaterialPaletteBrowser<LayerType::Structural>;
template class MaterialPaletteBrowser<LayerType::Electrical>;
template class MaterialPaletteBrowser<LayerType::Ropes>;

}