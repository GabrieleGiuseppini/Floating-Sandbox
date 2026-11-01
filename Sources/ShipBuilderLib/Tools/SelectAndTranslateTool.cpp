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
    : SelectAndTranslateTool(
        ToolType::StructuralSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

ElectricalSelectAndTranslateTool::ElectricalSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
        ToolType::ElectricalSelectAndTranslate,
        controller,
        gameAssetManager)
{
}

RopeSelectAndTranslateTool::RopeSelectAndTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : SelectAndTranslateTool(
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

            // TODOTEST
            LogMessage("MouseMove@Selecting: Layer=", int(TLayer), " SelectionEnd=", selectionEndCorner);

            break;
        }

        case EngagementData::StateType::Selected:
        {
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
            // TODO
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

        // TODOTEST
        LogMessage("MouseDown@None: Layer=", int(TLayer), " SelectionStart=", mEngagementData->SelectionStartCorner);

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

                    // TODOHERE
                }
                else
                {
                    // Clicked outside selection: back to square one from here

                    // Start selecting
                    mEngagementData.emplace(*clickSpaceCoords);

                    UpdateEphemeralSelection(TRect(mEngagementData->SelectionStartCorner, mEngagementData->SelectionStartCorner));

                    // TODOTEST
                    LogMessage("MouseDown@Selected/Outside: Layer=", int(TLayer), " SelectionStart=", mEngagementData->SelectionStartCorner);

                    mEngagementData->State = EngagementData::StateType::Selecting;

                    InternalSetCursor(mPointerCursor);
                }
            }
            else
            {
                // Outside of ship - back to square one from nothing

                mEngagementData.reset();
                StopEphemeralSelection();

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

    // TODO
    //assert(!mEngagementData);

    //auto const textureCoordinates = ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates());

    //// Engage at start position
    //if constexpr (TLayer == LayerType::ExteriorTexture)
    //{
    //    mEngagementData.emplace(
    //        textureCoordinates,
    //        mController.GetModelController().CloneExteriorTextureLayer());
    //}
    //else
    //{
    //    static_assert(TLayer == LayerType::InteriorTexture);

    //    mEngagementData.emplace(
    //        textureCoordinates,
    //        mController.GetModelController().CloneInteriorTextureLayer());
    //}
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

                UpdateEphemeralSelection(*mEngagementData->SelectionRect);

                // TODOTEST
                LogMessage("MouseUp@Selecting: Layer=", int(TLayer), " SelectionRect=", *mEngagementData->SelectionRect);

                mEngagementData->State = EngagementData::StateType::Selected;
            }

            break;
        }

        case EngagementData::StateType::Selected:
        {
            // TODO
            break;
        }

        case EngagementData::StateType::Moving:
        {
            // TODO
            break;
        }
    }

    // TODO
    //if (mEngagementData)
    //{
    //    // Disengage
    //    mEngagementData.reset();
    //}
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    // TODO
    //if (mEngagementData)
    //{
    //    if (mEngagementData->IsDirty) // Prevent dirtying if nothing has ever changed (i.e. if mouse hasn't moved)
    //    {
    //        DoTranslate(ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()));
    //    }
    //}
}

template<LayerType TLayer, typename TCoordsTag>
void SelectAndTranslateTool<TLayer, TCoordsTag>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    // TODO
    //if (mEngagementData)
    //{
    //    if (mEngagementData->IsDirty) // Prevent dirtying if nothing has ever changed (i.e. if mouse hasn't moved)
    //    {
    //        DoTranslate(ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()));
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

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