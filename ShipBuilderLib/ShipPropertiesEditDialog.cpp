/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipPropertiesEditDialog.h"

// TODO
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/settings.h>
#include <wx/statbox.h>

#include <cassert>

namespace ShipBuilder {

static constexpr int Border = 10;
static int constexpr CellBorder = 8;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr StaticBoxTopMargin = 7;

ShipPropertiesEditDialog::ShipPropertiesEditDialog(wxWindow * parent)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Properties"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    // TODO

    //
    // Finalize dialog
    //

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void ShipPropertiesEditDialog::Open(
    Controller & controller,
    ShipMetadata const & shipMetadata,
    ShipPhysicsData const & shipPhysicsData,
    std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings)
{
    // TODO
    this->ShowModal();
}

}