/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FloodTool.h"

#include <Controller.h>

#include <type_traits>

namespace ShipBuilder {

StructuralFloodTool::StructuralFloodTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : FloodTool(
        ToolType::StructuralFlood,
        controller,
        resourceLocator)
{}

template<LayerType TLayerType>
FloodTool<TLayerType>::FloodTool(
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mCursorImage(WxHelpers::LoadCursorImage("flood_tool_cursor", 12, 29, resourceLocator))
{
    SetCursor(mCursorImage);

    auto const mouseCoordinates = GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        mController.BroadcastSampledInformationUpdatedAt(ScreenToShipSpace(*mouseCoordinates), TLayerType);
    }
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
    DoEdit(
        GetCurrentMouseCoordinatesInShipSpace(),
        StrongTypedFalse<IsRightMouseButton>);
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnRightMouseDown()
{
    DoEdit(
        GetCurrentMouseCoordinatesInShipSpace(),
        StrongTypedTrue<IsRightMouseButton>);
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void FloodTool<TLayer>::DoEdit(
    ShipSpaceCoordinates const & mouseCoordinates,
    StrongTypedBool<struct IsRightMouseButton> isRightButton)
{
    // Take clone of current layer
    auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();
    auto layerClone = mController.GetModelController().CloneExistingLayer<TLayer>();

    // Do edit
    LayerMaterialType const * const floodMaterial = GetFloodMaterial(isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground);
    static_assert(TLayer == LayerType::Structural);
    std::optional<ShipSpaceRect> affectedRegion = mController.GetModelController().StructuralFlood(
        mouseCoordinates,
        floodMaterial,
        mController.GetWorkbenchState().GetStructuralFloodToolIsContiguous());

    if (affectedRegion.has_value())
    {
        // Create undo action

        // FUTUREWORK: instead of cloning here, might have a Layer method to "trim" its buffer in-place
        auto clippedLayerClone = layerClone.Clone(*affectedRegion);

        mController.StoreUndoAction(
            TLayer == LayerType::Structural ? _("Flood Structural") : _("Flood Electrical"),
            clippedLayerClone.Buffer.GetByteSize(),
            layerDirtyStateClone,
            [clippedLayerClone = std::move(clippedLayerClone), origin = affectedRegion->origin](Controller & controller) mutable
            {
                static_assert(TLayer == LayerType::Structural);
                controller.RestoreStructuralLayerRegionForUndo(std::move(clippedLayerClone), origin);
            });

        // Epilog
        mController.LayerChangeEpilog(TLayer);
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