/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LineTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

StructuralLineTool::StructuralLineTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::StructuralLine,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalLineTool::ElectricalLineTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::ElectricalLine,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayer>
LineTool<TLayer>::LineTool(
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
    , mOriginalLayerClone(modelController.GetModel().CloneExistingLayer<TLayer>())
    , mEphemeralVisualization()
    , mEngagementData()
    , mIsShiftDown(false)
{
    wxImage cursorImage = WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, resourceLocator);
    SetCursor(cursorImage);

    // Check if we need to immediately do an ephemeral visualization
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DoEphemeralVisualization(*mouseCoordinates);

        // Visualize
        mModelController.UploadVisualizations();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
LineTool<TLayer>::~LineTool()
{
    // Mend our ephemeral visualization, if any
    if (mEphemeralVisualization.has_value())
    {
        mEphemeralVisualization.reset();

        // Visualize
        mModelController.UploadVisualizations();
        mUserInterface.RefreshView();
    }
}

template<LayerType TLayer>
void LineTool<TLayer>::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Foreground);

        assert(mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();


    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Background);

        assert(mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();


    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnShiftKeyDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    mIsShiftDown = true;

    // Do ephemeral visualization
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnShiftKeyUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    mIsShiftDown = false;

    // Do ephemeral visualization
    DoEphemeralVisualization(mUserInterface.GetMouseCoordinates());

    // Visualize
    mModelController.UploadVisualizations();
    mUserInterface.RefreshView();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void LineTool<TLayer>::StartEngagement(
    ShipSpaceCoordinates const & mouseCoordinates,
    MaterialPlaneType plane)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        mModelController.GetModel().GetDirtyState(),
        mouseCoordinates,
        plane);
}

template<LayerType TLayer>
void LineTool<TLayer>::EndEngagement(ShipSpaceCoordinates const & mouseCoordinates)
{
    assert(mEngagementData);
    assert(!mEphemeralVisualization.has_value());

    //
    // Do edit
    //

    LayerMaterialType const * fillMaterial = GetFillMaterial(mEngagementData->Plane);

    std::optional<ShipSpaceRect> resultantEffectiveRect;

    DoLine(
        mEngagementData->StartCoords,
        mouseCoordinates,
        [&](ShipSpaceCoordinates const & pos)
        {
            auto const [effectiveRect, hasEdited] = TryFill<false>(pos, fillMaterial);

            if (effectiveRect && hasEdited)
            {
                if (!resultantEffectiveRect)
                {
                    resultantEffectiveRect = *effectiveRect;
                }
                else
                {
                    resultantEffectiveRect->UnionWith(*effectiveRect);
                }
            }
        });

    if (resultantEffectiveRect.has_value())
    {
        //
        // Mark layer as dirty
        //

        SetLayerDirty(TLayer);

        //
        // Create undo action
        //

        auto clippedLayerClone = mOriginalLayerClone.Clone(*resultantEffectiveRect);

        PushUndoAction(
            TLayer == LayerType::Structural ? _("Line Structural") : _("Line Electrical"),
            clippedLayerClone.Buffer.GetByteSize(),
            mEngagementData->OriginalDirtyState,
            [clippedLayerClone = std::move(clippedLayerClone), origin = resultantEffectiveRect->origin](Controller & controller) mutable
            {
                controller.RestoreLayerRegionForUndo(std::move(clippedLayerClone), origin);
            });
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();

    //
    // Re-take original layer clone
    //

    mOriginalLayerClone = mModelController.GetModel().CloneExistingLayer<TLayer>();
}

template<LayerType TLayer>
void LineTool<TLayer>::DoEphemeralVisualization(ShipSpaceCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        //
        // Temp viz with line + dashed line overlay
        //

        LayerMaterialType const * fillMaterial = GetFillMaterial(mEngagementData->Plane);

        std::optional<ShipSpaceRect> resultantEffectiveRect;
        View::OverlayMode resultantOverlayMode = View::OverlayMode::Default;

        DoLine(
            mEngagementData->StartCoords,
            mouseCoordinates,
            [&](ShipSpaceCoordinates const & pos)
            {
                auto const [effectiveRect, hasEdited] = TryFill<true>(pos, fillMaterial);

                if (effectiveRect)
                {
                    if (hasEdited)
                    {
                        if (!resultantEffectiveRect)
                        {
                            resultantEffectiveRect = *effectiveRect;
                        }
                        else
                        {
                            resultantEffectiveRect->UnionWith(*effectiveRect);
                        }
                    }
                    else
                    {
                        resultantOverlayMode = View::OverlayMode::Error;
                    }
                }
            });

        mView.UploadDashedLineOverlay(
            mEngagementData->StartCoords,
            mouseCoordinates,
            resultantOverlayMode);

        // Schedule cleanup
        mEphemeralVisualization.emplace(
            [this, resultantEffectiveRect]()
            {
                if (resultantEffectiveRect.has_value())
                {
                    if constexpr (TLayer == LayerType::Structural)
                    {
                        mModelController.RestoreStructuralLayerRegionForEphemeralVisualization(
                            mOriginalLayerClone,
                            *resultantEffectiveRect,
                            resultantEffectiveRect->origin);
                    }
                    else
                    {
                        static_assert(TLayer == LayerType::Electrical);

                        mModelController.RestoreElectricalLayerRegionForEphemeralVisualization(
                            mOriginalLayerClone,
                            *resultantEffectiveRect,
                            resultantEffectiveRect->origin);
                    }
                }

                mView.RemoveDashedLineOverlay();
            });
    }
    else
    {
        //
        // Temp viz with block fill + rect overlay 
        //

        // No mouse button information, hence choosing foreground plane arbitrarily
        LayerMaterialType const * fillMaterial = GetFillMaterial(MaterialPlaneType::Foreground);

        auto const [effectiveRect, hasEdited] = TryFill<true>(mouseCoordinates, fillMaterial);

        if (effectiveRect.has_value())
        {
            mView.UploadRectOverlay(
                *effectiveRect,
                hasEdited ? View::OverlayMode::Default : View::OverlayMode::Error);

            // Schedule cleanup
            mEphemeralVisualization.emplace(
                [this, effectiveRect = effectiveRect, hasEdited = hasEdited]()
                {
                    if (hasEdited)
                    {
                        if constexpr (TLayer == LayerType::Structural)
                        {
                            mModelController.RestoreStructuralLayerRegionForEphemeralVisualization(
                                mOriginalLayerClone,
                                *effectiveRect,
                                effectiveRect->origin);
                        }
                        else
                        {
                            static_assert(TLayer == LayerType::Electrical);

                            mModelController.RestoreElectricalLayerRegionForEphemeralVisualization(
                                mOriginalLayerClone,
                                *effectiveRect,
                                effectiveRect->origin);
                        }
                    }

                    mView.RemoveRectOverlay();
                });

        }
    }
}

template<LayerType TLayer>
template<typename TVisitor>
void LineTool<TLayer>::DoLine(
    ShipSpaceCoordinates const & startPoint,
    ShipSpaceCoordinates const & endPoint,
    TVisitor && visitor)
{
    ShipSpaceCoordinates actualEndPoint = endPoint;
    if (mIsShiftDown)
    {
        // Constrain to either horizontally or vertically
        if (std::abs(endPoint.x - startPoint.x) > std::abs(endPoint.y - startPoint.y))
        {
            // X is larger
            actualEndPoint.y = startPoint.y;
        }
        else
        {
            // Y is larger
            actualEndPoint.x = startPoint.x;
        }
    }

    if (TLayer == LayerType::Structural && mWorkbenchState.GetStructuralLineToolIsHullMode())
    {
        GenerateIntegralLinePath<IntegralLineType::WithAdjacentSteps>(startPoint, actualEndPoint, std::forward<TVisitor>(visitor));
    }
    else
    {
        GenerateIntegralLinePath<IntegralLineType::Minimal>(startPoint, actualEndPoint, std::forward<TVisitor>(visitor));
    }
}

template<LayerType TLayer>
template<bool TIsForEphemeralVisualization>
std::pair<std::optional<ShipSpaceRect>, StrongTypedBool<struct HasEdited>> LineTool<TLayer>::TryFill(
    ShipSpaceCoordinates const & pos,
    LayerMaterialType const * fillMaterial)
{
    std::optional<ShipSpaceRect> const affectedRect = CalculateApplicableRect(pos);
    if (affectedRect.has_value())
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            if constexpr (TIsForEphemeralVisualization)
            {
                mModelController.StructuralRegionFillForEphemeralVisualization(
                    *affectedRect,
                    fillMaterial);
            }
            else
            {
                mModelController.StructuralRegionFill(
                    *affectedRect,
                    fillMaterial);
            }

            return std::make_pair(affectedRect, StrongTypedTrue<HasEdited>);
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            assert(affectedRect->size == ShipSpaceSize(1, 1));
            if (mModelController.IsElectricalParticleAllowedAt(affectedRect->origin))
            {
                if constexpr (TIsForEphemeralVisualization)
                {
                    mModelController.ElectricalRegionFillForEphemeralVisualization(
                        *affectedRect,
                        fillMaterial);
                }
                else
                {
                    mModelController.ElectricalRegionFill(
                        *affectedRect,
                        fillMaterial);
                }

                return std::make_pair(affectedRect, StrongTypedTrue<HasEdited>);
            }
        }
    }

    // Haven't filled
    return std::make_pair(affectedRect, StrongTypedFalse<HasEdited>);
}

template<LayerType TLayer>
std::optional<ShipSpaceRect> LineTool<TLayer>::CalculateApplicableRect(ShipSpaceCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const lineSize = GetLineSize();
    int const topLeftLineSize =  (lineSize - 1) / 2;

    ShipSpaceCoordinates const origin = ShipSpaceCoordinates(coords.x, coords.y - (lineSize - 1));

    return ShipSpaceRect(origin - ShipSpaceSize(topLeftLineSize, -topLeftLineSize), { lineSize, lineSize })
        .MakeIntersectionWith({ { 0, 0 }, mModelController.GetModel().GetShipSize() });
}

template<LayerType TLayer>
int LineTool<TLayer>::GetLineSize() const
{
    if constexpr (TLayer == LayerType::Structural)
    {
        return static_cast<int>(mWorkbenchState.GetStructuralLineToolSize());
    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        return 1;
    }
}

template<LayerType TLayer>
typename LineTool<TLayer>::LayerMaterialType const * LineTool<TLayer>::GetFillMaterial(MaterialPlaneType plane) const
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

}