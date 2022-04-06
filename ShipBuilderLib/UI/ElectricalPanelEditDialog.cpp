/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "UI/ElectricalPanelEditDialog.h"

#include <GameCore/Log.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wupdlock.h>

namespace ShipBuilder {

ElectricalPanelEditDialog::ElectricalPanelEditDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
{
    int constexpr Margin = 20;

    Create(
        parent,
        wxID_ANY,
        _("Electrical Panel Edit"),
        wxDefaultPosition,
        wxSize(800, 600),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(Margin);

    // List panel
    {
        mListPanel = new wxScrolled<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxVSCROLL);
        mListPanel->SetScrollRate(0, 5);

        dialogVSizer->Add(
            mListPanel,
            2,
            wxEXPAND | wxLEFT | wxRIGHT,
            Margin);
    }

    dialogVSizer->AddSpacer(Margin);

    // Element layout control
    {
        mElectricalPanel = new ElectricalPanelLayoutControl(this, resourceLocator);

        dialogVSizer->Add(
            mElectricalPanel,
            1,
            wxEXPAND | wxLEFT | wxRIGHT,
            Margin);
    }

    dialogVSizer->AddSpacer(Margin);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(Margin);

        {
            auto button = new wxButton(this, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &ElectricalPanelEditDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(Margin);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ElectricalPanelEditDialog::OnCancelButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(Margin);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    }

    dialogVSizer->AddSpacer(Margin);

    //
    // Finalize dialog
    //

    SetSizer(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void ElectricalPanelEditDialog::ShowModal(
    Controller & controller,
    InstancedElectricalElementSet const & instancedElectricalElementSet,
    ElectricalPanelMetadata const & electricalPanelMetadata)
{
    mSessionData.emplace(
        controller,
        instancedElectricalElementSet,
        electricalPanelMetadata);

    ReconciliateUI();

    wxDialog::ShowModal();
}

void ElectricalPanelEditDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    //
    // Set in controller
    //

    // TODOHERE

    //
    // Close dialog
    //

    mSessionData.reset();
    EndModal(0);
}

void ElectricalPanelEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    EndModal(-1);
}

void ElectricalPanelEditDialog::ReconciliateUI()
{
    assert(mSessionData);

    wxWindowUpdateLocker scopedUpdateFreezer(this);

    auto instanceIndexFont = this->GetFont();
    instanceIndexFont.SetPointSize(instanceIndexFont.GetPointSize() + 2);

    //
    // Populate list
    //

    int constexpr ElementHeight = 40;

    mListPanel->DestroyChildren();

    wxBoxSizer * listVSizer = new wxBoxSizer(wxVERTICAL);

    for (auto const & instancedElement : mSessionData->ElementSet.GetElements())
    {
        wxPanel * elementPanel = new wxPanel(mListPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, ElementHeight), wxSIMPLE_BORDER);

        elementPanel->Bind(
            wxEVT_SET_FOCUS,
            [instancedElementIndex = instancedElement.first](wxFocusEvent &)
            {
                // TODOHERE
                LogMessage("TODOTEST: FOCUS@", instancedElementIndex);
            });

        wxBoxSizer * listElementHSizer = new wxBoxSizer(wxHORIZONTAL);

        listElementHSizer->AddSpacer(10);

        // Instance ID
        {
            auto label = new wxStaticText(elementPanel, wxID_ANY, std::to_string(instancedElement.first), wxDefaultPosition, wxSize(20, -1), wxALIGN_RIGHT);

            label->SetFont(instanceIndexFont);

            listElementHSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(10);

        // Material name
        {
            auto label = new wxStaticText(elementPanel, wxID_ANY, instancedElement.second->Name, wxDefaultPosition, wxSize(240, -1), wxALIGN_LEFT);

            label->SetFont(instanceIndexFont);

            listElementHSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddStretchSpacer(1);

        // Visibility checkbox
        {
            auto checkbox = new wxCheckBox(elementPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);

            {
                bool isVisible;
                if (auto const srchIt = mSessionData->PanelMetadata.find(instancedElement.first);
                    srchIt != mSessionData->PanelMetadata.end())
                {
                    isVisible = !srchIt->second.IsHidden;
                }
                else
                {
                    isVisible = true;
                }

                checkbox->SetValue(isVisible);
            }

            checkbox->Bind(
                wxEVT_CHECKBOX,
                [](wxCommandEvent & event)
                {
                    // TODOHERE: communicate to layout control
                    event.Skip();
                });

            listElementHSizer->Add(checkbox, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(20);

        // Label
        {
            std::optional<std::string> labelText;
            if (auto const srchIt = mSessionData->PanelMetadata.find(instancedElement.first);
                srchIt != mSessionData->PanelMetadata.end())
            {
                labelText = srchIt->second.Label;
            }

            if (!labelText.has_value())
            {
                labelText = instancedElement.second->MakeInstancedElementLabel(instancedElement.first);
            }

            auto textCtrl = new wxTextCtrl(elementPanel, wxID_ANY, *labelText,
                wxDefaultPosition, wxSize(240, -1), wxTE_CENTRE);

            textCtrl->SetFont(instanceIndexFont);

            textCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    // TODOHERE: communicate to layout control
                    event.Skip();
                });

            listElementHSizer->Add(textCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(10);

        elementPanel->SetSizer(listElementHSizer);

        listVSizer->Add(
            elementPanel,
            0,
            wxEXPAND,
            0);
    }

    mListPanel->SetSizer(listVSizer);

    //
    // Populate layout control
    //

    mElectricalPanel->Clear();

    // TODOHERE

    //
    // Finalize
    //

    LogMessage("TODOTEST: PRE:  ListPanel height:", mListPanel->GetSize().GetHeight());

    Layout();

    LogMessage("TODOTEST: POST: ListPanel height:", mListPanel->GetSize().GetHeight());
}

}