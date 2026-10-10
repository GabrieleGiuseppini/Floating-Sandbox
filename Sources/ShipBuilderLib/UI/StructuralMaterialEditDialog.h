/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/Materials.h>

#include <wx/dialog.h>

#include <optional>

namespace ShipBuilder {

class StructuralMaterialEditDialog : public wxDialog
{
public:

    StructuralMaterialEditDialog(wxWindow * parent);

    std::optional<StructuralMaterial::VariantOverridesType> RunForNew(StructuralMaterial::VariantOverridesType & overrides, StructuralMaterial const * baseMaterial);
    std::optional<StructuralMaterial::VariantOverridesType> RunForEdit(StructuralMaterial::VariantOverridesType & overrides, StructuralMaterial const * baseMaterial);

private:

    std::optional<StructuralMaterial::VariantOverridesType> Run(StructuralMaterial::VariantOverridesType & overrides, StructuralMaterial const * baseMaterial);

private:
};

}