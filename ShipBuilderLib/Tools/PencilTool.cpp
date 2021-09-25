/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

#include <type_traits>

namespace ShipBuilder {

template<LayerType TLayerType>
PencilTool<TLayerType>::PencilTool(
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
    , mEngagementData()
    , mCursorImage(WxHelpers::LoadCursorImage("pencil_cursor", 1, 29, resourceLocator))
{
    SetCursor(mCursorImage);
}

StructuralPencilTool::StructuralPencilTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::StructuralPencil,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalPencilTool::ElectricalPencilTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::ElectricalPencil,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayer>
void PencilTool<TLayer>::Reset()
{
    mEngagementData.reset();
}

template<LayerType TLayer>
void PencilTool<TLayer>::OnMouseMove(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer>
void PencilTool<TLayer>::OnLeftMouseDown(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer>
void PencilTool<TLayer>::OnLeftMouseUp(InputState const & /*inputState*/)
{
    CheckEndEngagement();
}

template<LayerType TLayer>
void PencilTool<TLayer>::OnRightMouseDown(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer>
void PencilTool<TLayer>::OnRightMouseUp(InputState const & /*inputState*/)
{
    CheckEndEngagement();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void PencilTool<TLayer>::CheckStartEngagement(InputState const & inputState)
{
    if (!inputState.IsLeftMouseDown && !inputState.IsRightMouseDown)
    {
        // Not an engagement action, ignore
        return;
    }

    if (mEngagementData.has_value())
    {
        // Already engaged, ignore
        return;
    }

    ShipSpaceCoordinates const coords = mView.ScreenToShipSpace(inputState.MousePosition);
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        // Outside ship, ignore
        return;
    }

    //
    // Start engagement
    //

    std::unique_ptr<typename LayerTypeTraits<TLayer>::buffer_type> originalRegionClone;
    if constexpr (TLayer == LayerType::Structural)
    {
        originalRegionClone = mModelController.GetModel().CloneStructuralLayerBuffer();
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        originalRegionClone = mModelController.GetModel().CloneElectricalLayerBuffer();
    }

    mEngagementData.emplace(
        inputState.IsLeftMouseDown ? MaterialPlaneType::Foreground : MaterialPlaneType::Background,
        std::move(originalRegionClone),
        coords,
        mModelController.GetModel().GetDirtyState());
}

template<LayerType TLayer>
void PencilTool<TLayer>::CheckEndEngagement()
{
    if (!mEngagementData.has_value())
    {
        // Not engaged, ignore
        return;
    }

    //
    // Create undo action
    //

    auto clippedRegionClone = mEngagementData->OriginalRegionClone->MakeCopy(mEngagementData->EditRegion);

    auto undoAction = std::make_unique<LayerBufferRegionUndoAction<typename LayerTypeTraits<TLayer>::buffer_type>>(
        _("Pencil"),
        mEngagementData->OriginalDirtyState,
        std::move(*clippedRegionClone),
        mEngagementData->EditRegion.origin);

    PushUndoAction(std::move(undoAction));

    // Reset engagement
    mEngagementData.reset();
}

template<LayerType TLayer>
void PencilTool<TLayer>::CheckEdit(InputState const & inputState)
{
    if (!mEngagementData.has_value())
    {
        // Not engaged, ignore
        return;
    }

    ShipSpaceCoordinates const coords = mView.ScreenToShipSpace(inputState.MousePosition);
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        // Outside ship, ignore
        return;
    }

    //
    // Apply edit
    //

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.StructuralRegionFill(
            mEngagementData->Plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetStructuralForegroundMaterial()
            : mWorkbenchState.GetStructuralBackgroundMaterial(),
            coords,
            ShipSpaceSize(1, 1));
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mModelController.ElectricalRegionFill(
            mEngagementData->Plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetElectricalForegroundMaterial()
            : mWorkbenchState.GetElectricalBackgroundMaterial(),
            coords,
            ShipSpaceSize(1, 1));
    }

    // Update edit region
    mEngagementData->EditRegion.UpdateWith(coords);

    // Mark layer as dirty
    SetLayerDirty(TLayer);

    // Force view refresh
    mUserInterface.RefreshView();
}

}