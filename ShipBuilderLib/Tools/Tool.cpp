/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tool.h"

#include "../Controller.h"

#include <optional>

namespace ShipBuilder {

void Tool::SetCursor(wxImage const & cursorImage)
{
    mController.GetUserInterface().SetToolCursor(cursorImage);
}

DisplayLogicalCoordinates Tool::GetCurrentMouseCoordinates() const
{
    return mController.GetUserInterface().GetMouseCoordinates();
}

std::optional<DisplayLogicalCoordinates> Tool::GetCurrentMouseCoordinatesIfInWorkCanvas() const
{
    return mController.GetUserInterface().GetMouseCoordinatesIfInWorkCanvas();
}

ShipSpaceCoordinates Tool::GetCurrentMouseShipCoordinates() const
{
    return mController.GetView().ScreenToShipSpace(GetCurrentMouseCoordinates());
}

ShipSpaceCoordinates Tool::GetCurrentMouseShipCoordinatesClampedToShip() const
{
    return GetCurrentMouseShipCoordinatesClampedToShip(GetCurrentMouseCoordinates());
}

ShipSpaceCoordinates Tool::GetCurrentMouseShipCoordinatesClampedToShip(DisplayLogicalCoordinates const & mouseCoordinates) const
{
    auto const coords = mController.GetView().ScreenToShipSpace(mouseCoordinates);
    return ShipSpaceCoordinates(
        Clamp(coords.x, 0, mController.GetModelController().GetShipSize().width - 1),
        Clamp(coords.y, 0, mController.GetModelController().GetShipSize().height - 1));
}

std::optional<ShipSpaceCoordinates> Tool::GetCurrentMouseShipCoordinatesIfInShip() const
{
    return GetCurrentMouseShipCoordinatesIfInShip(GetCurrentMouseCoordinates());
}

std::optional<ShipSpaceCoordinates> Tool::GetCurrentMouseShipCoordinatesIfInShip(DisplayLogicalCoordinates const & mouseCoordinates) const
{
    auto const coords = mController.GetView().ScreenToShipSpace(mouseCoordinates);
    if (coords.IsInSize(mController.GetModelController().GetShipSize()))
    {
        return coords;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<ShipSpaceCoordinates> Tool::GetCurrentMouseShipCoordinatesIfInWorkCanvas() const
{
    auto const displayCoords = mController.GetUserInterface().GetMouseCoordinatesIfInWorkCanvas();
    if (displayCoords)
    {
        return ScreenToShipSpace(*displayCoords);
    }
    else
    {
        return std::nullopt;
    }
}

ShipSpaceCoordinates Tool::ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
{
    return mController.GetView().ScreenToShipSpace(displayCoordinates);
}

ShipSpaceCoordinates Tool::ScreenToShipSpaceNearest(DisplayLogicalCoordinates const & displayCoordinates) const
{
    return mController.GetView().ScreenToShipSpaceNearest(displayCoordinates);
}

ImageCoordinates Tool::ScreenToTextureSpace(LayerType layerType, DisplayLogicalCoordinates const & displayCoordinates) const
{
    if (layerType == LayerType::ExteriorTexture)
    {
        return mController.GetView().ScreenToExteriorTextureSpace(displayCoordinates);
    }
    else
    {
        assert(layerType == LayerType::InteriorTexture);
        return mController.GetView().ScreenToInteriorTextureSpace(displayCoordinates);
    }
}

}