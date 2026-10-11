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
#include <wx/sizer.h>

#include <array>

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

    SliderControl<float> * MakeHeatGeneratedSliderControl(wxPanel * parent);

private:

    //
    // UI
    //

    wxBoxSizer * mHSizer;

    SliderControl<float> * mHeatGeneratedSlider;

    enum SpecializedPanelKind
    {
        Others = 0,
        Lamp = 1,
        Engine = 2,
        WaterPump = 3,
        TimerSwitch = 4,

        _Last = TimerSwitch
    };

    std::array<wxPanel *, SpecializedPanelKind::_Last + 1> mSpecializedPanels;

    // Others
    SliderControl<float> * mOthersHeatGeneratedSlider;

    // Lamp
    SliderControl<float> * mLampHeatGeneratedSlider;
    SliderControl<float> * mLuminiscenceSlider;
    SliderControl<float> * mLightSpreadSlider;

    // Engine
    SliderControl<float> * mEngineHeatGeneratedSlider;
    SliderControl<float> * mEnginePowerSlider;

    // WaterPump
    SliderControl<float> * mWaterPumpHeatGeneratedSlider;
    SliderControl<float> * mWaterPumpNominalForceSlider;

    // TimerSwitch
    SliderControl<float> * mTimerDurationSecondsSlider;

    //
    // State
    //

    float mBaseWaterPumpNominalForce;
};

}