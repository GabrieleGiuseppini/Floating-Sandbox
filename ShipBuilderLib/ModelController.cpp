/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

#include <cassert>
#include <queue>

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
    /////
    , mIsStructuralLayerInEphemeralVisualization(false)
    , mIsElectricalLayerInEphemeralVisualization(false)
{
    // Model is not dirty now
    assert(!mModel.GetIsDirty());

    // Initialize layers
    InitializeStructuralLayer();
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
        std::move(*mModel.CloneStructuralLayer()),
        mModel.HasLayer(LayerType::Electrical)
            ? mModel.CloneElectricalLayer()
            : nullptr,
        nullptr, // TODOHERE
        mModel.HasLayer(LayerType::Texture)
            ? mModel.CloneTextureLayer()
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

    InitializeStructuralLayer();

    UpdateStructuralLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });

    mIsStructuralLayerInEphemeralVisualization = false;
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.SetStructuralLayer();

    InitializeStructuralLayer();

    UpdateStructuralLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });

    mIsStructuralLayerInEphemeralVisualization = false;
}

void ModelController::StructuralRegionFill(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    //
    // Update model
    //

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            WriteStructuralParticle(ShipSpaceCoordinates(x, y), material);
        }
    }

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization(region);
}

std::optional<ShipSpaceRect> ModelController::StructuralRegionFlood(
    ShipSpaceCoordinates const & start,
    StructuralMaterial const * material)
{
    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    StructuralMaterial const * const materialToFlood = structuralLayerBuffer[start].Material;

    if (material == materialToFlood)
    {
        // Nop
        return std::nullopt;
    }

    //
    // Init visit from this point
    //

    ShipSpaceRect affectedRect(start);

    std::queue<ShipSpaceCoordinates> pointsToPropagateFrom;
    pointsToPropagateFrom.push(start);

    //
    // Propagate from this point
    //

    ShipSpaceSize const shipSize = mModel.GetShipSize();

    while (!pointsToPropagateFrom.empty())
    {
        // Pop point that we have to propagate from
        auto const currentPoint = pointsToPropagateFrom.front();
        pointsToPropagateFrom.pop();

        // Visit point
        WriteStructuralParticle(currentPoint, material);
        affectedRect.UnionWith(currentPoint);

        // Push neighbors

        ShipSpaceCoordinates neighborCoords = { currentPoint.x - 1, currentPoint.y };
        if (neighborCoords.IsInSize(shipSize) && structuralLayerBuffer[neighborCoords].Material == materialToFlood)
        {
            pointsToPropagateFrom.push(neighborCoords);
        }

        neighborCoords = { currentPoint.x + 1, currentPoint.y };
        if (neighborCoords.IsInSize(shipSize) && structuralLayerBuffer[neighborCoords].Material == materialToFlood)
        {
            pointsToPropagateFrom.push(neighborCoords);
        }

        neighborCoords = { currentPoint.x, currentPoint.y - 1};
        if (neighborCoords.IsInSize(shipSize) && structuralLayerBuffer[neighborCoords].Material == materialToFlood)
        {
            pointsToPropagateFrom.push(neighborCoords);
        }

        neighborCoords = { currentPoint.x, currentPoint.y + 1};
        if (neighborCoords.IsInSize(shipSize) && structuralLayerBuffer[neighborCoords].Material == materialToFlood)
        {
            pointsToPropagateFrom.push(neighborCoords);
        }
    }

    LogMessage("TODOTEST: Flooded; rect=", affectedRect.ToString());

    return affectedRect;
}

void ModelController::RestoreStructuralLayer(
    StructuralLayerData && sourceLayerRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.GetStructuralLayer().Buffer.BlitFromRegion(
        sourceLayerRegion.Buffer,
        sourceRegion,
        targetOrigin);

    //
    // Re-initialize layer (analyses)
    //

    InitializeStructuralLayer();

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization({ targetOrigin, sourceRegion.size });
}

void ModelController::StructuralRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    //
    // Update model with just material - no analyses
    //

    auto & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            structuralLayerBuffer[ShipSpaceCoordinates(x, y)].Material = material;
        }
    }

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization(region);

    // Remember we are in temp visualization now
    mIsStructuralLayerInEphemeralVisualization = true;
}

void ModelController::RestoreStructuralLayerRegionForEphemeralVisualization(
    StructuralLayerData const & sourceLayerRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(mIsStructuralLayerInEphemeralVisualization);

    //
    // Restore model, and nothing else
    //

    mModel.GetStructuralLayer().Buffer.BlitFromRegion(
        sourceLayerRegion.Buffer,
        sourceRegion,
        targetOrigin);

    //
    // Update visualization
    //

    UpdateStructuralLayerVisualization({ targetOrigin, sourceRegion.size });

    // Remember we are not anymore in temp visualization
    mIsStructuralLayerInEphemeralVisualization = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Electrical
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    InitializeElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.SetElectricalLayer(/*TODO*/);

    InitializeElectricalLayer();

    UpdateElectricalLayerVisualization({ ShipSpaceCoordinates(0, 0), mModel.GetShipSize() });

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    assert(!mModel.HasLayer(LayerType::Electrical));

    InitializeElectricalLayer();

    mElectricalLayerVisualizationTexture.reset();
    mDirtyElectricalLayerVisualizationRegion.reset();

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::ElectricalRegionFill(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(!mIsElectricalLayerInEphemeralVisualization);

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

std::optional<ShipSpaceRect> ModelController::ElectricalRegionFlood(
    ShipSpaceCoordinates const & start,
    ElectricalMaterial const * material)
{
    // TODOHERE
    return std::nullopt;
}

void ModelController::RestoreElectricalLayer(
    ElectricalLayerData && sourceLayerRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(!mIsElectricalLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.GetElectricalLayer().Buffer.BlitFromRegion(
        sourceLayerRegion.Buffer,
        sourceRegion,
        targetOrigin);

    mModel.GetElectricalLayer().Panel = std::move(sourceLayerRegion.Panel);

    //
    // Re-initialize layer (analyses and instance IDs)
    //

    InitializeElectricalLayer();

    //
    // Update visualization
    //

    UpdateElectricalLayerVisualization({ targetOrigin, sourceRegion.size });
}

void ModelController::ElectricalRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(!mIsElectricalLayerInEphemeralVisualization);

    //
    // Update model just with material - no instance ID, no analyses, no panel
    //

    auto & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            electricalLayerBuffer[ShipSpaceCoordinates(x, y)].Material = material;
        }
    }

    //
    // Update visualization
    //

    UpdateElectricalLayerVisualization(region);

    // Remember we are in temp visualization now
    mIsElectricalLayerInEphemeralVisualization = true;
}

void ModelController::RestoreElectricalLayerRegionForEphemeralVisualization(
    ElectricalLayerData const & sourceLayerRegion,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(mIsElectricalLayerInEphemeralVisualization);

    //
    // Restore model, and nothing else
    //

    mModel.GetElectricalLayer().Buffer.BlitFromRegion(
        sourceLayerRegion.Buffer,
        sourceRegion,
        targetOrigin);

    /* TODOOLD
    // The source region is entirely in the source buffer
    assert(sourceRegion.IsContainedInRect({ {0, 0}, sourceLayerRegion.Buffer.Size }));

    // The target origin plus the region size are within this buffer
    assert(ShipSpaceRect(targetOrigin, sourceRegion.size).IsContainedInRect({ {0, 0}, mModel.GetElectricalLayer().Buffer.Size }));

    auto & sourceElectricalLayerBufferRegion = sourceLayerRegion.Buffer;
    auto & targetElectricalLayerBuffer = mModel.GetElectricalLayer().Buffer;

    for (int y = 0; y < sourceRegion.size.height; ++y)
    {
        for (int x = 0; x < sourceRegion.size.width; ++x)
        {
            targetElectricalLayerBuffer[{targetOrigin.x + x, targetOrigin.y + y }]
                = sourceElectricalLayerBufferRegion[{sourceRegion.origin.x + x, sourceRegion.origin.y + y}];
        }
    }
    */

    //
    // Update visualization
    //

    UpdateElectricalLayerVisualization({ targetOrigin, sourceRegion.size });

    // Remember we are not anymore in temp visualization
    mIsElectricalLayerInEphemeralVisualization = false;
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

void ModelController::InitializeStructuralLayer()
{
    // FUTUREWORK
}

void ModelController::InitializeElectricalLayer()
{
    // Reset factory
    mElectricalElementInstanceIndexFactory.Reset();

    // Reset particle count
    mElectricalParticleCount = 0;

    if (mModel.HasLayer(LayerType::Electrical))
    {
        // Register existing instance indices with factory, and initialize running analysis
        auto const & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;
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

void ModelController::WriteStructuralParticle(
    ShipSpaceCoordinates const & coords,
    StructuralMaterial const * material)
{
    //
    // FutureWork:
    // - Here we will also implement running analyses
    //

    auto & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
    structuralLayerBuffer[coords] = StructuralElement(material);
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

    auto & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;

    auto const & oldElement = electricalLayerBuffer[coords];

    // Decide instance index
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

    // Update electrical element count
    if (material != nullptr)
    {
        if (oldElement.Material == nullptr)
        {
            ++mElectricalParticleCount;
        }
    }
    else
    {
        if (oldElement.Material != nullptr)
        {
            assert(mElectricalParticleCount > 0);
            --mElectricalParticleCount;
        }
    }

    // Store
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

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
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

    auto const & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;
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