/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalMaterialEditDialog.h"

#include <Core/ExponentialSliderCore.h>

namespace ShipBuilder {

void ElectricalMaterialEditDialog::LayoutControls(wxBoxSizer * dialogVSizer)
{
    // TODO
    (void)dialogVSizer;
}

std::optional<ElectricalMaterial::VariantOverridesType> ElectricalMaterialEditDialog::Run(
    ElectricalMaterial::VariantOverridesType & overrides,
    ElectricalMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    mOverridesUnderEdit = overrides;
    auto baseOverrideValues = baseMaterial->MakeStartingVariantOverrides();

    mHeatGeneratedSlider->SetValueAndLimits(
        mOverridesUnderEdit->HeatGenerated,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.HeatGenerated * 0.1f,
            baseOverrideValues.HeatGenerated,
            baseOverrideValues.HeatGenerated * 10.0f));

    //
    // Run
    //

    return BaseRun();
}

}