/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

#include <GameCore/Log.h>

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>

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
    , mCurrentPlane()
{
    SetBackgroundColour(wxColour("WHITE"));

    {
        auto font = GetFont();
        font.SetPointSize(font.GetPointSize() - 2);

        SetFont(font);
    }

    mSizer = new wxBoxSizer(wxHORIZONTAL);

    // Category list
    {
        mCategoryListPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        mCategoryListPanel->SetScrollRate(0, 5);

        mCategoryListSizer = new wxBoxSizer(wxVERTICAL);

        // List
        {
            mCategoryListSizer->AddSpacer(4);

            // All material categories
            int TODO = 0;
            for (auto const & category : materialPalette.Categories)
            {
                //if (TODO++ > 5)
                //    break;
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
                            SetMaterialSelected(&categoryHeadMaterial, false);
                        });

                    mCategoryListSizer->Add(
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

                    mCategoryListSizer->Add(
                        label,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        3);
                }

                mCategoryListSizer->AddSpacer(10);
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

                    mCategoryListSizer->Add(
                        categoryButton,
                        0,
                        wxALIGN_CENTER_HORIZONTAL,
                        0);

                    mCategoryButtons.push_back(categoryButton);
                }

                // Create label
                {
                    wxStaticText * label = new wxStaticText(mCategoryListPanel, wxID_ANY, ClearMaterialName);

                    mCategoryListSizer->Add(
                        label,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        3);
                }
            }
        }

        mCategoryListPanel->SetSizerAndFit(mCategoryListSizer);

        mSizer->Add(
            mCategoryListPanel,
            0,
            wxEXPAND,
            0);
    }

    // Category panels
    {
        for (auto const & category : materialPalette.Categories)
        {
            wxPanel * categoryPanel = CreateCategoryPanel(
                this,
                category,
                shipTexturizer);

            mSizer->Add(
                categoryPanel,
                0,
                0,
                0);

            mCategoryPanels.push_back(categoryPanel);
        }
    }

    SetSizerAndFit(mSizer);
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::Open(
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    TMaterial const * initialMaterial)
{
    // Remember current plane for this session
    mCurrentPlane = planeType;

    // Select material, scorlling to it
    SetMaterialSelected(initialMaterial, true);

    // Position and dimension
    SetPosition(referenceArea.GetLeftTop());
    SetMaxSize(referenceArea.GetSize());
    Fit();

    // Take care of appearing vertical scrollbar
    mCategoryListSizer->SetSizeHints(mCategoryListPanel);
    Layout();
    Fit();

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
    wxScrolledWindow * categoryPanel = new wxScrolledWindow(parent);
    categoryPanel->SetScrollRate(5, 5);

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

                    materialButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this, material](wxMouseEvent & /*event*/)
                        {
                            OnMaterialClicked(material);
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
                        << " | S=" << material->Strength;

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

template<typename TMaterial>
void MaterialPalette<TMaterial>::SetMaterialSelected(
    TMaterial const * material,
    bool doScrollCategoryList)
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

    if (doScrollCategoryList)
    {
        // Make sure category list is scrolled so that button is visible
        auto const categoryButtonPosition = mCategoryButtons[iCategorySelected]->GetPosition();
        // TODOHERE
        LogMessage("TODOHERE: ", categoryButtonPosition.y);
    }

    //
    // Select category panel, its material, and unselect all other materials
    //

    for (size_t i = 0; i < mCategoryPanels.size(); ++i)
    {
        if (i == iCategorySelected)
        {
            mSizer->Show(mCategoryPanels[i], true);

            for (auto * button : mMaterialButtons[iCategorySelected])
            {
                button->SetValue(button->GetClientData() == material);
            }
        }
        else
        {
            mSizer->Show(mCategoryPanels[i], false);
        }
    }

    Layout();

    // Resize ourselves now
    mSizer->SetSizeHints(this);
    Fit();

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