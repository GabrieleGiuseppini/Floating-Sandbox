/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StructuralMaterialEditDialog.h"

#include <Simulation/SimulationParameters.h>

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

#include <wx/colordlg.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

namespace ShipBuilder {

int constexpr DialogHeight = 600;
int constexpr DialogWidth = 300;

int constexpr MarginSize = 4;
int constexpr ColorPickerSideSize = 200;
int constexpr SliderWidth = 72; // Min

int constexpr VMarginAroundButtons = 10;
int constexpr MarginAroundCells = 8;

StructuralMaterialEditDialog::StructuralMaterialEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxSize(-1, DialogHeight),
        wxCAPTION | wxCLOSE_BOX | wxBORDER_STATIC | wxSTAY_ON_TOP);

    //
    // Layout
    //

    mMainPanel = new wxPanel(this);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

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

        // Render color
        {
            mRenderColorButton = new wxPanel(panel,wxID_ANY, wxDefaultPosition,
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

            panelVSizer->Add(
                mRenderColorButton,
                0,
                wxALIGN_CENTER_HORIZONTAL);
        }

        panelVSizer->AddSpacer(MarginSize);

        // Name
        {
            mNameTextCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                wxSize(DialogWidth - 2 * MarginSize, -1), wxTE_CENTRE);

            // No events: we'll take the name when user is done

            auto font = panel->GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mNameTextCtrl->SetFont(font);

            mNameTextCtrl->SetMaxLength(64);

            panelVSizer->Add(
                mNameTextCtrl,
                0,
                wxALIGN_CENTER_HORIZONTAL);
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

    dialogVSizer->AddSpacer(VMarginAroundButtons);

    //
    // Buttons
    //

    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddStretchSpacer(1);

        mOkButton = new wxButton(mMainPanel, wxID_OK, _("OK"));
        buttonsSizer->Add(mOkButton, 0, 0, 0);

        buttonsSizer->AddStretchSpacer(1);

        mCancelButton = new wxButton(mMainPanel, wxID_CANCEL, _("Cancel"));
        buttonsSizer->Add(mCancelButton, 0, 0, 0);

        buttonsSizer->AddStretchSpacer(1);

        dialogVSizer->Add(buttonsSizer, 0, wxEXPAND, 0);
    }

    dialogVSizer->AddSpacer(VMarginAroundButtons);

    //
    // Finalize dialog
    //

    mMainPanel->SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::RunForNew(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
{
    SetTitle(_T("Create New Material"));
    return Run(overrides, baseMaterial);
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::RunForEdit(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
{
    SetTitle(_T("Edit Material"));
    return Run(overrides, baseMaterial);
}

std::optional<StructuralMaterial::VariantOverridesType> StructuralMaterialEditDialog::Run(
    StructuralMaterial::VariantOverridesType & overrides,
    StructuralMaterial const * baseMaterial)
{
    //
    // Sync controls
    //

    mOverridesUnderEdit = overrides;
    mBaseNominalMass = baseMaterial->NominalMass;
    auto baseOverrideValues = baseMaterial->MakeStartingVariantOverrides();

    wxColor const renderColor = wxColor(mOverridesUnderEdit->RenderColor.r, mOverridesUnderEdit->RenderColor.g, mOverridesUnderEdit->RenderColor.b);
    mRenderColorButton->SetForegroundColour(renderColor);
    mRenderColorButton->SetBackgroundColour(renderColor);
    mRenderColorButton->Refresh();

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

    auto const result = ShowModal();
    if (result == wxID_OK)
    {
        // Take name now, and normalize it
        mOverridesUnderEdit->Name = mNameTextCtrl->GetValue().ToStdString();

        // TODO: normalize

        return mOverridesUnderEdit;
    }
    else
    {
        return std::nullopt;
    }
}

wxColor StructuralMaterialEditDialog::GetOverridesRenderColor(StructuralMaterial::VariantOverridesType const & overrides)
{
    return wxColor(overrides.RenderColor.r, overrides.RenderColor.g, overrides.RenderColor.b);
}

}