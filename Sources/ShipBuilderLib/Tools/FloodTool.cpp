/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FloodTool.h"

#include "../Controller.h"

#include <type_traits>

namespace ShipBuilder {

StructuralFloodTool::StructuralFloodTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : FloodTool(
        ToolType::StructuralFlood,
        controller,
        gameAssetManager)
{}

template<LayerType TLayerType>
FloodTool<TLayerType>::FloodTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
{
    SetCursor(WxHelpers::LoadCursorImage("flood_tool_cursor", 12, 29, gameAssetManager));

    mController.BroadcastSampledInformationUpdatedAt(ScreenToShipSpace(GetCurrentMouseCoordinates()), TLayerType);
}

template<LayerType TLayerType>
FloodTool<TLayerType>::~FloodTool()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

template<LayerType TLayerType>
void FloodTool<TLayerType>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    mController.BroadcastSampledInformationUpdatedAt(ScreenToShipSpace(mouseCoordinates), TLayerType);
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnLeftMouseDown()
{
    auto const mouseCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseCoordinates)
    {
        DoEdit(
            *mouseCoordinates,
            StrongTypedFalse<IsRightMouseButton>);
    }
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnRightMouseDown()
{
    auto const mouseCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseCoordinates)
    {
        DoEdit(
            *mouseCoordinates,
            StrongTypedTrue<IsRightMouseButton>);
    }
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnMouseLeft()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void FloodTool<TLayer>::DoEdit(
    ShipSpaceCoordinates const & mouseCoordinates,
    StrongTypedBool<struct IsRightMouseButton> isRightButton)
{
    auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();

    //
    // Do edit
    //

    LayerMaterialType const * const floodMaterial = GetFloodMaterial(isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground);

    static_assert(TLayer == LayerType::Structural); // At this moment this is only structural
    auto undoPayload = mController.GetModelController().StructuralFlood(
        mouseCoordinates,
        floodMaterial,
        mController.GetWorkbenchState().GetStructuralFloodToolIsContiguous());

    if (undoPayload.has_value())
    {
        //
        // Create undo action
        //

        auto const totalCost = undoPayload->GetTotalCost();

        mController.StoreUndoAction(
            TLayer == LayerType::Structural ? _("Flood Structural") : _("Flood Electrical"),
            totalCost,
            layerDirtyStateClone,
            [undoPayload = std::move(*undoPayload)](Controller & controller) mutable
            {
                controller.RestoreForUndo(std::move(undoPayload));
            });

        // Display sampled material
        mController.BroadcastSampledInformationUpdatedAt(mouseCoordinates, TLayer);

        // Epilog
        mController.LayerChangeEpilog({ TLayer });
    }
}

template<LayerType TLayer>
typename FloodTool<TLayer>::LayerMaterialType const * FloodTool<TLayer>::GetFloodMaterial(MaterialPlaneType plane) const
{
    static_assert(TLayer == LayerType::Structural);

    return plane == MaterialPlaneType::Foreground
        ? mController.GetWorkbenchState().GetStructuralForegroundMaterial()
        : mController.GetWorkbenchState().GetStructuralBackgroundMaterial();
}

}