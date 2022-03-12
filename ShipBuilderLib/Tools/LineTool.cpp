/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LineTool.h"

#include <Controller.h>

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

StructuralLineTool::StructuralLineTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::StructuralLine,
        controller,
        resourceLocator)
{}

ElectricalLineTool::ElectricalLineTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : LineTool(
        ToolType::ElectricalLine,
        controller,
        resourceLocator)
{}

template<LayerType TLayer>
LineTool<TLayer>::LineTool(
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mOriginalLayerClone(mController.GetModel().CloneExistingLayer<TLayer>())
    , mEphemeralVisualization()
    , mEngagementData()
    , mIsShiftDown(false)
{
    wxImage cursorImage = WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, resourceLocator);
    SetCursor(cursorImage);

    // Check if we need to immediately do an ephemeral visualization
    auto const mouseCoordinates = GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DoEphemeralVisualization(ScreenToShipSpace(*mouseCoordinates));

        mController.LayerChangeEpilog();
    }
}

template<LayerType TLayer>
LineTool<TLayer>::~LineTool()
{
    // Mend our ephemeral visualization, if any
    if (mEphemeralVisualization.has_value())
    {
        mEphemeralVisualization.reset();

        mController.LayerChangeEpilog();
    }
}

template<LayerType TLayer>
void LineTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    // Assuming L/R button transitions already communicated

    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    // Do ephemeral visualization
    DoEphemeralVisualization(ScreenToShipSpace(mouseCoordinates));

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Foreground);

        assert(mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnLeftMouseUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Engage
    if (!mEngagementData)
    {
        StartEngagement(mouseCoordinates, MaterialPlaneType::Background);

        assert(mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnRightMouseUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseCoordinatesInShipSpace();

    // Disengage, eventually
    if (mEngagementData)
    {
        EndEngagement(mouseCoordinates);

        assert(!mEngagementData);
    }

    // Do ephemeral visualization
    DoEphemeralVisualization(mouseCoordinates);

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnShiftKeyDown()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    mIsShiftDown = true;

    // Do ephemeral visualization
    DoEphemeralVisualization(GetCurrentMouseCoordinatesInShipSpace());

    mController.LayerChangeEpilog();
}

template<LayerType TLayer>
void LineTool<TLayer>::OnShiftKeyUp()
{
    // Restore ephemeral visualization (if any)
    mEphemeralVisualization.reset();

    mIsShiftDown = false;

    // Do ephemeral visualization
    DoEphemeralVisualization(GetCurrentMouseCoordinatesInShipSpace());

    mController.LayerChangeEpilog();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void LineTool<TLayer>::StartEngagement(
    ShipSpaceCoordinates const & mouseCoordinates,
    MaterialPlaneType plane)
{
    assert(!mEngagementData);

    mEngagementData.emplace(
        mController.GetDirtyState(),
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
        // Create undo action
        //

        auto clippedLayerClone = mOriginalLayerClone.Clone(*resultantEffectiveRect);

        mController.StoreUndoAction(
            TLayer == LayerType::Structural ? _("Line Structural") : _("Line Electrical"),
            clippedLayerClone.Buffer.GetByteSize(),
            mEngagementData->OriginalDirtyState,
            [clippedLayerClone = std::move(clippedLayerClone), origin = resultantEffectiveRect->origin](Controller & controller) mutable
            {
                if constexpr (TLayer == LayerType::Structural)
                {
                    controller.RestoreStructuralLayerRegionForUndo(std::move(clippedLayerClone), origin);
                }
                else
                {
                    static_assert(TLayer == LayerType::Electrical);

                    controller.RestoreElectricalLayerRegionForUndo(std::move(clippedLayerClone), origin);
                }
            });

        // Epilog (if no applicable rect then we haven't changed anything, not even eph viz)
        mController.LayerChangeEpilog(TLayer);
    }

    //
    // Reset engagement
    //

    mEngagementData.reset();

    //
    // Re-take original layer clone
    //

    mOriginalLayerClone = mController.GetModel().CloneExistingLayer<TLayer>();
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

        mController.GetView().UploadDashedLineOverlay(
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
                        mController.GetModelController().RestoreStructuralLayerRegionForEphemeralVisualization(
                            mOriginalLayerClone,
                            *resultantEffectiveRect,
                            resultantEffectiveRect->origin);
                    }
                    else
                    {
                        static_assert(TLayer == LayerType::Electrical);

                        mController.GetModelController().RestoreElectricalLayerRegionForEphemeralVisualization(
                            mOriginalLayerClone,
                            *resultantEffectiveRect,
                            resultantEffectiveRect->origin);
                    }
                }

                mController.GetView().RemoveDashedLineOverlay();
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
            mController.GetView().UploadRectOverlay(
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
                            mController.GetModelController().RestoreStructuralLayerRegionForEphemeralVisualization(
                                mOriginalLayerClone,
                                *effectiveRect,
                                effectiveRect->origin);
                        }
                        else
                        {
                            static_assert(TLayer == LayerType::Electrical);

                            mController.GetModelController().RestoreElectricalLayerRegionForEphemeralVisualization(
                                mOriginalLayerClone,
                                *effectiveRect,
                                effectiveRect->origin);
                        }
                    }

                    mController.GetView().RemoveRectOverlay();
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
    // Apply SHIFT lock
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

    // Generate line
    if (TLayer == LayerType::Structural && mController.GetWorkbenchState().GetStructuralLineToolIsHullMode())
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
std::pair<std::optional<ShipSpaceRect>, typename LineTool<TLayer>::HasEdited> LineTool<TLayer>::TryFill(
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
                mController.GetModelController().StructuralRegionFillForEphemeralVisualization(
                    *affectedRect,
                    fillMaterial);
            }
            else
            {
                mController.GetModelController().StructuralRegionFill(
                    *affectedRect,
                    fillMaterial);
            }

            return std::make_pair(affectedRect, StrongTypedTrue<_HasEdited>);
        }
        else
        {
            static_assert(TLayer == LayerType::Electrical);

            assert(affectedRect->size == ShipSpaceSize(1, 1));
            if (mController.GetModelController().IsElectricalParticleAllowedAt(affectedRect->origin))
            {
                if constexpr (TIsForEphemeralVisualization)
                {
                    mController.GetModelController().ElectricalRegionFillForEphemeralVisualization(
                        *affectedRect,
                        fillMaterial);
                }
                else
                {
                    mController.GetModelController().ElectricalRegionFill(
                        *affectedRect,
                        fillMaterial);
                }

                return std::make_pair(affectedRect, StrongTypedTrue<_HasEdited>);
            }
        }
    }

    // Haven't filled
    return std::make_pair(affectedRect, StrongTypedFalse<_HasEdited>);
}

template<LayerType TLayer>
std::optional<ShipSpaceRect> LineTool<TLayer>::CalculateApplicableRect(ShipSpaceCoordinates const & coords) const
{
    // Anchor in the middle, and vertically from top

    int const lineSize = GetLineSize();
    int const topLeftLineSize =  (lineSize - 1) / 2;

    ShipSpaceCoordinates const origin = ShipSpaceCoordinates(coords.x, coords.y - (lineSize - 1));

    return ShipSpaceRect(origin - ShipSpaceSize(topLeftLineSize, -topLeftLineSize), { lineSize, lineSize })
        .MakeIntersectionWith({ { 0, 0 }, mController.GetModel().GetShipSize() });
}

template<LayerType TLayer>
int LineTool<TLayer>::GetLineSize() const
{
    if constexpr (TLayer == LayerType::Structural)
    {
        return static_cast<int>(mController.GetWorkbenchState().GetStructuralLineToolSize());
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
            return mController.GetWorkbenchState().GetStructuralForegroundMaterial();
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);

            return mController.GetWorkbenchState().GetStructuralBackgroundMaterial();
        }

    }
    else
    {
        static_assert(TLayer == LayerType::Electrical);

        if (plane == MaterialPlaneType::Foreground)
        {
            return mController.GetWorkbenchState().GetElectricalForegroundMaterial();
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);

            return mController.GetWorkbenchState().GetElectricalBackgroundMaterial();
        }
    }
}

}