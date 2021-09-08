/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "InputState.h"
#include "IUserInterface.h"
#include "ModelController.h"
#include "View.h"
#include "WorkbenchState.h"

#include <filesystem>
#include <memory>

namespace ShipBuilder {

/*
 * This class implements the core of the ShipBuilder logic. It orchestrates interactions between
 * UI, View, and Model.
 */
class Controller
{
public:

    static std::unique_ptr<Controller> CreateNew(
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface);

    static std::unique_ptr<Controller> LoadShip(
        std::filesystem::path const & shipFilePath,
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface);

    void OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition);
    void OnLeftMouseDown();
    void OnLeftMouseUp();
    void OnRightMouseDown();
    void OnRightMouseUp();
    void OnShiftKeyDown();
    void OnShiftKeyUp();

private:

    Controller(
        std::unique_ptr<ModelController> modelController,
        View & view,
        WorkbenchState & workbenchState,
        IUserInterface & userInterface);

private:

    View & mView;
    std::unique_ptr<ModelController> mModelController;

    WorkbenchState & mWorkbenchState;
    IUserInterface & mUserInterface;

    // Input state
    InputState mInputState;
};

}