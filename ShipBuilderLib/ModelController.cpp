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

    // Prepare all visualizations
    ShipSpaceRect const wholeShipSpace = ShipSpaceRect({ 0, 0 }, mModel.GetShipSize());
    UpdateStructuralLayerVisualization(wholeShipSpace);
    UpdateElectricalLayerVisualization(wholeShipSpace);
    UpdateRopesLayerVisualization(wholeShipSpace);
    UpdateTextureLayerVisualization(wholeShipSpace);

    // Upload visualization
    UploadVisualization();
}

ShipDefinition ModelController::MakeShipDefinition() const
{
    return ShipDefinition(
        mModel.GetShipSize(),
        std::move(*mModel.CloneStructuralLayerBuffer()),
        nullptr, // TODOHERE
        nullptr, // TODOHERE
        mModel.HasLayer(LayerType::Texture)
            ? mModel.CloneTextureLayerBuffer()
            : nullptr,
        mModel.GetShipMetadata(),
        ShipPhysicsData(), // TODOHERE
        std::nullopt);
}

void ModelController::UploadVisualization()
{
    //
    // Upload visualizations that are dirty
    //

    if (mDirtyStructuralLayerVisualizationRegion.has_value())
    {
        assert(mStructuralLayerVisualizationTexture);
        mView.UploadStructuralLayerVisualizationTexture(*mStructuralLayerVisualizationTexture);

        mDirtyStructuralLayerVisualizationRegion.reset();
    }

    if (mDirtyElectricalLayerVisualizationRegion.has_value())
    {
        assert(mElectricalLayerVisualizationTexture);
        // TODO: upload to view

        mDirtyElectricalLayerVisualizationRegion.reset();
    }

    // TODO: other layers
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Structural
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewStructuralLayer()
{
    mModel.NewStructuralLayer();

    UpdateStructuralLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.SetStructuralLayer();

    UpdateStructuralLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::StructuralRegionFill(
    StructuralElement const & element,
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
            structuralLayerBuffer[ShipSpaceCoordinates(x, y)] = element;
        }
    }

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization(region);
}

void ModelController::StructuralRegionReplace(
    StructuralLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    assert(mModel.HasLayer(LayerType::Structural));

    //
    // Update model
    //

    mModel.GetStructuralLayerBuffer().BlitFromRegion(
        layerBufferRegion,
        { {0, 0}, layerBufferRegion.Size },
        origin);

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization({ origin, layerBufferRegion.Size });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Electrical
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.SetElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    mElectricalLayerVisualizationTexture.reset();
    mDirtyElectricalLayerVisualizationRegion.reset();
}

void ModelController::ElectricalRegionFill(
    ElectricalElement const & element,
    ShipSpaceRect const & region)
{
    // TODOHERE - copy from Structural
}

void ModelController::ElectricalRegionReplace(
    ElectricalLayerBuffer const & layerBufferRegion,
    ShipSpaceCoordinates const & origin)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    // TODOHERE - copy from Structural
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ropes
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewRopesLayer()
{
    mModel.NewRopesLayer();

    UpdateRopesLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.SetRopesLayer();

    UpdateRopesLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    // TODO: remove visualization members
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Texture
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();

    UpdateTextureLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::SetTextureLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.SetTextureLayer();

    UpdateTextureLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::RemoveTextureLayer()
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.RemoveTextureLayer();

    // TODO: remove visualization members
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::UpdateStructuralLayerVisualization(ShipSpaceRect const & region)
{
    assert(mModel.HasLayer(LayerType::Structural));

    // Make sure we have a texture
    if (!mStructuralLayerVisualizationTexture)
    {
        mStructuralLayerVisualizationTexture = std::make_unique<RgbaImageData>(mModel.GetShipSize().width, mModel.GetShipSize().height);
    }

    assert(mStructuralLayerVisualizationTexture->Size.width == mModel.GetShipSize().width
        && mStructuralLayerVisualizationTexture->Size.height == mModel.GetShipSize().height);

    // Update visualization

    // TODO: check current visualization settings and decide how to visualize

    rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 255);

    StructuralLayerBuffer const & structuralLayerBuffer = mModel.GetStructuralLayerBuffer();
    RgbaImageData & structuralRenderColorTexture = *mStructuralLayerVisualizationTexture;

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

    // Remember dirty region
    ImageRect const imageRegion = ImageRect({ region.origin.x, region.origin.y }, { region.size.width, region.size.height });
    if (!mDirtyStructuralLayerVisualizationRegion.has_value())
    {
        mDirtyStructuralLayerVisualizationRegion = imageRegion;
    }
    else
    {
        mDirtyStructuralLayerVisualizationRegion->UnionWith(imageRegion);
    }
}

void ModelController::UpdateElectricalLayerVisualization(ShipSpaceRect const & region)
{
    if (!mModel.HasLayer(LayerType::Electrical))
    {
        return;
    }

    // Make sure we have a texture
    if (!mElectricalLayerVisualizationTexture)
    {
        mElectricalLayerVisualizationTexture = std::make_unique<RgbaImageData>(mModel.GetShipSize().width, mModel.GetShipSize().height);
    }

    assert(mElectricalLayerVisualizationTexture->Size.width == mModel.GetShipSize().width
        && mElectricalLayerVisualizationTexture->Size.height == mModel.GetShipSize().height);

    // TODO
}

void ModelController::UpdateRopesLayerVisualization(ShipSpaceRect const & region)
{
    if (!mModel.HasLayer(LayerType::Ropes))
    {
        return;
    }

    // TODO
}

void ModelController::UpdateTextureLayerVisualization(ShipSpaceRect const & region)
{
    if (!mModel.HasLayer(LayerType::Texture))
    {
        return;
    }

    // TODO
}

}