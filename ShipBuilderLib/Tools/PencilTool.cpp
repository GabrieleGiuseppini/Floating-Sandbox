/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

#include <type_traits>

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
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView.ScreenToShipSpace(inputState.MousePosition);

    // Check if within ship canvas
    if (mouseShipSpaceCoordinates.IsInRect(mModelController.GetModel().GetShipSize()))
    {
        if (inputState.IsLeftMouseDown)
        {
            ApplyEditAt(mouseShipSpaceCoordinates, MaterialPlaneType::Foreground);
        }
        else if (inputState.IsRightMouseDown)
        {
            ApplyEditAt(mouseShipSpaceCoordinates, MaterialPlaneType::Background);
        }
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::OnLeftMouseDown(InputState const & inputState)
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView.ScreenToShipSpace(inputState.MousePosition);

    // Check if within ship canvas
    if (mouseShipSpaceCoordinates.IsInRect(mModelController.GetModel().GetShipSize()))
    {
        ApplyEditAt(mouseShipSpaceCoordinates, MaterialPlaneType::Foreground);
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::OnRightMouseDown(InputState const & inputState)
{
    // Calculate ship coordinates
    ShipSpaceCoordinates mouseShipSpaceCoordinates = mView.ScreenToShipSpace(inputState.MousePosition);

    // Check if within ship canvas
    if (mouseShipSpaceCoordinates.IsInRect(mModelController.GetModel().GetShipSize()))
    {
        ApplyEditAt(mouseShipSpaceCoordinates, MaterialPlaneType::Background);
    }
}

template<typename TMaterial>
void PencilTool<TMaterial>::ApplyEditAt(
    ShipSpaceCoordinates const & position,
    MaterialPlaneType plane)
{
    // TODOTEST
    LogMessage("TODOTEST: PencilTool::ApplyEditAt: ", position.ToString(), " plane=", static_cast<int>(plane));

    std::unique_ptr<UndoEntry> undoEntry;

    if constexpr (std::is_same<TMaterial, StructuralMaterial>())
    {
        undoEntry = mModelController.StructuralRegionFill(
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetStructuralForegroundMaterial()
            : mWorkbenchState.GetStructuralBackgroundMaterial(),
            position,
            ShipSpaceSize(1, 1));
    }
    else
    {
        static_assert(std::is_same<TMaterial, ElectricalMaterial>());

        undoEntry = mModelController.ElectricalRegionFill(
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetElectricalForegroundMaterial()
            : mWorkbenchState.GetElectricalBackgroundMaterial(),
            position,
            ShipSpaceSize(1, 1));
    }

    // TODO: hook with undo stack

    // Notify we're dirty now
    mUserInterface.OnModelDirtyChanged(mModelController.GetModel().GetIsDirty());

    // Force view refresh
    mUserInterface.RefreshView();
}

}