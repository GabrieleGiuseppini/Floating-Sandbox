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
    // TODOHERE
    /*
    // Take clone of current layer
    auto layerDirtyStateClone = mController.GetModelController().GetDirtyState();
    auto layerClone = mController.GetModelController().CloneExistingLayer<TLayer>();

    // Do edit

    LayerMaterialType const * const floodMaterial = GetFloodMaterial(isRightButton ? MaterialPlaneType::Background : MaterialPlaneType::Foreground);

    static_assert(TLayer == LayerType::Structural);
    std::optional<ShipSpaceRect> affectedRegion = mController.GetModelController().StructuralFlood(
        mouseCoordinates,
        floodMaterial,
        mController.GetWorkbenchState().GetStructuralFloodToolIsContiguous());

    if (affectedRegion.has_value())
    {
        // Create undo action

        layerClone.Trim(*affectedRegion);

        mController.StoreUndoAction(
            TLayer == LayerType::Structural ? _("Flood Structural") : _("Flood Electrical"),
            layerClone.Buffer.GetByteSize(),
            layerDirtyStateClone,
            [layerClone = std::move(layerClone), origin = affectedRegion->origin](Controller & controller) mutable
            {
                static_assert(TLayer == LayerType::Structural);
                controller.RestoreStructuralLayerRegionForUndo(std::move(layerClone), origin);
            });

        // Display sampled material
        mController.BroadcastSampledInformationUpdatedAt(mouseCoordinates, TLayer);

        // Epilog
        mController.LayerChangeEpilog(TLayer);
    }
    */
}

}