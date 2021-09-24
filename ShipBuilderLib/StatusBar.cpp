/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StatusBar.h"

#include <GameCore/Log.h>

#include <sstream>

namespace ShipBuilder {

StatusBar::StatusBar(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : wxStatusBar(parent, wxID_ANY, wxSTB_ELLIPSIZE_END)
{
    Connect(wxEVT_SIZE, (wxObjectEventFunction)&StatusBar::OnResize, 0, this);

    // TODOHERE: use SetStatusWidths, and add all fields here
    SetFieldsCount(1);

    //
    // Create controls
    //

    // TODOHERE: create bitmaps all at 0,0, without sizer
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void StatusBar::OnResize(wxSizeEvent & event)
{
    LogMessage("OnStatusBarResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    // TODOHERE: re-position all our custom controls

    event.Skip();
}

}