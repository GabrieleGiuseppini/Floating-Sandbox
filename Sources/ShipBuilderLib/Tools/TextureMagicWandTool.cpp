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

template<LayerType TLayerType>
TextureMagicWandTool<TLayerType>::TextureMagicWandTool(
    ToolType toolType,
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        toolType,
        controller)
{
    SetCursor(WxHelpers::LoadCursorImage("magic_wand_cursor", 8, 8, gameAssetManager));
}

template<LayerType TLayerType>
void TextureMagicWandTool<TLayerType>::OnLeftMouseDown()
{
    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(TLayerType, GetCurrentMouseCoordinates());
    if (mouseCoordinatesInTextureSpace.IsInRect(ImageRect({0, 0}, GetTextureSize())))
    {
        // Take clone of current layer
        auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();
        auto layerClone = mController.GetModelController().CloneExistingLayer<TLayerType>();

        // Do edit

        std::optional<ImageRect> affectedRegion;
        if constexpr (TLayerType == LayerType::ExteriorTexture)
        {
            affectedRegion = mController.GetModelController().ExteriorTextureMagicWandEraseBackground(
                mouseCoordinatesInTextureSpace,
                mController.GetWorkbenchState().GetTextureMagicWandTolerance(),
                mController.GetWorkbenchState().GetTextureMagicWandIsAntiAliased(),
                mController.GetWorkbenchState().GetTextureMagicWandIsContiguous());
        }
        else
        {
            static_assert(TLayerType == LayerType::InteriorTexture);

            affectedRegion = mController.GetModelController().InteriorTextureMagicWandEraseBackground(
                mouseCoordinatesInTextureSpace,
                mController.GetWorkbenchState().GetTextureMagicWandTolerance(),
                mController.GetWorkbenchState().GetTextureMagicWandIsAntiAliased(),
                mController.GetWorkbenchState().GetTextureMagicWandIsContiguous());
        }

        if (affectedRegion.has_value())
        {
            // Create undo action

            auto clippedLayerBackup = layerClone.MakeRegionBackup(*affectedRegion);
            auto const cloneByteSize = clippedLayerBackup.Buffer.GetByteSize();

            mController.StoreUndoAction(
                _("Background Erase"),
                cloneByteSize,
                layerDirtyStateClone,
                [clippedLayerBackup = std::move(clippedLayerBackup), origin = affectedRegion->origin](Controller & controller) mutable
                {
                    if constexpr (TLayerType == LayerType::ExteriorTexture)
                    {
                        controller.RestoreExteriorTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
                    }
                    else
                    {
                        static_assert(TLayerType == LayerType::InteriorTexture);

                        controller.RestoreInteriorTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
                    }
                });

            // Epilog
            mController.LayerChangeEpilog({ TLayerType });
        }
    }
}

template<LayerType TLayerType>
ImageSize TextureMagicWandTool<TLayerType>::GetTextureSize() const
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