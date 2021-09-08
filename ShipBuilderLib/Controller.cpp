/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

namespace ShipBuilder {

std::unique_ptr<Controller> Controller::CreateNew(
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
{
    auto modelController = ModelController::CreateNew(
        WorkSpaceSize(400, 200), // TODO: from preferences
        view,
        userInterface);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface));

    return controller;
}

std::unique_ptr<Controller> Controller::LoadShip(
    std::filesystem::path const & shipFilePath,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
{
    auto modelController = ModelController::Load(
        shipFilePath,
        view,
        userInterface);

    std::unique_ptr<Controller> controller = std::unique_ptr<Controller>(
        new Controller(
            std::move(modelController),
            view,
            workbenchState,
            userInterface));

    return controller;
}

Controller::Controller(
    std::unique_ptr<ModelController> modelController,
    View & view,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface)
    : mModelController(std::move(modelController))
    , mView(view)
    , mWorkbenchState(workbenchState)
    , mUserInterface(userInterface)
    //
    , mInputState()
{
}

void Controller::OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition)
{
    // Update input state
    mInputState.PreviousMousePosition = mInputState.MousePosition;
    mInputState.MousePosition = mouseScreenPosition;

    // TODO: FW to tool

    // Tell UI
    mUserInterface.DisplayToolCoordinates(mView.DisplayToWorkSpace(mInputState.MousePosition));
}

void Controller::OnLeftMouseDown()
{
    // Update input state
    mInputState.IsLeftMouseDown = true;

    // TODO: FW to tool
}

void Controller::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // TODO: FW to tool
}

void Controller::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // TODO: FW to tool
}

void Controller::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    // TODO: FW to tool
}

void Controller::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // TODO: FW to tool
}

void Controller::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // TODO: FW to tool
}

///////////////////////////////////////////////////////////////////////////////

}