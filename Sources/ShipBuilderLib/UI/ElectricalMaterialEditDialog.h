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

class ElectricalMaterialEditDialog : public wxDialog
{
public:

    ElectricalMaterialEditDialog(wxWindow * parent);

    std::optional<ElectricalMaterial::VariantOverridesType> RunForNew(ElectricalMaterial::VariantOverridesType & overrides, ElectricalMaterial const * baseMaterial);
    std::optional<ElectricalMaterial::VariantOverridesType> RunForEdit(ElectricalMaterial::VariantOverridesType & overrides, ElectricalMaterial const * baseMaterial);

private:

    std::optional<ElectricalMaterial::VariantOverridesType> Run(ElectricalMaterial::VariantOverridesType & overrides, ElectricalMaterial const * baseMaterial);

private:
};

}