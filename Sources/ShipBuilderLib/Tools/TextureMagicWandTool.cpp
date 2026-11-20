/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureMagicWandTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

ExteriorTextureMagicWandTool::ExteriorTextureMagicWandTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureMagicWandTool(
        ToolType::ExteriorTextureMagicWand,
        controller,
        gameAssetManager)
{}

InteriorTextureMagicWandTool::InteriorTextureMagicWandTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : TextureMagicWandTool(
        ToolType::ExteriorTextureMagicWand,
        controller,
        gameAssetManager)
{}

template<LayerType TLayerType, typename TBodyRegion>
TextureMagicWandTool<TLayerType, TBodyRegion>::TextureMagicWandTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
{
    SetCursor(WxHelpers::LoadCursorImage("magic_wand_cursor", 8, 8, gameAssetManager));
}

template<LayerType TLayerType, typename TBodyRegion>
void TextureMagicWandTool<TLayerType, TBodyRegion>::OnLeftMouseDown()
{
    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(TLayerType, GetCurrentMouseCoordinates());
    if (mouseCoordinatesInTextureSpace.IsInRect(ImageRect({0, 0}, GetTextureSize())))
    {
        auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();

        // Do edit

        std::optional<TBodyRegion> undoPayload;
        if constexpr (TLayerType == LayerType::ExteriorTexture)
        {
            undoPayload = mController.GetModelController().ExteriorTextureMagicWandEraseBackground(
                mouseCoordinatesInTextureSpace,
                mController.GetWorkbenchState().GetTextureMagicWandTolerance(),
                mController.GetWorkbenchState().GetTextureMagicWandIsAntiAliased(),
                mController.GetWorkbenchState().GetTextureMagicWandIsContiguous());
        }
        else
        {
            static_assert(TLayerType == LayerType::InteriorTexture);

            undoPayload = mController.GetModelController().InteriorTextureMagicWandEraseBackground(
                mouseCoordinatesInTextureSpace,
                mController.GetWorkbenchState().GetTextureMagicWandTolerance(),
                mController.GetWorkbenchState().GetTextureMagicWandIsAntiAliased(),
                mController.GetWorkbenchState().GetTextureMagicWandIsContiguous());
        }

        if (undoPayload.has_value())
        {
            auto const totalCost = undoPayload->GetTotalCost();

            // Create undo action
            mController.StoreUndoAction(
                _("Background Erase"),
                totalCost,
                layerDirtyStateClone,
                [undoPayload = std::move(*undoPayload)](Controller & controller) mutable
                {
                    controller.RestoreForUndo(std::move(undoPayload));
                });

            // Epilog
            mController.LayerChangeEpilog({ TLayerType });
        }
    }
}

template<LayerType TLayerType, typename TBodyRegion>
ImageSize TextureMagicWandTool<TLayerType, TBodyRegion>::GetTextureSize() const
{
    if constexpr (TLayerType == LayerType::ExteriorTexture)
    {
        return mController.GetModelController().GetExteriorTextureSize();
    }
    else
    {
        static_assert(TLayerType == LayerType::InteriorTexture);

        return mController.GetModelController().GetInteriorTextureSize();
    }
}

}