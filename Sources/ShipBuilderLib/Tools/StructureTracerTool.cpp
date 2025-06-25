/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-06-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StructureTracerTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

StructureTracerTool::StructureTracerTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::StructureTracer,
        controller)
    , mStartCorner()
{
    SetCursor(WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, gameAssetManager));
}

StructureTracerTool::~StructureTracerTool()
{
    if (mStartCorner.has_value())
    {
        DrawOverlay(std::nullopt);
    }
}

void StructureTracerTool::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
{
    if (mStartCorner.has_value())
    {
        auto const cornerCoordinates = ScreenToTextureSpace(LayerType::ExteriorTexture, GetCurrentMouseCoordinates());
        DrawOverlay(cornerCoordinates);
    }
}

void StructureTracerTool::OnLeftMouseDown()
{
    // Begin engagement

    assert(!mStartCorner.has_value());
    mStartCorner = ScreenToTextureSpace(LayerType::ExteriorTexture, GetCurrentMouseCoordinates());

    DrawOverlay(*mStartCorner);
}

void StructureTracerTool::OnLeftMouseUp()
{
    if (mStartCorner.has_value())
    {
        // Stop overlay
        DrawOverlay(std::nullopt);

        // Calc rect
        auto const cornerCoordinates = ScreenToTextureSpace(LayerType::ExteriorTexture, GetCurrentMouseCoordinates());
        auto const textureRect = CalculateApplicableRect(cornerCoordinates);

        if (textureRect.has_value())
        {
            // Do action
            DoTracing(*textureRect);
        }

        // Reset state
        mStartCorner.reset();
    }
}

//////////////////////////////////////////////////////////////////////////////

void StructureTracerTool::DrawOverlay(std::optional<ImageCoordinates> const & cornerCoordinates)
{
    if (cornerCoordinates.has_value())
    {
        // Calc rect
        auto const textureRect = CalculateApplicableRect(*cornerCoordinates);

        if (textureRect.has_value())
        {
            // Draw overlay
            mController.GetView().UploadDashedRectangleOverlay_Exterior(
                textureRect->MinMin(),
                textureRect->MaxMax());
        }
    }
    else
    {
        mController.GetView().RemoveDashedRectangleOverlay();
    }

    mController.GetUserInterface().RefreshView();
}

std::optional<ImageRect> StructureTracerTool::CalculateApplicableRect(ImageCoordinates const & cornerCoordinates) const
{
    assert(mStartCorner.has_value());

    ImageRect theoreticalRect(*mStartCorner, cornerCoordinates);

    return theoreticalRect.MakeIntersectionWith(ImageRect{ { 0, 0}, mController.GetModelController().GetExteriorTextureSize() });
}

void StructureTracerTool::DoTracing(ImageRect const & textureRect)
{
    // Do

    auto undoPayload = mController.GetModelController().StructureTrace(
        textureRect,
        mController.GetWorkbenchState().GetStructuralForegroundMaterial(),
        mController.GetWorkbenchState().GetStructuralBackgroundMaterial(),
        mController.GetWorkbenchState().GetTextureStructureTracerAlphaThreshold());

    // Store undo

    size_t const undoPayloadCost = undoPayload.GetTotalCost();

    mController.StoreUndoAction(
        _("Tracer"),
        undoPayloadCost,
        mController.GetModelController().GetDirtyState(),
        [undoPayload = std::move(undoPayload)](Controller & controller) mutable
        {
            controller.Restore(std::move(undoPayload));
        });

    // Epilog
    mController.LayerChangeEpilog({ LayerType::Structural });
}

}