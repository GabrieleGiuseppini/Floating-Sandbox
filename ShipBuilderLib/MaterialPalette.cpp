/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

#include <GameCore/Log.h>

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/scrolwin.h>

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
        dynamic_cast<wxScrolledWindow *>(mCategoryListPanel)->SetScrollRate(0, 5);

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
                            SelectMaterial(&categoryHeadMaterial);
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
                    wxStaticText * label = new wxStaticText(mCategoryListPanel, wxID_ANY, category.Name);

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
                            OnMaterialSelected(nullptr);
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
    wxPoint const & position,
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    TMaterial const * initialMaterial)
{
    // Remember current plane for this session
    assert(!mCurrentPlane.has_value());
    mCurrentPlane = planeType;

    // Select material
    SelectMaterial(initialMaterial);

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
    int constexpr RowsPerSubcategory = (TMaterial::Layer == MaterialLayerType::Structural ? 3 : 2);

    wxScrolledWindow * categoryPanel = new wxScrolledWindow(parent);
    categoryPanel->SetScrollRate(5, 5);

    wxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

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
                TMaterial const & material = subCategory.Materials[iMaterial].get();

                // Button
                {
                    wxToggleButton * categoryButton = CreateMaterialButton(categoryPanel, PaletteButtonSize, material, shipTexturizer);

                    categoryButton->Bind(
                        wxEVT_LEFT_DOWN,
                        [this, material](wxMouseEvent & /*event*/)
                        {
                            OnMaterialSelected(&material);
                        });

                    gridSizer->Add(
                        categoryButton,
                        wxGBPosition(iSubCategory * RowsPerSubcategory, iMaterial),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
                        0);
                }

                // Name
                {
                    wxStaticText * nameLabel = new wxStaticText(categoryPanel, wxID_ANY, material.Name);

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
                        << "Mass: " << material.GetMass()
                        << " Strength: " << material.Strength;

                    wxStaticText * dataLabel = new wxStaticText(categoryPanel, wxID_ANY, ss.str());

                    gridSizer->Add(
                        dataLabel,
                        wxGBPosition(iSubCategory * RowsPerSubcategory + 2, iMaterial),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP,
                        0);
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
void MaterialPalette<TMaterial>::SelectMaterial(TMaterial const * material)
{
    //
    // Find category index
    //

    size_t iCategoryToSelect = 0;

    if (material != nullptr)
    {
        assert(material->PaletteCoordinates.has_value());

        // Select specified material
        for (size_t iCategory = 0; iCategory < mMaterialPalette.Categories.size(); ++iCategory)
        {
            auto const & category = mMaterialPalette.Categories[iCategory];
            if (category.Name == material->PaletteCoordinates->Category)
            {
                iCategoryToSelect = iCategory;
                break;
            }
        }
    }
    else
    {
        // "Clear" material
        iCategoryToSelect = mMaterialPalette.Categories.size();
    }

    Freeze();

    //
    // Select category button
    //

    for (size_t i = 0; i < mCategoryButtons.size(); ++i)
    {
        mCategoryButtons[i]->SetValue(i == iCategoryToSelect);
    }

    //
    // Select category panel
    //

    for (size_t i = 0; i < mCategoryPanels.size(); ++i)
    {
        mSizer->Show(
            mCategoryPanels[i],
            i == iCategoryToSelect);

        // TODO: select right toggle button
    }

    Layout();

    // Resize ourselves now
    mSizer->SetSizeHints(this);
    Fit();

    Thaw();
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::OnMaterialSelected(TMaterial const * material)
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

    // Forget plane for this session
    mCurrentPlane.reset();
}

//
// Explicit specializations for all material layers
//

template class MaterialPalette<StructuralMaterial>;
template class MaterialPalette<ElectricalMaterial>;

}