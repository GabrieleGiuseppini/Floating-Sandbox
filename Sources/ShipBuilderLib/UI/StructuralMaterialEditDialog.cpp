/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StructuralMaterialEditDialog.h"

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

#include <wx/colordlg.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

namespace ShipBuilder {

int constexpr ColorPickerSideSize = 200;

void StructuralMaterialEditDialog::LayoutControls(wxBoxSizer * dialogVSizer)
{
    wxNotebook * notebook = new wxNotebook(
        mMainPanel,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxNB_TOP);

    //
    // Basic
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        wxBoxSizer * panelVSizer = new wxBoxSizer(wxVERTICAL);

        panelVSizer->AddSpacer(MarginSize);

        // Render color & opacity
        {
            wxBoxSizer * renderColorHSizer = new wxBoxSizer(wxHORIZONTAL);

            renderColorHSizer->AddSpacer(MarginSize);

            // Render color
            {
                mRenderColorButton = new wxPanel(panel, wxID_ANY, wxDefaultPosition,
                    wxSize(ColorPickerSideSize, ColorPickerSideSize), wxBORDER_SIMPLE);

                mRenderColorButton->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent &)
                    {
                        assert(mOverridesUnderEdit.has_value());

                        wxColourData data;
                        data.SetColour(GetOverridesRenderColor(*mOverridesUnderEdit));
                        data.SetChooseFull(true);

                        wxColourDialog dlg(this, &data);
                        if (dlg.ShowModal() == wxID_OK)
                        {
                            auto const color = dlg.GetColourData().GetColour();

                            mOverridesUnderEdit->RenderColor = rgbaColor(color.Red(), color.Green(), color.Blue(), mOverridesUnderEdit->RenderColor.a);
                            mRenderColorButton->SetForegroundColour(color);
                            mRenderColorButton->SetBackgroundColour(color);
                            mRenderColorButton->Refresh();
                        }
                    });

                renderColorHSizer->Add(mRenderColorButton, 0, wxALIGN_CENTER_VERTICAL);
            }

            // Opacity
            {
                mOpacitySlider = new SliderControl<float>(
                    panel,
                    SliderControl<float>::DirectionType::Vertical,
                    SliderWidth,
                    -1,
                    _("Opacity"),
                    wxEmptyString,
                    [this](float value)
                    {
                        assert(mOverridesUnderEdit.has_value());
                        mOverridesUnderEdit->RenderColor.a = static_cast<typename rgbaColor::data_type>(static_cast<float>(rgbaColor::data_type_max) * value);
                    },
                    std::make_unique<LinearSliderCore>(
                        0.2f, // Barely visible
                        1.0f));

                renderColorHSizer->Add(mOpacitySlider, 1, wxEXPAND);
            }

            renderColorHSizer->AddSpacer(MarginSize);

            panelVSizer->Add(
                renderColorHSizer,
                0,
                wxEXPAND);
        }

        panelVSizer->AddSpacer(MarginSize);

        // Name
        {
            MakeNameTextCtrl(panel);

            panelVSizer->Add(
                mNameTextCtrl,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                MarginSize);
        }

        panelVSizer->AddSpacer(MarginSize);

        // Mass, Strength sliders
        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            hSizer->AddStretchSpacer();

            mMassSlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Mass (Kg)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->Density = value / mBaseNominalMass;
                },
                nullptr);

            hSizer->Add(mMassSlider, 0, wxEXPAND);

            hSizer->AddStretchSpacer();

            mStrengthSlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Strength"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->Strength = value;
                },
                nullptr);

            hSizer->Add(mStrengthSlider, 0, wxEXPAND);

            hSizer->AddStretchSpacer();

            panelVSizer->Add(
                hSizer,
                1,
                wxEXPAND);
        }

        panelVSizer->AddSpacer(MarginSize);

        panel->SetSizerAndFit(panelVSizer);

        notebook->AddPage(panel, _("Basic"));
    }

    //
    // Advanced
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

        // Ignition temperature
        {
            mIgnitionTemperatureSlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Ignition Temperature (K)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->IgnitionTemperature = value;
                },
                nullptr);

            gridSizer->Add(
                mIgnitionTemperatureSlider,
                wxGBPosition(0, 0),
                wxGBSpan(1, 3),
                wxEXPAND | wxALL,
                MarginAroundCells);
        }

        // Melting temperature
        {
            mMeltingTemperatureSlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Melting Temperature (K)"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->MeltingTemperature = value;
                },
                nullptr);

            gridSizer->Add(
                mMeltingTemperatureSlider,
                wxGBPosition(0, 3),
                wxGBSpan(1, 3),
                wxEXPAND | wxALL,
                MarginAroundCells);
        }

        // Rot receptivity
        {
            mRotReceptivitySlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Rot Sensitivity"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->RotReceptivity = value;
                },
                std::make_unique<LinearSliderCore>(
                    0.0f,
                    1.0f));

            gridSizer->Add(
                mRotReceptivitySlider,
                wxGBPosition(1, 0),
                wxGBSpan(1, 2),
                wxEXPAND | wxALL,
                MarginAroundCells);
        }

        // Rust receptivity
        {
            mRustReceptivitySlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Rust Sensitivity"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->RustReceptivity = value;
                },
                std::make_unique<LinearSliderCore>(
                    0.0f,
                    1.0f));

            gridSizer->Add(
                mRustReceptivitySlider,
                wxGBPosition(1, 2),
                wxGBSpan(1, 2),
                wxEXPAND | wxALL,
                MarginAroundCells);
        }

        // Water solubility
        {
            mWaterSolubilitySlider = new SliderControl<float>(
                panel,
                SliderControl<float>::DirectionType::Vertical,
                SliderWidth,
                -1,
                _("Water Solubility"),
                wxEmptyString,
                [this](float value)
                {
                    assert(mOverridesUnderEdit.has_value());
                    mOverridesUnderEdit->WaterSolubility = value;
                },
                std::make_unique<LinearSliderCore>(
                    0.0f,
                    1.0f));

            gridSizer->Add(
                mWaterSolubilitySlider,
                wxGBPosition(1, 4),
                wxGBSpan(1, 2),
                wxEXPAND | wxALL,
                MarginAroundCells);
        }

        WxHelpers::MakeAllExpandable(gridSizer);

        panel->SetSizerAndFit(gridSizer);

        notebook->AddPage(panel, _("Advanced"));
    }

    dialogVSizer->Add(
        notebook,
        1,
        wxEXPAND);

    dialogVSizer->Fit(notebook); // Workaround for multi-line bug
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::Run(
    StructuralMaterial::VariantOverridesType const & startingOverrides,
    StructuralMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    mOverridesUnderEdit = startingOverrides;
    mBaseNominalMass = baseMaterial->NominalMass;
    auto const baseOverrideValues = baseMaterial->MakeStartingVariantOverrides();

    wxColor const renderColor = wxColor(mOverridesUnderEdit->RenderColor.r, mOverridesUnderEdit->RenderColor.g, mOverridesUnderEdit->RenderColor.b);
    mRenderColorButton->SetForegroundColour(renderColor);
    mRenderColorButton->SetBackgroundColour(renderColor);
    mRenderColorButton->Refresh();

    mOpacitySlider->SetValue(static_cast<float>(mOverridesUnderEdit->RenderColor.a) / static_cast<float>(rgbaColor::data_type_max));

    mNameTextCtrl->ChangeValue(mOverridesUnderEdit->Name);

    float const baseMass = mBaseNominalMass * baseOverrideValues.Density;
    mMassSlider->SetValueAndLimits(
        mBaseNominalMass * mOverridesUnderEdit->Density,
        std::make_unique<ExponentialSliderCore>(
            baseMass * 0.1f,
            baseMass,
            baseMass * 10.0f));

    mStrengthSlider->SetValueAndLimits(
        mOverridesUnderEdit->Strength,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.Strength * 0.1f,
            baseOverrideValues.Strength,
            baseOverrideValues.Strength * 10.0f));

    mIgnitionTemperatureSlider->SetValueAndLimits(
        mOverridesUnderEdit->IgnitionTemperature,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.IgnitionTemperature * 0.1f,
            baseOverrideValues.IgnitionTemperature,
            baseOverrideValues.IgnitionTemperature * 10.0f));

    mMeltingTemperatureSlider->SetValueAndLimits(
        mOverridesUnderEdit->MeltingTemperature,
        std::make_unique<ExponentialSliderCore>(
            baseOverrideValues.MeltingTemperature * 0.1f,
            baseOverrideValues.MeltingTemperature,
            baseOverrideValues.MeltingTemperature * 10.0f));

    mRotReceptivitySlider->SetValue(mOverridesUnderEdit->RotReceptivity);
    mRustReceptivitySlider->SetValue(mOverridesUnderEdit->RustReceptivity);
    mWaterSolubilitySlider->SetValue(mOverridesUnderEdit->WaterSolubility);

    //
    // Run
    //

    return BaseRun();
}

wxColor StructuralMaterialEditDialog::GetOverridesRenderColor(StructuralMaterial::VariantOverridesType const & overrides)
{
    return wxColor(overrides.RenderColor.r, overrides.RenderColor.g, overrides.RenderColor.b);
}

}