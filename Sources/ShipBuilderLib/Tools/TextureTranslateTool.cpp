/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureTranslateTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

ExteriorTextureTranslateTool::ExteriorTextureTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureTranslateTool(
        ToolType::ExteriorTextureTranslate,
        controller,
        gameAssetManager)
{
}

InteriorTextureTranslateTool::InteriorTextureTranslateTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureTranslateTool(
        ToolType::InteriorTextureTranslate,
        controller,
        gameAssetManager)
{
}

template<LayerType TLayer>
TextureTranslateTool<TLayer>::TextureTranslateTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("pan_cursor", 15, 15, gameAssetManager));
}

template<LayerType TLayer>
void TextureTranslateTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
{
    if (mEngagementData)
    {
        DoTranslate(
            mEngagementData->StartPosition,
            ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()),
            mEngagementData->IsDirty);
    }
}

template<LayerType TLayer>
void TextureTranslateTool<TLayer>::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const textureCoordinates = ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates());

    // Engage at start position
    if constexpr (TLayer == LayerType::ExteriorTexture)
    {
        mEngagementData.emplace(
            textureCoordinates,
            mController.GetModelController().CloneExteriorTextureLayer());
    }
    else
    {
        static_assert(TLayer == LayerType::InteriorTexture);

        mEngagementData.emplace(
            textureCoordinates,
            mController.GetModelController().CloneInteriorTextureLayer());
    }
}

template<LayerType TLayer>
void TextureTranslateTool<TLayer>::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        // Disengage
        mEngagementData.reset();
    }
}

template<LayerType TLayer>
void TextureTranslateTool<TLayer>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        if (mEngagementData->IsDirty) // Prevent dirtying if nothing has ever changed (i.e. if mouse hasn't moved)
        {
            DoTranslate(
                mEngagementData->StartPosition,
                ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()),
                mEngagementData->IsDirty);
        }
    }
}

template<LayerType TLayer>
void TextureTranslateTool<TLayer>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        if (mEngagementData->IsDirty) // Prevent dirtying if nothing has ever changed (i.e. if mouse hasn't moved)
        {
            DoTranslate(
                mEngagementData->StartPosition,
                ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()),
                mEngagementData->IsDirty);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////


template<LayerType TLayer>
void TextureTranslateTool<TLayer>::DoTranslate(
    ImageCoordinates const & startPosition,
    ImageCoordinates const & endPosition,
    bool & isFirst)
{
    //
    // Apply SHIFT lock
    //

    ImageCoordinates actualEndPosition = endPosition;
    if (mIsShiftDown)
    {
        // Constrain to either horizontally or vertically
        if (std::abs(actualEndPosition.x - startPosition.x) > std::abs(actualEndPosition.y - startPosition.y))
        {
            // X is larger
            actualEndPosition.y = startPosition.y;
        }
        else
        {
            // Y is larger
            actualEndPosition.x = startPosition.x;
        }
    }

    //
    // Translate
    //

    ImageSize offset = endPosition - startPosition;

    LogMessage("TODOHERE: offset=", offset);

    if constexpr (TLayer == LayerType::ExteriorTexture)
    {
        // TODOHERE
        //auto undoPayload = mController.GetModelController().StructuralRectangle(
        //    rect,
        //    mController.GetWorkbenchState().GetStructuralRectangleLineSize(),
        //    lineMaterial,
        //    fillMaterial);
    }
    else
    {
        static_assert(TLayer == LayerType::InteriorTexture);

        // TODOHERE
    }

    //
    // Store undo
    //

    if (!isFirst)
    {
        // TODOHERE: from session

        //size_t const undoPayloadCost = undoPayload.GetTotalCost();

        //mController.StoreUndoAction(
        //    _("Translate"),
        //    undoPayloadCost,
        //    mController.GetModelController().GetDirtyState(),
        //    [undoPayload = std::move(undoPayload)](Controller & controller) mutable
        //    {
        //        controller.Restore(std::move(undoPayload));
        //    });

        isFirst = false;
    }

    //
    // Finalize
    //

    mController.LayerChangeEpilog({ TLayer });
}

}