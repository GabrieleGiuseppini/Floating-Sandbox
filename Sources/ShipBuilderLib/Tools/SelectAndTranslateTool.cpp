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
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::StructuralSelectAndTranslate,
        isAllLayers,
        isTransparent,
        controller,
        gameAssetManager)
{
}

ElectricalSelectAndTranslateTool::ElectricalSelectAndTranslateTool(
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::ElectricalSelectAndTranslate,
        isAllLayers,
        isTransparent,
        controller,
        gameAssetManager)
{
}

RopeSelectAndTranslateTool::RopeSelectAndTranslateTool(
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::RopeSelectAndTranslate,
        isAllLayers,
        isTransparent,
        controller,
        gameAssetManager)
{
}

ExteriorSelectAndTranslateTool::ExteriorSelectAndTranslateTool(
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::ExteriorSelectAndTranslate,
        isAllLayers,
        isTransparent,
        controller,
        gameAssetManager)
{
}

InteriorSelectAndTranslateTool::InteriorSelectAndTranslateTool(
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::InteriorSelectAndTranslate,
        isAllLayers,
        isTransparent,
        controller,
        gameAssetManager)
{
}


template<LayerType TLayer, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TCoordsTag>::SelectAndTranslateTool(
    ToolType toolType,
    bool isAllLayers,
    bool isTransparent,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
    // State
    , mEngagementData()
    , mIsShiftDown(false)
    , mIsAllLayers(isAllLayers)
    , mIsTransparent(isTransparent)
    // UI
    , mPointerCursor(WxHelpers::LoadCursorImage("selection_cursor", 11, 11, gameAssetManager))
    , mMoveCursor(WxHelpers::LoadCursorImage("pan_cursor", 15, 15, gameAssetManager))
{
    InternalSetCursor(mPointerCursor);
}

template<LayerType TLayer, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TCoordsTag>::~SelectAndTranslateTool()
{
    if (mEngagementData && mEngagementData->SelectionRect)
    {
        StopEphemeralSelection();
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
            UpdateEphemeralSelection(TRect(mEngagementData->SelectionStartCorner, selectionEndCorner));

            break;
        }

        case EngagementData::StateType::Selected:
        {
            //
            // Just update cursor
            //

            assert(mEngagementData->SelectionRect.has_value());

            auto const mouseSpaceCoords = GetMouseSelectionCoordsIfInSpace();
            if (mouseSpaceCoords && mEngagementData->SelectionRect->Contains(*mouseSpaceCoords))
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

            assert(mEngagementData->MoveStartCoords);

            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->MoveStartCoords;
            if (!totalMoveOffset.IsEmpty())
            {
                if (!mEngagementData->HasStartedMove)
                {
                    //
                    // First of move - push Undo, remove selection from Model, and take Background and Selected areas
                    //

                    InternalStartMove();
                }

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

        UpdateEphemeralSelection(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

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
            assert(mEngagementData->SelectionRect.has_value());

            auto const clickSpaceCoords = GetMouseSelectionCoordsIfInSpace();
            if (clickSpaceCoords)
            {
                if (mEngagementData->SelectionRect->Contains(*clickSpaceCoords))
                {
                    // Clicked inside selection: start moving

                    mEngagementData->MoveStartCoords = *clickSpaceCoords;
                    mEngagementData->HasStartedMove = false;
                    mEngagementData->MoveShiftConstraints.reset();

                    mEngagementData->State = EngagementData::StateType::Moving;

                    InternalSetCursor(mMoveCursor);
                }
                else
                {
                    // Clicked outside selection: back to square one from here

                    // Start selecting
                    mEngagementData.emplace(*clickSpaceCoords);

                    UpdateEphemeralSelection(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

                    mEngagementData->State = EngagementData::StateType::Selecting;

                    InternalSetCursor(mPointerCursor);
                }
            }
            else
            {
                // Outside of ship - back to square one from nothing

                auto const newSelectionStartCorner = GetMouseSelectionCoordsClamped();

                // Start selecting
                mEngagementData.emplace(newSelectionStartCorner);

                UpdateEphemeralSelection(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

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
                StopEphemeralSelection();

                // Back to start state
                mEngagementData.reset();
            }
            else
            {
                // Finalize selection

                mEngagementData->SelectionRect.emplace(
                    mEngagementData->SelectionStartCorner,
                    selectionEndCorner);

                // Set to real origin
                mEngagementData->SelectionStartCorner = mEngagementData->SelectionRect->origin;

                UpdateEphemeralSelection(*mEngagementData->SelectionRect);

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

            mEngagementData->MoveStartCoords.reset();
            mEngagementData->State = EngagementData::StateType::Selected;

            // Finalize selection start coords to new starting point - which has been translating during move
            assert(mEngagementData->SelectionRect);
            mEngagementData->SelectionStartCorner = mEngagementData->SelectionRect->origin;

            // Can leave cursor @ Move, pointer should still be on it

            break;
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        // Restart constraints
        mEngagementData->MoveShiftConstraints.reset();

        if (mEngagementData->State == EngagementData::StateType::Moving)
        {
            // Refresh - and eventually reinstate constraints
            assert(mEngagementData->MoveStartCoords);
            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->MoveStartCoords;
            InternalDoMove(totalMoveOffset);
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        // Stop constraints
        mEngagementData->MoveShiftConstraints.reset();

        if (mEngagementData->State == EngagementData::StateType::Moving)
        {
            // Refresh
            assert(mEngagementData->MoveStartCoords);
            TSize const totalMoveOffset = GetMouseSelectionCoords() - *mEngagementData->MoveStartCoords;
            InternalDoMove(totalMoveOffset);
        }
    }
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::SetIsAllLayers(bool value)
{
    mIsAllLayers = value;

    // TODOHERE: react iff we have a LastMoveDestination
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::SetIsTransparent(bool value)
{
    mIsTransparent = value;

    // TODOHERE: react iff we have a LastMoveDestination
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::InternalStartMove()
{
    //
    // Create Undo with all interested layers
    //

    // TODOHERE





    mEngagementData->HasStartedMove = true;
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::InternalDoMove(TSize totalMoveOffset)
{
    //
    // Determine SHIFT constraints, if needed
    //

    if (!mEngagementData->MoveShiftConstraints)
    {
        if (mIsShiftDown)
        {
            int constexpr MinOffsetForDeduction = 2;

            if (std::fabsf(totalMoveOffset.width) >= std::fabsf(totalMoveOffset.height) + MinOffsetForDeduction)
            {
                mEngagementData->MoveShiftConstraints.emplace(1, 0);
            }
            else if (std::fabsf(totalMoveOffset.height) >= std::fabsf(totalMoveOffset.width) + MinOffsetForDeduction)
            {
                mEngagementData->MoveShiftConstraints.emplace(0, 1);
            }
        }
    }

    //
    // Apply SHIFT constraints to trajectory
    //

    if (mEngagementData->MoveShiftConstraints)
    {
        totalMoveOffset.width *= mEngagementData->MoveShiftConstraints->width;
        totalMoveOffset.height *= mEngagementData->MoveShiftConstraints->height;
    }

    //
    // Clamp move offset so that selection rect stays in ship area
    //

    auto const spaceSize = GetSpaceSize();

    assert(mEngagementData->SelectionRect);

    totalMoveOffset.width = Clamp(
        totalMoveOffset.width,
        -(mEngagementData->SelectionStartCorner.x + mEngagementData->SelectionRect->size.width),
        spaceSize.width - mEngagementData->SelectionStartCorner.x);

    totalMoveOffset.height = Clamp(
        totalMoveOffset.height,
        -(mEngagementData->SelectionStartCorner.y + mEngagementData->SelectionRect->size.height),
        spaceSize.height - mEngagementData->SelectionStartCorner.y);

    //
    // Move selected area
    //

    // TODOHERE

    //
    // Move selection overlay
    //

    assert(mEngagementData->SelectionRect);
    mEngagementData->SelectionRect->origin = mEngagementData->SelectionStartCorner + totalMoveOffset;
    UpdateEphemeralSelection(*mEngagementData->SelectionRect);
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::UpdateEphemeralSelection(TRect const & rect)
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
void SelectAndTranslateTool<TLayer, TCoordsTag>::StopEphemeralSelection()
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
    auto const spaceSize = GetSpaceSize();

    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(mouseCoordinates);
        return mouseShipCoordinates.Clamp(spaceSize);
    }
    else
    {
        static_assert(TLayer == LayerType::ExteriorTexture || TLayer == LayerType::InteriorTexture);

        ImageCoordinates const mouseImageCoordinates = ScreenToTextureSpace(TLayer, mouseCoordinates);
        return mouseImageCoordinates.Clamp(spaceSize);
    }
}

template<LayerType TLayer, typename TCoordsTag>
typename SelectAndTranslateTool<TLayer, TCoordsTag>::TSize SelectAndTranslateTool<TLayer, TCoordsTag>::GetSpaceSize() const
{
    if constexpr (TLayer == LayerType::Structural
        || TLayer == LayerType::Electrical
        || TLayer == LayerType::Ropes)
    {
        return mController.GetModelController().GetShipSize();
    }
    else if constexpr (TLayer == LayerType::ExteriorTexture)
    {
        return mController.GetModelController().GetExteriorTextureSize();
    }
    else
    {
        static_assert(TLayer == LayerType::InteriorTexture);

        return mController.GetModelController().GetInteriorTextureSize();
    }
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