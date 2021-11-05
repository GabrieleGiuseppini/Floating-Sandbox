/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipPropertiesEditDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h> // TODO
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

int const VerticalSeparatorSize = 20;

ShipPropertiesEditDialog::ShipPropertiesEditDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
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

        PopulateDescriptionPanel(panel);

        notebook->AddPage(panel, _("Description"));
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
            mOkButton = new wxButton(this, wxID_ANY, _("OK"));
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
    std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings,
    bool hasTexture)
{
    mSessionData.emplace(
        controller,
        shipMetadata,
        shipPhysicsData,
        shipAutoTexturizationSettings,
        hasTexture);

    InitializeUI();

    wxDialog::ShowModal();
}

void ShipPropertiesEditDialog::PopulateMetadataPanel(wxPanel * panel)
{
    int const EditBoxWidth = 150;

    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    // Ship name
    {
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
                    OnDirty();
                });

            auto font = panel->GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mShipNameTextCtrl->SetFont(font);

            vSizer->Add(mShipNameTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Name of the ship, e.g. \"R.M.S. Titanic\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Author(s)
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Author(s)"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mShipAuthorTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(EditBoxWidth, -1),
                wxTE_CENTRE);

            mShipAuthorTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & /*event*/)
                {
                    OnDirty();
                });

            vSizer->Add(mShipAuthorTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Author(s), e.g. \"Ellen Ripley; David Gahan\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    // TODO: Art Credits

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Year Built
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Year Built"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mYearBuiltTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(100, -1),
                wxTE_CENTRE);

            mYearBuiltTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & /*event*/)
                {
                    OnDirty();
                });

            vSizer->Add(mYearBuiltTextCtrl, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Year in which the ship was built, e.g. \"1911\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    // TODO

    panel->SetSizer(vSizer);
}

void ShipPropertiesEditDialog::PopulateDescriptionPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
    }

    panel->SetSizer(vSizer);
}

void ShipPropertiesEditDialog::PopulatePhysicsDataPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
    }

    panel->SetSizer(vSizer);
}

void ShipPropertiesEditDialog::PopulateAutoTexturizationPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
    }

    panel->SetSizer(vSizer);
}

void ShipPropertiesEditDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // TODO: inspect dirty flags and communicate parts to Controller
    // mShipNameTextCtrl->IsModified()

    mSessionData.reset();
    EndModal(0);
}

void ShipPropertiesEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(-1);
}

void ShipPropertiesEditDialog::InitializeUI()
{
    assert(mSessionData);

    //
    // Metadata
    //

    mShipNameTextCtrl->ChangeValue(mSessionData->Metadata.ShipName);

    mShipAuthorTextCtrl->ChangeValue(mSessionData->Metadata.Author.value_or(""));

    // TODO

    mYearBuiltTextCtrl->ChangeValue(mSessionData->Metadata.YearBuilt.value_or(""));

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
    // We assume at least one of the controls is dirty

    if (!mOkButton->IsEnabled())
    {
        mOkButton->Enable(true);
    }
}

}