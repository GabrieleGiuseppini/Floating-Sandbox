/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

namespace ShipBuilder {

std::unique_ptr<ModelController> ModelController::CreateNew(
    WorkSpaceSize const & workSpaceSize,
    View & view,
    IUserInterface & userInterface)
{
    return std::unique_ptr<ModelController>(
        new ModelController(
            workSpaceSize,
            view,
            userInterface));
}

std::unique_ptr<ModelController> ModelController::Load(
    std::filesystem::path const & shipFilePath,
    View & view,
    IUserInterface & userInterface)
{
    // TODOHERE
    return std::unique_ptr<ModelController>(
        new ModelController(
            WorkSpaceSize(400, 200),
            view,
            userInterface));
}

ModelController::ModelController(
    WorkSpaceSize const & workSpaceSize,
    View & view,
    IUserInterface & userInterface)
    : mView(view)
    , mUserInterface(userInterface)
    , mModel(workSpaceSize)
{
}

}