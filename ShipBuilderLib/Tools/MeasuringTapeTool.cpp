/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MeasuringTapeTool.h"

#include <Controller.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameMath.h>

namespace ShipBuilder {

MeasuringTapeTool::MeasuringTapeTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::StructuralMeasuringTapeTool,
        controller)
    , mIsShiftDown(false)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("measuring_tape_cursor", 0, 25, resourceLocator));

    // Check if we draw the overlay right away
    auto const mouseCoordinates = GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DrawOverlay(ClipToWorkCanvas(ScreenToShipSpace(*mouseCoordinates)));
        mController.GetUserInterface().RefreshView();
    }
}

MeasuringTapeTool::~MeasuringTapeTool()
{
    if (mHasOverlay)
    {
        HideOverlay();
    }

    if (mEngagementData.has_value())
    {
        StopEngagement();
    }

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    auto const mouseShipSpaceCoords = ClipToWorkCanvas(ScreenToShipSpace(mouseCoordinates));

    if (mEngagementData.has_value())
    {
        // Do action
        DoAction(mouseShipSpaceCoords);
    }

    // Draw overlay
    DrawOverlay(mouseShipSpaceCoords);

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnLeftMouseDown()
{
    auto const shipSpaceMouseCoords = ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace());

    // Engage
    StartEngagement(shipSpaceMouseCoords);

    // Do action
    DoAction(shipSpaceMouseCoords);

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnLeftMouseUp()
{
    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        // Disengage
        StopEngagement();

        mController.GetUserInterface().RefreshView();
    }
}

void MeasuringTapeTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData.has_value())
    {
        DoAction(ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace()));

        mController.GetUserInterface().RefreshView();
    }
}

void MeasuringTapeTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData.has_value())
    {
        DoAction(ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace()));

        mController.GetUserInterface().RefreshView();
    }
}

//////////////////////////////////////////////////////////////////////////////

void MeasuringTapeTool::StartEngagement(ShipSpaceCoordinates const & coords)
{
    assert(!mEngagementData.has_value());

    mEngagementData.emplace(coords);
}

void MeasuringTapeTool::DoAction(ShipSpaceCoordinates const & coords)
{
    assert(mEngagementData.has_value());

    // Apply SHIFT lock
    ShipSpaceCoordinates endPoint = coords;
    if (mIsShiftDown)
    {
        // Constrain to either horizontally or vertically
        if (std::abs(endPoint.x - mEngagementData->StartCoords.x) > std::abs(endPoint.y - mEngagementData->StartCoords.y))
        {
            // X is larger
            endPoint.y = mEngagementData->StartCoords.y;
        }
        else
        {
            // Y is larger
            endPoint.x = mEngagementData->StartCoords.x;
        }
    }

    mController.GetView().UploadDashedLineOverlay(
        mEngagementData->StartCoords,
        endPoint,
        View::OverlayMode::Default);

    // Calculate length
    auto const ratio = mController.GetModelController().GetShipMetadata().Scale;
    vec2f const v(endPoint.ToFractionalCoords(ratio) - mEngagementData->StartCoords.ToFractionalCoords(ratio));
    mController.GetUserInterface().OnMeasuredWorldLengthChanged(static_cast<int>(std::round(v.length())));
}

void MeasuringTapeTool::StopEngagement()
{
    assert(mEngagementData.has_value());

    mController.GetView().RemoveDashedLineOverlay();

    mController.GetUserInterface().OnMeasuredWorldLengthChanged(std::nullopt);

    mEngagementData.reset();
}

void MeasuringTapeTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    mController.GetView().UploadCircleOverlay(
        coords,
        View::OverlayMode::Default);

    mHasOverlay = true;
}

void MeasuringTapeTool::HideOverlay()
{
    assert(mHasOverlay);

    mController.GetView().RemoveCircleOverlay();

    mHasOverlay = false;
}

ShipSpaceCoordinates MeasuringTapeTool::ClipToWorkCanvas(ShipSpaceCoordinates const & coords) const
{
    return ShipSpaceCoordinates(
        Clamp(coords.x, 0, mController.GetModelController().GetShipSize().width - 1),
        Clamp(coords.y, 0, mController.GetModelController().GetShipSize().height - 1));
}

}