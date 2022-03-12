/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tool.h"

#include "Controller.h"

namespace ShipBuilder {

void Tool::SetCursor(wxImage const & cursorImage)
{
    mController.GetUserInterface().SetToolCursor(cursorImage);
}

ShipSpaceCoordinates Tool::ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
{
    return mController.GetView().ScreenToShipSpace(displayCoordinates);
}

std::optional<DisplayLogicalCoordinates> Tool::GetMouseCoordinatesIfInWorkCanvas() const
{
    return mController.GetUserInterface().GetMouseCoordinatesIfInWorkCanvas();
}

ShipSpaceCoordinates Tool::GetCurrentMouseCoordinatesInShipSpace() const
{
    return mController.GetView().ScreenToShipSpace(mController.GetUserInterface().GetMouseCoordinates());
}

}