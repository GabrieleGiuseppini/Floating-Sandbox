/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

namespace ShipBuilder {

std::unique_ptr<ModelController> ModelController::CreateNew(
    WorkSpaceSize const & workSpaceSize,
    IUserInterface & userInterface,
    View & view)
{
    return std::unique_ptr<ModelController>(
        new ModelController(
            workSpaceSize,
            userInterface,
            view));
}

ModelController::ModelController(
    WorkSpaceSize const & workSpaceSize,
    IUserInterface & userInterface,
    View & view)
    : mUserInterface(userInterface)
    , mView(view)
    , mModel(workSpaceSize)
{
}

}