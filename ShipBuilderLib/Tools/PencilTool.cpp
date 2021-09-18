/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

namespace ShipBuilder {

template<typename TMaterial>
PencilTool<TMaterial>::PencilTool(
    ToolType toolType,
    ModelController & modelController,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        modelController,
        workbenchState,
        userInterface,
        view)
    , mCursorImage(WxHelpers::LoadCursorImage("pencil_cursor", 1, 29, resourceLocator))
{
    SetCursor(mCursorImage);
}

StructuralPencilTool::StructuralPencilTool(
    ModelController & modelController,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::StructuralPencil,
        modelController,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalPencilTool::ElectricalPencilTool(
    ModelController & modelController,
    WorkbenchState const & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : PencilTool(
        ToolType::ElectricalPencil,
        modelController,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<typename TMaterial>
void PencilTool<TMaterial>::OnMouseMove(InputState const & inputState)
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(inputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController.GetModel().GetWorkSpaceSize()))
    {
        if (inputState.IsLeftMouseDown)
        {
            ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Foreground);
        }
        else if (inputState.IsRightMouseDown)
        {
            ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Background);
        }
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::OnLeftMouseDown(InputState const & inputState)
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(inputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController.GetModel().GetWorkSpaceSize()))
    {
        ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Foreground);
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::OnRightMouseDown(InputState const & inputState)
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(inputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController.GetModel().GetWorkSpaceSize()))
    {
        ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Background);
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::ApplyEditAt(
    WorkSpaceCoordinates const & position,
    MaterialPlaneType plane)
{
    // TODOTEST
    LogMessage("TODOTEST: PencilTool::ApplyEditAt: ", position.ToString(), " plane=", static_cast<int>(plane));

    auto const editAction = MaterialRegionFillEditAction(
        plane == MaterialPlaneType::Foreground
        ? mWorkbenchState.GetStructuralForegroundMaterial()
        : mWorkbenchState.GetStructuralBackgroundMaterial(),
        position,
        WorkSpaceSize(1, 1));

    auto undoEntry = mModelController.Edit(editAction);

    // TODO: hook with undo stack

    // TODO: publish dirtyness

    // Force refresh
    mUserInterface.RefreshView();
}

}