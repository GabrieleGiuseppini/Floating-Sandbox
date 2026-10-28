/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-28
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "CustomMaterialsImportPickDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>

namespace ShipBuilder {

CustomMaterialsImportPickDialog::CustomMaterialsImportPickDialog(
    wxWindow * parent,
    std::vector<std::unique_ptr<StructuralMaterial const>> const & structuralCustomMaterials,
    std::vector<bool> const & duplicateStructuralCustomMaterials,
    std::vector<std::unique_ptr<ElectricalMaterial const>> const & electricalCustomMaterials,
    std::vector<bool> const & duplicateElectricalCustomMaterials,
    std::vector<std::unique_ptr<StructuralMaterial const>> const & ropesCustomMaterials,
    std::vector<bool> const & duplicateRopesCustomMaterials)
{
    Create(
        parent,
        wxID_ANY,
        _("Load custom materials"),
        wxDefaultPosition,
        wxDefaultSize,
        wxCAPTION | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    //
    // Lay out controls
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // TODOHERE
    (void)structuralCustomMaterials;
    (void)duplicateStructuralCustomMaterials;
    (void)electricalCustomMaterials;
    (void)duplicateElectricalCustomMaterials;
    (void)ropesCustomMaterials;
    (void)duplicateRopesCustomMaterials;

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddStretchSpacer(1);

        auto okButton = new wxButton(this, wxID_OK, _("OK"));
        buttonsSizer->Add(okButton, 0, 0, 0);

        buttonsSizer->AddStretchSpacer(1);

        auto cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
        buttonsSizer->Add(cancelButton, 0, 0, 0);

        buttonsSizer->AddStretchSpacer(1);

        dialogVSizer->Add(buttonsSizer, 0, wxEXPAND, 0);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

}