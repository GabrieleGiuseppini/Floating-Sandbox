/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ModelValidationDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>

#include <cassert>

namespace ShipBuilder {

ModelValidationDialog::ModelValidationDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Issues"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        dialogVSizer->Add(temp, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ModelValidationDialog::OnCancelButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void ModelValidationDialog::ShowModal(
    Controller & controller)
{
    mSessionData.emplace(controller);

    //
    // Cleanup previous open
    //

    // TODO: destroy panel

    // TODO
    auto const validationResults = mSessionData->BuilderController.ValidateModel();


    wxDialog::ShowModal();
}

void ModelValidationDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    EndModal(0);
}

void ModelValidationDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    EndModal(-1);
}

}