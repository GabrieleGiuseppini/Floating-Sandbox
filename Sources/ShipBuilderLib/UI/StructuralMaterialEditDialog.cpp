/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StructuralMaterialEditDialog.h"

#include <wx/colordlg.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

namespace ShipBuilder {

int constexpr DialogHeight = 600;
int constexpr DialogWidth = 300;

int constexpr MarginSize = 4;
int constexpr ColorPickerSideSize = 200;

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

        //panelVSizer->AddSpacer(MarginSize);

        // Render color
        {
            mRenderColorButton = new wxButton(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ColorPickerSideSize, ColorPickerSideSize));

            mRenderColorButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
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

            auto font = panel->GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mNameTextCtrl->SetFont(font);

            mNameTextCtrl->SetMaxLength(64);

            panelVSizer->Add(
                mNameTextCtrl,
                0,
                wxALIGN_CENTER_HORIZONTAL);
        }


        panel->SetSizerAndFit(panelVSizer);

        notebook->AddPage(panel, _("Basic"));
    }

    //
    // Advanced
    //

    {
        wxPanel * panel = new wxPanel(notebook);

        //PopulateMechanicsAndThermodynamicsPanel(panel, gameAssetManager);

        notebook->AddPage(panel, _("Advanced"));
    }

    dialogVSizer->Add(
        notebook,
        0,
        wxEXPAND);

    dialogVSizer->Fit(notebook); // Workaround for multi-line bug

    dialogVSizer->AddSpacer(20);

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

    dialogVSizer->AddSpacer(20);

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

    wxColor const renderColor = wxColor(mOverridesUnderEdit->RenderColor.r, mOverridesUnderEdit->RenderColor.g, mOverridesUnderEdit->RenderColor.b);
    mRenderColorButton->SetForegroundColour(renderColor);
    mRenderColorButton->SetBackgroundColour(renderColor);
    mNameTextCtrl->ChangeValue(mOverridesUnderEdit->Name);
    // TODO: others

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