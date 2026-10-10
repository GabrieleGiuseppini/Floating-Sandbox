/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalMaterialEditDialog.h"

namespace ShipBuilder {

ElectricalMaterialEditDialog::ElectricalMaterialEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP);

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

std::optional<ElectricalMaterial::VariantOverridesType> ElectricalMaterialEditDialog::RunForNew(
    ElectricalMaterial::VariantOverridesType & overrides,
    ElectricalMaterial const * baseMaterial)
{
    SetTitle(_T("Create New Material"));
    return Run(overrides, baseMaterial);
}

std::optional<ElectricalMaterial::VariantOverridesType> ElectricalMaterialEditDialog::RunForEdit(
    ElectricalMaterial::VariantOverridesType & overrides,
    ElectricalMaterial const * baseMaterial)
{
    SetTitle(_T("Edit Material"));
    return Run(overrides, baseMaterial);
}

std::optional<ElectricalMaterial::VariantOverridesType> ElectricalMaterialEditDialog::Run(
    ElectricalMaterial::VariantOverridesType & overrides,
    ElectricalMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    // TODO
    (void)overrides;
    (void)baseMaterial;

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

}