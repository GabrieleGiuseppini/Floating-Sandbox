/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RopePencilTool.h"

#include <GameCore/GameGeometry.h>

#include <UILib/WxHelpers.h>

#include <type_traits>

namespace ShipBuilder {

RopePencilTool::RopePencilTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        ToolType::RopePencil,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mHasOverlay(false)
    , mEngagementData()
{
    SetCursor(WxHelpers::LoadCursorImage("pencil_cursor", 2, 22, resourceLocator));

    // Check overlay
    auto const mouseCoordinates = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (mouseCoordinates)
    {
        auto const positionApplicability = GetPositionApplicability(*mouseCoordinates);
        if (positionApplicability.has_value())
        {
            DrawOverlay(*mouseCoordinates, *positionApplicability);

            mUserInterface.RefreshView();
        }
    }
}

RopePencilTool::~RopePencilTool()
{
    // Mend our ephemeral visualization, if any
    // TODO

    // Remove overlay, if any
    if (mHasOverlay)
    {
        HideOverlay();
    }

    mUserInterface.RefreshView();
}

void RopePencilTool::OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates)
{
    // Temp visualization
    if (mEngagementData.has_value())
    {
        // TODOHERE

    }

    // Overlay
    auto const positionApplicability = GetPositionApplicability(mouseCoordinates);
    if (positionApplicability.has_value())
    {
        DrawOverlay(mouseCoordinates, *positionApplicability);
    }
    else if (mHasOverlay)
    {
        HideOverlay();
    }

    mUserInterface.RefreshView();
}

void RopePencilTool::OnLeftMouseDown()
{
    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    if (!mEngagementData)
    {
        CheckEngagement(mouseCoordinates, StrongTypedFalse<IsRightMouseButton>);
    }

    if (mEngagementData)
    {
        DoEdit(mouseCoordinates);
    }
}

void RopePencilTool::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        CommmitAndStopEngagement();

        assert(!mEngagementData);
    }
}

void RopePencilTool::OnRightMouseDown()
{
    ShipSpaceCoordinates const mouseCoordinates = mUserInterface.GetMouseCoordinates();

    if (!mEngagementData)
    {
        CheckEngagement(mouseCoordinates, StrongTypedTrue<IsRightMouseButton>);
    }

    if (mEngagementData)
    {
        DoEdit(mouseCoordinates);
    }
}

void RopePencilTool::OnRightMouseUp()
{
    if (mEngagementData)
    {
        CommmitAndStopEngagement();

        assert(!mEngagementData);
    }
}

//////////////////////////////////////////////////////////////////////////////

void RopePencilTool::CheckEngagement(
    ShipSpaceCoordinates const & coords,
    StrongTypedBool<struct IsRightMouseButton> isRightButton)
{
    // TODO
}

void RopePencilTool::DoEdit(ShipSpaceCoordinates const & coords)
{
    // TODO
}

void RopePencilTool::CommmitAndStopEngagement()
{
    // TODO

    mEngagementData.reset();
}

void RopePencilTool::DrawOverlay(
    ShipSpaceCoordinates const & coords,
    bool isApplicablePosition)
{
    mView.UploadCircleOverlay(
        coords,
        isApplicablePosition ? View::OverlayMode::Default : View::OverlayMode::Error);

    mHasOverlay = true;
}

void RopePencilTool::HideOverlay()
{
    mView.RemoveCircleOverlay();

    mHasOverlay = false;
}

std::optional<bool> RopePencilTool::GetPositionApplicability(ShipSpaceCoordinates const & coords) const
{
    if (!coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        return std::nullopt;
    }

    return mModelController.IsRopeEndpointAllowedAt(coords);
}


}