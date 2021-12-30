/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FloodTool.h"

#include "Controller.h"

#include <type_traits>

namespace ShipBuilder {

StructuralFloodTool::StructuralFloodTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : FloodTool(
        ToolType::StructuralFlood,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayerType>
FloodTool<TLayerType>::FloodTool(
    ToolType toolType,
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mCursorImage(WxHelpers::LoadCursorImage("flood_tool_cursor", 12, 29, resourceLocator))
{
    SetCursor(mCursorImage);
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnLeftMouseDown()
{
    DoEdit(
        mUserInterface.GetMouseCoordinates(),
        StrongTypedFalse<IsRightMouseButton>);
}

template<LayerType TLayer>
void FloodTool<TLayer>::OnRightMouseDown()
{
    DoEdit(
        mUserInterface.GetMouseCoordinates(),
        StrongTypedTrue<IsRightMouseButton>);
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void FloodTool<TLayer>::DoEdit(
    ShipSpaceCoordinates const & mouseCoordinates,
    StrongTypedBool<struct IsRightMouseButton> isRightButton)
{
    // Take clone of current layer
    auto layerDirtyStateClone = mModelController.GetModel().GetDirtyState();
    auto layerClone = mModelController.GetModel().CloneExistingLayer<TLayer>();

    // Do edit
    LayerMaterialType const * const floodMaterial = GetFloodMaterial(isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground);
    static_assert(TLayer == LayerType::Structural);
    std::optional<ShipSpaceRect> affectedRegion = mModelController.StructuralFlood(
        mouseCoordinates,
        floodMaterial,
        mWorkbenchState.GetStructuralFloodToolIsContiguous());

    if (affectedRegion.has_value())
    {
        // Create undo action

        // FUTUREWORK: instead of cloning here, might have a Layer method to "trim" its buffer in-place
        auto clippedLayerClone = layerClone.Clone(*affectedRegion);

        PushUndoAction(
            TLayer == LayerType::Structural ? _("Flood Structural") : _("Flood Electrical"),
            clippedLayerClone.Buffer.GetByteSize(),
            layerDirtyStateClone,
            [clippedLayerClone = std::move(clippedLayerClone), origin = affectedRegion->origin](Controller & controller) mutable
            {
                static_assert(TLayer == LayerType::Structural);
                controller.RestoreStructuralLayerRegionForUndo(std::move(clippedLayerClone), origin);
            });

        // Mark layer as dirty
        SetLayerDirty(TLayer);

        // Refresh model visualizations
        mModelController.UpdateVisualizations(mView);
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
typename FloodTool<TLayer>::LayerMaterialType const * FloodTool<TLayer>::GetFloodMaterial(MaterialPlaneType plane) const
{
    static_assert(TLayer == LayerType::Structural);

    return plane == MaterialPlaneType::Foreground
        ? mWorkbenchState.GetStructuralForegroundMaterial()
        : mWorkbenchState.GetStructuralBackgroundMaterial();
}

}