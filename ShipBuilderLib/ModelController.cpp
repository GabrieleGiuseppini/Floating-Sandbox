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
    ShipTexturizer const & shipTexturizer)
{
    Model model = Model(shipSpaceSize, shipName);

    return std::unique_ptr<ModelController>(
        new ModelController(
            std::move(model),
            shipTexturizer));
}

std::unique_ptr<ModelController> ModelController::CreateForShip(
    ShipDefinition && shipDefinition,
    ShipTexturizer const & shipTexturizer)
{
    Model model = Model(std::move(shipDefinition));

    return std::unique_ptr<ModelController>(
        new ModelController(
            std::move(model),
            shipTexturizer));
}

ModelController::ModelController(
    Model && model,
    ShipTexturizer const & shipTexturizer)
    : mModel(std::move(model))
    , mShipTexturizer(shipTexturizer)
    , mElectricalElementInstanceIndexFactory()
    , mElectricalParticleCount(0)
    /////
    , mGameVisualizationMode(GameVisualizationModeType::None)
    , mGameVisualizationAutoTexturizationTexture()
    , mGameVisualizationTexture()
    , mGameVisualizationTextureMagnificationFactor(0)
    , mStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::None)
    , mStructuralLayerVisualizationTexture()
    , mElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::None)
    , mElectricalLayerVisualizationTexture()
    , mRopesLayerVisualizationMode(RopesLayerVisualizationModeType::None)
    , mTextureLayerVisualizationMode(TextureLayerVisualizationModeType::None)
    /////
    , mIsStructuralLayerInEphemeralVisualization(false)
    , mIsElectricalLayerInEphemeralVisualization(false)
    , mIsRopesLayerInEphemeralVisualization(false)
{
    mDirtyVisualizationRegions.fill(std::nullopt);

    // Model is not dirty now
    assert(!mModel.GetIsDirty());

    // Initialize layers' analyses
    InitializeStructuralLayerAnalysis();
    InitializeElectricalLayerAnalysis();
    InitializeRopesLayerAnalysis();
}

ShipDefinition ModelController::MakeShipDefinition() const
{
    return mModel.MakeShipDefinition();
}

std::unique_ptr<RgbaImageData> ModelController::MakePreview() const
{
    assert(mModel.HasLayer(LayerType::Structural));

    auto previewTexture = std::make_unique<RgbaImageData>(ImageSize(mModel.GetShipSize().width, mModel.GetShipSize().height));

    RenderStructureInto(
        GetWholeShipRect(),
        *previewTexture);

    return previewTexture;
}

std::optional<ShipSpaceRect> ModelController::CalculateBoundingBox() const
{
    std::optional<ShipSpaceRect> boundingBox;

    //
    // Structural layer
    //

    assert(mModel.HasLayer(LayerType::Structural));

    StructuralLayerData const & structuralLayer = mModel.GetStructuralLayer();

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            ShipSpaceCoordinates coords(x, y);

            if (structuralLayer.Buffer[{x, y}].Material != nullptr)
            {
                if (!boundingBox.has_value())
                    boundingBox = ShipSpaceRect(coords);
                else
                    boundingBox->UnionWith(coords);
            }
        }
    }

    //
    // Ropes layer
    //

    if (mModel.HasLayer(LayerType::Ropes))
    {
        for (auto const & e : mModel.GetRopesLayer().Buffer)
        {
            if (!boundingBox.has_value())
                boundingBox = ShipSpaceRect(e.StartCoords);
            else
                boundingBox->UnionWith(e.StartCoords);

            boundingBox->UnionWith(e.EndCoords);
        }
    }

    return boundingBox;
}

ModelValidationResults ModelController::ValidateModel() const
{
    std::vector<ModelValidationIssue> issues;

    //
    // Visit structural layer
    //

    assert(mModel.HasLayer(LayerType::Structural));

    StructuralLayerData const & structuralLayer = mModel.GetStructuralLayer();

    size_t structuralParticlesCount = 0;

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            if (structuralLayer.Buffer[{x, y}].Material != nullptr)
            {
                ++structuralParticlesCount;
            }
        }
    }

    //
    // Check: empty structural layer
    //

    issues.emplace_back(
        ModelValidationIssue::CheckClassType::EmptyStructuralLayer,
        (structuralParticlesCount == 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);

    if (structuralParticlesCount != 0)
    {
        //
        // Check: structure too large
        //

        size_t constexpr MaxStructuralParticles = 100000;

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::StructureTooLarge,
            (structuralParticlesCount > MaxStructuralParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
    }

    if (mModel.HasLayer(LayerType::Electrical))
    {
        //
        // Visit electrical layer
        //

        ElectricalLayerData const & electricalLayer = mModel.GetElectricalLayer();

        size_t electricalParticlesWithNoStructuralSubstratumCount = 0;
        size_t lightEmittingParticlesCount = 0;

        assert(structuralLayer.Buffer.Size == electricalLayer.Buffer.Size);
        for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
        {
            for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
            {
                auto const coords = ShipSpaceCoordinates(x, y);
                auto const electricalMaterial = electricalLayer.Buffer[coords].Material;
                if (electricalMaterial != nullptr
                    && structuralLayer.Buffer[coords].Material == nullptr)
                {
                    ++electricalParticlesWithNoStructuralSubstratumCount;

                    if (electricalMaterial->Luminiscence != 0.0f)
                    {
                        ++lightEmittingParticlesCount;
                    }
                }
            }
        }

        //
        // Check: electrical substratum
        //

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::MissingElectricalSubstratum,
            (electricalParticlesWithNoStructuralSubstratumCount > 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);

        //
        // Check: too many lights
        //

        size_t constexpr MaxLightEmittingParticles = 5000;

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::TooManyLights,
            (lightEmittingParticlesCount > MaxLightEmittingParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
    }

    return ModelValidationResults(std::move(issues));
}

void ModelController::Flip(DirectionType direction)
{
    // Structural layer
    {
        assert(mModel.HasLayer(LayerType::Structural));

        assert(!mIsStructuralLayerInEphemeralVisualization);

        mModel.GetStructuralLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mIsElectricalLayerInEphemeralVisualization);

        mModel.GetElectricalLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mIsRopesLayerInEphemeralVisualization);

        mModel.GetRopesLayer().Buffer.Flip(direction, mModel.GetShipSize());

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
    }

    // Texture layer
    if (mModel.HasLayer(LayerType::Texture))
    {
        mModel.GetTextureLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
    }

    //...and Game we do regardless, as there's always a structural layer at least
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
}

void ModelController::ResizeShip(
    ShipSpaceSize const & newSize,
    ShipSpaceCoordinates const & originOffset)
{
    //
    // Calculate "static" (remaining) rect - wrt old coordinates 
    //

    auto const originalShipRect = GetWholeShipRect();

    std::optional<ShipSpaceRect> staticShipRect = originalShipRect.MakeIntersectionWith(ShipSpaceRect(originOffset, newSize));

    if (staticShipRect.has_value())
    {
        // Make origin wrt old coords
        staticShipRect->origin.x = std::max(0, -originOffset.x);
        staticShipRect->origin.y = std::max(0, -originOffset.y);
    }


    //
    // Resize model
    //

    ShipSpaceRect newWholeShipRect({ 0, 0 }, newSize);

    mModel.SetShipSize(newSize);

    // Structural layer
    {
        assert(mModel.HasLayer(LayerType::Structural));

        assert(!mIsStructuralLayerInEphemeralVisualization);

        mModel.GetStructuralLayer().Buffer = mModel.GetStructuralLayer().Buffer.MakeReframed(
            newSize,
            originOffset,
            StructuralElement(nullptr));

        InitializeStructuralLayerAnalysis();

        // Initialize visualization
        mStructuralLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(newWholeShipRect);
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mIsElectricalLayerInEphemeralVisualization);

        // Panel
        if (staticShipRect.has_value())
        {
            for (int y = 0; y < mModel.GetElectricalLayer().Buffer.Size.height; ++y)
            {
                for (int x = 0; x < mModel.GetElectricalLayer().Buffer.Size.width; ++x)
                {
                    auto const coords = ShipSpaceCoordinates({ x, y });

                    auto const instanceIndex = mModel.GetElectricalLayer().Buffer[coords].InstanceIndex;
                    if (instanceIndex != NoneElectricalElementInstanceIndex
                        && !coords.IsInRect(*staticShipRect))
                    {
                        // This instanced element will be gone
                        auto searchIt = mModel.GetElectricalLayer().Panel.find(instanceIndex);
                        if (searchIt != mModel.GetElectricalLayer().Panel.end())
                        {
                            mModel.GetElectricalLayer().Panel.erase(searchIt);
                        }
                    }
                }
            }
        }
        else
        {
            mModel.GetElectricalLayer().Panel.clear();
        }

        // Elements
        mModel.GetElectricalLayer().Buffer = mModel.GetElectricalLayer().Buffer.MakeReframed(
            newSize,
            originOffset,
            ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));

        InitializeElectricalLayerAnalysis();

        // Initialize visualization
        mElectricalLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(newWholeShipRect);
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mIsRopesLayerInEphemeralVisualization);

        mModel.GetRopesLayer().Buffer.Reframe(newSize, originOffset);

        InitializeRopesLayerAnalysis();

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(newWholeShipRect);
    }

    // Texture layer
    if (mModel.HasLayer(LayerType::Texture))
    {
        // Calc rect in texture coordinates space, assuming the original ratio matches
#ifdef _DEBUG
        float const textureRatio = static_cast<float>(mModel.GetTextureLayer().Buffer.Size.width) / static_cast<float>(mModel.GetTextureLayer().Buffer.Size.height);
        float const shipRatio = static_cast<float>(originalShipRect.size.width) / static_cast<float>(originalShipRect.size.height);
#endif
        assert(std::abs(1.0f - textureRatio / shipRatio) < 0.1f);
        float const shipToImage = static_cast<float>(mModel.GetTextureLayer().Buffer.Size.width) / static_cast<float>(originalShipRect.size.width);
        ImageSize const imageNewSize = ImageSize::FromFloatRound(newSize.ToFloat() * shipToImage);
        ImageCoordinates imageOriginOffset = ImageCoordinates::FromFloatRound(originOffset.ToFloat() * shipToImage);

        mModel.GetTextureLayer().Buffer = mModel.GetTextureLayer().Buffer.MakeReframed(
            imageNewSize,
            imageOriginOffset,
            rgbaColor(0, 0, 0, 0));

        RegisterDirtyVisualization<VisualizationType::TextureLayer>(newWholeShipRect);
    }

    // Initialize game visualizations
    {
        mGameVisualizationTexture.reset();
        mGameVisualizationAutoTexturizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::Game>(newWholeShipRect);
    }

    assert(mModel.GetShipSize() == newSize);
    assert(GetWholeShipRect() == newWholeShipRect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Structural
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewStructuralLayer()
{
    mModel.NewStructuralLayer();

    InitializeStructuralLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());

    mIsStructuralLayerInEphemeralVisualization = false;
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.SetStructuralLayer(/*TODO*/);

    InitializeStructuralLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());

    mIsStructuralLayerInEphemeralVisualization = false;
}

StructuralLayerData ModelController::CloneStructuralLayer() const
{
    return mModel.CloneStructuralLayer();
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
            WriteParticle(ShipSpaceCoordinates(x, y), material);
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(region);
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(region);
}

std::optional<ShipSpaceRect> ModelController::StructuralFlood(
    ShipSpaceCoordinates const & start,
    StructuralMaterial const * material,
    bool doContiguousOnly)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect = Flood<LayerType::Structural>(
        start,
        material,
        doContiguousOnly,
        mModel.GetStructuralLayer());

    if (affectedRect.has_value())
    {
        //
        // Update visualization
        //

        RegisterDirtyVisualization<VisualizationType::Game>(*affectedRect);
        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(*affectedRect);
    }

    return affectedRect;
}

void ModelController::RestoreStructuralLayerRegion(
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
    // Re-initialize layer analysis
    //

    InitializeStructuralLayerAnalysis();

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(ShipSpaceRect(targetOrigin, sourceRegion.size));
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(ShipSpaceRect(targetOrigin, sourceRegion.size));
}

void ModelController::RestoreStructuralLayer(StructuralLayerData && sourceLayer)
{    
    assert(!mIsStructuralLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.RestoreStructuralLayer(std::move(sourceLayer));

    //
    // Re-initialize layer analysis
    //

    InitializeStructuralLayerAnalysis();

    //
    // Update visualization
    //

    mGameVisualizationTexture.reset();
    mGameVisualizationAutoTexturizationTexture.reset();
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    mStructuralLayerVisualizationTexture.reset();
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());
}

void ModelController::StructuralRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Structural));

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

    RegisterDirtyVisualization<VisualizationType::Game>(region);
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(region);

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

    RegisterDirtyVisualization<VisualizationType::Game>(ShipSpaceRect(targetOrigin, sourceRegion.size));
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(ShipSpaceRect(targetOrigin, sourceRegion.size));

    // Remember we are not anymore in temp visualization
    mIsStructuralLayerInEphemeralVisualization = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Electrical
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewElectricalLayer()
{
    mModel.NewElectricalLayer();

    InitializeElectricalLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    mModel.SetElectricalLayer(/*TODO*/);

    InitializeElectricalLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    InitializeElectricalLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

    mIsElectricalLayerInEphemeralVisualization = false;
}

std::unique_ptr<ElectricalLayerData> ModelController::CloneElectricalLayer() const
{
    return mModel.CloneElectricalLayer();
}

bool ModelController::IsElectricalParticleAllowedAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    return mModel.GetStructuralLayer().Buffer[coords].Material != nullptr;
}

std::optional<ShipSpaceRect> ModelController::TrimElectricalParticlesWithoutSubstratum()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(!mIsElectricalLayerInEphemeralVisualization);

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect;

    StructuralLayerData const & structuralLayer = mModel.GetStructuralLayer();
    ElectricalLayerData const & electricalLayer = mModel.GetElectricalLayer();

    assert(structuralLayer.Buffer.Size == electricalLayer.Buffer.Size);

    ElectricalMaterial const * nullMaterial = nullptr;

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            if (electricalLayer.Buffer[coords].Material != nullptr
                && structuralLayer.Buffer[coords].Material == nullptr)
            {
                WriteParticle(coords, nullMaterial);

                if (!affectedRect.has_value())
                {
                    affectedRect = ShipSpaceRect(coords);
                }
                else
                {
                    affectedRect->UnionWith(coords);
                }
            }
        }
    }

    //
    // Update visualization
    //

    if (affectedRect.has_value())
    {
        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(*affectedRect);
    }

    return affectedRect;
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
            WriteParticle(ShipSpaceCoordinates(x, y), material);
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(region);
}

void ModelController::RestoreElectricalLayerRegion(
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
    // Re-initialize layer analysis (and instance IDs)
    //

    InitializeElectricalLayerAnalysis();

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(ShipSpaceRect(targetOrigin, sourceRegion.size));
}

void ModelController::RestoreElectricalLayer(std::unique_ptr<ElectricalLayerData> sourceLayer)
{
    assert(!mIsElectricalLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.RestoreElectricalLayer(std::move(sourceLayer));

    //
    // Re-initialize layer analysis (and instance IDs)
    //

    InitializeElectricalLayerAnalysis();

    //
    // Update visualization
    //

    mElectricalLayerVisualizationTexture.reset();
    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());
}

void ModelController::ElectricalRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Electrical));

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

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(region);

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

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(ShipSpaceRect(targetOrigin, sourceRegion.size));

    // Remember we are not anymore in temp visualization
    mIsElectricalLayerInEphemeralVisualization = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ropes
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewRopesLayer()
{
    mModel.NewRopesLayer();

    InitializeRopesLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    mIsRopesLayerInEphemeralVisualization = false;
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    mModel.SetRopesLayer(/*TODO*/);

    InitializeRopesLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    mIsRopesLayerInEphemeralVisualization = false;
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    InitializeRopesLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    mIsRopesLayerInEphemeralVisualization = false;
}

std::unique_ptr<RopesLayerData> ModelController::CloneRopesLayer() const
{
    return mModel.CloneRopesLayer();
}

std::optional<size_t> ModelController::GetRopeElementIndexAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mIsRopesLayerInEphemeralVisualization);

    auto const searchIt = std::find_if(
        mModel.GetRopesLayer().Buffer.cbegin(),
        mModel.GetRopesLayer().Buffer.cend(),
        [&coords](RopeElement const & e)
        {
            return coords == e.StartCoords
                || coords == e.EndCoords;
        });

    if (searchIt != mModel.GetRopesLayer().Buffer.cend())
    {
        return std::distance(mModel.GetRopesLayer().Buffer.cbegin(), searchIt);
    }
    else
    {
        return std::nullopt;
    }
}

void ModelController::AddRope(
    ShipSpaceCoordinates const & startCoords,
    ShipSpaceCoordinates const & endCoords,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mIsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    AppendRope(
        startCoords,
        endCoords,
        material);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
}

void ModelController::MoveRopeEndpoint(
    size_t ropeElementIndex,
    ShipSpaceCoordinates const & oldCoords,
    ShipSpaceCoordinates const & newCoords)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mIsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    assert(ropeElementIndex < mModel.GetRopesLayer().Buffer.GetSize());

    MoveRopeEndpoint(
        mModel.GetRopesLayer().Buffer[ropeElementIndex],
        oldCoords,
        newCoords);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
}

bool ModelController::EraseRopeAt(ShipSpaceCoordinates const & coords)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mIsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    auto const srchIt = std::find_if(
        mModel.GetRopesLayer().Buffer.cbegin(),
        mModel.GetRopesLayer().Buffer.cend(),
        [&coords](auto const & e)
        {
            return e.StartCoords == coords
                || e.EndCoords == coords;
        });

    if (srchIt != mModel.GetRopesLayer().Buffer.cend())
    {
        // Remove
        mModel.GetRopesLayer().Buffer.Erase(srchIt);

        // Update visualization
        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

        return true;
    }
    else
    {
        return false;
    }
}

void ModelController::RestoreRopesLayer(std::unique_ptr<RopesLayerData> sourceLayer)
{
    assert(!mIsRopesLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.RestoreRopesLayer(std::move(sourceLayer));

    //
    // Re-initialize layer analysis
    //

    InitializeRopesLayerAnalysis();

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
}

void ModelController::AddRopeForEphemeralVisualization(
    ShipSpaceCoordinates const & startCoords,
    ShipSpaceCoordinates const & endCoords,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    //
    // Update model with just material - no analyses
    //

    AppendRope(
        startCoords,
        endCoords,
        material);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are in temp visualization now
    mIsRopesLayerInEphemeralVisualization = true;
}

void ModelController::ModelController::MoveRopeEndpointForEphemeralVisualization(
    size_t ropeElementIndex,
    ShipSpaceCoordinates const & oldCoords,
    ShipSpaceCoordinates const & newCoords)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    //
    // Update model with jsut movement - no analyses
    //

    assert(ropeElementIndex < mModel.GetRopesLayer().Buffer.GetSize());

    MoveRopeEndpoint(
        mModel.GetRopesLayer().Buffer[ropeElementIndex],
        oldCoords,
        newCoords);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are in temp visualization now
    mIsRopesLayerInEphemeralVisualization = true;
}

void ModelController::RestoreRopesLayerForEphemeralVisualization(RopesLayerData const & sourceLayer)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(mIsRopesLayerInEphemeralVisualization);

    //
    // Restore model, and nothing else
    //

    mModel.GetRopesLayer().Buffer = sourceLayer.Buffer;

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are not anymore in temp visualization
    mIsRopesLayerInEphemeralVisualization = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Texture
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
}

void ModelController::SetTextureLayer(
    TextureLayerData && textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    mModel.SetTextureLayer(std::move(textureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
}

void ModelController::RemoveTextureLayer()
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.RemoveTextureLayer();
    mModel.GetShipMetadata().ArtCredits.reset();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
}

std::unique_ptr<TextureLayerData> ModelController::CloneTextureLayer() const
{
    return mModel.CloneTextureLayer();
}

void ModelController::RestoreTextureLayer(
    std::unique_ptr<TextureLayerData> textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    //
    // Restore model
    //
    
    mModel.RestoreTextureLayer(std::move(textureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    //
    // Update visualization
    //

    mGameVisualizationTexture.reset();
    mGameVisualizationAutoTexturizationTexture.release();
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Visualizations
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::SetGameVisualizationMode(GameVisualizationModeType mode)
{
    if (mode == mGameVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != GameVisualizationModeType::None)
    {
        if (mode != GameVisualizationModeType::AutoTexturizationMode)
        {
            mGameVisualizationAutoTexturizationTexture.reset();
        }

        mGameVisualizationMode = mode;

        RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    }
    else
    {
        // Shutdown game visualization
        mGameVisualizationMode = GameVisualizationModeType::None;
        mGameVisualizationAutoTexturizationTexture.reset();
        assert(mGameVisualizationTexture);
        mGameVisualizationTexture.reset();
    }
}

void ModelController::ForceWholeGameVisualizationRefresh()
{
    if (mGameVisualizationMode != GameVisualizationModeType::None)
    {
        RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    }
}

void ModelController::SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode)
{
    if (mode == mStructuralLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != StructuralLayerVisualizationModeType::None)
    {
        mStructuralLayerVisualizationMode = mode;

        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());
    }
    else
    {
        // Shutdown structural visualization
        mStructuralLayerVisualizationMode = StructuralLayerVisualizationModeType::None;
        assert(mStructuralLayerVisualizationTexture);
        mStructuralLayerVisualizationTexture.reset();
    }
}

void ModelController::SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode)
{
    if (mode == mElectricalLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != ElectricalLayerVisualizationModeType::None)
    {
        mElectricalLayerVisualizationMode = mode;

        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());
    }
    else
    {
        // Shutdown electrical visualization
        mElectricalLayerVisualizationMode = ElectricalLayerVisualizationModeType::None;
        assert(mElectricalLayerVisualizationTexture);
        mElectricalLayerVisualizationTexture.reset();
    }
}

void ModelController::SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode)
{
    if (mode == mRopesLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != RopesLayerVisualizationModeType::None)
    {
        mRopesLayerVisualizationMode = mode;

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
    }
    else
    {
        // Shutdown ropes visualization
        mRopesLayerVisualizationMode = RopesLayerVisualizationModeType::None;
    }
}

void ModelController::SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode)
{
    if (mode == mTextureLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != TextureLayerVisualizationModeType::None)
    {
        mTextureLayerVisualizationMode = mode;

        RegisterDirtyVisualization<VisualizationType::TextureLayer>(GetWholeShipRect());
    }
    else
    {
        // Shutdown texture visualization
        mTextureLayerVisualizationMode = TextureLayerVisualizationModeType::None;
    }
}

void ModelController::UpdateVisualizations(View & view)
{
    //
    // Update and upload visualizations that are dirty, and
    // remove visualizations that are not needed
    //

    // Game

    if (mGameVisualizationMode != GameVisualizationModeType::None)
    {
        if (!mGameVisualizationTexture)
        {
            // Initialize game visualization texture
            mGameVisualizationTextureMagnificationFactor = ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(mModel.GetShipSize());
            ImageSize const textureSize = ImageSize(
                mModel.GetShipSize().width * mGameVisualizationTextureMagnificationFactor,
                mModel.GetShipSize().height * mGameVisualizationTextureMagnificationFactor);

            mGameVisualizationTexture = std::make_unique<RgbaImageData>(textureSize);
        }
        
        if (mGameVisualizationMode == GameVisualizationModeType::AutoTexturizationMode && !mGameVisualizationAutoTexturizationTexture)
        {
            // Initialize auto-texturization texture
            mGameVisualizationAutoTexturizationTexture = std::make_unique<RgbaImageData>(mGameVisualizationTexture->Size);
        }

        auto const & dirtyShipRegion = mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::Game)];
        if (dirtyShipRegion.has_value())
        {
            // Update visualization
            ImageRect const dirtyTextureRegion = UpdateGameVisualization(*dirtyShipRegion);

            // Upload visualization
            if (dirtyTextureRegion != mGameVisualizationTexture->Size)
            {
                //
                // For better performance, we only upload the dirty sub-texture
                //

                auto subTexture = RgbaImageData(dirtyTextureRegion.size);
                subTexture.BlitFromRegion(
                    *mGameVisualizationTexture,
                    dirtyTextureRegion,
                    { 0, 0 });

                view.UpdateGameVisualizationTexture(
                    subTexture,
                    dirtyTextureRegion.origin);
            }
            else
            {
                view.UploadGameVisualization(*mGameVisualizationTexture);
            }
        }
    }
    else
    {
        assert(!mGameVisualizationTexture);

        if (view.HasGameVisualization())
        {
            view.RemoveGameVisualization();
        }
    }

    mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::Game)].reset();

    // Structural

    if (mStructuralLayerVisualizationMode != StructuralLayerVisualizationModeType::None)
    {
        if (!mStructuralLayerVisualizationTexture)
        {
            // Initialize structural visualization
            mStructuralLayerVisualizationTexture = std::make_unique<RgbaImageData>(ImageSize(mModel.GetShipSize().width, mModel.GetShipSize().height));
        }

        auto const & dirtyShipRegion = mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::StructuralLayer)];
        if (dirtyShipRegion.has_value())
        {
            // Refresh viz mode
            if (mStructuralLayerVisualizationMode == StructuralLayerVisualizationModeType::MeshMode)
            {
                view.SetStructuralLayerVisualizationDrawMode(View::StructuralLayerVisualizationDrawMode::MeshMode);
            }
            else
            {
                assert(mStructuralLayerVisualizationMode == StructuralLayerVisualizationModeType::PixelMode);
                view.SetStructuralLayerVisualizationDrawMode(View::StructuralLayerVisualizationDrawMode::PixelMode);
            }

            // Update visualization
            UpdateStructuralLayerVisualization(*dirtyShipRegion);

            // Upload visualization
            view.UploadStructuralLayerVisualization(*mStructuralLayerVisualizationTexture);
        }
    }
    else
    {
        assert(!mStructuralLayerVisualizationTexture);

        if (view.HasStructuralLayerVisualization())
        {
            view.RemoveStructuralLayerVisualization();
        }
    }

    mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::StructuralLayer)].reset();

    // Electrical

    if (mElectricalLayerVisualizationMode != ElectricalLayerVisualizationModeType::None)
    {
        if (!mElectricalLayerVisualizationTexture)
        {
            // Initialize electrical visualization
            mElectricalLayerVisualizationTexture = std::make_unique<RgbaImageData>(mModel.GetShipSize().width, mModel.GetShipSize().height);
        }

        auto const & dirtyShipRegion = mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::ElectricalLayer)];
        if (dirtyShipRegion.has_value())
        {
            // Update visualization
            UpdateElectricalLayerVisualization(*dirtyShipRegion);

            // Upload visualization
            view.UploadElectricalLayerVisualization(*mElectricalLayerVisualizationTexture);
        }
    }
    else
    {
        assert(!mElectricalLayerVisualizationTexture);

        if (view.HasElectricalLayerVisualization())
        {
            view.RemoveElectricalLayerVisualization();
        }
    }

    mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::ElectricalLayer)].reset();

    // Ropes

    if (mRopesLayerVisualizationMode != RopesLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::Ropes));

        auto const & dirtyShipRegion = mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::RopesLayer)];
        if (dirtyShipRegion.has_value())
        {
            // Update visualization
            UpdateRopesLayerVisualization(); // Dirty region not needed in this implementation

            // Upload visualization
            view.UploadRopesLayerVisualization(mModel.GetRopesLayer().Buffer);
        }
    }
    else
    {
        if (view.HasRopesLayerVisualization())
        {
            view.RemoveRopesLayerVisualization();
        }
    }

    mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::RopesLayer)].reset();

    // Texture

    if (mTextureLayerVisualizationMode != TextureLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::Texture));

        auto const & dirtyShipRegion = mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::TextureLayer)];
        if (dirtyShipRegion.has_value())
        {
            // Update visualization
            UpdateTextureLayerVisualization(); // Dirty region not needed in this implementation

            // Upload visualization
            view.UploadTextureLayerVisualization(mModel.GetTextureLayer().Buffer);
        }
    }
    else
    {
        if (view.HasTextureLayerVisualization())
        {
            view.RemoveTextureLayerVisualization();
        }
    }

    mDirtyVisualizationRegions[static_cast<size_t>(VisualizationType::TextureLayer)].reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::InitializeStructuralLayerAnalysis()
{
    // FUTUREWORK - reset analysis
}

void ModelController::InitializeElectricalLayerAnalysis()
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

void ModelController::InitializeRopesLayerAnalysis()
{
    // Nop
}

void ModelController::WriteParticle(
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

void ModelController::WriteParticle(
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

void ModelController::AppendRope(
    ShipSpaceCoordinates const & startCoords,
    ShipSpaceCoordinates const & endCoords,
    StructuralMaterial const * material)
{
    assert(material != nullptr);

    mModel.GetRopesLayer().Buffer.EmplaceBack(
        startCoords,
        endCoords,
        material,
        rgbaColor(material->RenderColor, 255));
}

void ModelController::MoveRopeEndpoint(
    RopeElement & ropeElement,
    ShipSpaceCoordinates const & oldCoords,
    ShipSpaceCoordinates const & newCoords)
{
    if (ropeElement.StartCoords == oldCoords)
    {
        ropeElement.StartCoords = newCoords;
    }
    else
    {
        assert(ropeElement.EndCoords == oldCoords);
        ropeElement.EndCoords = newCoords;
    }
}

template<LayerType TLayer>
std::optional<ShipSpaceRect> ModelController::Flood(
    ShipSpaceCoordinates const & start,
    typename LayerTypeTraits<TLayer>::material_type const * material,
    bool doContiguousOnly,
    typename LayerTypeTraits<TLayer>::layer_data_type const & layer)
{
    // Pick material to flood
    auto const * const startMaterial = layer.Buffer[start].Material;
    if (material == startMaterial)
    {
        // Nop
        return std::nullopt;
    }

    ShipSpaceSize const shipSize = mModel.GetShipSize();

    if (doContiguousOnly)
    {
        //
        // Flood from point
        //

        //
        // Init visit from this point
        //

        WriteParticle(start, material);
        ShipSpaceRect affectedRect(start);

        std::queue<ShipSpaceCoordinates> pointsToPropagateFrom;
        pointsToPropagateFrom.push(start);

        //
        // Propagate
        //

        auto const checkPropagateToNeighbor = [&](ShipSpaceCoordinates neighborCoords)
        {
            if (neighborCoords.IsInSize(shipSize) && layer.Buffer[neighborCoords].Material == startMaterial)
            {
                // Visit point
                WriteParticle(neighborCoords, material);
                affectedRect.UnionWith(neighborCoords);

                // Propagate from point
                pointsToPropagateFrom.push(neighborCoords);
            }
        };

        while (!pointsToPropagateFrom.empty())
        {
            // Pop point that we have to propagate from
            auto const currentPoint = pointsToPropagateFrom.front();
            pointsToPropagateFrom.pop();

            // Push neighbors
            checkPropagateToNeighbor({ currentPoint.x - 1, currentPoint.y });
            checkPropagateToNeighbor({ currentPoint.x + 1, currentPoint.y });
            checkPropagateToNeighbor({ currentPoint.x, currentPoint.y - 1 });
            checkPropagateToNeighbor({ currentPoint.x, currentPoint.y + 1 });
        }

        return affectedRect;
    }
    else
    {
        //
        // Replace material
        //

        std::optional<ShipSpaceRect> affectedRect;

        for (int y = 0; y < shipSize.height; ++y)
        {
            for (int x = 0; x < shipSize.width; ++x)
            {
                ShipSpaceCoordinates const coords(x, y);

                if (layer.Buffer[coords].Material == startMaterial)
                {
                    WriteParticle(coords, material);

                    if (!affectedRect.has_value())
                    {
                        affectedRect = ShipSpaceRect(coords);
                    }
                    else
                    {
                        affectedRect->UnionWith(coords);
                    }
                }
            }
        }

        return affectedRect;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////

template<VisualizationType TVisualization>
void ModelController::RegisterDirtyVisualization(ShipSpaceRect const & region)
{
    size_t constexpr VizIndex = static_cast<size_t>(TVisualization);

    if (!mDirtyVisualizationRegions[VizIndex].has_value())
    {
        mDirtyVisualizationRegions[VizIndex] = region;
    }
    else
    {
        mDirtyVisualizationRegions[VizIndex]->UnionWith(region);
    }
}

ImageRect ModelController::UpdateGameVisualization(ShipSpaceRect const & region)
{
    //
    // 1. Prepare source of triangularized rendering
    //

    RgbaImageData const * sourceTexture = nullptr;

    if (mGameVisualizationMode == GameVisualizationModeType::AutoTexturizationMode)
    {
        assert(mModel.HasLayer(LayerType::Structural));
        assert(mGameVisualizationAutoTexturizationTexture);

        ShipAutoTexturizationSettings settings = mModel.GetShipAutoTexturizationSettings().value_or(ShipAutoTexturizationSettings());

        mShipTexturizer.AutoTexturizeInto(
            mModel.GetStructuralLayer(),
            region,
            *mGameVisualizationAutoTexturizationTexture,
            mGameVisualizationTextureMagnificationFactor,
            settings);

        sourceTexture = mGameVisualizationAutoTexturizationTexture.get();
    }
    else
    {
        assert(mGameVisualizationMode == GameVisualizationModeType::TextureMode);

        assert(mModel.HasLayer(LayerType::Structural));
        assert(mModel.HasLayer(LayerType::Texture));

        sourceTexture = &mModel.GetTextureLayer().Buffer;
    }

    assert(sourceTexture != nullptr);

    //
    // 2. Do triangularized rendering
    //

    // Given that texturization looks at x+1 and y+1, we enlarge the region down and to the left
    ShipSpaceRect effectiveRegion = region;
    if (effectiveRegion.origin.x > 0)
    {
        effectiveRegion.origin.x -= 1;
        effectiveRegion.size.width += 1;
    }

    if (effectiveRegion.origin.y > 0)
    {
        effectiveRegion.origin.y -= 1;
        effectiveRegion.size.height += 1;
    }

    assert(mGameVisualizationTexture);
    
    mShipTexturizer.RenderShipInto(
        mModel.GetStructuralLayer(),
        effectiveRegion,
        *sourceTexture,
        *mGameVisualizationTexture,
        mGameVisualizationTextureMagnificationFactor);

    //
    // 3. Return dirty image region
    //

    return ImageRect(
        { effectiveRegion.origin.x * mGameVisualizationTextureMagnificationFactor, effectiveRegion.origin.y * mGameVisualizationTextureMagnificationFactor },
        { effectiveRegion.size.width * mGameVisualizationTextureMagnificationFactor, effectiveRegion.size.height * mGameVisualizationTextureMagnificationFactor });
}

void ModelController::UpdateStructuralLayerVisualization(ShipSpaceRect const & region)
{
    switch (mStructuralLayerVisualizationMode)
    {
        case StructuralLayerVisualizationModeType::MeshMode:
        case StructuralLayerVisualizationModeType::PixelMode:
        {
            assert(mStructuralLayerVisualizationTexture);

            RenderStructureInto(
                region,
                *mStructuralLayerVisualizationTexture);

            break;
        }

        case StructuralLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

void ModelController::RenderStructureInto(
    ShipSpaceRect const & structureRegion,
    RgbaImageData & texture) const
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(texture.Size.width == mModel.GetStructuralLayer().Buffer.Size.width
        && texture.Size.height == mModel.GetStructuralLayer().Buffer.Size.height);

    rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 0); // Fully transparent

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    for (int y = structureRegion.origin.y; y < structureRegion.origin.y + structureRegion.size.height; ++y)
    {
        for (int x = structureRegion.origin.x; x < structureRegion.origin.x + structureRegion.size.width; ++x)
        {
            auto const structuralMaterial = structuralLayerBuffer[{x, y}].Material;

            texture[{x, y}] = (structuralMaterial != nullptr)
                ? rgbaColor(structuralMaterial->RenderColor, 255)
                : emptyColor;
        }
    }
}

void ModelController::UpdateElectricalLayerVisualization(ShipSpaceRect const & region)
{
    switch (mElectricalLayerVisualizationMode)
    {
        case ElectricalLayerVisualizationModeType::PixelMode:
        {
            assert(mModel.HasLayer(LayerType::Electrical));

            assert(mElectricalLayerVisualizationTexture);
            assert(mElectricalLayerVisualizationTexture->Size.width == mModel.GetShipSize().width
                && mElectricalLayerVisualizationTexture->Size.height == mModel.GetShipSize().height);

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

            break;
        }

        case ElectricalLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

void ModelController::UpdateRopesLayerVisualization()
{
    switch (mRopesLayerVisualizationMode)
    {
        case RopesLayerVisualizationModeType::LinesMode:
        {
            assert(mModel.HasLayer(LayerType::Ropes));

            // Nop
            break;
        }

        case RopesLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

void ModelController::UpdateTextureLayerVisualization()
{
    switch(mTextureLayerVisualizationMode)
    {
        case TextureLayerVisualizationModeType::MatteMode:
        {
            assert(mModel.HasLayer(LayerType::Texture));

            // Nop
            break;
        }

        case TextureLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

}