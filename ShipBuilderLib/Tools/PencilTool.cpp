/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

#include <GameCore/GameGeometry.h>

#include <type_traits>

namespace ShipBuilder {

template<LayerType TLayerType, bool IsEraser>
PencilTool<TLayerType, IsEraser>::PencilTool(
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
    , mCursorImage(WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator))
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

StructuralEraserTool::StructuralEraserTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::StructuralEraser,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalEraserTool::ElectricalEraserTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::ElectricalEraser,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::Reset()
{
    mEngagementData.reset();
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnMouseMove(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseDown(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseUp(InputState const & /*inputState*/)
{
    CheckEndEngagement();
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnRightMouseDown(InputState const & inputState)
{
    CheckStartEngagement(inputState);
    CheckEdit(inputState);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnRightMouseUp(InputState const & /*inputState*/)
{
    CheckEndEngagement();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::CheckStartEngagement(InputState const & inputState)
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

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::CheckEndEngagement()
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

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::CheckEdit(InputState const & inputState)
{
    if (!mEngagementData.has_value())
    {
        // Not engaged, ignore
        return;
    }

    // TODO: now with applicableRegion, this is wrong
    ShipSpaceCoordinates const coords = mView.ScreenToShipSpace(inputState.MousePosition);
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        // Outside ship, ignore

        // Reset previous engagement
        mEngagementData->PreviousEngagementPosition.reset();

        return;
    }

    //
    // Apply edit
    //

    assert(!mEngagementData->PreviousEngagementPosition.has_value() || mEngagementData->PreviousEngagementPosition->IsInSize(mModelController.GetModel().GetShipSize()));
    assert(coords.IsInSize(mModelController.GetModel().GetShipSize()));

    // Calculate fill element and pencil size
    typename LayerTypeTraits<TLayer>::buffer_type::element_type fillElement;
    int pencilSize;
    if constexpr (!IsEraser)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            fillElement = {
                mEngagementData->Plane == MaterialPlaneType::Foreground
                    ? mWorkbenchState.GetStructuralForegroundMaterial()
                    : mWorkbenchState.GetStructuralBackgroundMaterial()
            };

            pencilSize = static_cast<int>(mWorkbenchState.GetStructuralPencilToolSize());
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            fillElement = {
                mEngagementData->Plane == MaterialPlaneType::Foreground
                    ? mWorkbenchState.GetElectricalForegroundMaterial()
                    : mWorkbenchState.GetElectricalBackgroundMaterial(),
                NoneElectricalElementInstanceIndex
            };

            pencilSize = 1;
        }
    }
    else
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            fillElement = { nullptr };

            pencilSize = static_cast<int>(mWorkbenchState.GetStructuralEraserToolSize());
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            fillElement = { nullptr, NoneElectricalElementInstanceIndex };

            pencilSize = 1;
        }
    }

    // Fill
    GenerateLinePath(
        mEngagementData->PreviousEngagementPosition.has_value()
            ? *mEngagementData->PreviousEngagementPosition
            : coords,
        coords,
        [&](ShipSpaceCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = ShipSpaceRect(pos, { pencilSize, pencilSize }).MakeIntersectionWith({ { 0, 0 }, mModelController.GetModel().GetShipSize() });
            if (applicableRect)
            {
                if constexpr (TLayer == LayerType::Structural)
                {
                    mModelController.StructuralRegionFill(
                        fillElement,
                        *applicableRect);
                }
                else
                {
                    static_assert(TLayer == LayerType::Electrical);

                    mModelController.ElectricalRegionFill(
                        fillElement,
                        *applicableRect);
                }

                // Update edit region
                mEngagementData->EditRegion.UnionWith(*applicableRect);
            }
        });

    // Mark layer as dirty
    SetLayerDirty(TLayer);

    // Force view refresh
    mUserInterface.RefreshView();

    // Update previous engagement
    mEngagementData->PreviousEngagementPosition = coords;
}

}