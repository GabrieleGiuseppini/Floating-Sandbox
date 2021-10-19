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

    // Take original layer clone
    TakeOriginalLayerBufferClone();

    //
    // Do temp visualization
    //

    // Calculate affected rect
    std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(mUserInterface.GetMouseCoordinates());

    // Apply (temporary) change
    if (affectedRect)
    {
        DoTempVisualization(*affectedRect);

        assert(mTempVisualizationDirtyShipRegion);

        // Visualize
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer, bool IsEraser>
PencilTool<TLayer, IsEraser>::~PencilTool()
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
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    if (!mEngagementData)
    {
        //
        // Temp visualization
        //

        // Restore previous temp visualization
        if (mTempVisualizationDirtyShipRegion)
        {
            MendTempVisualization();

            assert(!mTempVisualizationDirtyShipRegion);
        }

        // Calculate affected rect
        std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(mouseCoordinates);

        // Apply (temporary) change
        if (affectedRect)
        {
            DoTempVisualization(*affectedRect);

            assert(mTempVisualizationDirtyShipRegion);
        }

        // Visualize
        mModelController.UploadVisualization();
        mUserInterface.RefreshView();
    }
    else
    {
        DoEdit(mouseCoordinates);
    }
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseDown()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    if (!mEngagementData)
    {
        StartEngagement(StrongTypedFalse<IsRightMouseButton>);

        assert(mEngagementData);
    }

    DoEdit(mUserInterface.GetMouseCoordinates());
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnLeftMouseUp()
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
void PencilTool<TLayer, IsEraser>::OnRightMouseDown()
{
    // Restore temp visualization
    if (mTempVisualizationDirtyShipRegion)
    {
        MendTempVisualization();

        assert(!mTempVisualizationDirtyShipRegion);
    }

    if (!mEngagementData)
    {
        StartEngagement(StrongTypedTrue<IsRightMouseButton>);

        assert(mEngagementData);
    }

    DoEdit(mUserInterface.GetMouseCoordinates());
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::OnRightMouseUp()
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
void PencilTool<TLayer, IsEraser>::OnUncapturedMouseOut()
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
void PencilTool<TLayer, IsEraser>::StartEngagement(StrongTypedBool<struct IsRightMouseButton> isRightButton)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground,
        mModelController.GetModel().GetDirtyState());
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::DoEdit(ShipSpaceCoordinates const & mouseCoordinates)
{
    assert(mEngagementData);

    int const pencilSize = GetPencilSize();
    LayerElementType const fillElement = GetFillElement(mEngagementData->Plane);

    bool hasEdited = false;

    GenerateLinePath(
        mEngagementData->PreviousEngagementPosition.has_value()
            ? *mEngagementData->PreviousEngagementPosition
            : mouseCoordinates,
        mouseCoordinates,
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

                auto const old = mEngagementData->EditRegion;

                // Update edit region
                if (!mEngagementData->EditRegion)
                {
                    mEngagementData->EditRegion = *applicableRect;
                }
                else
                {
                    mEngagementData->EditRegion->UnionWith(*applicableRect);
                }

                hasEdited = true;
            }
        });

    if (hasEdited)
    {
        // Mark layer as dirty
        SetLayerDirty(TLayer);
    }

    // Refresh model visualization
    mModelController.UploadVisualization();
    mUserInterface.RefreshView();

    // Update previous engagement
    mEngagementData->PreviousEngagementPosition = mouseCoordinates;
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::EndEngagement()
{
    assert(mEngagementData);

    if (mEngagementData->EditRegion)
    {
        //
        // Create undo action
        //

        auto clippedRegionClone = mOriginalLayerBufferClone->MakeCopy(*mEngagementData->EditRegion);

        auto undoAction = std::make_unique<LayerBufferRegionUndoAction<typename LayerTypeTraits<TLayer>::buffer_type>>(
            IsEraser ? _("Eraser Tool") : _("Pencil Tool"),
            mEngagementData->OriginalDirtyState,
            std::move(*clippedRegionClone),
            mEngagementData->EditRegion->origin);

        PushUndoAction(std::move(undoAction));
    }

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
void PencilTool<TLayer, IsEraser>::DoTempVisualization(ShipSpaceRect const & affectedRect)
{
    // No buttons, hence choosing foreground plane
    LayerElementType const fillElement = GetFillElement(MaterialPlaneType::Foreground);

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.StructuralRegionFill(
            fillElement,
            affectedRect);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mModelController.ElectricalRegionFill(
            fillElement,
            affectedRect);
    }

    mTempVisualizationDirtyShipRegion = affectedRect;
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