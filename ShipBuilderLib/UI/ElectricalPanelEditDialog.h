/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "UI/ElectricalPanelLayoutControl.h"

#include "Controller.h"
#include "InstancedElectricalElementSet.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <wx/dialog.h>
#include <wx/scrolwin.h>

#include <map>
#include <optional>

namespace ShipBuilder {

class ElectricalPanelEditDialog : public wxDialog
{
public:

    ElectricalPanelEditDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void ShowModal(
        Controller & controller,
        InstancedElectricalElementSet const & instancedElectricalElementSet,
        ElectricalPanelMetadata const & electricalPanelMetadata);

private:

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);

    void SetListPanelSelected(ElectricalElementInstanceIndex selectedElement);

    void ReconciliateUI();

private:

    wxScrolled<wxPanel> * mListPanel;
    std::map<ElectricalElementInstanceIndex, wxPanel *> mListPanelPanelsByInstanceIndex;
    ElectricalPanelLayoutControl * mElectricalPanel;

    struct SessionData
    {
        Controller & BuilderController;
        InstancedElectricalElementSet const & ElementSet;
        ElectricalPanelMetadata const & PanelMetadata;

        std::optional<ElectricalElementInstanceIndex> CurrentlySelectedElement;

        SessionData(
            Controller & controller,
            InstancedElectricalElementSet const & elementSet,
            ElectricalPanelMetadata const & panelMetadata)
            : BuilderController(controller)
            , ElementSet(elementSet)
            , PanelMetadata(panelMetadata)
            , CurrentlySelectedElement()
        {}
    };

    std::optional<SessionData> mSessionData;
};

}