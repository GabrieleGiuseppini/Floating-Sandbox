/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipPropertiesEditDialog.h"

#include "NewPasswordDialog.h"

#include <UILib/WxHelpers.h>

#include <Game/ShipDefinitionFormatDeSerializer.h>

#include <GameCore/Version.h>

#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <cassert>

namespace ShipBuilder {

int const PanelInternalMargin = 10;
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
        wxSize(600, 200),
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

    {
        auto panel = new wxPanel(notebook);

        PopulatePasswordProtectionPanel(panel);

        notebook->AddPage(panel, _("Password Protection"));
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
                wxSize(150, -1),
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

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulateDescriptionPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulatePhysicsDataPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulateAutoTexturizationPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // TODO
    {
        auto temp = new wxStaticBitmap(panel, wxID_ANY, WxHelpers::LoadBitmap("under_construction_large", mResourceLocator));
        vSizer->Add(temp, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulatePasswordProtectionPanel(wxPanel * panel)
{
    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    wxGridBagSizer * gSizer = new wxGridBagSizer(10, 5);

    {
        mPasswordOnButton = new BitmapToggleButton(
            panel,
            mResourceLocator.GetBitmapFilePath("protected_medium"),
            [this]()
            {
                OnSetPassword();
            },
            _("Set a password to protect edits to this ship."));

        gSizer->Add(
            mPasswordOnButton,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, _("With a password you can prevent unauthorized people from making changes to this ship."), wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        label->SetFont(explanationFont);

        gSizer->Add(
            label,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        mPasswordOffButton = new BitmapToggleButton(
            panel,
            mResourceLocator.GetBitmapFilePath("unprotected_medium"),
            [this]()
            {
                OnClearPassword();
            },
            _("Remove the password lock."));

        gSizer->Add(
            mPasswordOffButton,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, _("Without a password anyone is allowed to make changes to this ship."), wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        label->SetFont(explanationFont);

        gSizer->Add(
            label,
            wxGBPosition(1, 1),
            wxGBSpan(1, 1),
            0,
            0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(gSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::OnSetPassword()
{
    // Ask password
    NewPasswordDialog dialog(this, mResourceLocator);
    auto const result = dialog.ShowModal();
    if (result == wxID_OK)
    {
        // Changed

        mPasswordHash = ShipDefinitionFormatDeSerializer::CalculatePasswordHash(dialog.GetPassword());
        mIsPasswordHashModified = true;
        OnDirty();

        ReconciliateUIWithPassword();
    }
    else
    {
        // Unchanged
    }
}

void ShipPropertiesEditDialog::OnClearPassword()
{
    auto const result = wxMessageBox(_("Are you sure you want to remove password protection for this ship, allowing everyone to edit it?"), ApplicationName, 
        wxICON_EXCLAMATION | wxYES_NO | wxCENTRE);

    if (result == wxYES)
    {
        // Changed

        mPasswordHash.reset();
        mIsPasswordHashModified = true;
        OnDirty();

        ReconciliateUIWithPassword();
    }
}

void ShipPropertiesEditDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    //
    // Inspect dirty flags and communicate parts to Controller
    //

    LogMessage("TODOTEST: ShipPropertiesEditDialog::OnOkButton: IsMetadataDirty=", IsMetadataDirty());
    if (IsMetadataDirty())
    {
        // TODO
    }

    if (IsPhysicsDataDirty())
    {
        // TODO
    }

    if (IsAutoTexturizationSettingsDirty())
    {
        // TODO
    }

    //
    // Close dialog
    //

    mSessionData.reset();

    EndModal(0);
}

void ShipPropertiesEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(-1);
}

void ShipPropertiesEditDialog::OnDirty()
{
    // We assume at least one of the controls is dirty

    if (!mOkButton->IsEnabled())
    {
        mOkButton->Enable(true);
    }
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
    // Password protection
    //

    mPasswordHash = mSessionData->Metadata.Password;
    mIsPasswordHashModified = false;
    ReconciliateUIWithPassword();

    //
    // Buttons
    //

    mOkButton->Enable(false);
}

void ShipPropertiesEditDialog::ReconciliateUIWithPassword()
{
    bool const hasPassword = mPasswordHash.has_value();

    mPasswordOnButton->Enable(!hasPassword);
    mPasswordOnButton->SetValue(hasPassword);

    mPasswordOffButton->Enable(hasPassword);
    mPasswordOffButton->SetValue(!hasPassword);
}

bool ShipPropertiesEditDialog::IsMetadataDirty() const
{
    // TODO: others
    return mShipNameTextCtrl->IsModified()
        || mShipAuthorTextCtrl->IsModified()
        || mYearBuiltTextCtrl->IsModified()
        || mIsPasswordHashModified;
}

bool ShipPropertiesEditDialog::IsPhysicsDataDirty() const
{
    // TODO
    return false;
}

bool ShipPropertiesEditDialog::IsAutoTexturizationSettingsDirty() const
{
    // TODO
    return false;
}



}