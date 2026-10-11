/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "BaseMaterialEditDialog.h"

#include <Simulation/Materials.h>

#include <UILib/SliderControl.h>

namespace ShipBuilder {

class ElectricalMaterialEditDialog final : public BaseMaterialEditDialog<ElectricalMaterial>
{
public:

    ElectricalMaterialEditDialog(wxWindow * parent)
        : BaseMaterialEditDialog<ElectricalMaterial>(parent)
    {
        LayoutBase();
    }

private:

    void LayoutControls(wxBoxSizer * dialogVSizer) override;

    std::optional<ElectricalMaterial::VariantOverridesType> Run(ElectricalMaterial::VariantOverridesType & overrides, ElectricalMaterial const * baseMaterial) override;

private:

    //
    // UI
    //

    SliderControl<float> * mHeatGeneratedSlider;
};

}