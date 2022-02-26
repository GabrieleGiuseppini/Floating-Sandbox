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
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::StructuralMeasuringTapeTool,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mIsShiftDown(false)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("measuring_tape_cursor", 0, 25, resourceLocator));

    // Check if we draw the overlay right away
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        DrawOverlay(ClipToWorkCanvas(ScreenToShipSpace(*mouseCoordinates)));
        mUserInterface.RefreshView();
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

    mUserInterface.RefreshView();
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

    mUserInterface.RefreshView();
}

void MeasuringTapeTool::OnLeftMouseDown()
{
    auto const shipSpaceMouseCoords = ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace());

    // Engage
    StartEngagement(shipSpaceMouseCoords);

    // Do action
    DoAction(shipSpaceMouseCoords);

    mUserInterface.RefreshView();
}

void MeasuringTapeTool::OnLeftMouseUp()
{
    // Check if should stop engagement
    if (mEngagementData.has_value())
    {
        // Disengage
        StopEngagement();
        
        mUserInterface.RefreshView();
    }
}

void MeasuringTapeTool::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData.has_value())
    {
        DoAction(ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace()));

        mUserInterface.RefreshView();
    }
}

void MeasuringTapeTool::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData.has_value())
    {
        DoAction(ClipToWorkCanvas(GetCurrentMouseCoordinatesInShipSpace()));

        mUserInterface.RefreshView();
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

    mView.UploadDashedLineOverlay(
        mEngagementData->StartCoords,
        endPoint,
        View::OverlayMode::Default);

    // Calculate length
    auto const ratio = mModelController.GetModel().GetShipMetadata().Scale;
    vec2f const v(endPoint.ToFractionalCoords(ratio) - mEngagementData->StartCoords.ToFractionalCoords(ratio));
    mUserInterface.OnMeasuredWorldLengthChanged(static_cast<int>(std::round(v.length())) + 1);
}

void MeasuringTapeTool::StopEngagement()
{
    assert(mEngagementData.has_value());

    mView.RemoveDashedLineOverlay();

    mUserInterface.OnMeasuredWorldLengthChanged(std::nullopt);

    mEngagementData.reset();
}

void MeasuringTapeTool::DrawOverlay(ShipSpaceCoordinates const & coords)
{
    mView.UploadCircleOverlay(
        coords,
        View::OverlayMode::Default);

    mHasOverlay = true;
}

void MeasuringTapeTool::HideOverlay()
{
    assert(mHasOverlay);

    mView.RemoveCircleOverlay();

    mHasOverlay = false;
}

ShipSpaceCoordinates MeasuringTapeTool::ClipToWorkCanvas(ShipSpaceCoordinates const & coords) const
{
    return ShipSpaceCoordinates(
        Clamp(coords.x, 0, mModelController.GetModel().GetShipSize().width - 1),
        Clamp(coords.y, 0, mModelController.GetModel().GetShipSize().height - 1));
}

}