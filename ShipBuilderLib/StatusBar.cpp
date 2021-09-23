/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StatusBar.h"

#include <sstream>

namespace ShipBuilder {

StatusBar::StatusBar(wxWindow * parent)
    : wxStatusBar(parent, wxID_ANY, 0)
{
    // TODOHERE
    SetFieldsCount(1);
}

void StatusBar::SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates)
{
    std::stringstream ss;

    if (coordinates.has_value())
    {
        ss << coordinates->x << ", " << coordinates->y;
    }

    SetStatusText(ss.str(), 0);
}

}