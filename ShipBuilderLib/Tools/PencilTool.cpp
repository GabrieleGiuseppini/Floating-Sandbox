/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

#include <GameCore/GameGeometry.h>

#include <UILib/WxHelpers.h>

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
    , mOriginalLayerClone(modelController.GetModel().CloneLayer<TLayerType>())
    , mTempVisualizationDirtyShipRegion()
    , mEngagementData()
{
    wxImage cursorImage;
    if constexpr (IsEraser)
    {
        cursorImage = WxHelpers::LoadCursorImage("eraser_cursor", 8, 27, resourceLocator);
    }
    else
    {
        cursorImage = WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator);
    }

    SetCursor(cursorImage);

    // Check if we need to immediately do a visualization
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        // Calculate affected rect
        std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(*mouseCoordinates);

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

//////////////////////////////////////////////////////////////////////////////

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
    LayerMaterialType const * const fillMaterial = GetFillMaterial(mEngagementData->Plane);

    bool hasEdited = false;

    GenerateLinePath(
        (TLayer == LayerType::Structural) && mEngagementData->PreviousEngagementPosition.has_value()
            ? *mEngagementData->PreviousEngagementPosition
            : mouseCoordinates,
        mouseCoordinates,
        [&](ShipSpaceCoordinates const & pos)
        {
            // Calc applicable rect intersecting pencil with workspace size
            auto const applicableRect = CalculateApplicableRect(pos);
            if (applicableRect)
            {
                bool isAllowed;

                if constexpr (TLayer == LayerType::Structural)
                {
                    isAllowed = true;

                    mModelController.StructuralRegionFill(
                        *applicableRect,
                        fillMaterial);
                }
                else
                {
                    static_assert(TLayer == LayerType::Electrical);

                    assert(applicableRect->size == ShipSpaceSize(1, 1));
                    isAllowed = mModelController.IsElectricalParticleAllowedAt(applicableRect->origin);

                    if (isAllowed)
                    {
                        mModelController.ElectricalRegionFill(
                            *applicableRect,
                            fillMaterial);
                    }
                }

                if (isAllowed)
                {
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

        auto clippedRegionClone = mOriginalLayerClone.Clone(*mEngagementData->EditRegion);

        auto undoAction = std::make_unique<LayerRegionUndoAction<typename LayerTypeTraits<TLayer>::layer_data_type>>(
            IsEraser ? _("Eraser Tool") : _("Pencil Tool"),
            mEngagementData->OriginalDirtyState,
            std::move(clippedRegionClone),
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

    // Re-take original layer clone
    mOriginalLayerClone = mModelController.GetModel().CloneLayer<TLayer>();

    assert(!mTempVisualizationDirtyShipRegion);
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::DoTempVisualization(ShipSpaceRect const & affectedRect)
{
    // No buttons, hence choosing foreground plane
    LayerMaterialType const * fillMaterial = GetFillMaterial(MaterialPlaneType::Foreground);

    View::OverlayMode overlayMode = View::OverlayMode::Default;

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.StructuralRegionFillForEphemeralVisualization(
            affectedRect,
            fillMaterial);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        assert(affectedRect.size == ShipSpaceSize(1, 1));
        if (!mModelController.IsElectricalParticleAllowedAt(affectedRect.origin))
        {
            overlayMode = View::OverlayMode::Error;
        }

        mModelController.ElectricalRegionFillForEphemeralVisualization(
            affectedRect,
            fillMaterial);
    }

    mView.UploadRectOverlay(
        affectedRect,
        overlayMode);

    mTempVisualizationDirtyShipRegion = affectedRect;
}

template<LayerType TLayer, bool IsEraser>
void PencilTool<TLayer, IsEraser>::MendTempVisualization()
{
    assert(mTempVisualizationDirtyShipRegion);

    if constexpr (TLayer == LayerType::Structural)
    {
        mModelController.RestoreStructuralLayerRegionForEphemeralVisualization(
            mOriginalLayerClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        mModelController.RestoreElectricalLayerRegionForEphemeralVisualization(
            mOriginalLayerClone,
            *mTempVisualizationDirtyShipRegion,
            mTempVisualizationDirtyShipRegion->origin);
    }

    mView.RemoveRectOverlay();

    mTempVisualizationDirtyShipRegion.reset();
}

template<LayerType TLayer, bool IsEraser>
std::optional<ShipSpaceRect> PencilTool<TLayer, IsEraser>::CalculateApplicableRect(ShipSpaceCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const pencilSize = GetPencilSize();
    int const topLeftPencilSize =  (pencilSize - 1) / 2;

    ShipSpaceCoordinates const origin = ShipSpaceCoordinates(coords.x, coords.y - (pencilSize - 1));

    return ShipSpaceRect(origin - ShipSpaceSize(topLeftPencilSize, -topLeftPencilSize), { pencilSize, pencilSize })
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
typename PencilTool<TLayer, IsEraser>::LayerMaterialType const * PencilTool<TLayer, IsEraser>::GetFillMaterial(MaterialPlaneType plane) const
{
    if constexpr (!IsEraser)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            if (plane == MaterialPlaneType::Foreground)
            {
                return mWorkbenchState.GetStructuralForegroundMaterial();
            }
            else
            {
                assert(plane == MaterialPlaneType::Background);

                return mWorkbenchState.GetStructuralBackgroundMaterial();
            }

        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            if (plane == MaterialPlaneType::Foreground)
            {
                return mWorkbenchState.GetElectricalForegroundMaterial();
            }
            else
            {
                assert(plane == MaterialPlaneType::Background);

                return mWorkbenchState.GetElectricalBackgroundMaterial();
            }
        }
    }
    else
    {
        return nullptr;
    }
}

}