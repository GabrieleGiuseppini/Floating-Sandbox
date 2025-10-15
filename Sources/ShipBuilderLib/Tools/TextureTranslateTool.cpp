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
        DoTranslate(ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()));
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
            DoTranslate(ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()));
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
            DoTranslate(ScreenToTextureSpace(TLayer, GetCurrentMouseCoordinates()));
        }
    }
}

//////////////////////////////////////////////////////////////////////////////


template<LayerType TLayer>
void TextureTranslateTool<TLayer>::DoTranslate(ImageCoordinates const & endPosition)
{
    assert(mEngagementData.has_value());
    assert(mEngagementData->OriginalTextureLayerData);

    //
    // Apply SHIFT lock
    //

    ImageCoordinates actualEndPosition = endPosition;
    if (mIsShiftDown)
    {
        // Constrain to either horizontally or vertically
        if (std::abs(actualEndPosition.x - mEngagementData->StartPosition.x) > std::abs(actualEndPosition.y - mEngagementData->StartPosition.y))
        {
            // X is larger
            actualEndPosition.y = mEngagementData->StartPosition.y;
        }
        else
        {
            // Y is larger
            actualEndPosition.x = mEngagementData->StartPosition.x;
        }
    }

    //
    // Translate
    //

    ImageSize offset = actualEndPosition - mEngagementData->StartPosition;

    LogMessage("TODOHERE: offset=", offset);

    ImageCoordinates sourceOrigin(0, 0);
    ImageCoordinates targetOrigin(0, 0);

    if (offset.width >= 0)
    {
        sourceOrigin.x = 0;
        targetOrigin.x = offset.width;
    }
    else
    {
        sourceOrigin.x = -offset.width;
        targetOrigin.x = 0;
    }

    if (offset.height >= 0)
    {
        sourceOrigin.y = 0;
        targetOrigin.y = offset.height;
    }
    else
    {
        sourceOrigin.y = -offset.height;
        targetOrigin.y = 0;
    }

    auto const sourceRegion =
        ImageRect(sourceOrigin, mEngagementData->OriginalTextureLayerData->Buffer.Size)
        .MakeIntersectionWith(ImageRect(ImageCoordinates(0, 0), mEngagementData->OriginalTextureLayerData->Buffer.Size));

    if (sourceRegion.has_value())
    {
        if constexpr (TLayer == LayerType::ExteriorTexture)
        {
            mController.GetModelController().MakeExteriorLayerFromImage(
                *mEngagementData->OriginalTextureLayerData,
                *sourceRegion,
                targetOrigin);
        }
        else
        {
            static_assert(TLayer == LayerType::InteriorTexture);

            mController.GetModelController().MakeInteriorLayerFromImage(
                *mEngagementData->OriginalTextureLayerData,
                *sourceRegion,
                targetOrigin);
        }

        //
        // Store undo
        //

        if (!mEngagementData->IsDirty)
        {
            size_t const undoPayloadCost = mEngagementData->OriginalTextureLayerData->Buffer.GetByteSize();

            mController.StoreUndoAction(
                _("Translate"),
                undoPayloadCost,
                mController.GetModelController().GetDirtyState(),
                [sourceLayerData = mEngagementData->OriginalTextureLayerData->Clone()](Controller & controller) mutable
                {
                    if constexpr (TLayer == LayerType::ExteriorTexture)
                    {
                        controller.Restore(
                            GenericUndoPayload(
                                { 0, 0 },
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                std::move(sourceLayerData),
                                std::nullopt));
                    }
                    else
                    {
                        static_assert(TLayer == LayerType::InteriorTexture);

                        controller.Restore(
                            GenericUndoPayload(
                                { 0, 0 },
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                std::move(sourceLayerData)));
                    }
                });

            mEngagementData->IsDirty = true;
        }

        //
        // Finalize
        //

        mController.LayerChangeEpilog({ TLayer });
    }
}

}