/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipPropertiesEditDialog.h"

#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

static constexpr int Border = 10;
static int constexpr CellBorderOuter = 4;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr StaticBoxTopMargin = 7;

ShipPropertiesEditDialog::ShipPropertiesEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Properties"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);

    {
        auto panel = new wxPanel(notebook);

        PopulateMetadataPanel(panel);

        notebook->AddPage(panel, _("Metadata"));
    }

    {
        auto panel = new wxPanel(notebook);

        PopulatePhysicsDataPanel(panel);

        notebook->AddPage(panel, _("Physics"));
    }

    {
        auto panel = new wxPanel(notebook);

        PopulateAutoTexturizationPanel(panel);

        notebook->AddPage(panel, _("Auto-Texturization"));
    }

    dialogVSizer->Add(notebook, 1, wxEXPAND);

    dialogVSizer->AddSpacer(20);


    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            mOkButton = new wxButton(this, wxID_ANY, _("Ok"));
            mOkButton->Bind(wxEVT_BUTTON, &ShipPropertiesEditDialog::OnOkButton, this);
            buttonsSizer->Add(mOkButton, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ShipPropertiesEditDialog::OnCancelButton, this);
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

void ShipPropertiesEditDialog::ShowModal(
    Controller & controller,
    ShipMetadata const & shipMetadata,
    ShipPhysicsData const & shipPhysicsData,
    std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings)
{
    Initialize(
        shipMetadata,
        shipPhysicsData,
        shipAutoTexturizationSettings);

    wxDialog::ShowModal();
}

void ShipPropertiesEditDialog::PopulateMetadataPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Ship name
    {
        wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Ship Name"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mShipNameTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(350, -1),
                wxTE_CENTRE);

            mShipNameTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & /*event*/)
                {
                    // TODO
                    LogMessage("TODOHERE: IsModified=", mShipNameTextCtrl->IsModified());

                    OnDirty();
                });

            auto font = panel->GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mShipNameTextCtrl->SetFont(font);

            vSizer->Add(mShipNameTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        gridSizer->Add(
            vSizer,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1), // TODO: all cols
            wxEXPAND | wxALL,
            CellBorderOuter);
    }

    // TODO

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void ShipPropertiesEditDialog::PopulatePhysicsDataPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // TODO

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void ShipPropertiesEditDialog::PopulateAutoTexturizationPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // TODO

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void ShipPropertiesEditDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // TODO: inspect dirty flags and communicate parts to Controller
    // mShipNameTextCtrl->IsModified()

    EndModal(0);
}

void ShipPropertiesEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    EndModal(-1);
}

void ShipPropertiesEditDialog::Initialize(
    ShipMetadata const & shipMetadata,
    ShipPhysicsData const & shipPhysicsData,
    std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings)
{
    //
    // Metadata
    //

    mShipNameTextCtrl->ChangeValue(shipMetadata.ShipName);

    // TODO

    //
    // Physics
    //

    // TODO

    //
    // Auto-Texturization
    //

    // TODO

    //
    // Buttons
    //

    mOkButton->Enable(false);
}

void ShipPropertiesEditDialog::OnDirty()
{
    if (!mOkButton->IsEnabled())
    {
        mOkButton->Enable(true);
    }
}

}