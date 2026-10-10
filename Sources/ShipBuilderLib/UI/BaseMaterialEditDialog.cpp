/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "BaseMaterialEditDialog.h"

#include <Simulation/Materials.h>

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

#include <wx/colordlg.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

namespace ShipBuilder {

template<typename TMaterial>
BaseMaterialEditDialog<TMaterial>::BaseMaterialEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        wxString(),
        wxDefaultPosition,
        wxSize(-1, DialogHeight),
        wxCAPTION | wxCLOSE_BOX | wxBORDER_STATIC | wxSTAY_ON_TOP);
}

template<typename TMaterial>
void BaseMaterialEditDialog<TMaterial>::LayoutBase()
{
    mMainPanel = new wxPanel(this);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    //
    // Controls
    //

    LayoutControls(dialogVSizer);

    //
    // Buttons
    //

    dialogVSizer->AddSpacer(VMarginAroundButtons);

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

template<typename TMaterial>
std::optional<typename TMaterial::VariantOverridesType> BaseMaterialEditDialog<TMaterial>::RunForNew(
    typename TMaterial::VariantOverridesType & overrides,
    TMaterial const * baseMaterial)
{
    SetTitle(_T("Create New Material"));
    return Run(overrides, baseMaterial);
}

template<typename TMaterial>
std::optional<typename TMaterial::VariantOverridesType> BaseMaterialEditDialog<TMaterial>::RunForEdit(
    typename TMaterial::VariantOverridesType & overrides,
    TMaterial const * baseMaterial)
{
    SetTitle(_T("Edit Material"));
    return Run(overrides, baseMaterial);
}

//
// Explicit specializations for all materials
//

template class BaseMaterialEditDialog<StructuralMaterial>;
template class BaseMaterialEditDialog<ElectricalMaterial>;

}