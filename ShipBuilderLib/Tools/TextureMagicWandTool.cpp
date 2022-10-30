/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureMagicWandTool.h"

#include <Controller.h>

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

TextureMagicWandTool::TextureMagicWandTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::TextureMagicWand,
        controller)
    , mCursorImage(WxHelpers::LoadCursorImage("magic_wand_cursor", 8, 8, resourceLocator))
{
    SetCursor(mCursorImage);
}

void TextureMagicWandTool::OnLeftMouseDown()
{
    auto const mouseCoordinatesInTextureSpace = ScreenToTextureSpace(GetCurrentMouseCoordinates());
    if (mouseCoordinatesInTextureSpace.IsInRect(ImageRect({0, 0}, mController.GetModelController().GetTextureSize())))
    {
        // Take clone of current layer
        auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();
        auto layerClone = mController.GetModelController().CloneExistingLayer<LayerType::Texture>();

        // Do edit

        std::optional<ImageRect> affectedRegion = mController.GetModelController().TextureMagicWandEraseBackground(
            mouseCoordinatesInTextureSpace,
            mController.GetWorkbenchState().GetTextureMagicWandTolerance(),
            mController.GetWorkbenchState().GetTextureMagicWandIsAntiAliased(),
            mController.GetWorkbenchState().GetTextureMagicWandIsContiguous());

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
                    controller.RestoreTextureLayerRegionBackupForUndo(std::move(clippedLayerBackup), origin);
                });

            // Epilog
            mController.LayerChangeEpilog(LayerType::Texture);
        }
    }
}

}