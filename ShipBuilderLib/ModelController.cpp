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
    ShipSpaceSize const & shipSpaceSize,
    View & view)
{
    return std::unique_ptr<ModelController>(
        new ModelController(
            shipSpaceSize,
            view));
}

std::unique_ptr<ModelController> ModelController::CreateForShip(
    /* TODO: loaded ship ,*/
    View & view)
{
    // TODOHERE
    return std::unique_ptr<ModelController>(
        new ModelController(
            ShipSpaceSize(400, 200),
            view));
}

ModelController::ModelController(
    ShipSpaceSize const & shipSpaceSize,
    View & view)
    : mView(view)
    , mModel(shipSpaceSize)
{
    UploadStructuralLayerToView();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Structural
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewStructuralLayer()
{
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
    ShipSpaceCoordinates const & origin,
    ShipSpaceSize const & size)
{
    // TODOHERE
    return nullptr;
    /*
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
        ? rgbaColor(material->RenderColor, 255)
        : rgbaColor(MaterialDatabase::EmptyMaterialColorKey, 255);

    for (int y = origin.y; y < origin.y + size.height; ++y)
    {
        for (int x = origin.x; x < origin.x + size.width; ++x)
        {
            // TODO: see if can use [][]
            structuralMaterialMatrix[ShipSpaceCoordinates(x, y)] = material;
            structuralRenderColorTexture[ImageCoordinates(x, y)] = renderColor;
        }
    }

    //
    // Update view
    //

    if (size.height == 1)
    {
        // Just one row - upload partial
        UploadStructuralLayerRowToView(
            origin,
            size.width);
    }
    else
    {
        // More than one row - upload whole
        UploadStructuralLayerToView();
    }

    //
    // Bake undo entry
    //

    return std::make_unique<UndoEntry>(
        std::move(undoEditAction),
        std::make_unique<MaterialRegionUndoEditAction<StructuralMaterial>>(
            mModel.GetStructuralMaterialMatrix().MakeCopy(origin, size),
            origin));
    */
}

std::unique_ptr<UndoEntry> ModelController::StructuralRegionReplace(
    MaterialBuffer<StructuralMaterial> const & region,
    ShipSpaceCoordinates const & origin)
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
    ShipSpaceCoordinates const & origin,
    ShipSpaceSize const & size)
{
    // TODO
    return nullptr;
}

std::unique_ptr<UndoEntry> ModelController::ElectricalRegionReplace(
    MaterialBuffer<ElectricalMaterial> const & region,
    ShipSpaceCoordinates const & origin)
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

void ModelController::UploadStructuralLayerRowToView(
    ShipSpaceCoordinates const & origin,
    int width)
{
    int const linearOffset = origin.y * mModel.GetShipSize().width + origin.x;

    mView.UpdateStructuralTextureRegion(
        &(mModel.GetStructuralRenderColorTexture().Data[linearOffset]),
        origin.x,
        origin.y,
        width,
        1);
}

}