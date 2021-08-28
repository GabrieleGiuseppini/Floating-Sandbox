/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Controller.h"

namespace ShipBuilder {

Controller::Controller(
    IUserInterface & userInterface,
    View & view)
    : mUserInterface(userInterface)
    , mView(view)
    , mInputState()
{
}

void Controller::CreateNewShip()
{
    mModelController = ModelController::CreateNew(
        WorkSpaceSize(400, 200), // TODO: from preferences
        mUserInterface,
        mView);

    OnNewModelController();
}

void Controller::LoadShip(std::filesystem::path const & shipFilePath)
{
    mModelController = ModelController::Load(
        shipFilePath,
        mUserInterface,
        mView);

    OnNewModelController();
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

void Controller::OnNewModelController()
{
    // TODO: reset tool and pseudo-tools

    // Tell UI
    mUserInterface.OnWorkSpaceSizeChanged();
}

}