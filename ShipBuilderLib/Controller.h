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

#include <memory>

namespace ShipBuilder {

/*
 * This class implements the core of the ShipBuilder logic. It orchestrates interactions between
 * UI, View, and Model.
 */
class Controller
{
public:

    Controller(
        IUserInterface & userInterface,
        View & view);

    void OnMouseMove(DisplayLogicalCoordinates const & mouseScreenPosition);
    void OnLeftMouseDown();
    void OnLeftMouseUp();
    void OnRightMouseDown();
    void OnRightMouseUp();
    void OnShiftKeyDown();
    void OnShiftKeyUp();

private:

    IUserInterface & mUserInterface;
    View & mView;

    // TODO: decide if member of uq_ptr
    std::unique_ptr<ModelController> mModelController;

    // Input state
    InputState mInputState;
};

}