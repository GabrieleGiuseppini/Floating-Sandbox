/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "BaseResizeDialog.h"

#include <wx/button.h>

namespace ShipBuilder {

void BaseResizeDialog::CreateLayout(
    wxWindow * parent,
    wxString const & caption,
    GameAssetManager const & gameAssetManager)
{
    Create(
        parent,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP);

    SetTitle(caption);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    //
    // Layout
    //

    InternalCreateLayout(dialogVSizer, gameAssetManager);

    //
    // Finalize
    //

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &BaseResizeDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &BaseResizeDialog::OnCancelButton, this);
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

bool BaseResizeDialog::ShowModal(
    RgbaImageData const & image,
    ShipSpaceSize const & shipSize)
{
    InternalReconciliateUI(image, shipSize);

    return wxDialog::ShowModal() == wxID_OK;
}

void BaseResizeDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    InternalOnClose();
    EndModal(wxID_OK);
}

void BaseResizeDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    InternalOnClose();
    EndModal(wxID_CANCEL);
}

}