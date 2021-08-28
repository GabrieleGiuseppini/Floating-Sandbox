/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

namespace ShipBuilder {

ModelController::ModelController(
    IUserInterface & userInterface,
    View & view)
    : mUserInterface(userInterface)
    , mView(view)
{
    //
    // Create Model
    //

    mModel = std::make_unique<Model>();
}

}