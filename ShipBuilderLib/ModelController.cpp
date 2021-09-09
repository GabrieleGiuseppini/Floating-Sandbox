/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

#include <cassert>

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

void ModelController::NewStructuralLayer()
{
    mModel.NewStructuralLayer();
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    mModel.SetStructuralLayer();
}

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    mModel.SetElectricalLayer();
}

void ModelController::RemoveElectricalLayer()
{
    mModel.RemoveElectricalLayer();
}

void ModelController::NewRopesLayer()
{
    mModel.NewRopesLayer();
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    mModel.SetRopesLayer();
}

void ModelController::RemoveRopesLayer()
{
    mModel.RemoveRopesLayer();
}

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();
}

void ModelController::SetTextureLayer(/*TODO*/)
{
    mModel.SetTextureLayer();
}

void ModelController::RemoveTextureLayer()
{
    mModel.RemoveTextureLayer();
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

///////////////////////////////////////////////////////////////////////////

}