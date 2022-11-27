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
    , mCurrentRectangle()
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, resourceLocator));
}

StructuralRectangleTool::~StructuralRectangleTool()
{
    if (mCurrentRectangle || mEngagementData)
    {
        // Remove overlay
        mController.GetView().RemoveSelectionOverlay();
        mController.GetUserInterface().RefreshView();

        // Remove measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
    }
}

void StructuralRectangleTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged(mouseCoordinates);

        UpdateEphemeralRectangle(cornerCoordinates);
    }
}

void StructuralRectangleTool::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const cornerCoordinates = GetCornerCoordinatesFree();
    if (cornerCoordinates)
    {
        // Engage at selection start corner
        mEngagementData.emplace(*cornerCoordinates);

        UpdateEphemeralRectangle(*cornerCoordinates);
    }
}

void StructuralRectangleTool::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        // Calculate corner
        ShipSpaceCoordinates const cornerCoordinates = GetCornerCoordinatesEngaged();

        // Calculate rectangle
        std::optional<ShipSpaceRect> rectangle;
        if (cornerCoordinates.x != mEngagementData->StartCorner.x
            && cornerCoordinates.y != mEngagementData->StartCorner.y)
        {
            // Non-empty rectangle
            rectangle = ShipSpaceRect(
                mEngagementData->StartCorner,
                cornerCoordinates);

            // Update overlay
            mController.GetView().UploadSelectionOverlay(
                mEngagementData->StartCorner,
                cornerCoordinates);

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(rectangle->size);
        }
        else
        {
            // Empty rectangle

            // Update overlay
            mController.GetView().RemoveSelectionOverlay();

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
        }

        // Commit rectangle
        mCurrentRectangle = rectangle;

        // Disengage
        mEngagementData.reset();

        mController.GetUserInterface().RefreshView();
    }
}

void StructuralRectangleTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralRectangle(cornerCoordinates);
    }
}

void StructuralRectangleTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralRectangle(cornerCoordinates);
    }
}

//////////////////////////////////////////////////////////////////////////////

ShipSpaceCoordinates StructuralRectangleTool::GetCornerCoordinatesEngaged() const
{
    return GetCornerCoordinatesEngaged(GetCurrentMouseCoordinates());
}

ShipSpaceCoordinates StructuralRectangleTool::GetCornerCoordinatesEngaged(DisplayLogicalCoordinates const & input) const
{
    // TODOHERE

    // Convert to ship coords closest to grid point
    ShipSpaceCoordinates const nearestGridPointCoordinates = ScreenToShipSpaceNearest(input);

    // Clamp - allowing for point at (w,h)
    ShipSpaceCoordinates const cornerCoordinates = nearestGridPointCoordinates.Clamp(mController.GetModelController().GetShipSize());

    // Eventually constrain to square
    if (mIsShiftDown)
    {
        auto const width = cornerCoordinates.x - mEngagementData->StartCorner.x;
        auto const height = cornerCoordinates.y - mEngagementData->StartCorner.y;
        if (std::abs(width) < std::abs(height))
        {
            // Use width
            return ShipSpaceCoordinates(
                cornerCoordinates.x,
                mEngagementData->StartCorner.y + std::abs(width) * Sign(height));
        }
        else
        {
            // Use height
            return ShipSpaceCoordinates(
                mEngagementData->StartCorner.x + std::abs(height) * Sign(width),
                cornerCoordinates.y);
        }
    }
    else
    {
        return cornerCoordinates;
    }
}

std::optional<ShipSpaceCoordinates> StructuralRectangleTool::GetCornerCoordinatesFree() const
{
    // TODOHERE
    ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(GetCurrentMouseCoordinates());

    auto const shipSize = mController.GetModelController().GetShipSize();
    if (mouseShipCoordinates.IsInRect(
        ShipSpaceRect(
            ShipSpaceCoordinates(0, 0),
            ShipSpaceSize(shipSize.width + 1, shipSize.height + 1))))
    {
        return mouseShipCoordinates;
    }
    else
    {
        return std::nullopt;
    }
}

void StructuralRectangleTool::UpdateEphemeralRectangle(ShipSpaceCoordinates const & cornerCoordinates)
{
    // Update overlay
    mController.GetView().UploadSelectionOverlay(
        mEngagementData->StartCorner,
        cornerCoordinates);
    mController.GetUserInterface().RefreshView();

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(
        ShipSpaceSize(
            std::abs(cornerCoordinates.x - mEngagementData->StartCorner.x),
            std::abs(cornerCoordinates.y - mEngagementData->StartCorner.y)));
}

}