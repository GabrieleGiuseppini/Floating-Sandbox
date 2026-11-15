/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-11-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SelectAndTranslateTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

#include <cassert>

namespace ShipBuilder {

StructuralSelectAndTranslateTool::StructuralSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool_ShipSpace(
        ToolType::StructuralSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

ElectricalSelectAndTranslateTool::ElectricalSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool_ShipSpace(
        ToolType::ElectricalSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

RopeSelectAndTranslateTool::RopeSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool_ShipSpace(
        ToolType::RopeSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

ExteriorSelectAndTranslateTool::ExteriorSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::ExteriorSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

InteriorSelectAndTranslateTool::InteriorSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::InteriorSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

template<LayerType TLayer>
SelectAndTranslateTool_ShipSpace<TLayer>::SelectAndTranslateTool_ShipSpace(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool<TLayer, GenericShipSpaceUndoPayload, GenericShipSpaceEphemeralVisualizationRestorePayload, ShipSpaceTag>(
        toolType,
        controller,
        gameAssetManager)
{
    static_assert(TLayer == LayerType::Structural || TLayer == LayerType::Electrical || TLayer == LayerType::Ropes);
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::SelectAndTranslateTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
    // State
    , mEngagementData()
    , mIsShiftDown(false)
    // UI
    , mPointerCursor(WxHelpers::LoadCursorImage("selection_cursor", 11, 11, gameAssetManager))
    , mMoveCursor(WxHelpers::LoadCursorImage("pan_cursor", 15, 15, gameAssetManager))
{
    InternalSetCursor(mPointerCursor);
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnDestroy()
{
    if (mEngagementData)
    {
        // Remove selection viz
        StopSelectionViz();

        // Finalize
        // - Will undo eph viz if any
        Commit();
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
{
    if (!mEngagementData)
    {
        // Nop
        return;
    }

    switch (mEngagementData->State)
    {
        case EngagementData::StateType::Selecting:
        {
            // Update selection

            auto const selectionEndCorner = GetMouseSelectionCoordsClamped();
            mEngagementData->SourceRect = TRect(mEngagementData->SelectionStartCorner, selectionEndCorner);
            mEngagementData->PasteRect = mEngagementData->SourceRect;

            UpdateSelectionViz();
            mController.GetUserInterface().RefreshView();

            break;
        }

        case EngagementData::StateType::Selected:
        {
            //
            // Just update cursor
            //

            auto const mouseSpaceCoords = GetMouseSelectionCoords();
            if (mEngagementData->PasteRect.Contains(mouseSpaceCoords))
            {
                // Moved inside selection: show moving
                InternalSetCursor(mMoveCursor);
            }
            else
            {
                // Moved outside selection: show selection
                InternalSetCursor(mPointerCursor);
            }

            break;
        }

        case EngagementData::StateType::Moving:
        {
            assert(mEngagementData->CurrentMoveState);

            //
            // Move selection
            //

            InternalDoMoveForEphemeralVisualization(GetMouseSelectionCoords());

            break;
        }
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnLeftMouseDown()
{
    auto const clickSpaceCoords = GetMouseSelectionCoords();

    if (!mEngagementData)
    {
        //
        // Begin selection
        //

        // Start selecting
        // - Changes state -> Selecting
        mEngagementData.emplace(
            ClampSelectionCoords(clickSpaceCoords),
            MakeLayerSelection(),
            MakeIsTransparent());

        UpdateSelectionViz();
        mController.GetUserInterface().RefreshView();

        return;
    }

    switch (mEngagementData->State)
    {
        case EngagementData::StateType::Selecting:
        {
            // Unexpected, nop
            assert(false);
            break;
        }

        case EngagementData::StateType::Selected:
        {
            if (mEngagementData->PasteRect.Contains(clickSpaceCoords))
            {
                // Clicked inside selection: start move

                assert(!mEngagementData->CurrentMoveState);
                mEngagementData->CurrentMoveState.emplace(clickSpaceCoords, mEngagementData->PasteRect.origin);

                mEngagementData->State = EngagementData::StateType::Moving;

                InternalSetCursor(mMoveCursor);
            }
            else
            {
                // Clicked outside selection: back to square one from here

                // Commit (if needs to do so)
                // - Stops eph viz (if any)
                Commit();

                // Start selecting
                // - Changes state -> Selecting
                mEngagementData.emplace(
                    ClampSelectionCoords(clickSpaceCoords),
                    MakeLayerSelection(),
                    MakeIsTransparent());

                UpdateSelectionViz();
                mController.GetUserInterface().RefreshView();

                InternalSetCursor(mPointerCursor);
            }

            break;
        }

        case EngagementData::StateType::Moving:
        {
            // Unexpected, nop
            assert(false);
            break;
        }
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnLeftMouseUp()
{
    if (!mEngagementData)
    {
        // Nop
        return;
    }

    switch (mEngagementData->State)
    {
        case EngagementData::StateType::Selecting:
        {
            assert(!mEphemeralVisualization);

            auto const selectionEndCorner = GetMouseSelectionCoordsClamped();

            // Check if it's a null rect
            if (selectionEndCorner == mEngagementData->SelectionStartCorner)
            {
                // Null rect

                // Stop overlay
                StopSelectionViz();
                mController.GetUserInterface().RefreshView();

                // Back to start state
                mEngagementData.reset();
            }
            else
            {
                // Move to Selected
                mEngagementData->State = EngagementData::StateType::Selected;
            }

            break;
        }

        case EngagementData::StateType::Selected:
        {
            // Unexpected, nop
            assert(false);
            break;
        }

        case EngagementData::StateType::Moving:
        {
            // Stop moving, back to Selected

            // Stop move
            assert(mEngagementData->CurrentMoveState);
            mEngagementData->CurrentMoveState.reset();

            mEngagementData->State = EngagementData::StateType::Selected;

            // Can leave cursor @ Move, pointer should still be on it

            break;
        }
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData && mEngagementData->State == EngagementData::StateType::Moving)
    {
        assert(mEngagementData->CurrentMoveState);

        // Restart constraints
        mEngagementData->CurrentMoveState->MoveShiftConstraints.reset();

        // Refresh - and eventually reinstate constraints
        InternalDoMoveForEphemeralVisualization(GetMouseSelectionCoords());
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData && mEngagementData->State == EngagementData::StateType::Moving)
    {
        assert(mEngagementData->CurrentMoveState);

        // Restart constraints
        mEngagementData->CurrentMoveState->MoveShiftConstraints.reset();

        // Refresh
        InternalDoMoveForEphemeralVisualization(GetMouseSelectionCoords());
    }
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::InternalDoMoveForEphemeralVisualization(TCoords const & newPasteCoords)
{
    assert(mEngagementData);
    assert(mEngagementData->CurrentMoveState);

    //
    // Calculate move displacement, in space
    //
    // Paste rect will end up origined to SelectionRectStartOrigin + displacement
    //

    TSize moveDisplacement = newPasteCoords - mEngagementData->CurrentMoveState->MoveStartCoords;

    //
    // Determine SHIFT constraints, if needed
    //

    if (!mEngagementData->CurrentMoveState->MoveShiftConstraints)
    {
        if (mIsShiftDown)
        {
            int constexpr MinOffsetForDeduction = 2;

            if (std::fabsf(moveDisplacement.width) >= std::fabsf(moveDisplacement.height) + MinOffsetForDeduction)
            {
                mEngagementData->CurrentMoveState->MoveShiftConstraints.emplace(1, 0);
            }
            else if (std::fabsf(moveDisplacement.height) >= std::fabsf(moveDisplacement.width) + MinOffsetForDeduction)
            {
                mEngagementData->CurrentMoveState->MoveShiftConstraints.emplace(0, 1);
            }
        }
    }

    //
    // Apply SHIFT constraints to trajectory
    //

    if (mEngagementData->CurrentMoveState->MoveShiftConstraints)
    {
        moveDisplacement.width *= mEngagementData->CurrentMoveState->MoveShiftConstraints->width;
        moveDisplacement.height *= mEngagementData->CurrentMoveState->MoveShiftConstraints->height;
    }

    //
    // Calculate moved paste rect origin
    //

    TCoords movedPasteRectOrigin = mEngagementData->CurrentMoveState->SelectionRectStartOrigin + moveDisplacement;

    // Clamp origin so that paste rect stays in ship area

    auto const spaceSize = GetSpaceSize();

    movedPasteRectOrigin.x = Clamp(
        movedPasteRectOrigin.x,
        -mEngagementData->SourceRect.size.width,
        spaceSize.width);
    movedPasteRectOrigin.y = Clamp(
        movedPasteRectOrigin.y,
        -mEngagementData->SourceRect.size.height,
        spaceSize.height);

    //
    // Check if change wrt current - so to save on refreshes
    //

    if (movedPasteRectOrigin != mEngagementData->PasteRect.origin)
    {
        //
        // Undo ephemeral viz, if any
        //

        mEphemeralVisualization.reset();

        //
        // Move paste rect
        //

        mEngagementData->PasteRect = TRect(movedPasteRectOrigin, mEngagementData->SourceRect.size);

        //
        // Draw ephemeral visualization
        //

        DrawEphemeralVisualization();

        //
        // Draw selection overlay
        //

        UpdateSelectionViz();


        mController.LayerChangeEpilog();
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::Commit()
{
    assert(mEngagementData);

    //
    // Undo ephemeral viz, if any
    //

    mEphemeralVisualization.reset();

    //
    // Transfer
    //

    LayerSelector layerSelection;

    // Avoid creating undo for no-op transfer
    if (mEngagementData->PasteRect.origin != mEngagementData->SourceRect.origin
        && !mEngagementData->SourceRect.IsEmpty())
    {
        layerSelection = mEngagementData->LayerSelection;

        auto undoPayload = InternalCommit(
            mEngagementData->SourceRect,
            mEngagementData->PasteRect.origin,
            layerSelection,
            mEngagementData->IsTransparent);

        // Store undo

        size_t const undoPayloadCost = undoPayload.GetTotalCost();

        mController.StoreUndoAction(
            _("Translate"),
            undoPayloadCost,
            mController.GetModelController().GetDirtyState(),
            [undoPayload = std::move(undoPayload)](Controller & controller) mutable
            {
                controller.Restore(std::move(undoPayload));
            });
    }

    // Finalize

    mController.LayerChangeEpilog(layerSelection);
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::DrawEphemeralVisualization()
{
    // Assumption: NOT in eph viz
    assert(!mEphemeralVisualization);

    assert(mEngagementData);

    auto payload = std::make_shared<TEphemeralVisualizationRestorePayload>(
        InternalDrawEphemeralVisualization(
            mEngagementData->SourceRect,
            mEngagementData->PasteRect.origin,
            mEngagementData->LayerSelection,
            mEngagementData->IsTransparent));

    // Schedule cleanup
    mEphemeralVisualization.emplace(
        [this, payload = std::move(payload)]() mutable
        {
            mController.GetModelController().RestoreEphemeralVisualization(std::move(*payload));
        });
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::UpdateSelectionViz()
{
    assert(mEngagementData);

    if (!mEngagementData->PasteRect.IsEmpty())
    {
        // Update overlay

        if constexpr (TLayer == LayerType::Structural
            || TLayer == LayerType::Electrical
            || TLayer == LayerType::Ropes)
        {
            mController.GetView().UploadDashedRectangleOverlay(mEngagementData->PasteRect.MinMin(), mEngagementData->PasteRect.MaxMax());
        }
        else if constexpr (TLayer == LayerType::ExteriorTexture)
        {
            mController.GetView().UploadDashedRectangleOverlay_Exterior(mEngagementData->PasteRect.MinMin(), mEngagementData->PasteRect.MaxMax());
        }
        else
        {
            static_assert(TLayer == LayerType::InteriorTexture);
            // TODO: interior layer not supported yet
            //mController.GetView().UploadDashedRectangleOverlay_Interior(mEngagementData->PasteRect.MinMin(), mEngagementData->PasteRect.MaxMax());
        }
    }

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(
        TSize(
            std::abs(mEngagementData->PasteRect.size.width),
            std::abs(mEngagementData->PasteRect.size.height)));
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::StopSelectionViz()
{
    // Remove overlay
    mController.GetView().RemoveDashedRectangleOverlay();

    // Remove measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged();
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::GetMouseSelectionCoords() const
{
    auto const mouseCoordinates = GetCurrentMouseCoordinates();
    auto const spaceSize = GetSpaceSize();

    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        return ScreenToShipSpaceNearest(mouseCoordinates);
    }
    else
    {
        static_assert(TLayer == LayerType::ExteriorTexture || TLayer == LayerType::InteriorTexture);

        return ScreenToTextureSpace(TLayer, mouseCoordinates);
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
std::optional<typename SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::TCoords> SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::GetMouseSelectionCoordsIfInSpace() const
{
    auto const mouseCoordinates = GetCurrentMouseCoordinates();
    auto const spaceSize = GetSpaceSize();

    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(mouseCoordinates);
        if (mouseShipCoordinates.IsInRect(
            ShipSpaceRect(
                ShipSpaceCoordinates(0, 0),
                ShipSpaceSize(spaceSize.width + 1, spaceSize.height + 1))))
        {
            return mouseShipCoordinates;
        }
        else
        {
            return std::nullopt;
        }
    }
    else
    {
        static_assert(TLayer == LayerType::ExteriorTexture || TLayer == LayerType::InteriorTexture);

        ImageCoordinates const mouseImageCoordinates = ScreenToTextureSpace(TLayer, mouseCoordinates);
        if (mouseImageCoordinates.IsInRect(
            ImageRect(
                ImageCoordinates(0, 0),
                ImageSize(spaceSize.width + 1, spaceSize.height + 1))))
        {
            return mouseImageCoordinates;
        }
        else
        {
            return std::nullopt;
        }
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::GetMouseSelectionCoordsClamped() const
{
    auto const mouseCoordinates = GetCurrentMouseCoordinates();

    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(mouseCoordinates);
        return ClampSelectionCoords(mouseShipCoordinates);
    }
    else
    {
        static_assert(TLayer == LayerType::ExteriorTexture || TLayer == LayerType::InteriorTexture);

        ImageCoordinates const mouseImageCoordinates = ScreenToTextureSpace(TLayer, mouseCoordinates);
        return ClampSelectionCoords(mouseImageCoordinates);
    }
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TUndoPayload, TEphemeralVisualizationRestorePayload, TCoordsTag>::ClampSelectionCoords(TCoords const & coords) const
{
    auto const spaceSize = GetSpaceSize();
    return coords.Clamp(spaceSize);
}

template<LayerType TLayer>
GenericShipSpaceUndoPayload SelectAndTranslateTool_ShipSpace<TLayer>::InternalCommit(
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().ShipSpaceRegionTransfer(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

template<LayerType TLayer>
GenericShipSpaceEphemeralVisualizationRestorePayload SelectAndTranslateTool_ShipSpace<TLayer>::InternalDrawEphemeralVisualization(
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().ShipSpaceRegionTransferForEphemeralVisualization(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

template<LayerType TLayer>
LayerSelector SelectAndTranslateTool_ShipSpace<TLayer>::MakeLayerSelection() const
{
    return this->mController.GetWorkbenchState().GetShipSpaceSelectAndTranslateToolIsAllLayers() ?
        this->mController.GetModelController().GetAllPresentLayers()
        : LayerSelector({TLayer});
}

template<LayerType TLayer>
bool SelectAndTranslateTool_ShipSpace<TLayer>::MakeIsTransparent() const
{
    return this->mController.GetWorkbenchState().GetShipSpaceSelectAndTranslateToolIsTransparent();
}

template<LayerType TLayer>
ShipSpaceSize SelectAndTranslateTool_ShipSpace<TLayer>::GetSpaceSize() const
{
    return this->mController.GetModelController().GetShipSize();
}

GenericExteriorTextureSpaceUndoPayload ExteriorSelectAndTranslateTool::InternalCommit(
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().ExteriorTextureSpaceRegionTransfer(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload ExteriorSelectAndTranslateTool::InternalDrawEphemeralVisualization(
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().ExteriorTextureSpaceRegionTransferForEphemeralVisualization(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

LayerSelector ExteriorSelectAndTranslateTool::MakeLayerSelection() const
{
    auto layers = LayerSelector({ LayerType::ExteriorTexture });
    if (this->mController.GetWorkbenchState().GetTextureSpaceSelectAndTranslateToolIsAllLayers()
        && this->mController.GetModelController().HasLayer(LayerType::InteriorTexture))
    {
        layers.Add(LayerType::InteriorTexture);
    }

    return layers;
}

bool ExteriorSelectAndTranslateTool::MakeIsTransparent() const
{
    return this->mController.GetWorkbenchState().GetTextureSpaceSelectAndTranslateToolIsTransparent();
}

ImageSize ExteriorSelectAndTranslateTool::GetSpaceSize() const
{
    return mController.GetModelController().GetExteriorTextureSize();
}

GenericInteriorTextureSpaceUndoPayload InteriorSelectAndTranslateTool::InternalCommit(
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().InteriorTextureSpaceRegionTransfer(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload InteriorSelectAndTranslateTool::InternalDrawEphemeralVisualization(
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    return this->mController.GetModelController().InteriorTextureSpaceRegionTransferForEphemeralVisualization(
        sourceRegion,
        targetPos,
        layerSelection,
        isTransparent);
}

LayerSelector InteriorSelectAndTranslateTool::MakeLayerSelection() const
{
    auto layers = LayerSelector({ LayerType::InteriorTexture });
    if (this->mController.GetWorkbenchState().GetTextureSpaceSelectAndTranslateToolIsAllLayers()
        && this->mController.GetModelController().HasLayer(LayerType::ExteriorTexture))
    {
        layers.Add(LayerType::ExteriorTexture);
    }

    return layers;
}

bool InteriorSelectAndTranslateTool::MakeIsTransparent() const
{
    return this->mController.GetWorkbenchState().GetTextureSpaceSelectAndTranslateToolIsTransparent();
}

ImageSize InteriorSelectAndTranslateTool::GetSpaceSize() const
{
    return mController.GetModelController().GetInteriorTextureSize();
}

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TUndoPayload,TEphemeralVisualizationRestorePayload, TCoordsTag>::InternalSetCursor(wxImage const & cursorImage)
{
    if (mCurrentCursor != &cursorImage)
    {
        SetCursor(cursorImage);
        mCurrentCursor = &cursorImage;
    }
}

}