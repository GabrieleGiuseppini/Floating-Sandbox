/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ResizeDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>

#include <cassert>

namespace ShipBuilder {

ResizeDialog::ResizeDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Resize Ship"),
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
            button->Bind(wxEVT_BUTTON, &ResizeDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ResizeDialog::OnCancelButton, this);
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

int ResizeDialog::ShowModalForResize()
{
    mSessionData.emplace(ModeType::ForResize);

    InitializeUI();

    return wxDialog::ShowModal();
}

int ResizeDialog::ShowModalForTexture()
{
    mSessionData.emplace(ModeType::ForTexture);

    InitializeUI();

    return wxDialog::ShowModal();
}

void ResizeDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(wxID_OK);
}

void ResizeDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(wxID_CANCEL);
}

void ResizeDialog::InitializeUI()
{
    assert(mSessionData);

    // TODOHERE
}

}