/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalMaterialEditDialog.h"

#include <Simulation/SimulationParameters.h>

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

#include <cassert>

namespace ShipBuilder {

void ElectricalMaterialEditDialog::LayoutControls(wxBoxSizer * dialogVSizer)
{
    dialogVSizer->AddSpacer(MarginSize);

    // Name
    {
        MakeNameTextCtrl(mMainPanel);

        dialogVSizer->Add(
            mNameTextCtrl,
            0,
            wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(MarginSize);

    mHSizer = new wxBoxSizer(wxHORIZONTAL);

    // Heating panel
    {
        wxPanel * heatingPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(1);

        // Heat generated
        {
            mHeatingHeatGeneratedSlider = MakeHeatGeneratedSliderControl(heatingPanel);

            gPanelSizer->Add(mHeatingHeatGeneratedSlider, 0, wxEXPAND);
        }

        // Finalize panel

        heatingPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            heatingPanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::Heating] = heatingPanel;
    }

    // Lamp panel
    {
        wxPanel * lampPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(3);

        // Heat generated
        {
            mLampHeatGeneratedSlider = MakeHeatGeneratedSliderControl(lampPanel);

            gPanelSizer->Add(mLampHeatGeneratedSlider, 0, wxEXPAND);
        }

        // Luminiscence
        {
            mLuminiscenceSlider = new SliderControl<float>(
                lampPanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Luminiscence"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->Luminiscence = value;
                },
                nullptr);

            gPanelSizer->Add(mLuminiscenceSlider, 0, wxEXPAND);
        }

        // Light spread
        {
            mLightSpreadSlider = new SliderControl<float>(
                lampPanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Light Spread"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->LightSpread = value;
                },
                nullptr);

            gPanelSizer->Add(mLightSpreadSlider, 0, wxEXPAND);
        }

        // Finalize panel

        lampPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            lampPanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::Lamp] = lampPanel;
    }

    // Engine panel
    {
        wxPanel * enginePanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(2);

        // Heat generated
        {
            mEngineHeatGeneratedSlider = MakeHeatGeneratedSliderControl(enginePanel);

            gPanelSizer->Add(mEngineHeatGeneratedSlider, 0, wxEXPAND);
        }

        // Engine power
        {
            mEnginePowerSlider = new SliderControl<float>(
                enginePanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Engine Power (HP)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->EnginePower = value;
                },
                nullptr);

            gPanelSizer->Add(mEnginePowerSlider, 0, wxEXPAND);
        }

        // Finalize panel

        enginePanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            enginePanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::Engine] = enginePanel;
    }

    // Water pump
    {
        wxPanel * waterPumpPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(2);

        // Heat generated
        {
            mWaterPumpHeatGeneratedSlider = MakeHeatGeneratedSliderControl(waterPumpPanel);

            gPanelSizer->Add(mWaterPumpHeatGeneratedSlider, 0, wxEXPAND);
        }

        // Nominal force
        {
            mWaterPumpNominalForceSlider = new SliderControl<float>(
                waterPumpPanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Pump Power"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    if (mBaseWaterPumpNominalForce < 0.0f)
                        value = -value;
                    mOverridesUnderEdit->WaterPumpNominalForce = value;
                },
                nullptr);

            gPanelSizer->Add(mWaterPumpNominalForceSlider, 0, wxEXPAND);
        }

        // Finalize panel

        waterPumpPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            waterPumpPanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::WaterPump] = waterPumpPanel;
    }

    // Timer switch
    {
        wxPanel * timerSwitchPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(1);

        // Duration
        {
            mTimerDurationSecondsSlider = new SliderControl<float>(
                timerSwitchPanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Delay (s)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->TimerDurationSeconds = value;
                },
                std::make_unique<LinearSliderCore>(
                    0.1f,
                    20.0f));

            gPanelSizer->Add(mTimerDurationSecondsSlider, 0, wxEXPAND);
        }

        // Finalize panel

        timerSwitchPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            timerSwitchPanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::TimerSwitch] = timerSwitchPanel;
    }

    // Thermal switch
    {
        wxPanel * thermalSwitchPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(1);

        // Transition temperature
        {
            mThermalSwitchTransitionTemperatureSlider = new SliderControl<float>(
                thermalSwitchPanel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Temperature Threshold (K)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->ThermalSwitchTransitionTemperature = value;
                },
                std::make_unique<ExponentialSliderCore>(
                    0.1f,
                    SimulationParameters::Temperature0,
                    100000.0f));

            gPanelSizer->Add(mThermalSwitchTransitionTemperatureSlider, 0, wxEXPAND);
        }

        // Finalize panel

        thermalSwitchPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            thermalSwitchPanel,
            1,
            wxEXPAND);

        mPanels[PanelKind::ThermalSwitch] = thermalSwitchPanel;
    }

    dialogVSizer->Add(
        mHSizer,
        1,
        wxEXPAND);
}

std::optional<ElectricalMaterial::VariantOverridesType> ElectricalMaterialEditDialog::Run(
    ElectricalMaterial::VariantOverridesType & overrides,
    ElectricalMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    mOverridesUnderEdit = overrides;
    mBaseWaterPumpNominalForce = baseMaterial->WaterPumpNominalForce;
    auto baseOverrideValues = baseMaterial->MakeStartingVariantOverrides();

    mNameTextCtrl->ChangeValue(mOverridesUnderEdit->Name);

    auto heatGeneratedSliderCore = ExponentialSliderCore(
        baseOverrideValues.HeatGenerated * 0.1f,
        baseOverrideValues.HeatGenerated,
        baseOverrideValues.HeatGenerated * 10.0f);

    mHeatingHeatGeneratedSlider->SetValueAndLimits(
        mOverridesUnderEdit->HeatGenerated,
        std::make_unique<ExponentialSliderCore>(heatGeneratedSliderCore));

    mLampHeatGeneratedSlider->SetValueAndLimits(
        mOverridesUnderEdit->HeatGenerated,
        std::make_unique<ExponentialSliderCore>(heatGeneratedSliderCore));

    mLuminiscenceSlider->SetValueAndLimits(
        mOverridesUnderEdit->Luminiscence,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.Luminiscence * 0.1f,
            baseOverrideValues.Luminiscence,
            baseOverrideValues.Luminiscence * 10.0f));

    mLightSpreadSlider->SetValueAndLimits(
        mOverridesUnderEdit->LightSpread,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.LightSpread * 0.1f,
            baseOverrideValues.LightSpread,
            baseOverrideValues.LightSpread * 10.0f));

    mEngineHeatGeneratedSlider->SetValueAndLimits(
        mOverridesUnderEdit->HeatGenerated,
        std::make_unique<ExponentialSliderCore>(heatGeneratedSliderCore));

    mEnginePowerSlider->SetValueAndLimits(
        mOverridesUnderEdit->EnginePower,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.EnginePower * 0.1f,
            baseOverrideValues.EnginePower,
            baseOverrideValues.EnginePower * 10.0f));

    mWaterPumpHeatGeneratedSlider->SetValueAndLimits(
        mOverridesUnderEdit->HeatGenerated,
        std::make_unique<ExponentialSliderCore>(heatGeneratedSliderCore));

    mWaterPumpNominalForceSlider->SetValueAndLimits(
        std::fabsf(mOverridesUnderEdit->WaterPumpNominalForce),
        std::make_unique<ExponentialSliderCore>(
            std::fabsf(baseOverrideValues.WaterPumpNominalForce) * 0.1f,
            std::fabsf(baseOverrideValues.WaterPumpNominalForce),
            std::fabsf(baseOverrideValues.WaterPumpNominalForce) * 10.0f));

    mTimerDurationSecondsSlider->SetValue(mOverridesUnderEdit->TimerDurationSeconds);

    mThermalSwitchTransitionTemperatureSlider->SetValue(mOverridesUnderEdit->ThermalSwitchTransitionTemperature);

    //
    // Choose panels to show
    //

    for (int iPanel = 0; iPanel <= PanelKind::_Last; ++iPanel)
    {
        mHSizer->Hide(mPanels[iPanel]);
    }

    auto const panelKind = GetPanelKind(baseMaterial);
    assert(panelKind.has_value()); // We shouldn't be called for a material with no properties to verride
    mHSizer->Show(mPanels[*panelKind]);

    mHSizer->Layout();

    //
    // Run
    //

    return BaseRun();
}

SliderControl<float> * ElectricalMaterialEditDialog::MakeHeatGeneratedSliderControl(wxPanel * parent)
{
    return new SliderControl<float>(
        parent,
        SliderControl<float>::DirectionType::Vertical,
        SliderWidth,
        -1,
        _("Heat Generated (W)"),
        wxEmptyString,
        [this](float value)
        {
            assert(mOverridesUnderEdit.has_value());
            mOverridesUnderEdit->HeatGenerated = value;
        },
        nullptr);
}

std::optional<ElectricalMaterialEditDialog::PanelKind> ElectricalMaterialEditDialog::GetPanelKind(ElectricalMaterial const * baseMaterial)
{
    switch (baseMaterial->ElectricalType)
    {
        case ElectricalMaterial::ElectricalElementType::Cable:
        {
            return std::nullopt;
        }

        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            return PanelKind::Engine;
        }

        case ElectricalMaterial::ElectricalElementType::EngineController:
        case ElectricalMaterial::ElectricalElementType::EngineTransmission:
        {
            return std::nullopt;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            return PanelKind::Heating;
        }

        case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
        {
            return std::nullopt;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            return PanelKind::Lamp;
        }

        case ElectricalMaterial::ElectricalElementType::OtherSink:
        {
            return PanelKind::Heating;
        }

        case ElectricalMaterial::ElectricalElementType::PowerMonitor:
        case ElectricalMaterial::ElectricalElementType::ShipSound:
        case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
        {
            return std::nullopt;
        }

        case ElectricalMaterial::ElectricalElementType::ThermalSwitch:
        {
            return PanelKind::ThermalSwitch;
        }

        case ElectricalMaterial::ElectricalElementType::TimerSwitch:
        {
            return PanelKind::TimerSwitch;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            return PanelKind::WaterPump;
        }

        case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
        case ElectricalMaterial::ElectricalElementType::WatertightDoor:
        {
            return std::nullopt;
        }
    }

    assert(false);
    return std::nullopt;
}

}