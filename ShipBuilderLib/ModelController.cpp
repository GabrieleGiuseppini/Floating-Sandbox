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
    , mElectricalElementInstanceIndexFactory()
    , mElectricalParticleCount(0)
{
    // Model is not dirty now
    assert(!mModel.GetIsDirty());

    // Initialize layers
    InitializeElectricalLayer();

    // Prepare all visualizations
    ShipSpaceRect const wholeShipSpace = ShipSpaceRect({ 0, 0 }, mModel.GetShipSize());
    UpdateStructuralLayerVisualization(wholeShipSpace);
    UpdateElectricalLayerVisualization(wholeShipSpace);
    UpdateRopesLayerVisualization(wholeShipSpace);
    UpdateTextureLayerVisualization(wholeShipSpace);
}

ShipDefinition ModelController::MakeShipDefinition() const
{
    return ShipDefinition(
        mModel.GetShipSize(),
        std::move(*mModel.CloneStructuralLayerBuffer()),
        mModel.HasLayer(LayerType::Electrical)
            ? mModel.CloneElectricalLayerBuffer()
            : nullptr,
        nullptr, // TODOHERE
        mModel.HasLayer(LayerType::Texture)
            ? mModel.CloneTextureLayerBuffer()
            : nullptr,
        mModel.GetShipMetadata(),
        mModel.GetShipPhysicsData(),
        std::nullopt); // TODOHERE
}

void ModelController::UploadVisualization()
{
    //
    // Upload visualizations that are dirty, and
    // remove visualizations that are not needed
    //

    if (mDirtyStructuralLayerVisualizationRegion.has_value())
    {
        assert(mStructuralLayerVisualizationTexture);
        mView.UploadStructuralLayerVisualizationTexture(*mStructuralLayerVisualizationTexture);

        mDirtyStructuralLayerVisualizationRegion.reset();
    }

    if (mElectricalLayerVisualizationTexture)
    {
        if (mDirtyElectricalLayerVisualizationRegion.has_value())
        {
            mView.UploadElectricalLayerVisualizationTexture(*mElectricalLayerVisualizationTexture);

            mDirtyElectricalLayerVisualizationRegion.reset();
        }
    }
    else
    {
        assert(!mDirtyElectricalLayerVisualizationRegion.has_value());

        if (mView.HasElectricalLayerVisualizationTexture())
        {
            mView.RemoveElectricalLayerVisualizationTexture();
        }
    }

    // TODOHERE: other layers
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
    ShipSpaceRect const & region,
    StructuralElement const & element)
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
    StructuralLayerBuffer const & sourceLayerBufferRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::Structural));

    //
    // Update model
    //

    mModel.GetStructuralLayerBuffer().BlitFromRegion(
        sourceLayerBufferRegion,
        sourceRegion,
        targetOrigin);

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization({ targetOrigin, sourceRegion.size });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Electrical
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    InitializeElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.SetElectricalLayer(/*TODO*/);

    InitializeElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    assert(!mModel.HasLayer(LayerType::Electrical));

    InitializeElectricalLayer();

    mElectricalLayerVisualizationTexture.reset();
    mDirtyElectricalLayerVisualizationRegion.reset();
}

void ModelController::ElectricalRegionFill(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    //
    // Update model
    //

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            WriteElectricalParticle(ShipSpaceCoordinates(x, y), material);
        }
    }

    //
    // Update visualization
    //

    UpdateElectricalLayerVisualization(region);
}

void ModelController::ElectricalRegionReplace(
    ElectricalLayerBuffer const & sourceLayerBufferRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    // TODOHERE

    assert(mModel.HasLayer(LayerType::Electrical));

    //
    // Update model
    //

    // The source region is entirely in the source buffer
    assert(sourceRegion.IsContainedInRect({ {0, 0}, sourceLayerBufferRegion.Size }));

    // The target origin plus the region size are within this buffer
    assert(ShipSpaceRect(targetOrigin, sourceRegion.size).IsContainedInRect({ {0, 0}, mModel.GetElectricalLayerBuffer().Size }));

    ElectricalLayerBuffer & targetElectricalLayerBuffer = mModel.GetElectricalLayerBuffer();

    for (int y = 0; y < sourceRegion.size.height; ++y)
    {
        for (int x = 0; x < sourceRegion.size.width; ++x)
        {
            targetElectricalLayerBuffer[{targetOrigin.x + x, targetOrigin.y + y }]
                = sourceLayerBufferRegion[{sourceRegion.origin.x + x, sourceRegion.origin.y + y}];
        }
    }

    //
    // Update visualization
    //

    UpdateElectricalLayerVisualization({ targetOrigin, sourceRegion.size });
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

void ModelController::InitializeElectricalLayer()
{
    // Reset factory
    mElectricalElementInstanceIndexFactory.Reset();

    // Reset particle count
    mElectricalParticleCount = 0;

    if (mModel.HasLayer(LayerType::Electrical))
    {
        // Register existing instance indices with factory, and initialize running analysis
        ElectricalLayerBuffer const & electricalLayerBuffer = mModel.GetElectricalLayerBuffer();
        for (size_t i = 0; i < electricalLayerBuffer.Size.GetLinearSize(); ++i)
        {
            if (electricalLayerBuffer.Data[i].Material != nullptr)
            {
                ++mElectricalParticleCount;

                if (electricalLayerBuffer.Data[i].Material->IsInstanced)
                {
                    assert(electricalLayerBuffer.Data[i].InstanceIndex != NoneElectricalElementInstanceIndex);
                    mElectricalElementInstanceIndexFactory.RegisterIndex(electricalLayerBuffer.Data[i].InstanceIndex);
                }
            }
        }
    }
}

void ModelController::WriteElectricalParticle(
    ShipSpaceCoordinates const & coords,
    ElectricalMaterial const * material)
{
    //
    // FutureWork:
    // - Here we will also implement running analyses, e.g. update the count of particles
    // - Here we will also take care of electrical panel: new/removed/updated-type components
    //

    ElectricalLayerBuffer & electricalLayerBuffer = mModel.GetElectricalLayerBuffer();

    auto const & oldElement = electricalLayerBuffer[coords];

    ElectricalElementInstanceIndex instanceIndex;
    if (oldElement.Material == nullptr
        || !oldElement.Material->IsInstanced)
    {
        if (material != nullptr
            && material->IsInstanced)
        {
            // New instanced element...

            // ...new instance index
            instanceIndex = mElectricalElementInstanceIndexFactory.MakeNewIndex();
        }
        else
        {
            // None instanced...

            // ...keep it none
            instanceIndex = NoneElectricalElementInstanceIndex;
        }
    }
    else
    {
        assert(oldElement.Material->IsInstanced);
        assert(oldElement.InstanceIndex != NoneElectricalElementInstanceIndex);

        if (material == nullptr
            || !material->IsInstanced)
        {
            // Old instanced, new one not...

            // ...disappeared instance index
            mElectricalElementInstanceIndexFactory.DisposeIndex(oldElement.InstanceIndex);
            instanceIndex = NoneElectricalElementInstanceIndex;
        }
        else
        {
            // Both instanced...

            // ...keep old instanceIndex
            instanceIndex = oldElement.InstanceIndex;
        }
    }

    LogMessage("TODOTEST: WriteElectricalParticle: InstanceId=", instanceIndex);

    electricalLayerBuffer[coords] = ElectricalElement(material, instanceIndex);
}

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

    rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 255); // Fully opaque

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

    // Update visualization

    // TODO: check current visualization settings and decide how to visualize

    rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 0); // Fully transparent

    ElectricalLayerBuffer const & electricalLayerBuffer = mModel.GetElectricalLayerBuffer();
    RgbaImageData & electricalRenderColorTexture = *mElectricalLayerVisualizationTexture;

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            auto const electricalMaterial = electricalLayerBuffer[{x, y}].Material;

            electricalRenderColorTexture[{x, y}] = electricalMaterial != nullptr
                ? rgbaColor(electricalMaterial->RenderColor, 255)
                : emptyColor;
        }
    }

    // Remember dirty region
    ImageRect const imageRegion = ImageRect({ region.origin.x, region.origin.y }, { region.size.width, region.size.height });
    if (!mDirtyElectricalLayerVisualizationRegion.has_value())
    {
        mDirtyElectricalLayerVisualizationRegion = imageRegion;
    }
    else
    {
        mDirtyElectricalLayerVisualizationRegion->UnionWith(imageRegion);
    }
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