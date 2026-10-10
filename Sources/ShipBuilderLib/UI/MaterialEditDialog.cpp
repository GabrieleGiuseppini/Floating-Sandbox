/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialEditDialog.h"

#include <Simulation/Materials.h>

#include <Core/GameTypes.h>

#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

template<typename TMaterial>
MaterialEditDialog<TMaterial>::MaterialEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    //
    // Layout
    //


    ////
    //// Finalize dialog
    ////

    //SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

template<typename TMaterial>
std::optional<typename MaterialEditDialog<TMaterial>::TVariantOverridesType> MaterialEditDialog<TMaterial>::RunForNew(
    TVariantOverridesType & overrides,
    TMaterial const * baseMaterial)
{
    SetTitle(_T("Create New Material"));
    return Run(overrides, baseMaterial);
}

template<typename TMaterial>
std::optional<typename MaterialEditDialog<TMaterial>::TVariantOverridesType> MaterialEditDialog<TMaterial>::RunForEdit(
    TVariantOverridesType & overrides,
    TMaterial const * baseMaterial)
{
    SetTitle(_T("Edit Material"));
    return Run(overrides, baseMaterial);
}

template<typename TMaterial>
std::optional<typename MaterialEditDialog<TMaterial>::TVariantOverridesType> MaterialEditDialog<TMaterial>::Run(
    TVariantOverridesType & overrides,
    TMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        // TODO
        (void)overrides;
        (void)baseMaterial;
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);

        // TODO
        (void)overrides;
        (void)baseMaterial;
    }

    //
    // Run
    //

    auto const result = wxDialog::ShowModal();
    if (result == wxID_OK)
    {
        // TODO
        return std::nullopt;
    }
    else
    {
        return std::nullopt;
    }
}

//
// Explicit specializations for all material layers
//

template class MaterialEditDialog<StructuralMaterial>;
template class MaterialEditDialog<ElectricalMaterial>;

}