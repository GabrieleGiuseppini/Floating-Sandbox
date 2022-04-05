/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-04-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <wx/dialog.h>

namespace ShipBuilder {

class ElectricalPanelEditDialog : public wxDialog
{
public:

    ElectricalPanelEditDialog(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

private:
};

}