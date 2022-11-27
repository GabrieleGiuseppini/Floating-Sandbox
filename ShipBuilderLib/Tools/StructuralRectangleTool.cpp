/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-11-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StructuralRectangleTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

StructuralRectangleTool::StructuralRectangleTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::StructuralRectangle,
        controller)
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, resourceLocator));
}

StructuralRectangleTool::~StructuralRectangleTool()
{
    if (mEngagementData)
    {
        // Undo eph viz
        UndoEphemeralRectangle();

        mController.LayerChangeEpilog();

        // Remove measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
    }
}

void StructuralRectangleTool::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
{
    if (mEngagementData)
    {
        UpdateEphViz();
    }
}

void StructuralRectangleTool::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const startCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (startCoordinates)
    {
        // Engage at selection start corner
        mEngagementData.emplace(*startCoordinates);

        // Calc rect
        ShipSpaceRect const rect = CalculateRect(*startCoordinates);

        // Draw eph viz
        mEngagementData->EphVizRestorePayload = DrawEphemeralRectangle(rect);

        mController.LayerChangeEpilog();

        // Update measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(rect.size);
    }
}

void StructuralRectangleTool::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        // Undo eph viz
        UndoEphemeralRectangle();

        // Calc rect
        ShipSpaceRect const rect = CalculateRect(GetCornerCoordinatesEngaged());

        // Draw rect
        DrawFinalRectangle(rect);

        mController.LayerChangeEpilog({LayerType::Structural});

        // Remove measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);

        // Disengage
        mEngagementData.reset();
    }
}

void StructuralRectangleTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        UpdateEphViz();
    }
}

void StructuralRectangleTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        UpdateEphViz();
    }
}

//////////////////////////////////////////////////////////////////////////////

ShipSpaceCoordinates StructuralRectangleTool::GetCornerCoordinatesEngaged() const
{
    ShipSpaceCoordinates const mouseCoordinates = GetCurrentMouseShipCoordinatesClampedToShip();

    // Eventually constrain to square
    if (mIsShiftDown)
    {
        auto const width = mouseCoordinates.x - mEngagementData->StartCorner.x;
        auto const height = mouseCoordinates.y - mEngagementData->StartCorner.y;
        if (std::abs(width) < std::abs(height))
        {
            // Use width
            return ShipSpaceCoordinates(
                mouseCoordinates.x,
                mEngagementData->StartCorner.y + std::abs(width) * Sign(height));
        }
        else
        {
            // Use height
            return ShipSpaceCoordinates(
                mEngagementData->StartCorner.x + std::abs(height) * Sign(width),
                mouseCoordinates.y);
        }
    }
    else
    {
        return mouseCoordinates;
    }
}

ShipSpaceRect StructuralRectangleTool::CalculateRect(ShipSpaceCoordinates const & cornerCoordinates) const
{
    assert(mEngagementData);

    ShipSpaceRect rect(mEngagementData->StartCorner, cornerCoordinates);
    rect.size += ShipSpaceSize(1, 1);

    return rect;
}

GenericEphemeralVisualizationRestorePayload StructuralRectangleTool::DrawEphemeralRectangle(ShipSpaceRect const & rect)
{
    auto const [lineMaterial, fillMaterial] = GetMaterials();

    auto restorePayload = mController.GetModelController().StructuralRectangleForEphemeralVisualization(
        rect,
        mController.GetWorkbenchState().GetStructuralRectangleLineSize(),
        lineMaterial,
        fillMaterial);

    mController.GetView().UploadSelectionOverlay(
        rect.MinMin(),
        rect.MaxMax());

    return restorePayload;
}

void StructuralRectangleTool::UndoEphemeralRectangle()
{
    mController.GetView().RemoveSelectionOverlay();

    if (mEngagementData && mEngagementData->EphVizRestorePayload)
    {
        mController.GetModelController().RestoreEphemeralVisualization(std::move(*mEngagementData->EphVizRestorePayload));
        mEngagementData->EphVizRestorePayload.reset();
    }
}

void StructuralRectangleTool::UpdateEphViz()
{
    assert(mEngagementData);

    // Undo eph viz
    UndoEphemeralRectangle();

    // Calc rect
    ShipSpaceRect const rect = CalculateRect(GetCornerCoordinatesEngaged());

    // Draw eph viz
    mEngagementData->EphVizRestorePayload = DrawEphemeralRectangle(rect);

    mController.LayerChangeEpilog();

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(rect.size);
}

void StructuralRectangleTool::DrawFinalRectangle(ShipSpaceRect const & rect)
{
    auto const [lineMaterial, fillMaterial] = GetMaterials();

    auto undoPayload = mController.GetModelController().StructuralRectangle(
        rect,
        mController.GetWorkbenchState().GetStructuralRectangleLineSize(),
        lineMaterial,
        fillMaterial);

    // Store undo

    size_t const undoPayloadCost = undoPayload.GetTotalCost();

    mController.StoreUndoAction(
        _("Rect"),
        undoPayloadCost,
        mController.GetModelController().GetDirtyState(),
        [undoPayload = std::move(undoPayload)](Controller & controller) mutable
        {
            controller.Restore(std::move(undoPayload));
        });
}

std::tuple<StructuralMaterial const *, StructuralMaterial const *> StructuralRectangleTool::GetMaterials() const
{
    StructuralMaterial const * fillMaterial;
    switch (mController.GetWorkbenchState().GetStructuralRectangleFillMode())
    {
        case FillMode::FillWithForeground:
        {
            fillMaterial = mController.GetWorkbenchState().GetStructuralForegroundMaterial();
            break;
        }

        case FillMode::FillWithBackground:
        {
            fillMaterial = mController.GetWorkbenchState().GetStructuralBackgroundMaterial();
            break;
        }

        case FillMode::NoFill:
        {
            fillMaterial = nullptr;
            break;
        }
    }

    return std::make_tuple(
        mController.GetWorkbenchState().GetStructuralForegroundMaterial(),
        fillMaterial);
}

}