/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-01
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "NewShipNameDialog.h"

#include "ShipNameSuggestionDialog.h"

#include <GameCore/Utils.h>

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ShipBuilder {

NewShipNameDialog::NewShipNameDialog(
    wxWindow * parent,
    ShipNameNormalizer const & shipNameNormalizer,
    ResourceLocator const & resourceLocator)
    : mParent(parent)
    , mShipNameNormalizer(shipNameNormalizer)
    , mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("New Ship Name"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    //
    // Lay out controls
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // Ship name
    {
        {
            auto label = new wxStaticText(this, wxID_ANY, _("What's the name of your new ship?"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            dialogVSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        dialogVSizer->AddSpacer(20);

        {
            mShipNameTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(350, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER);

            mShipNameTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            mShipNameTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                    mShipNameTextCtrl->Navigate();
                });

            auto font = GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mShipNameTextCtrl->SetFont(font);

            dialogVSizer->Add(mShipNameTextCtrl, 0, wxLEFT | wxRIGHT | wxEXPAND, 14);
        }
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        {
            mOkButton = new wxButton(this, wxID_OK, _("OK"));
            buttonsSizer->Add(mOkButton, 0);
        }

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

std::string NewShipNameDialog::AskName()
{
    std::string const defaultShipName = "New Ship " + Utils::MakeNowDateAndTimeString();
    mShipNameTextCtrl->SetValue(defaultShipName);

    wxDialog::ShowModal();

    auto const shipName = MakeString(mShipNameTextCtrl->GetValue());
    if (shipName.has_value())
    {
        // Normalize and check with user
        auto const normalizedShipName = mShipNameNormalizer.NormalizeName(*shipName);
        if (normalizedShipName != *shipName)
        {
            ShipNameSuggestionDialog dlg(mParent, mResourceLocator);
            if (!dlg.AskUserIfAcceptsSuggestedName(normalizedShipName))
            {
                return *shipName;
            }
        }

        return normalizedShipName;
    }
    else
    {
        return defaultShipName;
    }
}

void NewShipNameDialog::OnDirty()
{
    auto const doEnable = MakeString(mShipNameTextCtrl->GetValue()).has_value();
    if (mOkButton->IsEnabled() != doEnable)
    {
        mOkButton->Enable(doEnable);
    }
}

std::optional<std::string> NewShipNameDialog::MakeString(wxString && value)
{
    std::string trimmedValue = value.Trim(true).Trim(false).ToStdString();
    if (trimmedValue.empty())
        return std::nullopt;
    else
        return trimmedValue;
}


}