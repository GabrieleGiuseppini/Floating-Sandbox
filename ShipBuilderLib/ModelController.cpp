/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

#include <Game/MaterialDatabase.h>

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
    UploadStructuralLayerToView();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Structural
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewStructuralLayer()
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.NewStructuralLayer();

    UploadStructuralLayerToView();
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    mModel.SetStructuralLayer();

    UploadStructuralLayerToView();
}

std::unique_ptr<UndoEntry> ModelController::StructuralRegionFill(
    StructuralMaterial const * material,
    WorkSpaceCoordinates const & origin,
    WorkSpaceSize const & size)
{
    //
    // Create undo edit action
    //

    std::unique_ptr<UndoEditAction> undoEditAction = std::make_unique<MaterialRegionUndoEditAction<StructuralMaterial>>(
        mModel.GetStructuralMaterialMatrix().MakeCopy(origin, size),
        origin);

    //
    // Fill
    //

    MaterialBuffer<StructuralMaterial> & structuralMaterialMatrix = mModel.GetStructuralMaterialMatrix();
    RgbaImageData & structuralRenderColorTexture = mModel.GetStructuralRenderColorTexture();

    rgbaColor const renderColor = material != nullptr
        ? rgbaColor(material->RenderColor)
        : rgbaColor(MaterialDatabase::EmptyMaterialColorKey, 255);

    for (int y = origin.y; y < origin.y + size.height; ++y)
    {
        for (int x = origin.x; x < origin.x + size.width; ++x)
        {
            vec2i const coords = vec2i(x, y);

            structuralMaterialMatrix[coords] = material;
            structuralRenderColorTexture[coords] = renderColor;
        }
    }

    // Update view
    UploadStructuralLayerToView(origin, size);

    //
    // Bake undo entry
    //

    return std::make_unique<UndoEntry>(
        std::move(undoEditAction),
        std::make_unique<MaterialRegionUndoEditAction<StructuralMaterial>>(
            mModel.GetStructuralMaterialMatrix().MakeCopy(origin, size),
            origin));
}

std::unique_ptr<UndoEntry> ModelController::StructuralRegionReplace(
    MaterialBuffer<StructuralMaterial> const & region,
    WorkSpaceCoordinates const & origin)
{
    // TODOHERE
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Electrical
////////////////////////////////////////////////////////////////////////////////////////////////////

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

std::unique_ptr<UndoEntry> ModelController::ElectricalRegionFill(
    ElectricalMaterial const * material,
    WorkSpaceCoordinates const & origin,
    WorkSpaceSize const & size)
{
    // TODO
    return nullptr;
}

std::unique_ptr<UndoEntry> ModelController::ElectricalRegionReplace(
    MaterialBuffer<ElectricalMaterial> const & region,
    WorkSpaceCoordinates const & origin)
{
    // TODOHERE
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ropes
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Texture
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::UploadStructuralLayerToView()
{
    mView.UploadStructuralTexture(mModel.GetStructuralRenderColorTexture());
}

void ModelController::UploadStructuralLayerToView(
    WorkSpaceCoordinates const & origin,
    WorkSpaceSize const & size)
{
    mView.UpdateStructuralTextureRegion(
        mModel.GetStructuralRenderColorTexture(),
        origin.x,
        origin.y,
        size.width,
        size.height);
}

}