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
    , mDirtyGameVisualizationRegion()
    , mStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::None)
    , mStructuralLayerVisualizationTexture()
    , mDirtyStructuralLayerVisualizationRegion()
    , mElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::None)
    , mElectricalLayerVisualizationTexture()
    , mDirtyElectricalLayerVisualizationRegion()
    , mRopesLayerVisualizationMode(RopesLayerVisualizationModeType::None)
    , mIsRopesLayerVisualizationDirty(false)
    , mTextureLayerVisualizationMode(TextureLayerVisualizationModeType::None)
    , mIsTextureLayerVisualizationDirty(false)
    /////
    , mIsStructuralLayerInEphemeralVisualization(false)
    , mIsElectricalLayerInEphemeralVisualization(false)
    , mIsRopesLayerInEphemeralVisualization(false)
{
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

        UpdateStructuralLayerVisualization(GetWholeShipRect());
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mIsElectricalLayerInEphemeralVisualization);

        mModel.GetElectricalLayer().Buffer.Flip(direction);

        UpdateElectricalLayerVisualization(GetWholeShipRect());
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mIsRopesLayerInEphemeralVisualization);

        mModel.GetRopesLayer().Buffer.Flip(direction, mModel.GetShipSize());

        UpdateRopesLayerVisualization();
    }

    // Texture layer
    if (mModel.HasLayer(LayerType::Texture))
    {
        mModel.GetTextureLayer().Buffer.Flip(direction);

        UpdateTextureLayerVisualization();
    }

    UpdateGameVisualization(GetWholeShipRect());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Structural
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewStructuralLayer()
{
    mModel.NewStructuralLayer();

    InitializeStructuralLayerAnalysis();

    UpdateGameVisualization(GetWholeShipRect());
    UpdateStructuralLayerVisualization(GetWholeShipRect());

    mIsStructuralLayerInEphemeralVisualization = false;
}

void ModelController::SetStructuralLayer(/*TODO*/)
{
    assert(mModel.HasLayer(LayerType::Structural));

    mModel.SetStructuralLayer(/*TODO*/);

    InitializeStructuralLayerAnalysis();

    UpdateGameVisualization(GetWholeShipRect());
    UpdateStructuralLayerVisualization(GetWholeShipRect());

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
            WriteParticle(ShipSpaceCoordinates(x, y), material);
        }
    }

    //
    // Update visualization
    //

    UpdateGameVisualization(region);
    UpdateStructuralLayerVisualization(region);
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

        UpdateGameVisualization(*affectedRect);
        UpdateStructuralLayerVisualization(*affectedRect);
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

    UpdateGameVisualization({ targetOrigin, sourceRegion.size });
    UpdateStructuralLayerVisualization({ targetOrigin, sourceRegion.size });
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

    UpdateGameVisualization(region);
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

    UpdateGameVisualization({ targetOrigin, sourceRegion.size });
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

    InitializeElectricalLayerAnalysis();

    UpdateElectricalLayerVisualization(GetWholeShipRect());

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::SetElectricalLayer(/*TODO*/)
{
    mModel.SetElectricalLayer(/*TODO*/);

    InitializeElectricalLayerAnalysis();

    UpdateElectricalLayerVisualization(GetWholeShipRect());

    mIsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    assert(!mModel.HasLayer(LayerType::Electrical));

    InitializeElectricalLayerAnalysis();

    mIsElectricalLayerInEphemeralVisualization = false;
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
        UpdateElectricalLayerVisualization(*affectedRect);
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

    UpdateElectricalLayerVisualization(region);
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

    UpdateElectricalLayerVisualization({ targetOrigin, sourceRegion.size });
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

    InitializeRopesLayerAnalysis();

    UpdateRopesLayerVisualization();

    mIsRopesLayerInEphemeralVisualization = false;
}

void ModelController::SetRopesLayer(/*TODO*/)
{
    mModel.SetRopesLayer(/*TODO*/);

    InitializeRopesLayerAnalysis();

    UpdateRopesLayerVisualization();

    mIsRopesLayerInEphemeralVisualization = false;
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    assert(!mModel.HasLayer(LayerType::Ropes));

    InitializeRopesLayerAnalysis();

    mIsRopesLayerInEphemeralVisualization = false;
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

    UpdateRopesLayerVisualization();
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

    UpdateRopesLayerVisualization();
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
        UpdateRopesLayerVisualization();

        return true;
    }
    else
    {
        return false;
    }
}

void ModelController::RestoreRopesLayer(RopesLayerData && sourceLayer)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mIsElectricalLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.GetRopesLayer().Buffer = std::move(sourceLayer.Buffer);

    //
    // Re-initialize layer analysis
    //

    InitializeRopesLayerAnalysis();

    //
    // Update visualization
    //

    UpdateRopesLayerVisualization();
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

    UpdateRopesLayerVisualization();

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

    UpdateRopesLayerVisualization();

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

    UpdateRopesLayerVisualization();

    // Remember we are not anymore in temp visualization
    mIsRopesLayerInEphemeralVisualization = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Texture
////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::NewTextureLayer()
{
    mModel.NewTextureLayer();

    UpdateGameVisualization(GetWholeShipRect());
    UpdateTextureLayerVisualization();
}

void ModelController::SetTextureLayer(
    TextureLayerData && textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    mModel.SetTextureLayer(std::move(textureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    UpdateGameVisualization(GetWholeShipRect());
    UpdateTextureLayerVisualization();
}

void ModelController::RemoveTextureLayer()
{
    assert(mModel.HasLayer(LayerType::Texture));

    mModel.RemoveTextureLayer();

    assert(!mModel.HasLayer(LayerType::Texture));

    // Remove art credits from metadata
    mModel.GetShipMetadata().ArtCredits.reset();
}

std::unique_ptr<TextureLayerData> ModelController::CloneTextureLayer() const
{
    return mModel.CloneTextureLayer();
}

void ModelController::RestoreTextureLayer(
    std::unique_ptr<TextureLayerData> textureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    mModel.RestoreTextureLayer(std::move(textureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    // Visualization will be updated by controller, as mode might change 
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
        if (mGameVisualizationMode == GameVisualizationModeType::None)
        {
            // Initialize game visualization texture

            assert(!mGameVisualizationTexture);

            mGameVisualizationTextureMagnificationFactor = ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(mModel.GetShipSize());
            ImageSize const textureSize = ImageSize(
                mModel.GetShipSize().width * mGameVisualizationTextureMagnificationFactor,
                mModel.GetShipSize().height * mGameVisualizationTextureMagnificationFactor);
            
            mGameVisualizationTexture = std::make_unique<RgbaImageData>(textureSize);
        }

        if (mode == GameVisualizationModeType::AutoTexturizationMode)
        {
            assert(mGameVisualizationTexture);
            mGameVisualizationAutoTexturizationTexture = std::make_unique<RgbaImageData>(mGameVisualizationTexture->Size);
        }
        else
        {
            mGameVisualizationAutoTexturizationTexture.reset();
        }

        mGameVisualizationMode = mode;

        UpdateGameVisualization(GetWholeShipRect());
    }
    else
    {
        // Shutdown game visualization
        mGameVisualizationMode = GameVisualizationModeType::None;
        mGameVisualizationAutoTexturizationTexture.reset();
        assert(mGameVisualizationTexture);
        mGameVisualizationTexture.reset();
        mDirtyGameVisualizationRegion.reset();
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
        if (mStructuralLayerVisualizationMode == StructuralLayerVisualizationModeType::None)
        {
            // Initialize structural visualization
            assert(!mStructuralLayerVisualizationTexture);
            mStructuralLayerVisualizationTexture = std::make_unique<RgbaImageData>(ImageSize(mModel.GetShipSize().width, mModel.GetShipSize().height));
        }

        mStructuralLayerVisualizationMode = mode;

        UpdateStructuralLayerVisualization(GetWholeShipRect());
    }
    else
    {
        // Shutdown structural visualization
        mStructuralLayerVisualizationMode = StructuralLayerVisualizationModeType::None;
        assert(mStructuralLayerVisualizationTexture);
        mStructuralLayerVisualizationTexture.reset();
        mDirtyStructuralLayerVisualizationRegion.reset();
    }
}

RgbaImageData const & ModelController::GetStructuralLayerVisualization() const
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mIsStructuralLayerInEphemeralVisualization);

    assert(mStructuralLayerVisualizationTexture);
    return *mStructuralLayerVisualizationTexture;
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
        if (mElectricalLayerVisualizationMode == ElectricalLayerVisualizationModeType::None)
        {
            // Initialize electrical visualization
            assert(!mElectricalLayerVisualizationTexture);
            mElectricalLayerVisualizationTexture = std::make_unique<RgbaImageData>(mModel.GetShipSize().width, mModel.GetShipSize().height);
        }

        mElectricalLayerVisualizationMode = mode;

        UpdateElectricalLayerVisualization(GetWholeShipRect());
    }
    else
    {
        // Shutdown electrical visualization
        mElectricalLayerVisualizationMode = ElectricalLayerVisualizationModeType::None;
        assert(mElectricalLayerVisualizationTexture);
        mElectricalLayerVisualizationTexture.reset();
        mDirtyElectricalLayerVisualizationRegion.reset();
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
        if (mRopesLayerVisualizationMode == RopesLayerVisualizationModeType::None)
        {
            // Initialize ropes visualization
            // ...nop for now
        }

        mRopesLayerVisualizationMode = mode;

        UpdateRopesLayerVisualization();
    }
    else
    {
        // Shutdown ropes visualization
        mRopesLayerVisualizationMode = RopesLayerVisualizationModeType::None;
        mIsRopesLayerVisualizationDirty = false;
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
        if (mTextureLayerVisualizationMode == TextureLayerVisualizationModeType::None)
        {
            // Initialize texture visualization
            // ...nop for now
        }

        mTextureLayerVisualizationMode = mode;

        UpdateTextureLayerVisualization();
    }
    else
    {
        // Shutdown texture visualization
        mTextureLayerVisualizationMode = TextureLayerVisualizationModeType::None;
        mIsTextureLayerVisualizationDirty = false;
    }
}

void ModelController::UploadVisualizations(View & view)
{
    //
    // Upload visualizations that are dirty, and
    // remove visualizations that are not needed
    //

    if (mGameVisualizationMode != GameVisualizationModeType::None)
    {
        assert(mGameVisualizationTexture);

        if (mDirtyGameVisualizationRegion.has_value())
        {
            if (*mDirtyGameVisualizationRegion != mGameVisualizationTexture->Size)
            {
                //
                // For better performance, we only upload the dirty sub-texture
                //

                auto subTexture = RgbaImageData(mDirtyGameVisualizationRegion->size);
                subTexture.BlitFromRegion(
                    *mGameVisualizationTexture,
                    *mDirtyGameVisualizationRegion,
                    { 0, 0 });

                view.UpdateGameVisualizationTexture(
                    subTexture,
                    mDirtyGameVisualizationRegion->origin);
            }
            else
            {
                view.UploadGameVisualizationTexture(*mGameVisualizationTexture);
            }

            mDirtyGameVisualizationRegion.reset();
        }
    }
    else
    {
        assert(!mGameVisualizationTexture);
        assert(!mDirtyGameVisualizationRegion.has_value());

        if (view.HasGameVisualizationTexture())
        {
            view.RemoveGameVisualizationTexture();
        }
    }

    if (mStructuralLayerVisualizationMode != StructuralLayerVisualizationModeType::None)
    {
        assert(mStructuralLayerVisualizationTexture);

        if (mDirtyStructuralLayerVisualizationRegion.has_value())
        {
            view.UploadStructuralLayerVisualizationTexture(*mStructuralLayerVisualizationTexture);

            mDirtyStructuralLayerVisualizationRegion.reset();
        }
    }
    else
    {
        assert(!mStructuralLayerVisualizationTexture);
        assert(!mDirtyStructuralLayerVisualizationRegion.has_value());

        if (view.HasStructuralLayerVisualizationTexture())
        {
            view.RemoveStructuralLayerVisualizationTexture();
        }
    }

    if (mElectricalLayerVisualizationMode != ElectricalLayerVisualizationModeType::None)
    {
        assert(mElectricalLayerVisualizationTexture);

        if (mDirtyElectricalLayerVisualizationRegion.has_value())
        {
            view.UploadElectricalLayerVisualizationTexture(*mElectricalLayerVisualizationTexture);

            mDirtyElectricalLayerVisualizationRegion.reset();
        }
    }
    else
    {
        assert(!mElectricalLayerVisualizationTexture);
        assert(!mDirtyElectricalLayerVisualizationRegion.has_value());

        if (view.HasElectricalLayerVisualizationTexture())
        {
            view.RemoveElectricalLayerVisualizationTexture();
        }
    }

    if (mRopesLayerVisualizationMode != RopesLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::Ropes));

        if (mIsRopesLayerVisualizationDirty)
        {
            view.UploadRopesLayerVisualization(mModel.GetRopesLayer().Buffer);

            mIsRopesLayerVisualizationDirty = false;
        }
    }
    else
    {
        assert(!mIsRopesLayerVisualizationDirty);

        if (view.HasRopesLayerVisualization())
        {
            view.RemoveRopesLayerVisualization();
        }
    }

    if (mTextureLayerVisualizationMode != TextureLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::Texture));

        if (mIsTextureLayerVisualizationDirty)
        {
            view.UploadTextureLayerVisualizationTexture(mModel.GetTextureLayer().Buffer);

            mIsTextureLayerVisualizationDirty = false;
        }
    }
    else
    {
        assert(!mIsTextureLayerVisualizationDirty);

        if (view.HasTextureLayerVisualizationTexture())
        {
            view.RemoveTextureLayerVisualizationTexture();
        }
    }
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

void ModelController::UpdateGameVisualization(ShipSpaceRect const & region)
{
    //
    // 1. Prepare source of triangularized rendering
    //

    RgbaImageData const * sourceTexture = nullptr;

    switch (mGameVisualizationMode)
    {
        case GameVisualizationModeType::AutoTexturizationMode:
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

            break;
        }

        case GameVisualizationModeType::TextureMode:
        {
            assert(mModel.HasLayer(LayerType::Structural));
            assert(mModel.HasLayer(LayerType::Texture));

            sourceTexture = &mModel.GetTextureLayer().Buffer;
            
            break;
        }

        case GameVisualizationModeType::None:
        {
            return;
        }
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
    // 3. Remember dirty region
    //

    ImageRect const imageRegion = ImageRect(
        { effectiveRegion.origin.x * mGameVisualizationTextureMagnificationFactor, effectiveRegion.origin.y * mGameVisualizationTextureMagnificationFactor },
        { effectiveRegion.size.width * mGameVisualizationTextureMagnificationFactor, effectiveRegion.size.height * mGameVisualizationTextureMagnificationFactor });

    if (!mDirtyGameVisualizationRegion.has_value())
    {
        mDirtyGameVisualizationRegion = imageRegion;
    }
    else
    {
        mDirtyGameVisualizationRegion->UnionWith(imageRegion);
    }
}

void ModelController::UpdateStructuralLayerVisualization(ShipSpaceRect const & region)
{
    switch (mStructuralLayerVisualizationMode)
    {
        case StructuralLayerVisualizationModeType::PixelMode:
        {
            assert(mModel.HasLayer(LayerType::Structural));
            assert(mStructuralLayerVisualizationTexture);

            assert(mStructuralLayerVisualizationTexture->Size.width == mModel.GetShipSize().width
                && mStructuralLayerVisualizationTexture->Size.height == mModel.GetShipSize().height);

            rgbaColor const emptyColor = rgbaColor(EmptyMaterialColorKey, 0); // Fully transparent

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

            break;
        }

        case StructuralLayerVisualizationModeType::None:
        {
            return;
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

    mIsRopesLayerVisualizationDirty = true;
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

    mIsTextureLayerVisualizationDirty = true;
}

}