/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ElectricalMaterialEditDialog.h"

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

namespace ShipBuilder {

void ElectricalMaterialEditDialog::LayoutControls(wxBoxSizer * dialogVSizer)
{
    dialogVSizer->AddSpacer(MarginSize);

    // Name
    {
        mNameTextCtrl = new wxTextCtrl(mMainPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxSize(DialogWidth - 2 * MarginSize, -1), wxTE_CENTRE);

        // No events: we'll take the name when user is done

        auto font = mMainPanel->GetFont();
        font.SetPointSize(font.GetPointSize() + 2);
        mNameTextCtrl->SetFont(font);

        mNameTextCtrl->SetMaxLength(64);

        dialogVSizer->Add(
            mNameTextCtrl,
            0,
            wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(MarginSize);

    mHSizer = new wxBoxSizer(wxHORIZONTAL);

    // Others panel
    {
        wxPanel * othersPanel = new wxPanel(mMainPanel);
        wxGridSizer * gPanelSizer = new wxGridSizer(1);

        // Heat generated
        {
            mOthersHeatGeneratedSlider = MakeHeatGeneratedSliderControl(othersPanel);

            gPanelSizer->Add(mOthersHeatGeneratedSlider, 0, wxEXPAND);
        }

        // Finalize panel

        othersPanel->SetSizer(gPanelSizer);

        mHSizer->Add(
            othersPanel,
            1,
            wxEXPAND);

        mSpecializedPanels[SpecializedPanelKind::Others] = othersPanel;
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

        mSpecializedPanels[SpecializedPanelKind::Lamp] = lampPanel;
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

        mSpecializedPanels[SpecializedPanelKind::Engine] = enginePanel;
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

        mSpecializedPanels[SpecializedPanelKind::WaterPump] = waterPumpPanel;
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

        mSpecializedPanels[SpecializedPanelKind::TimerSwitch] = timerSwitchPanel;
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
    auto baseOverrideValues = baseMaterial->MakeStartingVariantOverrides();

    mNameTextCtrl->ChangeValue(mOverridesUnderEdit->Name);

    auto heatGeneratedSliderCore = std::make_unique<ExponentialSliderCore>(
        baseOverrideValues.HeatGenerated * 0.1f,
        baseOverrideValues.HeatGenerated,
        baseOverrideValues.HeatGenerated * 10.0f);

    //
    // Choose panels to show
    //

    for (int iSpecializedPanel = 0; iSpecializedPanel <= SpecializedPanelKind::_Last; ++iSpecializedPanel)
    {
        mHSizer->Hide(mSpecializedPanels[iSpecializedPanel]);
    }

    switch (baseMaterial->ElectricalType)
    {
        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            mEngineHeatGeneratedSlider->SetValueAndLimits(
                mOverridesUnderEdit->HeatGenerated,
                std::move(heatGeneratedSliderCore));

            mEnginePowerSlider->SetValueAndLimits(
                mOverridesUnderEdit->EnginePower,
                std::make_unique<ExponentialSliderCore>(
                    baseOverrideValues.EnginePower * 0.1f,
                    baseOverrideValues.EnginePower,
                    baseOverrideValues.EnginePower * 10.0f));

            mHSizer->Show(mSpecializedPanels[SpecializedPanelKind::Engine]);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            mLampHeatGeneratedSlider->SetValueAndLimits(
                mOverridesUnderEdit->HeatGenerated,
                std::move(heatGeneratedSliderCore));

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

            mHSizer->Show(mSpecializedPanels[SpecializedPanelKind::Lamp]);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::TimerSwitch:
        {
            mTimerDurationSecondsSlider->SetValue(mOverridesUnderEdit->TimerDurationSeconds);

            mHSizer->Show(mSpecializedPanels[SpecializedPanelKind::TimerSwitch]);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            mWaterPumpHeatGeneratedSlider->SetValueAndLimits(
                mOverridesUnderEdit->HeatGenerated,
                std::move(heatGeneratedSliderCore));

            mWaterPumpNominalForceSlider->SetValueAndLimits(
                mOverridesUnderEdit->WaterPumpNominalForce,
                std::make_unique<ExponentialSliderCore>(
                    baseOverrideValues.WaterPumpNominalForce * 0.1f,
                    baseOverrideValues.WaterPumpNominalForce,
                    baseOverrideValues.WaterPumpNominalForce * 10.0f));

            mHSizer->Show(mSpecializedPanels[SpecializedPanelKind::WaterPump]);

            break;
        }

        default:
        {
            // Others

            mOthersHeatGeneratedSlider->SetValueAndLimits(
                mOverridesUnderEdit->HeatGenerated,
                std::move(heatGeneratedSliderCore));

            mHSizer->Show(mSpecializedPanels[SpecializedPanelKind::Others]);

            break;
        }
    }

    // TODO: which needed, if any?
    mHSizer->Layout();
    mMainPanel->Layout();

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
        _("Heat Generated (KJ/s)"),
        wxEmptyString,
        [this](float value)
        {
            assert(mOverridesUnderEdit.has_value());
            mOverridesUnderEdit->HeatGenerated = value;
        },
        nullptr);
}

}