/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "UI/ElectricalPanelEditDialog.h"

namespace ShipBuilder {

ElectricalPanelEditDialog::ElectricalPanelEditDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        _("Electrical Panel Edit"),
        wxDefaultPosition,
        wxSize(600, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);
}

}