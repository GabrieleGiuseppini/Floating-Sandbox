/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

#include <GameCore/ImageSize.h>
#include <GameCore/Log.h>

#include <UILib/WxHelpers.h>

#include <wx/scrolwin.h>

#include <cassert>


ImageSize constexpr CategoryButtonSize(80, 60);


namespace ShipBuilder {

template<typename TMaterial>
MaterialPalette<TMaterial>::MaterialPalette(
    wxWindow * parent,
    MaterialDatabase::Palette<TMaterial> const & materialPalette,
    ShipTexturizer const & shipTexturizer,
    ResourceLocator const & resourceLocator)
    : wxPopupTransientWindow(parent, wxPU_CONTAINS_CONTROLS | wxBORDER_SIMPLE)
    , mCurrentPlaneType()
{
    SetBackgroundColour(wxColour("WHITE"));

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
                TMaterial const & material = category.SubCategories[0].Materials[0];

                // Create category button
                {
                    wxToggleButton * categoryButton = new wxToggleButton(mCategoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

                    if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
                    {
                        categoryButton->SetBitmap(
                            WxHelpers::MakeBitmap(
                                shipTexturizer.MakeTextureSample(
                                    std::nullopt, // Use shared settings
                                    CategoryButtonSize,
                                    material)));
                    }
                    else
                    {
                        static_assert(TMaterial::Layer == MaterialLayerType::Electrical);

                        categoryButton->SetBitmap(
                            WxHelpers::MakeMatteBitmap(
                                rgbaColor(material.RenderColor),
                                CategoryButtonSize));
                    }

                    categoryButton->SetToolTip(category.Name);

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

                    categoryButton->SetToolTip(ClearMaterialName);

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
                category);

            // TODOHERE

            mSizer->Add(
                categoryPanel,
                0,
                0,
                0);

            // Start hidden
            mSizer->Hide(categoryPanel);

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
    // Select specified material
    // TODO

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
    typename MaterialDatabase::Palette<TMaterial>::Category const & materialCategory)
{
    wxPanel * categoryPanel = new wxPanel(parent);

    // TODOHERE

    return categoryPanel;
}

//
// Explicit specializations for all material layers
//

template class MaterialPalette<StructuralMaterial>;
template class MaterialPalette<ElectricalMaterial>;

}