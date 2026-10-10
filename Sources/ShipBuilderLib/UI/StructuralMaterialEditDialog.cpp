/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StructuralMaterialEditDialog.h"

namespace ShipBuilder {

StructuralMaterialEditDialog::StructuralMaterialEditDialog(wxWindow * parent)
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

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::RunForNew(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
{
    SetTitle(_T("Create New Material"));
    return Run(overrides, baseMaterial);
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::RunForEdit(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
{
    SetTitle(_T("Edit Material"));
    return Run(overrides, baseMaterial);
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::Run(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
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