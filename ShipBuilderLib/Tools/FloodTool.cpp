/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FloodTool.h"

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

ElectricalFloodTool::ElectricalFloodTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : FloodTool(
        ToolType::ElectricalFlood,
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
    auto layerClone = mModelController.GetModel().CloneLayer<TLayer>();

    // Do edit
    LayerMaterialType const * const floodMaterial = GetFloodMaterial(isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground);
    std::optional<ShipSpaceRect> affectedRegion;
    if constexpr (TLayer == LayerType::Structural)
    {
        affectedRegion = mModelController.StructuralFlood(
            mouseCoordinates,
            floodMaterial);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        if (floodMaterial->IsInstanced)
        {
            // Do not flood using instanced materials, that would make instance IDs explode
            mUserInterface.OnError(_("The flood tool cannot be used with instanced electrical materials."));
        }
        else
        {
            affectedRegion = mModelController.ElectricalFlood(
                mouseCoordinates,
                floodMaterial);
        }
    }

    if (affectedRegion.has_value())
    {
        // Create undo action

        // FUTUREWORK: instead of cloning here, might have a Layer method to "trim" its buffer in-place
        auto clippedLayerClone = layerClone.Clone(*affectedRegion);

        auto undoAction = std::make_unique<LayerRegionUndoAction<typename LayerTypeTraits<TLayer>::layer_data_type>>(
            _("Flood Tool"),
            std::move(layerDirtyStateClone),
            std::move(clippedLayerClone),
            affectedRegion->origin);

        PushUndoAction(std::move(undoAction));

        // Mark layer as dirty
        SetLayerDirty(TLayer);

        // Refresh model visualization
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
typename FloodTool<TLayer>::LayerMaterialType const * FloodTool<TLayer>::GetFloodMaterial(MaterialPlaneType plane) const
{
    if constexpr (TLayer == LayerType::Structural)
    {
        return plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetStructuralForegroundMaterial()
            : mWorkbenchState.GetStructuralBackgroundMaterial();
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        return plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetElectricalForegroundMaterial()
            : mWorkbenchState.GetElectricalBackgroundMaterial();
    }
}

}