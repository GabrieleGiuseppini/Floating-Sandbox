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

std::unique_ptr<ModelController> ModelController::CreateFromLoad(
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
    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnWorkSpaceSizeChanged(mModel.GetWorkSpaceSize());
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());

    UploadStructuralRenderColorTextureToView();
}

void ModelController::NewStructuralLayer()
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.NewStructuralLayer();

    UploadStructuralRenderColorTextureToView();

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    mModel.SetStructuralLayer();

    UploadStructuralRenderColorTextureToView();

    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.SetElectricalLayer();

    // TODO: upload to view

    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::NewRopesLayer()
{
    mModel.NewRopesLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.SetRopesLayer();

    // TODO: upload to view

    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::SetTextureLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.SetTextureLayer();

    // TODO: upload to view

    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

void ModelController::RemoveTextureLayer()
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.RemoveTextureLayer();

    // TODO: upload to view

    mUserInterface.OnLayerPresenceChanged();
    mUserInterface.OnModelDirtyChanged(mModel.GetIsDirty());
}

///////////////////////////////////////////////////////////////////////////

void ModelController::UploadStructuralRenderColorTextureToView()
{
    mView.UploadStructuralRenderColorTexture(mModel.GetStructuralRenderColorTexture());
}

}