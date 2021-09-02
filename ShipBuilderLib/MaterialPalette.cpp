/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

#include <GameCore/ImageSize.h>

#include <UILib/WxHelpers.h>

#include <wx/scrolwin.h>
#include <wx/sizer.h>

#include <cassert>


ImageSize constexpr CategoryButtonSize(60, 80);


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

    wxBoxSizer * mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Category list
    {
        wxScrolledWindow * categoryListPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        wxBoxSizer * categoryListSizer = new wxBoxSizer(wxVERTICAL);

        // All material categories
        for (auto const & category : materialPalette.Categories)
        {
            // Take first material
            assert(category.SubCategories.size() > 0 && category.SubCategories[0].Materials.size() > 0);
            TMaterial const & material = category.SubCategories[0].Materials[0];

            // Create category button
            {
                wxToggleButton * categoryButton = new wxToggleButton(categoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

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

                categoryListSizer->Add(
                    categoryButton,
                    0,
                    wxALIGN_CENTER_HORIZONTAL,
                    0);

                mCategoryButtons.push_back(categoryButton);
            }

            // Create label
            {
                wxStaticText * label = new wxStaticText(categoryListPanel, wxID_ANY, category.Name);

                categoryListSizer->Add(
                    label,
                    0,
                    wxBOTTOM | wxALIGN_CENTER_HORIZONTAL,
                    10);
            }

            // Create palette panel for this category
            {

                // tODOHERE
            }
        }

        // "clear" category
        {
            static std::string const ClearMaterialName = "Clear";

            // Create category button
            {
                wxToggleButton * categoryButton = new wxToggleButton(categoryListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

                categoryButton->SetBitmap(
                    WxHelpers::LoadBitmap(
                        "null_material",
                        CategoryButtonSize,
                        resourceLocator));

                categoryButton->SetToolTip(ClearMaterialName);

                categoryListSizer->Add(
                    categoryButton,
                    0,
                    wxALIGN_CENTER_HORIZONTAL,
                    0);

                mCategoryButtons.push_back(categoryButton);
            }

            // Create label
            {
                wxStaticText * label = new wxStaticText(categoryListPanel, wxID_ANY, ClearMaterialName);

                categoryListSizer->Add(
                    label,
                    0,
                    wxBOTTOM | wxALIGN_CENTER_HORIZONTAL,
                    10);
            }
        }

        categoryListPanel->SetSizerAndFit(categoryListSizer);

        mainSizer->Add(
            categoryListPanel,
            0,
            wxEXPAND,
            0);
    }

    SetSizerAndFit(mainSizer);
}

template<typename TMaterial>
void MaterialPalette<TMaterial>::Open(
    wxPoint const & position,
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    TMaterial const * initialMaterial)
{
    // TODOHERE
    SetPosition(position);
    Popup();
}

//
// Explicit specializations for all material layers
//

template class MaterialPalette<StructuralMaterial>;
template class MaterialPalette<ElectricalMaterial>;

}