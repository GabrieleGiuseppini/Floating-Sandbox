/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-10-28
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "CustomMaterialsImportPickDialog.h"

#include <Core/GameTypes.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

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

    // Panels
    {
        wxBoxSizer * panelsHSizer = new wxBoxSizer(wxHORIZONTAL);

        panelsHSizer->AddSpacer(10);

        {
            wxBoxSizer * panelVSizer = new wxBoxSizer(wxVERTICAL);

            auto label = new wxStaticText(this, wxID_ANY, _("Structural Materials"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

            panelVSizer->Add(
                label,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            auto sPanel = MakeSelectionPanel(structuralCustomMaterials, duplicateStructuralCustomMaterials);

            panelVSizer->Add(
                sPanel,
                1,
                wxEXPAND,
                0);

            panelsHSizer->Add(panelVSizer, 1, wxEXPAND);
        }

        panelsHSizer->AddSpacer(10);

        {
            wxBoxSizer * panelVSizer = new wxBoxSizer(wxVERTICAL);

            auto label = new wxStaticText(this, wxID_ANY, _("Electrical Materials"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

            panelVSizer->Add(
                label,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            auto ePanel = MakeSelectionPanel(electricalCustomMaterials, duplicateElectricalCustomMaterials);

            panelVSizer->Add(
                ePanel,
                1,
                wxEXPAND,
                0);

            panelsHSizer->Add(panelVSizer, 1, wxEXPAND);
        }

        panelsHSizer->AddSpacer(10);

        {
            wxBoxSizer * panelVSizer = new wxBoxSizer(wxVERTICAL);

            auto label = new wxStaticText(this, wxID_ANY, _("Ropes Materials"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);

            panelVSizer->Add(
                label,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            auto rPanel = MakeSelectionPanel(ropesCustomMaterials, duplicateRopesCustomMaterials);

            panelVSizer->Add(
                rPanel,
                1,
                wxEXPAND,
                0);

            panelsHSizer->Add(panelVSizer, 1, wxEXPAND);
        }

        panelsHSizer->AddSpacer(10);

        dialogVSizer->Add(panelsHSizer, 1, wxEXPAND);
    }

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

template<typename TMaterial>
wxScrolled<wxPanel> * CustomMaterialsImportPickDialog::MakeSelectionPanel(
    std::vector<std::unique_ptr<TMaterial const>> const & customMaterials,
    std::vector<bool> const & isDuplicate)
{
    assert(customMaterials.size() == isDuplicate.size());

    wxScrolled<wxPanel> * panel = new wxScrolled<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxVSCROLL);
    panel->SetMinSize(wxSize(300, 400));
    panel->SetBackgroundColour(*wxWHITE);

    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    int constexpr HMargin = 5;
    int constexpr InterRowMargin = 5;

    vSizer->AddSpacer(InterRowMargin);

    for (size_t icm = 0; icm < customMaterials.size(); ++icm)
    {
        TMaterial const & material = *customMaterials[icm];

        wxBoxSizer * rowSizer = new wxBoxSizer(wxHORIZONTAL);

        if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
        {
            auto renderColorButton = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxSize(20, 20), wxBORDER_SIMPLE);

            wxColor const back = *wxWHITE;
            double const a = static_cast<double>(material.RenderColor.a) / static_cast<double>(rgbaColor::data_type_max);
            wxColor const c(
                wxColor::AlphaBlend(material.RenderColor.r, back.Red(), a),
                wxColor::AlphaBlend(material.RenderColor.g, back.Green(), a),
                wxColor::AlphaBlend(material.RenderColor.b, back.Blue(), a));
            renderColorButton->SetForegroundColour(c);
            renderColorButton->SetBackgroundColour(c);

            rowSizer->Add(
                renderColorButton,
                0,
                wxALIGN_CENTER_VERTICAL | wxLEFT,
                HMargin);
        }

        {
            // TODO: "duplicate"
            wxCheckBox * chk = new wxCheckBox(panel, wxID_ANY, material.Name, wxDefaultPosition, wxDefaultSize);

            // TODO: enablement

            // TODO: binding

            rowSizer->Add(
                chk,
                1,
                wxALIGN_CENTER_VERTICAL | wxLEFT,
                HMargin);
        }

        vSizer->Add(
            rowSizer,
            0,
            wxEXPAND,
            10);

        vSizer->AddSpacer(InterRowMargin);
    }

    panel->SetSizer(vSizer);

    return panel;
}

}