/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"

#include <Game/ShipMetadata.h>
#include <Game/ShipPhysicsData.h>
#include <Game/ShipAutoTexturizationSettings.h>

#include <wx/dialog.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class ShipPropertiesEditDialog : public wxDialog
{
public:

    ShipPropertiesEditDialog(wxWindow * parent);

    void Open(
        Controller & controller,
        ShipMetadata const & shipMetadata,
        ShipPhysicsData const & shipPhysicsData,
        std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings);

private:

};

}