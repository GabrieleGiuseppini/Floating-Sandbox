/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipCanvasResizeDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/statbmp.h>

#include <cassert>

namespace ShipBuilder {

ShipCanvasResizeDialog::ShipCanvasResizeDialog(
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
            mOkButton = new wxButton(this, wxID_ANY, _("Ok"));
            mOkButton->Bind(wxEVT_BUTTON, &ShipCanvasResizeDialog::OnOkButton, this);
            buttonsSizer->Add(mOkButton, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ShipCanvasResizeDialog::OnCancelButton, this);
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

void ShipCanvasResizeDialog::ShowModal(
    Controller & controller)
{
    mSessionData.emplace(controller);

    InitializeUI();

    wxDialog::ShowModal();
}

void ShipCanvasResizeDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // TODO: inspect dirty flags and communicate parts to Controller
    // mShipNameTextCtrl->IsModified()

    mSessionData.reset();
    EndModal(0);
}

void ShipCanvasResizeDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(-1);
}

void ShipCanvasResizeDialog::InitializeUI()
{
    assert(mSessionData);

    // TODO

    //
    // Buttons
    //

    mOkButton->Enable(false);
}

void ShipCanvasResizeDialog::OnDirty()
{
    // We assume at least one of the controls is dirty

    if (!mOkButton->IsEnabled())
    {
        mOkButton->Enable(true);
    }
}

}