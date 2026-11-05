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
    : SelectAndTranslateTool_SER(
        ToolType::StructuralSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

ElectricalSelectAndTranslateTool::ElectricalSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool_SER(
        ToolType::ElectricalSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

RopeSelectAndTranslateTool::RopeSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool_SER(
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
SelectAndTranslateTool_SER<TLayer>::SelectAndTranslateTool_SER(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool<TLayer, struct ShipSpaceTag>(
        toolType,
        controller,
        gameAssetManager)
{
    static_assert(TLayer == LayerType::Structural || TLayer == LayerType::Electrical || TLayer == LayerType::Ropes);
}

template<LayerType TLayer, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TCoordsTag>::SelectAndTranslateTool(
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

template<LayerType TLayer, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TCoordsTag>::~SelectAndTranslateTool()
{
    if (mEngagementData && mEngagementData->CurrentPastedSelectionRect)
    {
        StopSelectionViz();
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
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
            // Update selection, but don't commit it yet

            auto const selectionEndCorner = GetMouseSelectionCoordsClamped();
            UpdateSelectionViz(TRect(mEngagementData->SelectionStartCorner, selectionEndCorner));

            break;
        }

        case EngagementData::StateType::Selected:
        {
            //
            // Just update cursor
            //

            assert(mEngagementData->CurrentPastedSelectionRect.has_value());

            auto const mouseSpaceCoords = GetMouseSelectionCoords();
            if (mEngagementData->CurrentPastedSelectionRect->Contains(mouseSpaceCoords))
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
            //
            // Move selection
            //

            assert(mEngagementData->CurrentMoveStartCoords);

            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->CurrentMoveStartCoords;
            if (!totalMoveOffset.IsEmpty())
            {
                if (!mEngagementData->SelectionMoves)
                {
                    //
                    // First move for this selection
                    //

                    // Take snapshot of settings
                    bool isAllLayers;
                    bool isTransparent;
                    switch (TLayer)
                    {
                        case LayerType::Structural:
                        case LayerType::Electrical:
                        case LayerType::Ropes:
                        {
                            isAllLayers = mController.GetWorkbenchState().GetSERSelectAndTranslateToolIsAllLayers();
                            isTransparent = mController.GetWorkbenchState().GetSERSelectAndTranslateToolIsTransparent();
                            break;
                        }

                        case LayerType::ExteriorTexture:
                        case LayerType::InteriorTexture:
                        {
                            isAllLayers = mController.GetWorkbenchState().GetEISelectAndTranslateToolIsAllLayers();
                            isTransparent = mController.GetWorkbenchState().GetEISelectAndTranslateToolIsTransparent();
                            break;
                        }
                    }

                    // Start moves for this selection
                    assert(mEngagementData->CurrentPastedSelectionRect);
                    mEngagementData->SelectionMoves.emplace(
                        mEngagementData->CurrentPastedSelectionRect->origin, // New current paste position
                        isAllLayers,
                        isTransparent);

                    //
                    // Push Undo, and initialize ingredients for move
                    //

                    InternalInitSelectionMoves();
                }

                assert(mEngagementData->SelectionMoves);

                if (!mEngagementData->SelectionMoves->MovingState)
                {
                    mEngagementData->SelectionMoves->MovingState.emplace(mEngagementData->SelectionMoves->CurrentSelectionPasteCornerCoords);
                }

                assert(mEngagementData->SelectionMoves->MovingState);

                InternalDoMove(totalMoveOffset);
            }

            break;
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnLeftMouseDown()
{
    if (!mEngagementData)
    {
        //
        // Begin selection
        //

        auto const selectionStartCorner = GetMouseSelectionCoordsClamped();

        // Start selecting
        mEngagementData.emplace(selectionStartCorner);

        UpdateSelectionViz(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

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
            assert(mEngagementData->CurrentPastedSelectionRect.has_value());

            auto const clickSpaceCoords = GetMouseSelectionCoords();
            if (mEngagementData->CurrentPastedSelectionRect->Contains(clickSpaceCoords))
            {
                // Clicked inside selection: start move

                // Remember start coords for this move
                mEngagementData->CurrentMoveStartCoords = clickSpaceCoords;

                mEngagementData->State = EngagementData::StateType::Moving;

                InternalSetCursor(mMoveCursor);
            }
            else
            {
                // Clicked outside selection: back to square one from here

                // Start selecting
                mEngagementData.emplace(ClampSelectionCoords(clickSpaceCoords));

                UpdateSelectionViz(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

                mEngagementData->State = EngagementData::StateType::Selecting;

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

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnLeftMouseUp()
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
            auto const selectionEndCorner = GetMouseSelectionCoordsClamped();

            // Check if it's a null rect
            if (selectionEndCorner == mEngagementData->SelectionStartCorner)
            {
                // Null rect

                // Stop overlay
                StopSelectionViz();

                // Back to start state
                mEngagementData.reset();
            }
            else
            {
                // Finalize selection

                mEngagementData->OriginalSelectionRect.emplace(
                    mEngagementData->SelectionStartCorner,
                    selectionEndCorner);

                mEngagementData->CurrentPastedSelectionRect = mEngagementData->OriginalSelectionRect;

                // Update real origin
                mEngagementData->SelectionStartCorner = mEngagementData->OriginalSelectionRect->origin;

                UpdateSelectionViz(*mEngagementData->CurrentPastedSelectionRect);

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

            assert(mEngagementData->SelectionMoves);
            assert(mEngagementData->SelectionMoves->MovingState);

            // Remember last pasted corner, for next move
            mEngagementData->SelectionMoves->CurrentSelectionPasteCornerCoords = mEngagementData->SelectionMoves->MovingState->CurrentSelectionPasteCornerCoords;

            // Stop move
            mEngagementData->SelectionMoves->MovingState.reset();

            // Forget current move start coords
            assert(mEngagementData->CurrentMoveStartCoords);
            mEngagementData->CurrentMoveStartCoords.reset();

            mEngagementData->State = EngagementData::StateType::Selected;

            // Can leave cursor @ Move, pointer should still be on it

            break;
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData && mEngagementData->State == EngagementData::StateType::Moving)
    {
        if (mEngagementData->SelectionMoves && mEngagementData->SelectionMoves->MovingState)
        {
            // Restart constraints
            mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints.reset();

            // Refresh - and eventually reinstate constraints
            assert(mEngagementData->CurrentMoveStartCoords);
            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->CurrentMoveStartCoords;
            InternalDoMove(totalMoveOffset);
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData && mEngagementData->State == EngagementData::StateType::Moving)
    {
        if (mEngagementData->SelectionMoves && mEngagementData->SelectionMoves->MovingState)
        {
            // Restart constraints
            mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints.reset();

            // Refresh
            assert(mEngagementData->CurrentMoveStartCoords);
            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->CurrentMoveStartCoords;
            InternalDoMove(totalMoveOffset);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::InternalInitSelectionMoves()
{
    //
    // Create Undo with all interested layers
    //

    // TODOHERE
    LogMessage("InternalStartMove!");

    //
    // Prepare ingredients
    //

    assert(mEngagementData->SelectionMoves);

    // TODOHERE: store stuff @ SelectionMoves
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::InternalDoMove(TSize totalMoveOffset)
{
    assert(mEngagementData->SelectionMoves);
    assert(mEngagementData->SelectionMoves->MovingState);

    //
    // Determine SHIFT constraints, if needed
    //

    if (!mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints)
    {
        if (mIsShiftDown)
        {
            int constexpr MinOffsetForDeduction = 2;

            if (std::fabsf(totalMoveOffset.width) >= std::fabsf(totalMoveOffset.height) + MinOffsetForDeduction)
            {
                mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints.emplace(1, 0);
            }
            else if (std::fabsf(totalMoveOffset.height) >= std::fabsf(totalMoveOffset.width) + MinOffsetForDeduction)
            {
                mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints.emplace(0, 1);
            }
        }
    }

    //
    // Apply SHIFT constraints to trajectory
    //

    if (mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints)
    {
        totalMoveOffset.width *= mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints->width;
        totalMoveOffset.height *= mEngagementData->SelectionMoves->MovingState->MoveShiftConstraints->height;
    }

    //
    // Clamp move offset so that selection rect stays in ship area
    //

    auto const spaceSize = GetSpaceSize();

    assert(mEngagementData->CurrentPastedSelectionRect);

    totalMoveOffset.width = Clamp(
        totalMoveOffset.width,
        -(mEngagementData->SelectionStartCorner.x + mEngagementData->CurrentPastedSelectionRect->size.width),
        spaceSize.width - mEngagementData->SelectionStartCorner.x);

    totalMoveOffset.height = Clamp(
        totalMoveOffset.height,
        -(mEngagementData->SelectionStartCorner.y + mEngagementData->CurrentPastedSelectionRect->size.height),
        spaceSize.height - mEngagementData->SelectionStartCorner.y);

    //
    // Move selected area
    //

    TCoords const newPasteOrigin = mEngagementData->SelectionMoves->CurrentSelectionPasteCornerCoords + totalMoveOffset;

    // TODOHERE

    // Update current move's paste pos
    mEngagementData->SelectionMoves->MovingState->CurrentSelectionPasteCornerCoords = newPasteOrigin;

    //
    // Move selection overlay
    //

    assert(mEngagementData->CurrentPastedSelectionRect);
    mEngagementData->CurrentPastedSelectionRect->origin = newPasteOrigin;
    UpdateSelectionViz(*mEngagementData->CurrentPastedSelectionRect);
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::UpdateSelectionViz(TRect const & rect)
{
    // Update overlay

    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        mController.GetView().UploadDashedRectangleOverlay(rect.MinMin(), rect.MaxMax());
    }
    else if constexpr (TLayer == LayerType::ExteriorTexture)
    {
        mController.GetView().UploadDashedRectangleOverlay_Exterior(rect.MinMin(), rect.MaxMax());
    }
    else
    {
        static_assert(TLayer == LayerType::InteriorTexture);
        // TODO: not supported yet
        //mController.GetView().UploadDashedRectangleOverlay_Interior(rect.MinMin(), rect.MaxMax());
    }

    mController.GetUserInterface().RefreshView();

    // Update measurement

    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(
        TSize(
            std::abs(rect.size.width),
            std::abs(rect.size.height)));
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::StopSelectionViz()
{
    // Remove overlay
    mController.GetView().RemoveDashedRectangleOverlay();
    mController.GetUserInterface().RefreshView();

    // Remove measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged();
}

template<LayerType TLayer, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TCoordsTag>::GetMouseSelectionCoords() const
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

template<LayerType TLayer, typename TCoordsTag>
std::optional<typename SelectAndTranslateTool<TLayer, TCoordsTag>::TCoords> SelectAndTranslateTool<TLayer, TCoordsTag>::GetMouseSelectionCoordsIfInSpace() const
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

template<LayerType TLayer, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TCoordsTag>::GetMouseSelectionCoordsClamped() const
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

template<LayerType TLayer, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TCoordsTag>::TCoords SelectAndTranslateTool<TLayer, TCoordsTag>::ClampSelectionCoords(TCoords const & coords) const
{
    auto const spaceSize = GetSpaceSize();
    return coords.Clamp(spaceSize);
}

template<LayerType TLayer>
ShipSpaceSize SelectAndTranslateTool_SER<TLayer>::GetSpaceSize() const
{
    return this->mController.GetModelController().GetShipSize();
}

ImageSize ExteriorSelectAndTranslateTool::GetSpaceSize() const
{
    return mController.GetModelController().GetExteriorTextureSize();
}

ImageSize InteriorSelectAndTranslateTool::GetSpaceSize() const
{
    return mController.GetModelController().GetInteriorTextureSize();
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::InternalSetCursor(wxImage const & cursorImage)
{
    if (mCurrentCursor != &cursorImage)
    {
        SetCursor(cursorImage);
        mCurrentCursor = &cursorImage;
    }
}
}