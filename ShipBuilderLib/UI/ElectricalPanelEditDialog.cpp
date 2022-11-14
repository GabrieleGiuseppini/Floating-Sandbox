/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "UI/ElectricalPanelEditDialog.h"

#include <UILib/WxHelpers.h>

#include <GameCore/Log.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/wupdlock.h>

namespace ShipBuilder {

int constexpr ListPanelElementHeight = 40;

ElectricalPanelEditDialog::ElectricalPanelEditDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mVisibleBitmap(WxHelpers::LoadBitmap("visible_icon_medium", resourceLocator))
    , mInvisibleBitmap(WxHelpers::LoadBitmap("invisible_icon_medium", resourceLocator))
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
        mListPanel->SetBackgroundColour(*wxWHITE);
        mListPanel->SetScrollRate(0, 5);

        dialogVSizer->Add(
            mListPanel,
            1, // Occupy all available V space
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
            0, // Maintain own height
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

    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &ElectricalPanelEditDialog::OnCloseWindow, this);

    //
    // Finalize dialog
    //

    SetSizer(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void ElectricalPanelEditDialog::ShowModal(
    Controller & controller,
    InstancedElectricalElementSet const & instancedElectricalElementSet,
    ElectricalPanel const & originalElectricalPanel)
{
    //
    // Create own electrical panel, fully populated
    //

    ElectricalPanel electricalPanel = originalElectricalPanel;

    for (auto const & elementEntry : instancedElectricalElementSet.GetElements())
    {
        auto [it, isInserted] = electricalPanel.TryAdd(
            elementEntry.first,
            ElectricalPanel::ElementMetadata(
                std::nullopt,
                elementEntry.second->MakeInstancedElementLabel(elementEntry.first),
                false)); // Not hidden by default

        // If we had it already, make sure there's a label
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
    assert(mSessionData.has_value());

    if (mSessionData->IsListPanelDirty || mElectricalPanel->IsDirty())
    {
        //
        // Set in controller
        //

        mSessionData->BuilderController.SetElectricalPanel(mSessionData->Panel);
    }

    //
    // Close dialog
    //

    mElectricalPanel->ResetPanel();
    mSessionData.reset();

    EndModal(0);
}

void ElectricalPanelEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mElectricalPanel->ResetPanel();
    mSessionData.reset();

    EndModal(-1);
}

void ElectricalPanelEditDialog::OnCloseWindow(wxCloseEvent & event)
{
    mElectricalPanel->ResetPanel();
    mSessionData.reset();

    event.Skip();
}

void ElectricalPanelEditDialog::SetListPanelSelected(ElectricalElementInstanceIndex selectedElement)
{
    // De-select previous

    if (mSessionData->CurrentlySelectedElement.has_value())
    {
        assert(mListPanelPanelsByInstanceIndex.count(*mSessionData->CurrentlySelectedElement) != 0);

        mListPanelPanelsByInstanceIndex[*mSessionData->CurrentlySelectedElement]->SetBackgroundColour(mListPanel->GetBackgroundColour());
        mListPanelPanelsByInstanceIndex[*mSessionData->CurrentlySelectedElement]->Refresh();
    }

    // Select new

    assert(mListPanelPanelsByInstanceIndex.count(selectedElement) != 0);
    auto * const panel = mListPanelPanelsByInstanceIndex[selectedElement];
    panel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRADIENTINACTIVECAPTION));

    panel->Refresh();

    mSessionData->CurrentlySelectedElement = selectedElement;
}

void ElectricalPanelEditDialog::ReconciliateUI()
{
    assert(mSessionData);

    wxWindowUpdateLocker scopedUpdateFreezer(this);

    auto instanceIndexFont = this->GetFont();
#if __WXGTK__
    instanceIndexFont.SetPointSize(instanceIndexFont.GetPointSize() + 1);
#else
    instanceIndexFont.SetPointSize(instanceIndexFont.GetPointSize() + 2);
#endif

    //
    // Populate list panel
    //

    mListPanel->DestroyChildren();
    mListPanelPanelsByInstanceIndex.clear();

    wxBoxSizer * listVSizer = new wxBoxSizer(wxVERTICAL);

    for (auto const & instancedElement : mSessionData->ElementSet.GetElements())
    {
        ElectricalElementInstanceIndex const instancedElementIndex = instancedElement.first;

        assert(mSessionData->Panel.Contains(instancedElementIndex));

        wxPanel * elementPanel = new wxPanel(mListPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, ListPanelElementHeight), wxSIMPLE_BORDER);

        elementPanel->Bind(
            wxEVT_LEFT_DOWN,
            [this, instancedElementIndex](wxMouseEvent &)
            {
                SetListPanelSelected(instancedElementIndex);
                mElectricalPanel->SelectElement(instancedElementIndex);
            });

        wxBoxSizer * listElementHSizer = new wxBoxSizer(wxHORIZONTAL);

        listElementHSizer->AddSpacer(30);

        // Instance ID
        {
            auto label = new wxStaticText(elementPanel, wxID_ANY, std::to_string(instancedElementIndex), wxDefaultPosition, wxSize(20, -1), wxALIGN_RIGHT);

            label->SetFont(instanceIndexFont);

            label->Bind(
                wxEVT_LEFT_DOWN,
                [this, instancedElementIndex](wxMouseEvent & event)
                {
                    SetListPanelSelected(instancedElementIndex);
                    mElectricalPanel->SelectElement(instancedElementIndex);

                    event.Skip();
                });

            listElementHSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddStretchSpacer(1);

        // Label
        {
            assert(mSessionData->Panel[instancedElementIndex].Label.has_value());

            auto textCtrl = new wxTextCtrl(elementPanel, wxID_ANY, *mSessionData->Panel[instancedElementIndex].Label,
                wxDefaultPosition, wxSize(240, -1), wxTE_CENTRE);

            textCtrl->SetMaxLength(32);
            textCtrl->SetFont(instanceIndexFont);

            textCtrl->Bind(
                wxEVT_SET_FOCUS,
                [this, instancedElementIndex](wxFocusEvent & event)
                {
                    SetListPanelSelected(instancedElementIndex);
                    mElectricalPanel->SelectElement(instancedElementIndex);

                    event.Skip();
                });

            textCtrl->Bind(
                wxEVT_TEXT,
                [this, instancedElementIndex](wxCommandEvent & event)
                {
                    assert(mSessionData.has_value());
                    assert(mSessionData->Panel.Contains(instancedElementIndex));
                    mSessionData->Panel[instancedElementIndex].Label = event.GetString();

                    // Remember we're dirty
                    mSessionData->IsListPanelDirty = true;

                    event.Skip();
                });

            listElementHSizer->Add(textCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(20);

        // Material name
        {
            auto label = new wxStaticText(elementPanel, wxID_ANY, "(" + instancedElement.second->Name + ")", wxDefaultPosition, wxSize(240, -1), wxALIGN_LEFT);

            label->SetFont(instanceIndexFont);

            label->Bind(
                wxEVT_LEFT_DOWN,
                [this, instancedElementIndex](wxMouseEvent & event)
                {
                    SetListPanelSelected(instancedElementIndex);
                    mElectricalPanel->SelectElement(instancedElementIndex);

                    event.Skip();
                });

            listElementHSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddStretchSpacer(1);

        // Visibility toggle
        {
            auto tglButton = new wxBitmapToggleButton(elementPanel, wxID_ANY, mInvisibleBitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            tglButton->SetBitmapPressed(mVisibleBitmap);

            tglButton->SetValue(!(mSessionData->Panel[instancedElementIndex].IsHidden));

            tglButton->Bind(
                wxEVT_TOGGLEBUTTON,
                [this, instancedElementIndex](wxCommandEvent & event)
                {
                    bool const isVisible = event.IsChecked();

                    if (isVisible)
                    {
                        SetListPanelSelected(instancedElementIndex);
                        mElectricalPanel->SelectElement(instancedElementIndex);
                    }

                    // Update visibility
                    assert(mSessionData.has_value());
                    assert(mSessionData->Panel.Contains(instancedElementIndex));
                    ElectricalPanel::ElementMetadata & panelElement = mSessionData->Panel[instancedElementIndex];
                    panelElement.IsHidden = !isVisible;

                    // Notify control of visibility change
                    mElectricalPanel->OnPanelUpdated();

                    // Remember we're dirty
                    mSessionData->IsListPanelDirty = true;
                });

            listElementHSizer->Add(tglButton, 0, wxALIGN_CENTER_VERTICAL, 0);
        }

        listElementHSizer->AddSpacer(30);

        elementPanel->SetSizer(listElementHSizer);

        listVSizer->Add(
            elementPanel,
            0,
            wxEXPAND,
            0);

        mListPanelPanelsByInstanceIndex[instancedElementIndex] = elementPanel;
    }

    mListPanel->SetSizer(listVSizer);

    //
    // Populate layout control
    //

    mElectricalPanel->SetPanel(mSessionData->Panel);

    //
    // Finalize
    //

    mElectricalPanel->SetFocus(); // Move focus away from list panel

    Layout();
}

}