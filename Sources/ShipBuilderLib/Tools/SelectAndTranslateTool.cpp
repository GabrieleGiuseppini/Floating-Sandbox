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
    SetCursor(mPointerCursor);
}

template<LayerType TLayer, typename TCoordsTag>
SelectAndTranslateTool<TLayer, TCoordsTag>::~SelectAndTranslateTool()
{
    if (mEngagementData && mEngagementData->SelectionRect)
    {
        // Remove overlay
        mController.GetView().RemoveDashedRectangleOverlay();
        mController.GetUserInterface().RefreshView();
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
            // TODOHERE
            //UpdateEphemeralSelection
            break;
        }

        case EngagementData::StateType::Selected:
        {
            // Nop
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
        // Beginning selection (iff in canvas)
        //

        auto const selectionStartCorner = GetMouseSelectionCoordsIfInSpace();
        if (selectionStartCorner)
        {
            // Start selecting
            mEngagementData.emplace(*selectionStartCorner);

            // TODOTEST
            LogMessage("MouseDown: Layer=", int(TLayer), " SelectionStart=", mEngagementData->SelectionStartCorner);
        }
    }
    else
    {
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
                // TODO
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
    else
    {
        switch (mEngagementData->State)
        {
            case EngagementData::StateType::Selecting:
            {
                // TODO
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
void SelectAndTranslateTool<TLayer, TCoordsTag>::UpdateEphemeralSelection(ShipSpaceRect const & rect)
{
    // Update overlay
    mController.GetView().UploadDashedRectangleOverlay(rect);
    mController.GetUserInterface().RefreshView();
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

}