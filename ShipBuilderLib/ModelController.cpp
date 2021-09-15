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
    View & view)
{
    return std::unique_ptr<ModelController>(
        new ModelController(
            workSpaceSize,
            view));
}

std::unique_ptr<ModelController> ModelController::CreateForShip(
    /* TODO: loaded ship ,*/
    View & view)
{
    // TODOHERE
    return std::unique_ptr<ModelController>(
        new ModelController(
            WorkSpaceSize(400, 200),
            view));
}

ModelController::ModelController(
    WorkSpaceSize const & workSpaceSize,
    View & view)
    : mView(view)
    , mModel(workSpaceSize)
{
    UploadStructuralRenderColorTextureToView();
}

void ModelController::NewStructuralLayer()
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.NewStructuralLayer();

    UploadStructuralRenderColorTextureToView();
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    mModel.SetStructuralLayer();

    UploadStructuralRenderColorTextureToView();
}

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    // TODO: upload to view
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.SetElectricalLayer();

    // TODO: upload to view
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    // TODO: upload to view
}

void ModelController::NewRopesLayer()
{
    mModel.NewRopesLayer();

    // TODO: upload to view
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.SetRopesLayer();

    // TODO: upload to view
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    // TODO: upload to view
}

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();

    // TODO: upload to view
}

void ModelController::SetTextureLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.SetTextureLayer();

    // TODO: upload to view
}

void ModelController::RemoveTextureLayer()
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.RemoveTextureLayer();

    // TODO: upload to view
}

///////////////////////////////////////////////////////////////////////////

void ModelController::UploadStructuralRenderColorTextureToView()
{
    mView.UploadStructuralRenderColorTexture(mModel.GetStructuralRenderColorTexture());
}

}