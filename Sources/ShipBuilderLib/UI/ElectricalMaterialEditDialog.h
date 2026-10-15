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
#include <optional>

namespace ShipBuilder {

class ElectricalMaterialEditDialog final : public BaseMaterialEditDialog<ElectricalMaterial>
{
public:

    ElectricalMaterialEditDialog(wxWindow * parent)
        : BaseMaterialEditDialog<ElectricalMaterial>(parent)
    {
        LayoutBase();
    }

    static bool IsOverridable(ElectricalMaterial const * baseMaterial)
    {
        return GetPanelKind(baseMaterial).has_value();
    }

private:

    void LayoutControls(wxBoxSizer * dialogVSizer) override;

    std::optional<ElectricalMaterial::VariantOverridesType> Run(ElectricalMaterial::VariantOverridesType const & startingOverrides, ElectricalMaterial const * baseMaterial) override;

    SliderControl<float> * MakeHeatGeneratedSliderControl(wxPanel * parent);

    enum PanelKind
    {
        Heating = 0,
        Lamp = 1,
        Engine = 2,
        WaterPump = 3,
        TimerSwitch = 4,
        ThermalSwitch = 5,

        _Last = ThermalSwitch
    };

    static std::optional<PanelKind> GetPanelKind(ElectricalMaterial const * baseMaterial);

private:

    //
    // UI
    //

    wxBoxSizer * mHSizer;

    SliderControl<float> * mHeatGeneratedSlider;

    std::array<wxPanel *, PanelKind::_Last + 1> mPanels;

    // Heating
    SliderControl<float> * mHeatingHeatGeneratedSlider;

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

    // ThermalSwitch
    SliderControl<float> * mThermalSwitchTransitionTemperatureSlider;

    //
    // State
    //

    float mBaseWaterPumpNominalForce;
};

}