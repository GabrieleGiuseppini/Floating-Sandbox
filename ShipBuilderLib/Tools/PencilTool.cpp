/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

#include <GameCore/GameGeometry.h>

#include <type_traits>

namespace ShipBuilder {

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
    , mOriginalLayerBufferClone()
    , mTempVisualizationDirtyShipRegion()
    , mEngagementData()
    , mCursorImage(WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator))
{
    SetCursor(mCursorImage);

    Reset();
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::Reset()
{
    // Mend our temporary visualization, if any
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);

        // Visualize
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }

    // Reset original layer clone
    TakeOriginalLayerBufferClone();

    // Reset engagement data
    mEngagementData.reset();
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnMouseMove(InputState const & inputState)
{
    // Assuming L/R button transitions already communicated

    if (!mEngagementData)
    {
        //
        // Restore temp visualization
        //

        if (mTempVisualizationDirtyShipRegion)
        {
            MendTempVisualization();

            assert(!mTempVisualizationDirtyShipRegion);
        }

        //
        // Calculate affected rect
        //

        ShipSpaceCoordinates const mouseCoords = mView.ScreenToShipSpace(inputState.MousePosition);
        std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(mouseCoords);

        //
        // Apply (temporary) change
        //

        if (affectedRect)
        {
            // No buttons, hence choosing foreground plane
            LayerElementType const fillElement = GetFillElement(MaterialPlaneType::Foreground);

            if constexpr (TLayer == LayerType::Structural)
            {
                mModelController.StructuralRegionFill(
                    fillElement,
                    *affectedRect);
            }
            else
            {
                static_assert(TLayer == LayerType::Electrical);

                mModelController.ElectricalRegionFill(
                    fillElement,
                    *affectedRect);
            }

            mTempVisualizationDirtyShipRegion = affectedRect;
        }

        //
        // Visualize
        //

        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
    else
    {
        DoEdit(inputState);
    }
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseDown(InputState const & inputState)
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    if (!mEngagementData)
    {
        StartEngagement(inputState);

        assert(mEngagementData);
    }

    DoEdit(inputState);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseUp(InputState const & /*inputState*/)
{
    if (mEngagementData)
    {
        EndEngagement();

        assert(!mEngagementData);
    }

    // Note: we don't start temp visualization, as the current mouse position
    // already has the edit (as permanent)
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnRightMouseDown(InputState const & inputState)
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    if (!mEngagementData)
    {
        StartEngagement(inputState);

        assert(mEngagementData);
    }

    DoEdit(inputState);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnRightMouseUp(InputState const & /*inputState*/)
{
    if (mEngagementData)
    {
        EndEngagement();

        assert(!mEngagementData);
    }

    // Note: we don't start temp visualization, as the current mouse position
    // already has the edit (as permanent)
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::TakeOriginalLayerBufferClone()
{
    if constexpr (TLayer == LayerType::Structural)
    {
        mOriginalLayerBufferClone = mModelController.GetModel().CloneStructuralLayerBuffer();
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mOriginalLayerBufferClone = mModelController.GetModel().CloneElectricalLayerBuffer();
    }
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::StartEngagement(InputState const & inputState)
{
    assert(inputState.IsLeftMouseDown || inputState.IsRightMouseDown);
    assert(!mEngagementData);

    ShipSpaceCoordinates const coords = mView.ScreenToShipSpace(inputState.MousePosition);

    mEngagementData.emplace(
        inputState.IsLeftMouseDown ? MaterialPlaneType::Foreground : MaterialPlaneType::Background,
        coords,
        mModelController.GetModel().GetDirtyState());
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::DoEdit(InputState const & inputState)
{
    assert(mEngagementData);

    ShipSpaceCoordinates const mouseCoords = mView.ScreenToShipSpace(inputState.MousePosition);

    int const pencilSize = GetPencilSize();
    LayerElementType const fillElement = GetFillElement(mEngagementData->Plane);

    // Fill
    GenerateLinePath(
        mEngagementData->PreviousEngagementPosition.has_value()
            ? *mEngagementData->PreviousEngagementPosition
            : mouseCoords,
        mouseCoords,
        [&](ShipSpaceCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = CalculateApplicableRect(pos);
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

    // Refresh model visualization
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();

    // Update previous engagement
    mEngagementData->PreviousEngagementPosition = mouseCoords;
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::EndEngagement()
{
    assert(mEngagementData);

    //
    // Create undo action
    //

    auto clippedRegionClone = mOriginalLayerBufferClone->MakeCopy(mEngagementData->EditRegion);

    auto undoAction = std::make_unique<LayerBufferRegionUndoAction<typename LayerTypeTraits<TLayer>::buffer_type>>(
        _("Pencil"),
        mEngagementData->OriginalDirtyState,
        std::move(*clippedRegionClone),
        mEngagementData->EditRegion.origin);

    PushUndoAction(std::move(undoAction));

    //
    // Reset engagement
    //

    mEngagementData.reset();

    //
    // Restart temp visualization
    //

    TakeOriginalLayerBufferClone();

    assert(!mTempVisualizationDirtyShipRegion);
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

    int const pencilSize = GetPencilSize();

    // Calculate fill element
    LayerElementType const fillElement = GetFillElement(mEngagementData->Plane);

    // Fill
    GenerateLinePath(
        mEngagementData->PreviousEngagementPosition.has_value()
            ? *mEngagementData->PreviousEngagementPosition
            : coords,
        coords,
        [&](ShipSpaceCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect =
                ShipSpaceRect(pos, { pencilSize, pencilSize })
                .MakeIntersectionWith({ { 0, 0 }, mModelController.GetModel().GetShipSize() });
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

    // Refresh model visualization
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();

    // Update previous engagement
    mEngagementData->PreviousEngagementPosition = coords;
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::MendTempVisualization()
{
    assert(mTempVisualizationDirtyShipRegion);

    assert(mOriginalLayerBufferClone);

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.StructuralRegionReplace(
            *mOriginalLayerBufferClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mModelController.ElectricalRegionReplace(
            *mOriginalLayerBufferClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }

    mTempVisualizationDirtyShipRegion.reset();
}

template<LayerType TLayer, bool IsEraser>
std::optional<ShipSpaceRect> PencilTool<TLayer, IsEraser>::CalculateApplicableRect(ShipSpaceCoordinates const & coords) const
{
    int const pencilSize = GetPencilSize();

    // CODEWORK: anchor
    return ShipSpaceRect(coords, { pencilSize, pencilSize })
        .MakeIntersectionWith({ { 0, 0 }, mModelController.GetModel().GetShipSize() });
}

template<LayerType TLayer, bool IsEraser>
int PencilTool<TLayer, IsEraser>::GetPencilSize() const
{
    if constexpr (!IsEraser)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            return static_cast<int>(mWorkbenchState.GetStructuralPencilToolSize());
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            return 1;
        }
    }
    else
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            return static_cast<int>(mWorkbenchState.GetStructuralEraserToolSize());
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            return 1;
        }
    }
}

template<LayerType TLayer, bool IsEraser>
typename PencilTool<TLayer, IsEraser>::LayerElementType PencilTool<TLayer, IsEraser>::GetFillElement(MaterialPlaneType plane) const
{
    if constexpr (!IsEraser)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            return LayerElementType(
                plane == MaterialPlaneType::Foreground
                    ? mWorkbenchState.GetStructuralForegroundMaterial()
                    : mWorkbenchState.GetStructuralBackgroundMaterial());
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            return LayerElementType(
                plane == MaterialPlaneType::Foreground
                    ? mWorkbenchState.GetElectricalForegroundMaterial()
                    : mWorkbenchState.GetElectricalBackgroundMaterial(),
                NoneElectricalElementInstanceIndex); // TODOHERE
        }
    }
    else
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            return LayerElementType(nullptr);
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            return LayerElementType(nullptr, NoneElectricalElementInstanceIndex);
        }
    }
}

}