/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

namespace ShipBuilder {

template<LayerType Layer>
PencilTool<Layer>::PencilTool(
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

template<LayerType Layer>
void PencilTool<Layer>::OnMouseMove(InputState const & inputState)
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

template<LayerType Layer>
void PencilTool<Layer>::OnLeftMouseDown(InputState const & inputState)
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(inputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController.GetModel().GetWorkSpaceSize()))
    {
        ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Foreground);
    }
}

template<LayerType Layer>
void PencilTool<Layer>::OnRightMouseDown(InputState const & inputState)
{
    // Calculate work coordinates
    WorkSpaceCoordinates mouseWorkSpaceCoordinates = mView.ScreenToWorkSpace(inputState.MousePosition);

    // Check if within work canvas
    if (mouseWorkSpaceCoordinates.IsInRect(mModelController.GetModel().GetWorkSpaceSize()))
    {
        ApplyEditAt(mouseWorkSpaceCoordinates, MaterialPlaneType::Background);
    }
}

template<LayerType Layer>
void PencilTool<Layer>::ApplyEditAt(
    WorkSpaceCoordinates const & position,
    MaterialPlaneType plane)
{
    // TODOHERE
    LogMessage("TODOTEST: PencilTool::ApplyEditAt: ", position.ToString(), " plane=", static_cast<int>(plane));
}

}