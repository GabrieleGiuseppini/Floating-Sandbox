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

int constexpr ListPanelElementHeight = 40;

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
        wxSize(880, 700),
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
            3,
            wxEXPAND | wxLEFT | wxRIGHT,
            Margin);
    }

    dialogVSizer->AddSpacer(Margin);

    // Element layout control
    {
        mElectricalPanel = new ElectricalPanelLayoutControl(
            this,
            [this](ElectricalElementInstanceIndex selectedInstanceIndex)
            {
                SetListPanelSelected(selectedInstanceIndex);

                // Scroll list panel to ensure element is visible at middle
                int xUnit, yUnit;
                mListPanel->GetScrollPixelsPerUnit(&xUnit, &yUnit);
                if (yUnit != 0)
                {
                    // Calculate ordinal of this element in list
                    int elementOrdinal = 0;
                    for (auto const & instancedElement : mSessionData->ElementSet.GetElements())
                    {
                        if (instancedElement.first == selectedInstanceIndex)
                        {
                            break;
                        }

                        ++elementOrdinal;
                    };

                    // Calculate virtual Y of (center of) this element
                    int const elementCenterVirtualY = elementOrdinal * ListPanelElementHeight + (ListPanelElementHeight / 2);

                    // Scroll so that element's center is in center of view

                    int topVirtual = std::max(
                        elementCenterVirtualY - mListPanel->GetSize().GetHeight() / 2,
                        0);

                    mListPanel->Scroll(
                        -1,
                        topVirtual / yUnit);
                }
            },
            resourceLocator);

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
    //
    // Create own electrical panel, fully populated
    //

    ElectricalPanelMetadata electricalPanel = electricalPanelMetadata;

    for (auto const & elementEntry : instancedElectricalElementSet.GetElements())
    {
        auto [it, isInserted] = electricalPanel.try_emplace(
            elementEntry.first,
            ElectricalPanelElementMetadata(
                std::nullopt,
                elementEntry.second->MakeInstancedElementLabel(elementEntry.first),
                false)); // Not hidden by default

        // Make sure there's a label
        if (!isInserted && !it->second.Label.has_value())
        {
            it->second.Label = elementEntry.second->MakeInstancedElementLabel(elementEntry.first);
        }
    }

    //
    // Create session
    //

    mSessionData.emplace(
        controller,
        instancedElectricalElementSet,
        std::move(electricalPanel));

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

void ElectricalPanelEditDialog::SetListPanelSelected(ElectricalElementInstanceIndex selectedElement)
{
    // De-select previous

    if (mSessionData->CurrentlySelectedElement.has_value())
    {
        assert(mListPanelPanelsByInstanceIndex.count(*mSessionData->CurrentlySelectedElement) != 0);

        mListPanelPanelsByInstanceIndex[*mSessionData->CurrentlySelectedElement]->SetBackgroundColour(GetDefaultAttributes().colBg);
        mListPanelPanelsByInstanceIndex[*mSessionData->CurrentlySelectedElement]->Refresh();
    }

    // Select new

    assert(mListPanelPanelsByInstanceIndex.count(selectedElement) != 0);
    auto * const panel = mListPanelPanelsByInstanceIndex[selectedElement];
    panel->SetFocus();
    panel->SetBackgroundColour(wxColour(214, 254, 255));

    panel->Refresh();

    mSessionData->CurrentlySelectedElement = selectedElement;
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

    mListPanel->DestroyChildren();
    mListPanelPanelsByInstanceIndex.clear();

    wxBoxSizer * listVSizer = new wxBoxSizer(wxVERTICAL);

    for (auto const & instancedElement : mSessionData->ElementSet.GetElements())
    {
        assert(mSessionData->PanelMetadata.count(instancedElement.first) == 1);

        wxPanel * elementPanel = new wxPanel(mListPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, ListPanelElementHeight), wxSIMPLE_BORDER);

        elementPanel->Bind(
            wxEVT_LEFT_DOWN,
            [this, instancedElementIndex = instancedElement.first](wxMouseEvent &)
            {
                SetListPanelSelected(instancedElementIndex);

                mElectricalPanel->SelectElement(instancedElementIndex);
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

            checkbox->SetValue(!(mSessionData->PanelMetadata.at(instancedElement.first).IsHidden));

            checkbox->Bind(
                wxEVT_CHECKBOX,
                [this](wxCommandEvent & /*event*/)
                {
                    mElectricalPanel->OnPanelUpdated();
                });

            listElementHSizer->Add(checkbox, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(20);

        // Label
        {
            assert(mSessionData->PanelMetadata.at(instancedElement.first).Label.has_value());

            auto textCtrl = new wxTextCtrl(elementPanel, wxID_ANY, *mSessionData->PanelMetadata.at(instancedElement.first).Label,
                wxDefaultPosition, wxSize(240, -1), wxTE_CENTRE);

            textCtrl->SetFont(instanceIndexFont);

            textCtrl->Bind(
                wxEVT_SET_FOCUS,
                [this, instancedElementIndex = instancedElement.first](wxFocusEvent &)
                {
                    SetListPanelSelected(instancedElementIndex);

                    mElectricalPanel->SelectElement(instancedElementIndex);
                });

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

        mListPanelPanelsByInstanceIndex[instancedElement.first] = elementPanel;
    }

    mListPanel->SetSizer(listVSizer);

    //
    // Populate layout control
    //

    mElectricalPanel->SetPanel(mSessionData->ElementSet, mSessionData->PanelMetadata);

    //
    // Finalize
    //

    Layout();
}

}