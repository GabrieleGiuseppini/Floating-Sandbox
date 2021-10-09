/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

#include <cassert>

namespace ShipBuilder {

std::unique_ptr<ModelController> ModelController::CreateNew(
    ShipSpaceSize const & shipSpaceSize,
    std::string const & shipName,
    View & view)
{
    Model model = Model(shipSpaceSize, shipName);
    RepopulateDerivedStructuralData(model, { ShipSpaceCoordinates(0, 0), model.GetShipSize() });

    return std::unique_ptr<ModelController>(
        new ModelController(
            std::move(model),
            view));
}

std::unique_ptr<ModelController> ModelController::CreateForShip(
    ShipDefinition && shipDefinition,
    View & view)
{
    Model model = Model(std::move(shipDefinition));
    RepopulateDerivedStructuralData(model, { ShipSpaceCoordinates(0, 0), model.GetShipSize() });

    return std::unique_ptr<ModelController>(
        new ModelController(
            std::move(model),
            view));
}

ModelController::ModelController(
    Model && model,
    View & view)
    : mView(view)
    , mModel(std::move(model))
{
    // Model is not dirty now
    assert(!mModel.GetIsDirty());

    // Upload view
    UploadStructuralLayerToView();
}

ShipDefinition ModelController::MakeShipDefinition() const
{
    return ShipDefinition(
        mModel.GetShipSize(),
        std::move(*mModel.CloneStructuralLayerBuffer()),
        nullptr, // TODOHERE
        nullptr, // TODOHERE
        mModel.CloneTextureLayerBuffer(),
        mModel.GetShipMetadata(),
        ShipPhysicsData(), // TODOHERE
        std::nullopt);
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
    ShipSpaceRect const & region)
{
    assert(mModel.HasLayer(LayerType::Structural));

    //
    // Update model
    //

    StructuralLayerBuffer & structuralLayerBuffer = mModel.GetStructuralLayerBuffer();

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            structuralLayerBuffer[ShipSpaceCoordinates(x, y)].Material = material;
        }
    }

    // Update derived data
    RepopulateDerivedStructuralData(mModel, region);

    //
    // Update view
    //

    if (region.size.height == 1)
    {
        // Just one row - upload partial
        UploadStructuralLayerRowToView(
            region.origin,
            region.size.width);
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

    //
    // Update model
    //

    mModel.GetStructuralLayerBuffer().Blit(
        layerBufferRegion,
        origin);

    // Update derived data
    RepopulateDerivedStructuralData(
        mModel,
        { origin, layerBufferRegion.Size });

    //
    // Update view
    //

    UploadStructuralLayerToView();
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
    ShipSpaceRect const & region)
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

void ModelController::RepopulateDerivedStructuralData(
    Model & model,
    ShipSpaceRect const & region)
{
    rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 255);

    StructuralLayerBuffer const & structuralLayerBuffer = model.GetStructuralLayerBuffer();
    RgbaImageData & structuralRenderColorTexture = model.GetStructuralRenderColorTexture();

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            auto const structuralMaterial = structuralLayerBuffer[{x, y}].Material;

            structuralRenderColorTexture[{x, y}] = structuralMaterial != nullptr
                ? rgbaColor(structuralMaterial->RenderColor, 255)
                : emptyColor;
        }
    }

    ////// TEST: initializing with checker pattern
    ////for (int y = 0; y < size.height; ++y)
    ////{
    ////    for (int x = 0; x < size.width; ++x)
    ////    {
    ////        rgbaColor color;
    ////        if (x == 0 || x == size.width - 1 || y == 0 || y == size.height - 1)
    ////            color = rgbaColor(0, 0, 255, 255);
    ////        else
    ////            color = rgbaColor(
    ////                ((x + y) % 2) ? 255 : 0,
    ////                ((x + y) % 2) ? 0 : 255,
    ////                0,
    ////                255);

    ////        (*mStructuralRenderColorTexture)[ImageCoordinates(x, y)] = color;
    ////    }
    ////}
}

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