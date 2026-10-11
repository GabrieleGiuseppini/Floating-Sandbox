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
        wxBoxSizer * hPanelSizer = new wxBoxSizer(wxHORIZONTAL);

        hPanelSizer->AddStretchSpacer();

        // Heat generated
        {
            mOthersHeatGeneratedSlider = new SliderControl<float>(
                othersPanel,
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

            hPanelSizer->Add(mOthersHeatGeneratedSlider, 0, wxEXPAND);
        }

        hPanelSizer->AddStretchSpacer(1);

        // Finalize panel

        othersPanel->SetSizer(hPanelSizer);

        mHSizer->Add(
            othersPanel,
            1,
            wxEXPAND);

        mSpecializedPanels[SpecializedPanelKind::Others] = othersPanel;
    }

    // Lamp panel
    {
        wxPanel * lampPanel = new wxPanel(mMainPanel);
        wxBoxSizer * hPanelSizer = new wxBoxSizer(wxHORIZONTAL);

        hPanelSizer->AddStretchSpacer();

        // TODO: in helper func
        // Heat generated
        {
            mLampHeatGeneratedSlider = new SliderControl<float>(
                lampPanel,
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

            hPanelSizer->Add(mLampHeatGeneratedSlider, 0, wxEXPAND);
        }

        hPanelSizer->AddStretchSpacer(1);

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

            hPanelSizer->Add(mLuminiscenceSlider, 0, wxEXPAND);
        }

        hPanelSizer->AddStretchSpacer(1);

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

            hPanelSizer->Add(mLightSpreadSlider, 0, wxEXPAND);
        }

        hPanelSizer->AddStretchSpacer(1);

        // Finalize panel

        lampPanel->SetSizer(hPanelSizer);

        mHSizer->Add(
            lampPanel,
            1,
            wxEXPAND);

        // TODOTEST
        //mHSizer->Hide(lampPanel);

        mSpecializedPanels[SpecializedPanelKind::Lamp] = lampPanel;
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
            // TODO
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
            // TODO
            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            // TODO
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

}