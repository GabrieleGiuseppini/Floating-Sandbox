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
    // Model is not dirty now
    assert(!mModel.GetIsDirty());

    // Upload view
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
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.SetStructuralLayer();

    UploadStructuralLayerToView();
}

void ModelController::StructuralRegionFill(
    StructuralMaterial const * material,
    ShipSpaceCoordinates const & origin,
    ShipSpaceSize const & size)
{
    assert(mModel.HasLayer(LayerType::Structural));

    StructuralLayerBuffer & structuralLayerBuffer = mModel.GetStructuralLayerBuffer();
    RgbaImageData & structuralRenderColorTexture = mModel.GetStructuralRenderColorTexture();

    rgbaColor const renderColor = material != nullptr
        ? rgbaColor(material->RenderColor, 255)
        : rgbaColor(MaterialDatabase::EmptyMaterialColorKey, 255);

    for (int y = origin.y; y < origin.y + size.height; ++y)
    {
        for (int x = origin.x; x < origin.x + size.width; ++x)
        {
            structuralLayerBuffer[ShipSpaceCoordinates(x, y)].Material = material;
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
}

void ModelController::StructuralRegionReplace(
    StructuralLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    assert(mModel.HasLayer(LayerType::Structural));

    // TODOHERE
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

void ModelController::ElectricalRegionFill(
    ElectricalMaterial const * material,
    ShipSpaceCoordinates const & origin,
    ShipSpaceSize const & size)
{
    // TODOHERE - copy from Structural
}

void ModelController::ElectricalRegionReplace(
    ElectricalLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    // TODOHERE
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