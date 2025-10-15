/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MeasuringTapeTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

#include <Core/GameMath.h>

namespace ShipBuilder {

MeasuringTapeTool::MeasuringTapeTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::StructuralMeasuringTape,
        controller)
    , mIsShiftDown(false)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("measuring_tape_cursor", 0, 25, gameAssetManager));

    // Check if we draw the overlay right away
    auto const mouseShipCoords = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoords)
    {
        UpdateOverlay(*mouseShipCoords);
        mController.GetUserInterface().RefreshView();
    }
}

MeasuringTapeTool::~MeasuringTapeTool()
{
    Leave();
}

void MeasuringTapeTool::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    std::optional<ShipSpaceCoordinates> mouseShipCoords;

    if (mEngagementData.has_value())
    {
        // Engaged: if mouse outside of ship, clamp to ship

        mouseShipCoords = GetCurrentMouseShipCoordinatesClampedToShip(mouseCoordinates);
        DoAction(*mouseShipCoords);

        UpdateOverlay(*mouseShipCoords);
    }
    else
    {
        // Not engaged: only show overlay when inside the ship

        mouseShipCoords = GetCurrentMouseShipCoordinatesIfInShip(mouseCoordinates);
        if (mouseShipCoords)
        {
            UpdateOverlay(*mouseShipCoords);
        }
        else if (mHasOverlay)
        {
            HideOverlay();
        }
    }

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnLeftMouseDown()
{
    auto const mouseShipCoords = GetCurrentMouseShipCoordinatesClampedToShip();

    // Engage
    StartEngagement(mouseShipCoords);

    // Do action
    DoAction(mouseShipCoords);

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnLeftMouseUp()
{
    if (mEngagementData.has_value())
    {
        StopEngagement();
    }

    mController.GetUserInterface().RefreshView();
}

void MeasuringTapeTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData.has_value())
    {
        DoAction(GetCurrentMouseShipCoordinatesClampedToShip());

        mController.GetUserInterface().RefreshView();
    }
}

void MeasuringTapeTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData.has_value())
    {
        DoAction(GetCurrentMouseShipCoordinatesClampedToShip());

        mController.GetUserInterface().RefreshView();
    }
}

void MeasuringTapeTool::OnMouseLeft()
{
    Leave();
}

//////////////////////////////////////////////////////////////////////////////

void MeasuringTapeTool::Leave()
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

void MeasuringTapeTool::UpdateOverlay(ShipSpaceCoordinates const & coords)
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

}