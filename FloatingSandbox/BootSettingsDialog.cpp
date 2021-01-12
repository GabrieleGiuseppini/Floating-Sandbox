/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "BootSettingsDialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <filesystem>

BootSettingsDialog::BootSettingsDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : wxDialog(parent, wxID_ANY, _("Boot Settings"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSTAY_ON_TOP)
    , mResourceLocator(resourceLocator)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    {
        auto label = new wxStaticText(this, wxID_ANY, _("WARNING! These settings will only be enforced after the simulator has been restarted!!!"), wxDefaultPosition,
            wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

        vSizer->Add(label, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 14);
    }

    {
#if wxUSE_STATLINE
        vSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 14);
#endif // wxUSE_STATLINE
    }

    {
        mDoForceNoGlFinishCheckBox = new wxCheckBox(this, wxID_ANY, _("Force no glFinish()"));

        vSizer->Add(mDoForceNoGlFinishCheckBox, 0, wxUP | wxLEFT | wxRIGHT | wxALIGN_LEFT, 14);
    }

    {
        vSizer->AddSpacer(8);
    }

    {
        mDoForceNoMultithrededRenderingCheckBox = new wxCheckBox(this, wxID_ANY, _("Force no multithreaded rendering"));

        vSizer->Add(mDoForceNoMultithrededRenderingCheckBox, 0, wxDOWN | wxLEFT | wxRIGHT | wxALIGN_LEFT, 14);
    }

    {
        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            wxButton * revertToDefaultsButton = new wxButton(this, wxID_ANY, _("Revert to Defaults"));
            revertToDefaultsButton->Bind(wxEVT_BUTTON, &BootSettingsDialog::OnRevertToDefaultsButton, this);

            hSizer->Add(revertToDefaultsButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        }

        {
            wxButton * saveAndQuitButton = new wxButton(this, wxID_OK, _("Save and Quit"));
            saveAndQuitButton->Bind(wxEVT_BUTTON, &BootSettingsDialog::OnSaveAndQuitButton, this);

            hSizer->Add(saveAndQuitButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        }

        vSizer->Add(hSizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    }

    PopulateCheckboxes(BootSettings::Load(resourceLocator.GetBootSettingsFilePath()));

    {
        this->SetSizerAndFit(vSizer);

        Centre(wxCENTER_ON_SCREEN | wxBOTH);
    }
}

BootSettingsDialog::~BootSettingsDialog()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BootSettingsDialog::PopulateCheckboxes(BootSettings const & settings)
{
    mDoForceNoGlFinishCheckBox->SetValue(settings.DoForceNoGlFinish);
    mDoForceNoMultithrededRenderingCheckBox->SetValue(settings.DoForceNoMultithreadedRendering);
}

void BootSettingsDialog::OnRevertToDefaultsButton(wxCommandEvent & /*event*/)
{
    BootSettings settings;

    PopulateCheckboxes(settings);
}

void BootSettingsDialog::OnSaveAndQuitButton(wxCommandEvent & /*event*/)
{
    BootSettings settings(
        mDoForceNoGlFinishCheckBox->GetValue(),
        mDoForceNoMultithrededRenderingCheckBox->GetValue());

    BootSettings defaultSettings;

    if (!(settings == defaultSettings))
    {
        BootSettings::Save(
            settings,
            mResourceLocator.GetBootSettingsFilePath());
    }
    else
    {
        try
        {
            std::filesystem::remove(mResourceLocator.GetBootSettingsFilePath());
        }
        catch (...)
        {
            // Ignore
        }
    }

    EndModal(wxID_OK);
}