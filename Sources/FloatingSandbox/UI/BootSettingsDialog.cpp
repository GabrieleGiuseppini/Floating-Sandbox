/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "BootSettingsDialog.h"

#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <filesystem>

static int constexpr InternalWindowMargin = 10;
static int constexpr StaticBoxTopMargin = 20;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr RadioButtonMargin = 4;
static int constexpr InterRadioBoxMargin = 0;

BootSettingsDialog::BootSettingsDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
    : wxDialog(parent, wxID_ANY, _("Boot Settings"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxSTAY_ON_TOP)
    , mGameAssetManager(gameAssetManager)
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
        wxBoxSizer * optionsSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            wxStaticBox * forceNoGlFinishBox = new wxStaticBox(this, wxID_ANY, _("Force no glFinish()"));

            {
                wxBoxSizer * forceNoGlFinishBoxSizer = new wxBoxSizer(wxVERTICAL);

                forceNoGlFinishBoxSizer->AddSpacer(StaticBoxTopMargin);

                mDoForceNoGlFinish_UnsetRadioButton = new wxRadioButton(forceNoGlFinishBox, wxID_ANY, _("Default"),
                    wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

                forceNoGlFinishBoxSizer->Add(
                    mDoForceNoGlFinish_UnsetRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoGlFinishBoxSizer->AddSpacer(InterRadioBoxMargin);

                mDoForceNoGlFinish_TrueRadioButton = new wxRadioButton(forceNoGlFinishBox, wxID_ANY, _("True"),
                    wxDefaultPosition, wxDefaultSize);

                forceNoGlFinishBoxSizer->Add(
                    mDoForceNoGlFinish_TrueRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoGlFinishBoxSizer->AddSpacer(InterRadioBoxMargin);

                mDoForceNoGlFinish_FalseRadioButton = new wxRadioButton(forceNoGlFinishBox, wxID_ANY, _("False"),
                    wxDefaultPosition, wxDefaultSize);

                forceNoGlFinishBoxSizer->Add(
                    mDoForceNoGlFinish_FalseRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoGlFinishBox->SetSizer(forceNoGlFinishBoxSizer);
            }

            optionsSizer->Add(forceNoGlFinishBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, InternalWindowMargin);
        }

        {
            wxStaticBox * forceNoMultithreadedRenderingBox = new wxStaticBox(this, wxID_ANY, _("Force no multithreaded rendering"));

            {
                wxBoxSizer * forceNoMultithreadedRenderingBoxSizer = new wxBoxSizer(wxVERTICAL);

                forceNoMultithreadedRenderingBoxSizer->AddSpacer(StaticBoxTopMargin);

                mDoForceNoMultithreadedRendering_UnsetRadioButton = new wxRadioButton(forceNoMultithreadedRenderingBox, wxID_ANY, _("Default"),
                    wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

                forceNoMultithreadedRenderingBoxSizer->Add(
                    mDoForceNoMultithreadedRendering_UnsetRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoMultithreadedRenderingBoxSizer->AddSpacer(InterRadioBoxMargin);

                mDoForceNoMultithreadedRendering_TrueRadioButton = new wxRadioButton(forceNoMultithreadedRenderingBox, wxID_ANY, _("True"),
                    wxDefaultPosition, wxDefaultSize);

                forceNoMultithreadedRenderingBoxSizer->Add(
                    mDoForceNoMultithreadedRendering_TrueRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoMultithreadedRenderingBoxSizer->AddSpacer(InterRadioBoxMargin);

                mDoForceNoMultithreadedRendering_FalseRadioButton = new wxRadioButton(forceNoMultithreadedRenderingBox, wxID_ANY, _("False"),
                    wxDefaultPosition, wxDefaultSize);

                forceNoMultithreadedRenderingBoxSizer->Add(
                    mDoForceNoMultithreadedRendering_FalseRadioButton,
                    0,
                    wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM,
                    RadioButtonMargin);

                forceNoMultithreadedRenderingBox->SetSizer(forceNoMultithreadedRenderingBoxSizer);
            }

            optionsSizer->Add(forceNoMultithreadedRenderingBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, InternalWindowMargin);
        }

        vSizer->Add(optionsSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, InternalWindowMargin);
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

    PopulateCheckboxes(BootSettings::Load(gameAssetManager.GetBootSettingsFilePath()));

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
    if (!settings.DoForceNoGlFinish.has_value())
        mDoForceNoGlFinish_UnsetRadioButton->SetValue(true);
    else
    {
        if (*(settings.DoForceNoGlFinish))
            mDoForceNoGlFinish_TrueRadioButton->SetValue(true);
        else
            mDoForceNoGlFinish_FalseRadioButton->SetValue(true);
    }

    if (!settings.DoForceNoMultithreadedRendering.has_value())
        mDoForceNoMultithreadedRendering_UnsetRadioButton->SetValue(true);
    else
    {
        if (*(settings.DoForceNoMultithreadedRendering))
            mDoForceNoMultithreadedRendering_TrueRadioButton->SetValue(true);
        else
            mDoForceNoMultithreadedRendering_FalseRadioButton->SetValue(true);
    }
}

void BootSettingsDialog::OnRevertToDefaultsButton(wxCommandEvent & /*event*/)
{
    BootSettings settings;

    PopulateCheckboxes(settings);
}

void BootSettingsDialog::OnSaveAndQuitButton(wxCommandEvent & /*event*/)
{
    std::optional<bool> doForceNoGlFinish;
    if (mDoForceNoGlFinish_TrueRadioButton->GetValue())
        doForceNoGlFinish = true;
    else if (mDoForceNoGlFinish_FalseRadioButton->GetValue())
        doForceNoGlFinish = false;

    std::optional<bool> doForceNoMultithrededRendering;
    if (mDoForceNoMultithreadedRendering_TrueRadioButton->GetValue())
        doForceNoMultithrededRendering = true;
    else if (mDoForceNoMultithreadedRendering_FalseRadioButton->GetValue())
        doForceNoMultithrededRendering = false;

    BootSettings settings(
        doForceNoGlFinish,
        doForceNoMultithrededRendering);

    BootSettings defaultSettings;

    if (!(settings == defaultSettings))
    {
        BootSettings::Save(
            settings,
            mGameAssetManager.GetBootSettingsFilePath());
    }
    else
    {
        try
        {
            std::filesystem::remove(mGameAssetManager.GetBootSettingsFilePath());
        }
        catch (...)
        {
            // Ignore
        }
    }

    EndModal(wxID_OK);
}