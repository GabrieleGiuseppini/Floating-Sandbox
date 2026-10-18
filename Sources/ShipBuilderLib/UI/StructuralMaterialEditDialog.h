/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "BaseMaterialEditDialog.h"

#include <Simulation/Materials.h>

#include <UILib/SliderControl.h>

#include <wx/panel.h>

namespace ShipBuilder {

class StructuralMaterialEditDialog final : public BaseMaterialEditDialog<StructuralMaterial>
{
public:

    StructuralMaterialEditDialog(wxWindow * parent)
        : BaseMaterialEditDialog<StructuralMaterial>(parent)
    {
        LayoutBase();
    }

private:

    void LayoutControls(wxBoxSizer * dialogVSizer) override;

    std::optional<StructuralMaterial::VariantOverridesType> Run(StructuralMaterial::VariantOverridesType const & startingOverrides, StructuralMaterial const * baseMaterial) override;

    void SetRenderColorButtonColor(rgbaColor const & color);

    static wxColor GetOverridesRenderColor(StructuralMaterial::VariantOverridesType const & overrides);

private:

    //
    // UI
    //

    // Basic
    wxPanel * mRenderColorButton;
    SliderControl<float> * mOpacitySlider;
    SliderControl<float> * mMassSlider;
    SliderControl<float> * mStrengthSlider;

    // Advanced
    SliderControl<float> * mIgnitionTemperatureSlider;
    SliderControl<float> * mMeltingTemperatureSlider;
    SliderControl<float> * mRotReceptivitySlider;
    SliderControl<float> * mRustReceptivitySlider;
    SliderControl<float> * mWaterSolubilitySlider;

    //
    // State
    //

    float mBaseNominalMass;
};

}