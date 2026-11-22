/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelController.h"

#include <algorithm>
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
    , mMassParticleCount(0)
    , mTotalMass(0.0f)
    , mCenterOfMassSum(vec2f::zero())
    , mInstancedElectricalElementSet()
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
    , mExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType::None)
    , mInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType::None)
    /////
    , mEphemeralVisualizationLayerState()
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

std::unique_ptr<RgbaImageData> ModelController::MakePreview() const
{
    // At this moment we can't but require a structural layer - there would be no preview without it
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

ModelValidationSession ModelController::StartValidation(Finalizer && finalizer) const
{
    return ModelValidationSession(
        mModel,
        std::move(finalizer));
}

std::optional<SampledInformation> ModelController::SampleInformationAt(ShipSpaceCoordinates const & coordinates, LayerType layer) const
{
    switch (layer)
    {
        case LayerType::Electrical:
        {
            assert(mModel.HasLayer(LayerType::Electrical));
            assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);
            assert(coordinates.IsInSize(GetShipSize()));

            ElectricalElement const & element = mModel.GetElectricalLayer().Buffer[coordinates];
            if (element.Material != nullptr)
            {
                assert((element.InstanceIndex != NoneElectricalElementInstanceIndex && element.Material->IsInstanced)
                    || (element.InstanceIndex == NoneElectricalElementInstanceIndex && !element.Material->IsInstanced));

                return SampledInformation(
                    element.Material->Name,
                    element.Material->IsInstanced ? element.InstanceIndex : std::optional<ElectricalElementInstanceIndex>());
            }
            else
            {
                return std::nullopt;
            }
        }

        case LayerType::Ropes:
        {
            assert(mModel.HasLayer(LayerType::Ropes));
            assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);
            assert(coordinates.IsInSize(mModel.GetShipSize()));

            auto const * material = mModel.GetRopesLayer().Buffer.SampleMaterialEndpointAt(coordinates);
            if (material != nullptr)
            {
                return SampledInformation(material->Name, std::nullopt);
            }
            else
            {
                return std::nullopt;
            }
        }

        case LayerType::Structural:
        {
            assert(mModel.HasLayer(LayerType::Structural));
            assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);
            assert(coordinates.IsInSize(GetShipSize()));

            auto const * material = mModel.GetStructuralLayer().Buffer[coordinates].Material;
            if (material != nullptr)
            {
                return SampledInformation(material->Name, std::nullopt);
            }
            else
            {
                return std::nullopt;
            }
        }

        case LayerType::ExteriorTexture:
        case LayerType::InteriorTexture:
        {
            // Nothing to do
            return std::nullopt;
        }
    }

    assert(false);
    return std::nullopt;
}

void ModelController::Flip(DirectionType direction)
{
    // Structural layer
    if (mModel.HasLayer(LayerType::Structural))
    {
        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        mModel.GetStructuralLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());

        InitializeStructuralLayerAnalysis();
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        mModel.GetElectricalLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

        InitializeElectricalLayerAnalysis();
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        mModel.GetRopesLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

        InitializeRopesLayerAnalysis();
    }

    // Exterior Texture layer
    if (mModel.HasLayer(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        mModel.GetExteriorTextureLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
    }

    // Interior Texture layer
    if (mModel.HasLayer(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        mModel.GetInteriorTextureLayer().Buffer.Flip(direction);

        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
    }

    //...and Game we do regardless, as there's always a structural layer at least
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
}

void ModelController::Rotate90(RotationDirectionType direction)
{
    // Rotate size
    auto const originalSize = GetShipSize();
    mModel.SetShipSize(ShipSpaceSize(originalSize.height, originalSize.width));

    // Structural layer
    if (mModel.HasLayer(LayerType::Structural))
    {
        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        mModel.GetStructuralLayer().Buffer.Rotate90(direction);

        mStructuralLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());

        InitializeStructuralLayerAnalysis();
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        mModel.GetElectricalLayer().Buffer.Rotate90(direction);

        mElectricalLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

        InitializeElectricalLayerAnalysis();
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        mModel.GetRopesLayer().Buffer.Rotate90(direction);

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

        InitializeRopesLayerAnalysis();
    }

    // Exterior Texture layer
    if (mModel.HasLayer(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        mModel.GetExteriorTextureLayer().Buffer.Rotate90(direction);

        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
    }

    // Interior Texture layer
    if (mModel.HasLayer(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        mModel.GetInteriorTextureLayer().Buffer.Rotate90(direction);

        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
    }

    //...and Game we do regardless, as there's always a structural layer at least
    mGameVisualizationTexture.reset();
    mGameVisualizationAutoTexturizationTexture.reset();
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
}

void ModelController::ResizeShip(
    ShipSpaceSize const & newSize,
    ShipSpaceCoordinates const & originOffset)
{
    ShipSpaceSize const originalShipSize = mModel.GetShipSize();

    ShipSpaceRect newWholeShipRect({ 0, 0 }, newSize);

    mModel.SetShipSize(newSize);

    // Structural layer
    {
        assert(mModel.HasLayer(LayerType::Structural));

        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        mModel.SetStructuralLayer(
            mModel.GetStructuralLayer().MakeReframed(
                newSize,
                originOffset,
                StructuralElement(nullptr)));

        InitializeStructuralLayerAnalysis();

        // Initialize visualization
        mStructuralLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(newWholeShipRect);
    }

    // Electrical layer
    if (mModel.HasLayer(LayerType::Electrical))
    {
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        mModel.SetElectricalLayer(
            mModel.GetElectricalLayer().MakeReframed(
                newSize,
                originOffset,
                ElectricalElement(nullptr, NoneElectricalElementInstanceIndex)));

        InitializeElectricalLayerAnalysis();

        // Initialize visualization
        mElectricalLayerVisualizationTexture.reset();
        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(newWholeShipRect);
    }

    // Ropes layer
    if (mModel.HasLayer(LayerType::Ropes))
    {
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        mModel.SetRopesLayer(
            mModel.GetRopesLayer().MakeReframed(
                newSize,
                originOffset));

        InitializeRopesLayerAnalysis();

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(newWholeShipRect);
    }

    // Exterior Texture layer
    if (mModel.HasLayer(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        // Convert to texture space
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(originalShipSize, GetExteriorTextureSize());

        mModel.SetExteriorTextureLayer(
            mModel.GetExteriorTextureLayer().MakeReframed(
                ImageSize::FromFloatRound(newSize.ToFloat<vec2f>().scale(shipToTexture)),
                ImageCoordinates::FromFloatRound(originOffset.ToFloat().scale(shipToTexture)),
                rgbaColor(0, 0, 0, 0)));

        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
    }

    // Interior Texture layer
    if (mModel.HasLayer(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        // Convert to texture space
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(originalShipSize, GetInteriorTextureSize());

        mModel.SetInteriorTextureLayer(
            mModel.GetInteriorTextureLayer().MakeReframed(
                ImageSize::FromFloatRound(newSize.ToFloat<vec2f>().scale(shipToTexture)),
                ImageCoordinates::FromFloatRound(originOffset.ToFloat().scale(shipToTexture)),
                rgbaColor(0, 0, 0, 0)));

        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
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

//
// Copy
//

ShipSpaceBodyRegions::ShipSpaceBodyRegion ModelController::CopyShipSpaceRegion(
    ShipSpaceRect const & region,
    LayerSelector const & layerSelection) const
{
    // Structural
    std::unique_ptr<StructuralLayerData> structuralLayerCopy;
    if (layerSelection.IsSet(LayerType::Structural))
    {
        assert(HasLayer(LayerType::Structural));
        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        structuralLayerCopy = std::make_unique<StructuralLayerData>(mModel.GetStructuralLayer().MakeRegionBackup(region));
    }

    // Electrical
    std::unique_ptr<ElectricalLayerData> electricalLayerCopy;
    if (layerSelection.IsSet(LayerType::Electrical))
    {
        assert(HasLayer(LayerType::Electrical));
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        electricalLayerCopy = std::make_unique<ElectricalLayerData>(mModel.GetElectricalLayer().MakeRegionBackup(region));
    }

    // Ropes
    std::unique_ptr<RopesLayerData> ropesLayerCopy;
    if (layerSelection.IsSet(LayerType::Ropes))
    {
        assert(HasLayer(LayerType::Ropes));
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        ropesLayerCopy = std::make_unique<RopesLayerData>(mModel.GetRopesLayer().MakeBackup()); // Whole
    }

    // Exterior Texture
    std::unique_ptr<TextureLayerData> exteriorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(HasLayer(LayerType::ExteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        exteriorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetExteriorTextureLayer().MakeRegionBackup(ShipSpaceToExteriorTextureSpace(region)));
    }

    // Interior Texture
    std::unique_ptr<TextureLayerData> interiorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(HasLayer(LayerType::InteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        interiorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetInteriorTextureLayer().MakeRegionBackup(ShipSpaceToInteriorTextureSpace(region)));
    }

    return ShipSpaceBodyRegions::ShipSpaceBodyRegion(
        region,
        std::move(structuralLayerCopy),
        std::move(electricalLayerCopy),
        std::move(ropesLayerCopy),
        std::move(exteriorTextureLayerCopy),
        std::move(interiorTextureLayerCopy));
}

ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion ModelController::CopyExteriorTextureSpaceRegion(
    ImageRect const & region,
    LayerSelector const & layerSelection) const
{
    // Exterior Texture
    std::unique_ptr<TextureLayerData> exteriorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(HasLayer(LayerType::ExteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        exteriorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetExteriorTextureLayer().MakeRegionBackup(region));
    }

    // Interior Texture
    std::unique_ptr<TextureLayerData> interiorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(HasLayer(LayerType::InteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        interiorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetInteriorTextureLayer().MakeRegionBackup(ExteriorTextureSpaceToInteriorTextureSpace(region)));
    }

    return ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
        region,
        std::move(exteriorTextureLayerCopy),
        std::move(interiorTextureLayerCopy));
}

InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion ModelController::CopyInteriorTextureSpaceRegion(
    ImageRect const & region,
    LayerSelector const & layerSelection) const
{
    // Exterior Texture
    std::unique_ptr<TextureLayerData> exteriorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(HasLayer(LayerType::ExteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        exteriorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetExteriorTextureLayer().MakeRegionBackup(InteriorTextureSpaceToExteriorTextureSpace(region)));
    }

    // Interior Texture
    std::unique_ptr<TextureLayerData> interiorTextureLayerCopy;
    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(HasLayer(LayerType::InteriorTexture));
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        interiorTextureLayerCopy = std::make_unique<TextureLayerData>(mModel.GetInteriorTextureLayer().MakeRegionBackup(region));
    }

    return InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
        region,
        std::move(exteriorTextureLayerCopy),
        std::move(interiorTextureLayerCopy));
}

//
// Cut / Paste
//

ShipSpaceBodyRegions::ShipSpaceBodyRegion ModelController::EraseRegion(
    ShipSpaceRect const & region,
    LayerSelector const & layerSelection)
{
    //
    // Prepare undo
    //

    auto undoPayload = CopyShipSpaceRegion(
        region,
        layerSelection);

    //
    // Erase
    //

    if (layerSelection.IsSet(LayerType::Structural))
    {
        StructuralRegionFill(region, nullptr);
    }

    if (layerSelection.IsSet(LayerType::Electrical))
    {
        ElectricalRegionFill(region, nullptr);
    }

    if (layerSelection.IsSet(LayerType::Ropes))
    {
        EraseRopesRegion(region);
    }

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        EraseExteriorTextureRegion(ShipSpaceToExteriorTextureSpace(region));
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        EraseInteriorTextureRegion(ShipSpaceToInteriorTextureSpace(region));
    }

    return undoPayload;
}

ShipSpaceBodyRegions::ShipSpaceBodyRegion ModelController::Paste(
    ShipSpaceBodyRegions::ShipSpaceBodyRegion const & sourceRegion,
    ShipSpaceCoordinates const & pasteOrigin,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    ShipSpaceBodyRegions::ShipSpaceBodyRegion undoPayload;

    // Clamp target region to ship - won't have one if target outside
    // of ship
    std::optional<ShipSpaceRect> const affectedTargetRectShip =
        ShipSpaceRect(pasteOrigin, sourceRegion.GetSize())
        .MakeIntersectionWith(GetWholeShipRect());

    if (affectedTargetRectShip.has_value())
    {
        // Prepare undo for target
        undoPayload = CopyShipSpaceRegion(
            *affectedTargetRectShip,
            layerSelection);

        // Calculate clipped source region
        ShipSpaceRect const clippedSourceRegion(
            ShipSpaceCoordinates(0, 0) + (affectedTargetRectShip->origin - pasteOrigin),
            affectedTargetRectShip->size);

        if (layerSelection.IsSet(LayerType::Structural))
        {
            assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

            // Paste - no analyses
            DoStructuralBlit(
                sourceRegion.LayerRegions.StructuralLayer->Buffer,
                clippedSourceRegion,
                affectedTargetRectShip->origin,
                isTransparent);

            // Re-initialize layer analysis
            InitializeStructuralLayerAnalysis();
        }

        if (layerSelection.IsSet(LayerType::Electrical))
        {
            assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

            // Paste - maintaining analysis
            DoElectricalBlit(
                sourceRegion.LayerRegions.ElectricalLayer->Buffer,
                clippedSourceRegion,
                affectedTargetRectShip->origin,
                [isTransparent, this, &sourceRegion](ElectricalElement const & src, ElectricalElement const & dst) -> ElectricalElement
                {
                    // Take care of deletion of old
                    if (dst.Material != nullptr && (src.Material != nullptr || !isTransparent))
                    {
                        // Old is deleted
                        --mElectricalParticleCount;

                        // See whether old was instanced
                        if (dst.InstanceIndex != NoneElectricalElementInstanceIndex)
                        {
                            // De-register instance ID
                            mInstancedElectricalElementSet.Remove(dst.InstanceIndex);

                            // Remove from panel (if there)
                            mModel.GetElectricalLayer().Panel.TryRemove(dst.InstanceIndex);
                        }
                    }

                    // Take care of creation of new
                    if (src.Material != nullptr)
                    {
                        // New is created
                        ++mElectricalParticleCount;

                        // See whether new is instanced
                        if (src.InstanceIndex != NoneElectricalElementInstanceIndex)
                        {
                            // Source is instanced

                            // Register new instance index
                            auto const newInstanceIndex = mInstancedElectricalElementSet.IsRegistered(src.InstanceIndex)
                                ? mInstancedElectricalElementSet.Add(src.Material) // Rename
                                : mInstancedElectricalElementSet.Register(src.InstanceIndex, src.Material); // Reuse

                            // Copy panel element - if any
                            if (sourceRegion.LayerRegions.ElectricalLayer->Panel.Contains(src.InstanceIndex))
                            {
                                mModel.GetElectricalLayer().Panel.Add(
                                    newInstanceIndex,
                                    sourceRegion.LayerRegions.ElectricalLayer->Panel[src.InstanceIndex]);
                            }

                            return ElectricalElement(
                                src.Material,
                                newInstanceIndex);
                        }
                        else
                        {
                            return src;
                        }
                    }
                    else
                    {
                        // No source: if transparent, return dst
                        return isTransparent ? dst : src;
                    }
                });

            VerifyElectricalLayerInvariants();
        }

        if (layerSelection.IsSet(LayerType::Ropes))
        {
            assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

            // Paste
            // TODO: args will change
            DoRopesBlit(
                sourceRegion.LayerRegions.RopesLayer->Buffer,
                pasteOrigin, // Unclipped! For "hanging ropes"
                isTransparent);

            // Re-initialize layer analysis
            InitializeRopesLayerAnalysis();
        }

        if (layerSelection.IsSet(LayerType::ExteriorTexture))
        {
            assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

            assert(sourceRegion.LayerRegions.ExteriorTextureLayer);

            ImageCoordinates const pasteOriginTexture = ShipSpaceToExteriorTextureSpace(pasteOrigin);

            std::optional<ImageRect> const affectedTargetRectTexture =
                ImageRect(pasteOriginTexture, sourceRegion.LayerRegions.ExteriorTextureLayer->Buffer.Size)
                .MakeIntersectionWith(GetWholeExteriorTextureRect());

            if (affectedTargetRectTexture.has_value())
            {
                // Calculate clipped source region
                ImageRect const clippedSourceRegionTexture(
                    ImageCoordinates(0, 0) + (affectedTargetRectTexture->origin - pasteOriginTexture),
                    affectedTargetRectTexture->size);

                // Paste
                DoExteriorTextureBlit(
                    sourceRegion.LayerRegions.ExteriorTextureLayer->Buffer,
                    clippedSourceRegionTexture,
                    affectedTargetRectTexture->origin,
                    isTransparent);
            }
        }

        if (layerSelection.IsSet(LayerType::InteriorTexture))
        {
            assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

            assert(sourceRegion.LayerRegions.InteriorTextureLayer);

            ImageCoordinates const pasteOriginTexture = ShipSpaceToInteriorTextureSpace(pasteOrigin);

            std::optional<ImageRect> const affectedTargetRectTexture =
                ImageRect(pasteOriginTexture, sourceRegion.LayerRegions.InteriorTextureLayer->Buffer.Size)
                .MakeIntersectionWith(GetWholeInteriorTextureRect());

            if (affectedTargetRectTexture.has_value())
            {
                // Calculate clipped source region
                ImageRect const clippedSourceRegionTexture(
                    ImageCoordinates(0, 0) + (affectedTargetRectTexture->origin - pasteOriginTexture),
                    affectedTargetRectTexture->size);

                // Paste
                DoInteriorTextureBlit(
                    sourceRegion.LayerRegions.InteriorTextureLayer->Buffer,
                    clippedSourceRegionTexture,
                    affectedTargetRectTexture->origin,
                    isTransparent);
            }
        }
    }

    return undoPayload;
}

ShipSpaceEphemeralVisualizationRestoreRegions::ShipSpaceRestoreRegion ModelController::PasteForEphemeralVisualization(
    ShipSpaceBodyRegions::ShipSpaceBodyRegion const & sourceRegion,
    ShipSpaceCoordinates const & pasteOrigin,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    ShipSpaceEphemeralVisualizationRestoreRegions::ShipSpaceRestoreRegion restorePayload;

    // Clamp target region to ship - won't have one if target outside
    // of ship
    std::optional<ShipSpaceRect> const affectedTargetRectShip =
        ShipSpaceRect(pasteOrigin, sourceRegion.GetSize())
        .MakeIntersectionWith(GetWholeShipRect());

    if (affectedTargetRectShip.has_value())
    {
        // Prepare restore for target
        restorePayload = MakeShipSpaceEphemeralVisualizationRestoreRegion(
            *affectedTargetRectShip,
            layerSelection);

        // Calculate clipped source region
        ShipSpaceRect const clippedSourceRegion(
            ShipSpaceCoordinates(0, 0) + (affectedTargetRectShip->origin - pasteOrigin),
            affectedTargetRectShip->size);

        if (layerSelection.IsSet(LayerType::Structural))
        {
            // Paste
            DoStructuralBlit(
                sourceRegion.LayerRegions.StructuralLayer->Buffer,
                clippedSourceRegion,
                affectedTargetRectShip->origin,
                isTransparent);

            // Remember we are in temp visualization now
            mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = true;
        }

        if (layerSelection.IsSet(LayerType::Electrical))
        {
            assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

            // Paste
            DoElectricalBlit_Raw(
                sourceRegion.LayerRegions.ElectricalLayer->Buffer,
                clippedSourceRegion,
                affectedTargetRectShip->origin,
                isTransparent);

            // Remember we are in temp visualization now
            mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = true;
        }

        if (layerSelection.IsSet(LayerType::Ropes))
        {
            assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

            // Paste
            // TODO: args will change
            DoRopesBlit(
                sourceRegion.LayerRegions.RopesLayer->Buffer,
                pasteOrigin, // Unclipped! For "hanging ropes"
                isTransparent);

            // Remember we are in temp visualization now
            mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = true;
        }

        if (layerSelection.IsSet(LayerType::ExteriorTexture))
        {
            assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

            assert(sourceRegion.LayerRegions.ExteriorTextureLayer);

            ImageCoordinates const pasteOriginTexture = ShipSpaceToExteriorTextureSpace(pasteOrigin);

            std::optional<ImageRect> const affectedTargetRectTexture =
                ImageRect(pasteOriginTexture, sourceRegion.LayerRegions.ExteriorTextureLayer->Buffer.Size)
                .MakeIntersectionWith(GetWholeExteriorTextureRect());

            if (affectedTargetRectTexture.has_value())
            {
                // Calculate clipped source region
                ImageRect const clippedSourceRegionTexture(
                    ImageCoordinates(0, 0) + (affectedTargetRectTexture->origin - pasteOriginTexture),
                    affectedTargetRectTexture->size);

                // Paste
                DoExteriorTextureBlit(
                    sourceRegion.LayerRegions.ExteriorTextureLayer->Buffer,
                    clippedSourceRegionTexture,
                    affectedTargetRectTexture->origin,
                    isTransparent);
            }

            // Remember we are in temp visualization now
            mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = true;
        }

        if (layerSelection.IsSet(LayerType::InteriorTexture))
        {
            assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

            assert(sourceRegion.LayerRegions.InteriorTextureLayer);

            ImageCoordinates const pasteOriginTexture = ShipSpaceToInteriorTextureSpace(pasteOrigin);

            std::optional<ImageRect> const affectedTargetRectTexture =
                ImageRect(pasteOriginTexture, sourceRegion.LayerRegions.InteriorTextureLayer->Buffer.Size)
                .MakeIntersectionWith(GetWholeInteriorTextureRect());

            if (affectedTargetRectTexture.has_value())
            {
                // Calculate clipped source region
                ImageRect const clippedSourceRegionTexture(
                    ImageCoordinates(0, 0) + (affectedTargetRectTexture->origin - pasteOriginTexture),
                    affectedTargetRectTexture->size);

                // Paste
                DoInteriorTextureBlit(
                    sourceRegion.LayerRegions.InteriorTextureLayer->Buffer,
                    clippedSourceRegionTexture,
                    affectedTargetRectTexture->origin,
                    isTransparent);
            }

            // Remember we are in temp visualization now
            mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = true;
        }
    }

    //
    // Finalize restore
    //

    return restorePayload;
}


//
// Undo
//

void ModelController::RestoreForUndo(ShipSpaceBodyRegions && undoPayload)
{
    // Note: no layer presence nor size changes

    bool hasStructuralLayerBeenUpdated = false;
    bool hasElectricalLayerBeenUpdated = false;
    bool hasRopesLayerBeenUpdated = false;

    undoPayload.ConsumeAll(
        [&](auto && regionPayload)
        {
            for (LayerType const affectedLayerType : regionPayload.GetAffectedLayers())
            {
                switch (affectedLayerType)
                {
                    case LayerType::Structural:
                    {
                        assert(mModel.HasLayer(LayerType::Structural));

                        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

                        //
                        // Restore model
                        //

                        mModel.GetStructuralLayer().RestoreRegionBackup(
                            std::move(*regionPayload.LayerRegions.StructuralLayer),
                            regionPayload.OriginatingRegion.origin);

                        //
                        // Update visualization
                        //

                        RegisterDirtyVisualization<VisualizationType::Game>(regionPayload.OriginatingRegion);
                        RegisterDirtyVisualization<VisualizationType::StructuralLayer>(regionPayload.OriginatingRegion);

                        hasStructuralLayerBeenUpdated = true;

                        break;
                    }

                    case LayerType::Electrical:
                    {
                        assert(mModel.HasLayer(LayerType::Electrical));

                        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

                        //
                        // Restore model
                        //

                        // Whole panel
                        mModel.GetElectricalLayer().RestoreRegionBackup(
                            std::move(*regionPayload.LayerRegions.ElectricalLayer),
                            regionPayload.OriginatingRegion.origin);

                        //
                        // Update visualization
                        //

                        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(regionPayload.OriginatingRegion);

                        hasElectricalLayerBeenUpdated = true;

                        break;
                    }

                    case LayerType::Ropes:
                    {
                        assert(mModel.HasLayer(LayerType::Ropes));

                        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

                        //
                        // Restore model
                        //

                        // Whole layer
                        mModel.GetRopesLayer().RestoreBackup(std::move(*regionPayload.LayerRegions.RopesLayer));

                        //
                        // Update visualization
                        //

                        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

                        hasRopesLayerBeenUpdated = true;

                        break;
                    }

                    case LayerType::ExteriorTexture:
                    {
                        InternalRestoreExteriorTextureLayerBackup(
                            std::move(*regionPayload.LayerRegions.ExteriorTextureLayer),
                            ShipSpaceToExteriorTextureSpace(regionPayload.OriginatingRegion));

                        break;
                    }

                    case LayerType::InteriorTexture:
                    {
                        InternalRestoreInteriorTextureLayerBackup(
                            std::move(*regionPayload.LayerRegions.InteriorTextureLayer),
                            ShipSpaceToInteriorTextureSpace(regionPayload.OriginatingRegion));

                        break;
                    }
                }
            }
        });

    if (hasStructuralLayerBeenUpdated)
    {
        //
        // Re-initialize layer analysis
        //

        InitializeStructuralLayerAnalysis();
    }

    if (hasElectricalLayerBeenUpdated)
    {
        //
        // Re-initialize layer analysis (and instance IDs)
        //
        // Note: panel doesn't require massaging, it's been restored as a whole
        //

        InitializeElectricalLayerAnalysis();

        VerifyElectricalLayerInvariants();
    }

    if (hasRopesLayerBeenUpdated)
    {
        //
        // Re-initialize layer analysis
        //

        InitializeRopesLayerAnalysis();
    }
}

void ModelController::RestoreForUndo(ExteriorTextureSpaceBodyRegions && undoPayload)
{
    // Note: no layer presence nor size changes

    undoPayload.ConsumeAll(
        [this](auto && regionPayload)
        {
            if (regionPayload.ExteriorTextureLayerRegion)
            {
                InternalRestoreExteriorTextureLayerBackup(
                    std::move(*regionPayload.ExteriorTextureLayerRegion),
                    regionPayload.OriginatingRegion);
            }

            if (regionPayload.InteriorTextureLayerRegion)
            {
                InternalRestoreInteriorTextureLayerBackup(
                    std::move(*regionPayload.InteriorTextureLayerRegion),
                    ExteriorTextureSpaceToInteriorTextureSpace(regionPayload.OriginatingRegion));
            }
        });
}

void ModelController::RestoreForUndo(InteriorTextureSpaceBodyRegions && undoPayload)
{
    // Note: no layer presence nor size changes

    undoPayload.ConsumeAll(
        [this](auto && regionPayload)
        {
            if (regionPayload.ExteriorTextureLayerRegion)
            {
                InternalRestoreExteriorTextureLayerBackup(
                    std::move(*regionPayload.ExteriorTextureLayerRegion),
                    InteriorTextureSpaceToExteriorTextureSpace(regionPayload.OriginatingRegion));
            }

            if (regionPayload.InteriorTextureLayerRegion)
            {
                InternalRestoreInteriorTextureLayerBackup(
                    std::move(*regionPayload.InteriorTextureLayerRegion),
                    regionPayload.OriginatingRegion);
            }
        });
}

//
// Eph viz
//

void ModelController::RestoreEphemeralVisualization(ShipSpaceEphemeralVisualizationRestoreRegions && restorePayload)
{
    EphemeralVisualizationLayerState newEphVizState = mEphemeralVisualizationLayerState;

    restorePayload.ConsumeAll(
        [&](auto && regionPayload)
        {
            for (LayerType const affectedLayerType : regionPayload.GetAffectedLayers())
            {
                switch (affectedLayerType)
                {
                    case LayerType::Structural:
                    {
                        assert(mModel.HasLayer(LayerType::Structural));

                        assert(mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

                        DoStructuralBlit(
                            *regionPayload.StructuralLayerBufferRegion,
                            ShipSpaceRect(regionPayload.StructuralLayerBufferRegion->Size), // Whole backup
                            regionPayload.OriginatingRegion.origin,
                            false);

                        // Remember we are not anymore in temp visualization
                        newEphVizState.IsStructuralLayerInEphemeralVisualization = false;

                        break;
                    }

                    case LayerType::Electrical:
                    {
                        assert(mModel.HasLayer(LayerType::Electrical));

                        assert(mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

                        DoElectricalBlit(
                            *regionPayload.ElectricalLayerBufferRegion,
                            ShipSpaceRect(regionPayload.ElectricalLayerBufferRegion->Size), // Whole backup
                            regionPayload.OriginatingRegion.origin,
                            [](ElectricalElement const & src, ElectricalElement const &) -> ElectricalElement const &
                            {
                                return src;
                            });

                        // Remember we are not anymore in temp visualization
                        newEphVizState.IsElectricalLayerInEphemeralVisualization = false;

                        break;
                    }

                    case LayerType::Ropes:
                    {
                        assert(mModel.HasLayer(LayerType::Ropes));

                        assert(mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

                        // Backup buffer is the whole buffer
                        mModel.GetRopesLayer().Buffer = std::move(*regionPayload.RopesLayerBuffer);

                        // Update visualization
                        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

                        // Remember we are not anymore in temp visualization
                        newEphVizState.IsRopesLayerInEphemeralVisualization = false;

                        break;
                    }

                    case LayerType::ExteriorTexture:
                    {
                        InternalRestoreExteriorTextureLayerEphemeralVisualizationRegion(
                            *regionPayload.ExteriorTextureLayerBufferRegion,
                            ImageRect(regionPayload.ExteriorTextureLayerBufferRegion->Size), // Whole backup
                            ShipSpaceToExteriorTextureSpace(regionPayload.OriginatingRegion.origin),
                            newEphVizState);

                        break;
                    }

                    case LayerType::InteriorTexture:
                    {
                        InternalRestoreInteriorTextureLayerEphemeralVisualizationRegion(
                            *regionPayload.InteriorTextureLayerBufferRegion,
                            ImageRect(regionPayload.InteriorTextureLayerBufferRegion->Size), // Whole backup
                            ShipSpaceToInteriorTextureSpace(regionPayload.OriginatingRegion.origin),
                            newEphVizState);

                        break;
                    }
                }
            }
        });

    mEphemeralVisualizationLayerState = newEphVizState;
}

void ModelController::RestoreEphemeralVisualization(ExteriorTextureSpaceEphemeralVisualizationRestoreRegions && restorePayload)
{
    EphemeralVisualizationLayerState newEphVizState = mEphemeralVisualizationLayerState;

    restorePayload.ConsumeAll(
        [&](auto && regionPayload)
        {
            if (regionPayload.ExteriorTextureLayerBufferRegion)
            {
                InternalRestoreExteriorTextureLayerEphemeralVisualizationRegion(
                    *regionPayload.ExteriorTextureLayerBufferRegion,
                    ImageRect(regionPayload.ExteriorTextureLayerBufferRegion->Size), // Whole backup
                    regionPayload.OriginatingRegion.origin,
                    newEphVizState);
            }

            if (regionPayload.InteriorTextureLayerBufferRegion)
            {
                InternalRestoreInteriorTextureLayerEphemeralVisualizationRegion(
                    *regionPayload.InteriorTextureLayerBufferRegion,
                    ImageRect(regionPayload.InteriorTextureLayerBufferRegion->Size), // Whole backup
                    ExteriorTextureSpaceToInteriorTextureSpace(regionPayload.OriginatingRegion.origin),
                    newEphVizState);
            }
        });

    mEphemeralVisualizationLayerState = newEphVizState;
}

void ModelController::RestoreEphemeralVisualization(InteriorTextureSpaceEphemeralVisualizationRestoreRegions && restorePayload)
{
    EphemeralVisualizationLayerState newEphVizState = mEphemeralVisualizationLayerState;

    restorePayload.ConsumeAll(
        [&](auto && regionPayload)
        {
            if (regionPayload.ExteriorTextureLayerBufferRegion)
            {
                InternalRestoreExteriorTextureLayerEphemeralVisualizationRegion(
                    *regionPayload.ExteriorTextureLayerBufferRegion,
                    ImageRect(regionPayload.ExteriorTextureLayerBufferRegion->Size), // Whole backup
                    InteriorTextureSpaceToExteriorTextureSpace(regionPayload.OriginatingRegion.origin),
                    newEphVizState);
            }

            if (regionPayload.InteriorTextureLayerBufferRegion)
            {
                InternalRestoreInteriorTextureLayerEphemeralVisualizationRegion(
                    *regionPayload.InteriorTextureLayerBufferRegion,
                    ImageRect(regionPayload.InteriorTextureLayerBufferRegion->Size), // Whole backup
                    regionPayload.OriginatingRegion.origin,
                    newEphVizState);
            }
        });

    mEphemeralVisualizationLayerState = newEphVizState;
}

void ModelController::RestoreStructuralLayerEphemeralVisualizationRegions(
    typename LayerTypeTraits<LayerType::Structural>::buffer_type const & backupBuffer,
    CombinedShipSpaceRegions const & regions)
{
    assert(mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    regions.Apply(
        [&](ShipSpaceRect const & region)
        {
            DoStructuralBlit(
                backupBuffer,
                region,
                region.origin,
                false);
        });

    // Remember we are not anymore in temp visualization
    mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = false;
}

void ModelController::RestoreElectricalLayerEphemeralVisualizationRegions(
    typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & backupBuffer,
    CombinedShipSpaceRegions const & regions)
{
    assert(mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

    regions.Apply(
        [&](ShipSpaceRect const & region)
        {
            DoElectricalBlit(
                backupBuffer,
                region,
                region.origin,
                [](ElectricalElement const & src, ElectricalElement const &) -> ElectricalElement const &
                {
                    return src;
                });
        });

    // Remember we are not anymore in temp visualization
    mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::RestoreRopesLayerEphemeralVisualization(
    typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & backupBuffer)
{
    assert(mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    //
    // Restore model, and nothing else
    //

    mModel.GetRopesLayer().Buffer = backupBuffer;

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are not anymore in temp visualization
    mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = false;
}

void ModelController::RestoreExteriorTextureLayerEphemeralVisualizationRegions(
    typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & backupBuffer,
    CombinedTextureSpaceRegions const & regions)
{
    assert(mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    regions.Apply(
        [&](ImageRect const & region)
        {
            DoExteriorTextureBlit(
                backupBuffer,
                region,
                region.origin,
                false);
        });

    // Remember we are not anymore in temp visualization
    mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = false;
}

void ModelController::RestoreInteriorTextureLayerEphemeralVisualizationRegions(
    typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & backupBuffer,
    CombinedTextureSpaceRegions const & regions)
{
    assert(mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    regions.Apply(
        [&](ImageRect const & region)
        {
            DoInteriorTextureBlit(
                backupBuffer,
                region,
                region.origin,
                false);
        });

    // Remember we are not anymore in temp visualization
    mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = false;
}

//
// Structural
//

StructuralLayerData const & ModelController::GetStructuralLayer() const
{
    // This is used, among others, by WaterlineAnalyzer - at this moment
    // we require the structural layer to be always present
    assert(mModel.HasLayer(LayerType::Structural));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization); // BUGBUG: if we are in a paste and have not yet committed when WaterlineAnalyzer runs, this would fire

    return mModel.GetStructuralLayer();
}

void ModelController::SetStructuralLayer(StructuralLayerData && structuralLayer)
{
    mModel.SetStructuralLayer(std::move(structuralLayer));

    InitializeStructuralLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(GetWholeShipRect());

    mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = false;
}

std::unique_ptr<StructuralLayerData> ModelController::CloneStructuralLayer() const
{
    return mModel.CloneStructuralLayer();
}

StructuralMaterial const * ModelController::SampleStructuralMaterialAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Structural));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    assert(coords.IsInSize(mModel.GetShipSize()));

    return mModel.GetStructuralLayer().Buffer[coords].Material;
}

std::set<StructuralMaterial const *> ModelController::GetStructuralMaterialsInUse(std::vector<StructuralMaterial const *> const & candidates) const
{
    std::set<StructuralMaterial const *> materialsInUse;

    // Make set for faster lookup
    std::set<StructuralMaterial const *> candidateSet(
        candidates.cbegin(),
        candidates.cend());

    // Lookup
    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
    for (size_t i = 0; i < structuralLayerBuffer.GetLinearSize(); ++i)
    {
        auto const * layerMaterial = structuralLayerBuffer.Data[i].Material;
        if (candidateSet.find(layerMaterial) != candidateSet.end())
        {
            materialsInUse.insert(layerMaterial);
        }
    }

    return materialsInUse;
}

void ModelController::StructuralRegionFill(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Structural));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

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

void ModelController::StructuralRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    // Raw
    DoStructuralRegionFill(region, material);

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = true;
}

ShipSpaceBodyRegions::ShipSpaceBodyRegion ModelController::StructuralRectangle(
    ShipSpaceRect const & rect,
    std::uint32_t lineSize,
    StructuralMaterial const * lineMaterial,
    std::optional<StructuralMaterial const *> fillMaterial)
{
    assert(mModel.HasLayer(LayerType::Structural));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    //
    // Prepare undo
    //

    auto structuralLayerRegionBackup = std::make_unique<StructuralLayerData>(mModel.GetStructuralLayer().MakeRegionBackup(rect));

    //
    // Update model
    //

    DoStructuralRectangle<false>(
        rect,
        lineSize,
        lineMaterial,
        fillMaterial);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(rect);
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(rect);

    return ShipSpaceBodyRegions::ShipSpaceBodyRegion(
        rect,
        std::move(structuralLayerRegionBackup),
        nullptr,
        nullptr,
        nullptr,
        nullptr);
}

ShipSpaceEphemeralVisualizationRestoreRegions::ShipSpaceRestoreRegion ModelController::StructuralRectangleForEphemeralVisualization(
    ShipSpaceRect const & rect,
    std::uint32_t lineSize,
    StructuralMaterial const * lineMaterial,
    std::optional<StructuralMaterial const *> fillMaterial)
{
    assert(mModel.HasLayer(LayerType::Structural));

    //
    // Prepare restore
    //

    auto structuralLayerRestoreRegion = mModel.GetStructuralLayer().Buffer.CloneRegion(rect);

    //
    // Update model with just material - no analyses
    //

    DoStructuralRectangle<true>(
        rect,
        lineSize,
        lineMaterial,
        fillMaterial);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(rect);
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(rect);

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = true;

    return ShipSpaceEphemeralVisualizationRestoreRegions::ShipSpaceRestoreRegion(
        rect,
        std::move(structuralLayerRestoreRegion),
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt);
}

ShipSpaceBodyRegions ModelController::StructureTrace(
    ImageRect const & textureRect,
    StructuralMaterial const * edgeMaterial,
    StructuralMaterial const * fillMaterial,
    std::uint8_t alphaThreshold)
{
    assert(mModel.HasLayer(LayerType::Structural));
    assert(mModel.HasLayer(LayerType::ExteriorTexture));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    //
    // Calculate rect in ship space
    //

    ShipSpaceRect const shipRect = ExteriorImageRectToContainingShipSpaceRect(textureRect);

    //
    // Prepare undo
    //

    auto structuralLayerRegionBackup = std::make_unique<StructuralLayerData>(mModel.GetStructuralLayer().MakeRegionBackup(shipRect));

    //
    // Create region
    //

    auto tracedRegion = MakeStructureTrace(
        shipRect,
        textureRect,
        edgeMaterial,
        fillMaterial,
        alphaThreshold);

    //
    // Blit region into structural layer
    //

    DoStructuralBlit(
        tracedRegion,
        ShipSpaceRect(shipRect.size), // All of it
        shipRect.origin,
        false); // Overwrite, even if with "transparent" particles

    // Re-initialize layer analysis
    InitializeStructuralLayerAnalysis();

    // Note: DoStructuralRegionBufferPaste also updates viz

    return ShipSpaceBodyRegions::Make(
        shipRect,
        std::move(structuralLayerRegionBackup),
        nullptr,
        nullptr,
        nullptr,
        nullptr);
}

std::optional<ShipSpaceBodyRegions::ShipSpaceBodyRegion> ModelController::StructuralFlood(
    ShipSpaceCoordinates const & start,
    StructuralMaterial const * material,
    bool doContiguousOnly)
{
    assert(mModel.HasLayer(LayerType::Structural));

    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    //
    // Prepare Undo
    //

    auto wholeLayerClone = mModel.GetStructuralLayer().Clone();

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect = DoFlood<LayerType::Structural>(
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

        //
        // Create Undo
        //

        return ShipSpaceBodyRegions::ShipSpaceBodyRegion(
            *affectedRect,
            std::make_unique<StructuralLayerData>(wholeLayerClone.MakeRegionBackup(*affectedRect)),
            nullptr,
            nullptr,
            nullptr,
            nullptr);
    }

    return std::nullopt;
}

void ModelController::RestoreStructuralLayerComplete(std::unique_ptr<StructuralLayerData> sourceLayer)
{
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

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

//
// Electrical
//

ElectricalPanel const & ModelController::GetElectricalPanel() const
{
    assert(mModel.HasLayer(LayerType::Electrical));

    return mModel.GetElectricalLayer().Panel;
}

void ModelController::SetElectricalLayer(ElectricalLayerData && electricalLayer)
{
    mModel.SetElectricalLayer(std::move(electricalLayer));

    InitializeElectricalLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

    mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = false;
}

void ModelController::RemoveElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mModel.RemoveElectricalLayer();

    InitializeElectricalLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(GetWholeShipRect());

    mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = false;
}

std::unique_ptr<ElectricalLayerData> ModelController::CloneElectricalLayer() const
{
    return mModel.CloneElectricalLayer();
}

ElectricalMaterial const * ModelController::SampleElectricalMaterialAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Electrical));
    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

    assert(coords.IsInSize(mModel.GetShipSize()));

    return mModel.GetElectricalLayer().Buffer[coords].Material;
}

std::set<ElectricalMaterial const *> ModelController::GetElectricalMaterialsInUse(std::vector<ElectricalMaterial const *> const & candidates) const
{
    assert(mModel.HasLayer(LayerType::Electrical));
    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

    std::set<ElectricalMaterial const *> materialsInUse;

    // Make set for faster lookup
    std::set<ElectricalMaterial const *> candidateSet(
        candidates.cbegin(),
        candidates.cend());

    // Lookup
    auto const & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;
    for (size_t i = 0; i < electricalLayerBuffer.GetLinearSize(); ++i)
    {
        auto const * layerMaterial = electricalLayerBuffer.Data[i].Material;
        if (candidateSet.find(layerMaterial) != candidateSet.end())
        {
            materialsInUse.insert(layerMaterial);
        }
    }

    return materialsInUse;
}

bool ModelController::IsElectricalParticleAllowedAt(ShipSpaceCoordinates const & coords) const
{
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    if (mModel.HasLayer(LayerType::Structural)
        && mModel.GetStructuralLayer().Buffer[coords].Material != nullptr)
    {
        return true;
    }

    if (mModel.HasLayer(LayerType::Ropes)
        && mModel.GetRopesLayer().Buffer.HasEndpointAt(coords))
    {
        return true;
    }

    return false;
}

std::optional<ShipSpaceRect> ModelController::TrimElectricalParticlesWithoutSubstratum()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

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

    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

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

void ModelController::ElectricalRegionFillForEphemeralVisualization(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    DoElectricalRegionFill(region, material);

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = true;
}

void ModelController::RestoreElectricalLayerComplete(std::unique_ptr<ElectricalLayerData> sourceLayer)
{
    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

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

//
// Ropes
//

void ModelController::SetRopesLayer(RopesLayerData && ropesLayer)
{
    mModel.SetRopesLayer(std::move(ropesLayer));

    InitializeRopesLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = false;
}

void ModelController::RemoveRopesLayer()
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.RemoveRopesLayer();

    InitializeRopesLayerAnalysis();

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = false;
}

std::unique_ptr<RopesLayerData> ModelController::CloneRopesLayer() const
{
    return mModel.CloneRopesLayer();
}

StructuralMaterial const * ModelController::SampleRopesMaterialAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Ropes));
    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    assert(coords.IsInSize(mModel.GetShipSize()));

    return mModel.GetRopesLayer().Buffer.SampleMaterialEndpointAt(coords);
}

std::set<StructuralMaterial const *> ModelController::GetRopesMaterialsInUse(std::vector<StructuralMaterial const *> const & candidates) const
{
    assert(mModel.HasLayer(LayerType::Ropes));
    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    std::set<StructuralMaterial const *> materialsInUse;

    // Make set for faster lookup
    std::set<StructuralMaterial const *> candidateSet(
        candidates.cbegin(),
        candidates.cend());

    // Lookup
    auto const & ropesLayerBuffer = mModel.GetRopesLayer().Buffer;
    for (size_t i = 0; i < ropesLayerBuffer.GetElementCount(); ++i)
    {
        auto const * layerMaterial = ropesLayerBuffer[i].Material;
        if (candidateSet.find(layerMaterial) != candidateSet.end())
        {
            materialsInUse.insert(layerMaterial);
        }
    }

    return materialsInUse;
}

std::optional<size_t> ModelController::GetRopeElementIndexAt(ShipSpaceCoordinates const & coords) const
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

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

    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    InternalAppendRope(
        startCoords,
        endCoords,
        material);

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

    InternalAppendRope(
        startCoords,
        endCoords,
        material);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = true;
}

void ModelController::MoveRopeEndpoint(
    size_t ropeElementIndex,
    ShipSpaceCoordinates const & oldCoords,
    ShipSpaceCoordinates const & newCoords)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    assert(ropeElementIndex < mModel.GetRopesLayer().Buffer.GetElementCount());

    InternalMoveRopeEndpoint(
        mModel.GetRopesLayer().Buffer[ropeElementIndex],
        oldCoords,
        newCoords);

    InitializeRopesLayerAnalysis();

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
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

    assert(ropeElementIndex < mModel.GetRopesLayer().Buffer.GetElementCount());

    InternalMoveRopeEndpoint(
        mModel.GetRopesLayer().Buffer[ropeElementIndex],
        oldCoords,
        newCoords);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = true;
}

bool ModelController::EraseRopeAt(ShipSpaceCoordinates const & coords)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    bool const hasRemoved = InternalEraseRopeAt(coords);

    if (hasRemoved)
    {
        // Update visualization
        RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());

        return true;
    }
    else
    {
        return false;
    }
}

void ModelController::EraseRopesRegion(ShipSpaceRect const & region)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    DoRopesRegionErase(region);
}

void ModelController::RestoreRopesLayerComplete(std::unique_ptr<RopesLayerData> sourceLayer)
{
    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

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

//
// Exterior Texture
//

TextureLayerData const & ModelController::GetExteriorTextureLayer() const
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    return mModel.GetExteriorTextureLayer();
}

void ModelController::SetExteriorTextureLayer(
    TextureLayerData && exteriorTextureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    mModel.SetExteriorTextureLayer(std::move(exteriorTextureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());

    mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = false;
}

void ModelController::RemoveExteriorTextureLayer()
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    auto const oldWholeTextureRect = GetWholeExteriorTextureRect();

    mModel.RemoveExteriorTextureLayer();
    mModel.GetShipMetadata().ArtCredits.reset();

    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(oldWholeTextureRect);

    mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = false;
}

std::unique_ptr<TextureLayerData> ModelController::CloneExteriorTextureLayer() const
{
    return mModel.CloneExteriorTextureLayer();
}

void ModelController::EraseExteriorTextureRegion(ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    DoExteriorTextureRegionErase(region);
}

void ModelController::EraseExteriorTextureRegionForEphemeralVisualization(ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    //
    // Update model
    //

    auto & textureLayerBuffer = mModel.GetExteriorTextureLayer().Buffer;

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            textureLayerBuffer[{x, y}].a = 0;
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(ExteriorImageRectToContainingShipSpaceRect(region));
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(region);

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = true;
}

std::optional<ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion> ModelController::ExteriorTextureMagicWandEraseBackground(
    ImageCoordinates const & start,
    unsigned int tolerance,
    bool isAntiAlias,
    bool doContiguousOnly)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    //
    // Prepare Undo
    //

    auto wholeLayerClone = mModel.GetExteriorTextureLayer().Clone();

    //
    // Update model
    //

    std::optional<ImageRect> affectedRect = DoTextureMagicWandEraseBackground(
        start,
        tolerance,
        isAntiAlias,
        doContiguousOnly,
        mModel.GetExteriorTextureLayer());

    if (affectedRect.has_value())
    {
        //
        // Update visualization
        //

        RegisterDirtyVisualization<VisualizationType::Game>(ExteriorImageRectToContainingShipSpaceRect(*affectedRect));
        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(*affectedRect);

        //
        // Create Undo
        //

        return ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
            *affectedRect,
            std::make_unique<TextureLayerData>(wholeLayerClone.MakeRegionBackup(*affectedRect)),
            nullptr);
    }

    return std::nullopt;
}

void ModelController::MakeExteriorLayerFromImage(
    TextureLayerData const & source,
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));
    assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    mModel.SetExteriorTextureLayer(
        DoMakeTextureLayerFromImage(
            source,
            sourceRegion,
            targetOrigin));

    //
    // Update visualization
    //

    mGameVisualizationTexture.reset();
    mGameVisualizationAutoTexturizationTexture.release();
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
}

void ModelController::RestoreExteriorTextureLayerComplete(
    std::unique_ptr<TextureLayerData> exteriorTextureLayer,
    std::optional<std::string> originalTextureArtCredits)
{
    assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.RestoreExteriorTextureLayer(std::move(exteriorTextureLayer));
    mModel.GetShipMetadata().ArtCredits = std::move(originalTextureArtCredits);

    //
    // Update visualization
    //

    mGameVisualizationTexture.reset();
    mGameVisualizationAutoTexturizationTexture.release();
    RegisterDirtyVisualization<VisualizationType::Game>(GetWholeShipRect());
    if (mModel.HasLayer(LayerType::ExteriorTexture))
    {
        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
    }
    else
    {
        // We've just removed the texture layer; we rely now on texture viz mode being set to None
        // before we UpdateVisualizations(), causing the View texture to be removed
    }
}

//
// Interior Texture
//

TextureLayerData const & ModelController::GetInteriorTextureLayer() const
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    return mModel.GetInteriorTextureLayer();
}

void ModelController::SetInteriorTextureLayer(TextureLayerData && textureLayer)
{
    mModel.SetInteriorTextureLayer(std::move(textureLayer));

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());

    mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = false;
}

void ModelController::RemoveInteriorTextureLayer()
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    auto const oldWholeTextureRect = GetWholeInteriorTextureRect();

    mModel.RemoveInteriorTextureLayer();

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(oldWholeTextureRect);

    mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = false;
}

std::unique_ptr<TextureLayerData> ModelController::CloneInteriorTextureLayer() const
{
    return mModel.CloneInteriorTextureLayer();
}

void ModelController::EraseInteriorTextureRegion(ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    DoInteriorTextureRegionErase(region);
}

void ModelController::EraseInteriorTextureRegionForEphemeralVisualization(ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    //
    // Update model
    //

    auto & textureLayerBuffer = mModel.GetInteriorTextureLayer().Buffer;

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            textureLayerBuffer[{x, y}].a = 0;
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(region);

    // Remember we are in temp visualization now
    mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = true;
}


std::optional<InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion> ModelController::InteriorTextureMagicWandEraseBackground(
    ImageCoordinates const & start,
    unsigned int tolerance,
    bool isAntiAlias,
    bool doContiguousOnly)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    //
    // Prepare Undo
    //

    auto wholeLayerClone = mModel.GetInteriorTextureLayer().Clone();

    //
    // Update model
    //

    std::optional<ImageRect> affectedRect = DoTextureMagicWandEraseBackground(
        start,
        tolerance,
        isAntiAlias,
        doContiguousOnly,
        mModel.GetInteriorTextureLayer());

    if (affectedRect.has_value())
    {
        //
        // Update visualization
        //

        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(*affectedRect);

        //
        // Create Undo
        //

        return InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
            *affectedRect,
            nullptr,
            std::make_unique<TextureLayerData>(wholeLayerClone.MakeRegionBackup(*affectedRect)));
    }

    return std::nullopt;
}

void ModelController::MakeInteriorLayerFromImage(
    TextureLayerData const & source,
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetOrigin)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));
    assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    mModel.SetInteriorTextureLayer(
        DoMakeTextureLayerFromImage(
            source,
            sourceRegion,
            targetOrigin));

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
}

void ModelController::RestoreInteriorTextureLayerComplete(std::unique_ptr<TextureLayerData> interiorTextureLayer)
{
    assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.RestoreInteriorTextureLayer(std::move(interiorTextureLayer));

    //
    // Update visualization
    //

    if (mModel.HasLayer(LayerType::InteriorTexture))
    {
        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
    }
    else
    {
        // We've just removed the texture layer; we rely now on texture viz mode being set to None
        // before we UpdateVisualizations(), causing the View texture to be remove
    }
}

//
// Multi
//

ShipSpaceBodyRegions ModelController::ShipSpaceRegionTransfer(
    ShipSpaceRect const & sourceRegion, // In current model
    ShipSpaceCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeShipRect()));

    // Prepare undo for erase
    auto sourceUndoPayload = CopyShipSpaceRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ShipSpaceRect> const adjustedDestinationRegion =
        ShipSpaceRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeShipRect());

    std::optional<ShipSpaceBodyRegions::payload_type> destinationUndoPayload;
    std::optional<ShipSpaceRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste undo
        destinationUndoPayload = CopyShipSpaceRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped paste source region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ShipSpaceRect(
            ShipSpaceCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::Structural))
    {
        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        // Erase - no analyses
        DoStructuralRegionFill(sourceRegion, nullptr);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - no analyses
            assert(sourceUndoPayload.LayerRegions.StructuralLayer);
            DoStructuralBlit(
                sourceUndoPayload.LayerRegions.StructuralLayer->Buffer,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Re-initialize layer analysis
        InitializeStructuralLayerAnalysis();
    }

    if (layerSelection.IsSet(LayerType::Electrical))
    {
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        //
        // Plan:
        //  - Erase: leave elec panel elements behind
        //  - Paste:
        //      - Can keep all pasted instance IDs, as they're gone (were from erased region)
        //          - It might resuscitate 1..N of the erased ones
        //      - It might kill overwritten ones
        //  - Final:
        //      - Redo elec layer analysis
        //      - Trim panel for those from Erase that might be gone (not pasted, i.e. not resuscitated)
        //

        // Erase - raw (no instance IDs, no panel, etc.)
        DoElectricalRegionFill(sourceRegion, nullptr);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - raw (no instance IDs, no panel, etc.)
            assert(sourceUndoPayload.LayerRegions.ElectricalLayer);
            DoElectricalBlit_Raw(
                sourceUndoPayload.LayerRegions.ElectricalLayer->Buffer,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Re-initialize layer analysis
        InitializeElectricalLayerAnalysis();

        // Trim panel for orphaned elements (i.e. the erased ones which were not pasted, i.e. not resuscitated)
        mModel.GetElectricalLayer().Panel.RemoveAbsentFrom(mInstancedElectricalElementSet.GetElements());

        VerifyElectricalLayerInvariants();
    }

    if (layerSelection.IsSet(LayerType::Ropes))
    {
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        // Erase - all ropes with at least one endpoint in region
        DoRopesRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - including ropes with endpoints outside of copied region
            assert(sourceUndoPayload.LayerRegions.RopesLayer);
            // TODO: args will change
            DoRopesBlit(
                // TODOHERE!
                sourceUndoPayload.LayerRegions.RopesLayer->Buffer, // TODOOLD: WAS: .CloneRegion(sourceRegion), // Source undo payload is _whole_ ropes buffer, thus endpoints are not shifted; we shift them here
                destinationPos, // Unclipped! For "hanging ropes"
                isTransparent);
        }

        // Re-initialize layer analysis
        InitializeRopesLayerAnalysis();
    }

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        assert(sourceUndoPayload.LayerRegions.ExteriorTextureLayer);

        ImageRect const sourceRegionTexture = ImageRect(
            ShipSpaceToExteriorTextureSpace(sourceRegion.origin),
            sourceUndoPayload.LayerRegions.ExteriorTextureLayer->Buffer.Size);

        // Erase
        DoExteriorTextureRegionErase(sourceRegionTexture);

        ImageCoordinates const destinationPosTexture = ShipSpaceToExteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionTexture =
            ImageRect(destinationPosTexture, sourceUndoPayload.LayerRegions.ExteriorTextureLayer->Buffer.Size)
            .MakeIntersectionWith(GetWholeExteriorTextureRect());

        if (adjustedDestinationRegionTexture.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedSourceRegionTexture(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionTexture->origin - destinationPosTexture),
                adjustedDestinationRegionTexture->size);

            // Paste
            DoExteriorTextureBlit(
                sourceUndoPayload.LayerRegions.ExteriorTextureLayer->Buffer,
                clippedSourceRegionTexture,
                adjustedDestinationRegionTexture->origin,
                isTransparent);
        }
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        assert(sourceUndoPayload.LayerRegions.InteriorTextureLayer);

        ImageRect const sourceRegionTexture = ImageRect(
            ShipSpaceToInteriorTextureSpace(sourceRegion.origin),
            sourceUndoPayload.LayerRegions.InteriorTextureLayer->Buffer.Size);

        // Erase
        DoInteriorTextureRegionErase(sourceRegionTexture);

        ImageCoordinates const destinationPosTexture = ShipSpaceToInteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionTexture =
            ImageRect(destinationPosTexture, sourceUndoPayload.LayerRegions.InteriorTextureLayer->Buffer.Size)
            .MakeIntersectionWith(GetWholeInteriorTextureRect());

        if (adjustedDestinationRegionTexture.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedSourceRegionTexture(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionTexture->origin - destinationPosTexture),
                adjustedDestinationRegionTexture->size);

            // Paste
            DoInteriorTextureBlit(
                sourceUndoPayload.LayerRegions.InteriorTextureLayer->Buffer,
                clippedSourceRegionTexture,
                adjustedDestinationRegionTexture->origin,
                isTransparent);
        }
    }

    //
    // Finalize undo
    //

    ShipSpaceBodyRegions undoPayload;

    undoPayload.AddRegion(std::move(sourceUndoPayload));

    if (destinationUndoPayload)
    {
        undoPayload.AddRegion(std::move(*destinationUndoPayload));
    }

    return undoPayload;
}

ExteriorTextureSpaceBodyRegions ModelController::ExteriorTextureSpaceRegionTransfer(
    ImageRect const & sourceRegion, // In current model
    ImageCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeExteriorTextureRect()));

    // Prepare undo for erase
    auto sourceUndoPayload = CopyExteriorTextureSpaceRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ImageRect> const adjustedDestinationRegion =
        ImageRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeExteriorTextureRect());

    std::optional<ExteriorTextureSpaceBodyRegions::payload_type> destinationUndoPayload;
    std::optional<ImageRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste undo
        destinationUndoPayload = CopyExteriorTextureSpaceRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped paste region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ImageRect(
            ImageCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        // Erase
        DoExteriorTextureRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste
            assert(sourceUndoPayload.ExteriorTextureLayerRegion);
            DoExteriorTextureBlit(
                sourceUndoPayload.ExteriorTextureLayerRegion->Buffer,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        assert(sourceUndoPayload.InteriorTextureLayerRegion);

        ImageRect const sourceRegionInterior = ImageRect(
            ExteriorTextureSpaceToInteriorTextureSpace(sourceRegion.origin),
            sourceUndoPayload.InteriorTextureLayerRegion->Buffer.Size);

        // Erase
        DoInteriorTextureRegionErase(sourceRegionInterior);

        ImageCoordinates const destinationPosInterior = ExteriorTextureSpaceToInteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionInterior =
            ImageRect(destinationPosInterior, sourceUndoPayload.InteriorTextureLayerRegion->Buffer.Size)
            .MakeIntersectionWith(GetWholeInteriorTextureRect());

        if (adjustedDestinationRegionInterior.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedSourceRegionInterior(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionInterior->origin - destinationPosInterior),
                adjustedDestinationRegionInterior->size);

            // Paste
            DoInteriorTextureBlit(
                sourceUndoPayload.InteriorTextureLayerRegion->Buffer,
                clippedSourceRegionInterior,
                adjustedDestinationRegionInterior->origin,
                isTransparent);
        }
    }

    //
    // Finalize undo
    //

    ExteriorTextureSpaceBodyRegions undoPayload;

    undoPayload.AddRegion(std::move(sourceUndoPayload));

    if (destinationUndoPayload)
    {
        undoPayload.AddRegion(std::move(*destinationUndoPayload));
    }

    return undoPayload;
}

InteriorTextureSpaceBodyRegions ModelController::InteriorTextureSpaceRegionTransfer(
    ImageRect const & sourceRegion, // In current model
    ImageCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeInteriorTextureRect()));

    // Prepare undo for erase
    auto sourceUndoPayload = CopyInteriorTextureSpaceRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ImageRect> const adjustedDestinationRegion =
        ImageRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeInteriorTextureRect());

    std::optional<InteriorTextureSpaceBodyRegions::payload_type> destinationUndoPayload;
    std::optional<ImageRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste undo
        destinationUndoPayload = CopyInteriorTextureSpaceRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped source region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ImageRect(
            ImageCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        assert(sourceUndoPayload.ExteriorTextureLayerRegion);

        ImageRect const sourceRegionExterior = ImageRect(
            InteriorTextureSpaceToExteriorTextureSpace(sourceRegion.origin),
            sourceUndoPayload.ExteriorTextureLayerRegion->Buffer.Size);

        // Erase
        DoExteriorTextureRegionErase(sourceRegionExterior);

        ImageCoordinates const destinationPosExterior = InteriorTextureSpaceToExteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionExterior =
            ImageRect(destinationPosExterior, sourceUndoPayload.ExteriorTextureLayerRegion->Buffer.Size)
            .MakeIntersectionWith(GetWholeExteriorTextureRect());

        if (adjustedDestinationRegionExterior.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedSourceRegionExterior(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionExterior->origin - destinationPosExterior),
                adjustedDestinationRegionExterior->size);

            // Paste
            DoExteriorTextureBlit(
                sourceUndoPayload.ExteriorTextureLayerRegion->Buffer,
                clippedSourceRegionExterior,
                adjustedDestinationRegionExterior->origin,
                isTransparent);
        }
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        // Erase
        DoInteriorTextureRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste
            assert(sourceUndoPayload.InteriorTextureLayerRegion);
            DoInteriorTextureBlit(
                sourceUndoPayload.InteriorTextureLayerRegion->Buffer,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }
    }

    //
    // Finalize undo
    //

    InteriorTextureSpaceBodyRegions undoPayload;

    undoPayload.AddRegion(std::move(sourceUndoPayload));

    if (destinationUndoPayload)
    {
        undoPayload.AddRegion(std::move(*destinationUndoPayload));
    }

    return undoPayload;
}

ShipSpaceEphemeralVisualizationRestoreRegions ModelController::ShipSpaceRegionTransferForEphemeralVisualization(
    ShipSpaceRect const & sourceRegion, // In current model
    ShipSpaceCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeShipRect()));

    // Prepare restore for erase
    auto sourceRestorePayload = MakeShipSpaceEphemeralVisualizationRestoreRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ShipSpaceRect> const adjustedDestinationRegion =
        ShipSpaceRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeShipRect());

    std::optional<ShipSpaceEphemeralVisualizationRestoreRegions::payload_type> destinationRestorePayload;
    std::optional<ShipSpaceRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste restore
        destinationRestorePayload = MakeShipSpaceEphemeralVisualizationRestoreRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped paste source region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ShipSpaceRect(
            ShipSpaceCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::Structural))
    {
        assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

        // Erase - no analyses
        DoStructuralRegionFill(sourceRegion, nullptr);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - no analyses
            assert(sourceRestorePayload.StructuralLayerBufferRegion);
            DoStructuralBlit(
                *sourceRestorePayload.StructuralLayerBufferRegion,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::Electrical))
    {
        assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

        // Erase - raw (no instance IDs, no panel, etc.)
        DoElectricalRegionFill(sourceRegion, nullptr);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - raw (no instance IDs, no panel, etc.)
            assert(sourceRestorePayload.ElectricalLayerBufferRegion);
            DoElectricalBlit_Raw(
                *sourceRestorePayload.ElectricalLayerBufferRegion,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::Ropes))
    {
        assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

        // Erase - all ropes with at least one endpoint in region
        DoRopesRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste - including ropes with endpoints outside of copied region
            assert(sourceRestorePayload.RopesLayerBuffer);
            // TODO: args will change
            DoRopesBlit(
                // TODOHERE!
                *sourceRestorePayload.RopesLayerBuffer, // TODOOLD: WAS: ->CloneRegion(sourceRegion), // Source restore payload is _whole_ ropes buffer, thus endpoints are not shifted; we shift them here
                destinationPos, // Unclipped! For "hanging ropes"
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        assert(sourceRestorePayload.ExteriorTextureLayerBufferRegion);

        ImageRect const sourceRegionTexture = ImageRect(
            ShipSpaceToExteriorTextureSpace(sourceRegion.origin),
            sourceRestorePayload.ExteriorTextureLayerBufferRegion->Size);

        // Erase
        DoExteriorTextureRegionErase(sourceRegionTexture);

        ImageCoordinates const destinationPosTexture = ShipSpaceToExteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionTexture =
            ImageRect(destinationPosTexture, sourceRestorePayload.ExteriorTextureLayerBufferRegion->Size)
            .MakeIntersectionWith(GetWholeExteriorTextureRect());

        if (adjustedDestinationRegionTexture.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedPasteSourceRegionTexture(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionTexture->origin - destinationPosTexture),
                adjustedDestinationRegionTexture->size);

            // Paste
            DoExteriorTextureBlit(
                *sourceRestorePayload.ExteriorTextureLayerBufferRegion,
                clippedPasteSourceRegionTexture,
                adjustedDestinationRegionTexture->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        assert(sourceRestorePayload.InteriorTextureLayerBufferRegion);

        ImageRect const sourceRegionTexture = ImageRect(
            ShipSpaceToInteriorTextureSpace(sourceRegion.origin),
            sourceRestorePayload.InteriorTextureLayerBufferRegion->Size);

        // Erase
        DoInteriorTextureRegionErase(sourceRegionTexture);

        ImageCoordinates const destinationPosTexture = ShipSpaceToInteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionTexture =
            ImageRect(destinationPosTexture, sourceRestorePayload.InteriorTextureLayerBufferRegion->Size)
            .MakeIntersectionWith(GetWholeInteriorTextureRect());

        if (adjustedDestinationRegionTexture.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedPasteSourceRegionTexture(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionTexture->origin - destinationPosTexture),
                adjustedDestinationRegionTexture->size);

            // Paste
            DoInteriorTextureBlit(
                *sourceRestorePayload.InteriorTextureLayerBufferRegion,
                clippedPasteSourceRegionTexture,
                adjustedDestinationRegionTexture->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = true;
    }

    //
    // Finalize restore
    //

    ShipSpaceEphemeralVisualizationRestoreRegions restorePayload;

    restorePayload.AddRegion(std::move(sourceRestorePayload));

    if (destinationRestorePayload)
    {
        restorePayload.AddRegion(std::move(*destinationRestorePayload));
    }

    return restorePayload;
}

ExteriorTextureSpaceEphemeralVisualizationRestoreRegions ModelController::ExteriorTextureSpaceRegionTransferForEphemeralVisualization(
    ImageRect const & sourceRegion, // In current model
    ImageCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeExteriorTextureRect()));

    // Prepare restore for erase
    auto sourceRestorePayload = MakeExteriorTextureSpaceEphemeralVisualizationRestoreRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ImageRect> const adjustedDestinationRegion =
        ImageRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeExteriorTextureRect());

    std::optional<ExteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type> destinationRestorePayload;
    std::optional<ImageRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste restore
        destinationRestorePayload = MakeExteriorTextureSpaceEphemeralVisualizationRestoreRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped paste source region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ImageRect(
            ImageCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        // Erase
        DoExteriorTextureRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste
            assert(sourceRestorePayload.ExteriorTextureLayerBufferRegion);
            DoExteriorTextureBlit(
                *sourceRestorePayload.ExteriorTextureLayerBufferRegion,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        assert(sourceRestorePayload.InteriorTextureLayerBufferRegion);

        ImageRect const sourceRegionInterior = ImageRect(
            ExteriorTextureSpaceToInteriorTextureSpace(sourceRegion.origin),
            sourceRestorePayload.InteriorTextureLayerBufferRegion->Size);

        // Erase
        DoInteriorTextureRegionErase(sourceRegionInterior);

        ImageCoordinates const destinationPosInterior = ExteriorTextureSpaceToInteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionInterior =
            ImageRect(destinationPosInterior, sourceRestorePayload.InteriorTextureLayerBufferRegion->Size)
            .MakeIntersectionWith(GetWholeInteriorTextureRect());

        if (adjustedDestinationRegionInterior.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedPasteSourceRegionInterior(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionInterior->origin - destinationPosInterior),
                adjustedDestinationRegionInterior->size);

            // Paste
            DoInteriorTextureBlit(
                *sourceRestorePayload.InteriorTextureLayerBufferRegion,
                clippedPasteSourceRegionInterior,
                adjustedDestinationRegionInterior->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = true;
    }

    //
    // Finalize restore
    //

    ExteriorTextureSpaceEphemeralVisualizationRestoreRegions restorePayload;

    restorePayload.AddRegion(std::move(sourceRestorePayload));

    if (destinationRestorePayload)
    {
        restorePayload.AddRegion(std::move(*destinationRestorePayload));
    }

    return restorePayload;
}

InteriorTextureSpaceEphemeralVisualizationRestoreRegions ModelController::InteriorTextureSpaceRegionTransferForEphemeralVisualization(
    ImageRect const & sourceRegion, // In current model
    ImageCoordinates const & destinationPos,
    LayerSelector const & layerSelection,
    bool isTransparent)
{
    // Source region expected to be in ship
    assert(sourceRegion.IsContainedInRect(GetWholeInteriorTextureRect()));

    // Prepare restore for erase
    auto sourceRestorePayload = MakeInteriorTextureSpaceEphemeralVisualizationRestoreRegion(sourceRegion, layerSelection);

    // Clamp destination region to ship - won't have one if target outside
    // of ship
    std::optional<ImageRect> const adjustedDestinationRegion =
        ImageRect(destinationPos, sourceRegion.size)
        .MakeIntersectionWith(GetWholeInteriorTextureRect());

    std::optional<InteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type> destinationRestorePayload;
    std::optional<ImageRect> clippedSourceRegion;

    if (adjustedDestinationRegion.has_value())
    {
        // Prepare paste restore
        destinationRestorePayload = MakeInteriorTextureSpaceEphemeralVisualizationRestoreRegion(
            *adjustedDestinationRegion,
            layerSelection);

        // Calculate clipped paste source region - expected to fit in ship once translated to target pos
        clippedSourceRegion = ImageRect(
            ImageCoordinates(0, 0) + (adjustedDestinationRegion->origin - destinationPos),
            adjustedDestinationRegion->size);
    }

    assert(!adjustedDestinationRegion.has_value() || clippedSourceRegion.has_value());

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

        assert(sourceRestorePayload.ExteriorTextureLayerBufferRegion);

        ImageRect const sourceRegionExterior = ImageRect(
            InteriorTextureSpaceToExteriorTextureSpace(sourceRegion.origin),
            sourceRestorePayload.ExteriorTextureLayerBufferRegion->Size);

        // Erase
        DoExteriorTextureRegionErase(sourceRegionExterior);

        ImageCoordinates const destinationPosExterior = InteriorTextureSpaceToExteriorTextureSpace(destinationPos);

        std::optional<ImageRect> const adjustedDestinationRegionExterior =
            ImageRect(destinationPosExterior, sourceRestorePayload.ExteriorTextureLayerBufferRegion->Size)
            .MakeIntersectionWith(GetWholeExteriorTextureRect());

        if (adjustedDestinationRegionExterior.has_value())
        {
            // Calculate clipped source region - expected to fit in ship once translated to target pos
            ImageRect const clippedPasteSourceRegionExterior(
                ImageCoordinates(0, 0) + (adjustedDestinationRegionExterior->origin - destinationPosExterior),
                adjustedDestinationRegionExterior->size);

            // Paste
            DoExteriorTextureBlit(
                *sourceRestorePayload.ExteriorTextureLayerBufferRegion,
                clippedPasteSourceRegionExterior,
                adjustedDestinationRegionExterior->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = true;
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

        // Erase
        DoInteriorTextureRegionErase(sourceRegion);

        if (adjustedDestinationRegion.has_value())
        {
            // Paste
            assert(sourceRestorePayload.InteriorTextureLayerBufferRegion);
            DoInteriorTextureBlit(
                *sourceRestorePayload.InteriorTextureLayerBufferRegion,
                *clippedSourceRegion,
                adjustedDestinationRegion->origin,
                isTransparent);
        }

        // Remember we are in temp visualization now
        mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = true;
    }

    //
    // Finalize restore
    //

    InteriorTextureSpaceEphemeralVisualizationRestoreRegions restorePayload;

    restorePayload.AddRegion(std::move(sourceRestorePayload));

    if (destinationRestorePayload)
    {
        restorePayload.AddRegion(std::move(*destinationRestorePayload));
    }

    return restorePayload;
}

template<>
std::optional<ShipSpaceRect> ModelController::ReplaceCustomMaterials<LayerType::Structural>(std::map<StructuralMaterial const *, StructuralMaterial const *> const & replacementMap)
{
    assert(mModel.HasLayer(LayerType::Structural));
    assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect;

    auto & layer = mModel.GetStructuralLayer();
    ShipSpaceSize const shipSize = mModel.GetShipSize();
    for (int y = 0; y < shipSize.height; ++y)
    {
        for (int x = 0; x < shipSize.width; ++x)
        {
            ShipSpaceCoordinates const coords(x, y);

            auto const srchIt = replacementMap.find(layer.Buffer[coords].Material);
            if (srchIt != replacementMap.end())
            {
                WriteParticle(coords, srchIt->second);

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

template<>
std::optional<ShipSpaceRect> ModelController::ReplaceCustomMaterials<LayerType::Electrical>(std::map<ElectricalMaterial const *, ElectricalMaterial const *> const & replacementMap)
{
    assert(mModel.HasLayer(LayerType::Electrical)); // Assuming caller has checked
    assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect;

    auto & layer = mModel.GetElectricalLayer();
    ShipSpaceSize const shipSize = mModel.GetShipSize();
    for (int y = 0; y < shipSize.height; ++y)
    {
        for (int x = 0; x < shipSize.width; ++x)
        {
            ShipSpaceCoordinates const coords(x, y);

            auto const srchIt = replacementMap.find(layer.Buffer[coords].Material);
            if (srchIt != replacementMap.end())
            {
                WriteParticle(coords, srchIt->second);

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

    if (affectedRect.has_value())
    {
        //
        // Update visualization
        //

        RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(*affectedRect);
    }

    return affectedRect;
}

template<>
std::optional<ShipSpaceRect> ModelController::ReplaceCustomMaterials<LayerType::Ropes>(std::map<StructuralMaterial const *, StructuralMaterial const *> const & replacementMap)
{
    assert(mModel.HasLayer(LayerType::Ropes)); // Assuming caller has checked
    assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);

    //
    // Update model
    //

    std::optional<ShipSpaceRect> affectedRect;

    auto & layer = mModel.GetRopesLayer();
    for (auto ropeElementIt = layer.Buffer.begin(); ropeElementIt != layer.Buffer.end(); /*incremented in loop*/)
    {
        auto const srchIt = replacementMap.find(ropeElementIt->Material);
        if (srchIt != replacementMap.end())
        {
            if (srchIt->second != nullptr)
            {
                // Update element
                ropeElementIt->Material = srchIt->second;
                ropeElementIt->RenderColor = srchIt->second->RenderColor;

                // Advance
                ++ropeElementIt;
            }
            else
            {
                // Nuke element
                ropeElementIt = layer.Buffer.Erase(ropeElementIt);
            }

            affectedRect = GetWholeShipRect();
        }
        else
        {
            // Advance
            ++ropeElementIt;
        }
    }

    if (affectedRect.has_value())
    {
        //
        // Update visualization
        //

        RegisterDirtyVisualization<VisualizationType::RopesLayer>(*affectedRect);
    }

    return affectedRect;
}

//
// Visualizations
//

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

void ModelController::SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType mode)
{
    if (mode == mExteriorTextureLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != ExteriorTextureLayerVisualizationModeType::None)
    {
        mExteriorTextureLayerVisualizationMode = mode;

        assert(mModel.HasLayer(LayerType::ExteriorTexture));

        RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(GetWholeExteriorTextureRect());
    }
    else
    {
        // Shutdown texture visualization
        mExteriorTextureLayerVisualizationMode = ExteriorTextureLayerVisualizationModeType::None;
    }
}

void ModelController::SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType mode)
{
    if (mode == mInteriorTextureLayerVisualizationMode)
    {
        // Nop
        return;
    }

    if (mode != InteriorTextureLayerVisualizationModeType::None)
    {
        mInteriorTextureLayerVisualizationMode = mode;

        assert(mModel.HasLayer(LayerType::InteriorTexture));

        RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(GetWholeInteriorTextureRect());
    }
    else
    {
        // Shutdown texture visualization
        mInteriorTextureLayerVisualizationMode = InteriorTextureLayerVisualizationModeType::None;
    }
}

void ModelController::UpdateVisualizations(
    View & view,
    GameAssetManager const & gameAssetManager)
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
            mGameVisualizationTextureMagnificationFactor = ShipTexturizer::CalculateHighDefinitionTextureMagnificationFactor(
                mModel.GetShipSize(),
                ShipTexturizer::MaxHighDefinitionTextureSize);
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

        mDirtyGameVisualizationRegions.Apply(
            [&](ShipSpaceRect const & region)
            {
                // Update visualization
                ImageRect const dirtyTextureRegion = UpdateGameVisualization(region, gameAssetManager);

                // Upload visualization
                if (dirtyTextureRegion != ImageRect(mGameVisualizationTexture->Size))
                {
                    //
                    // For better performance, we only upload the dirty sub-texture
                    //

                    auto subTexture = RgbaImageData(dirtyTextureRegion.size);
                    subTexture.BlitFromRegion(
                        *mGameVisualizationTexture,
                        dirtyTextureRegion,
                        { 0, 0 });

                    view.UpdateGameVisualization(
                        subTexture,
                        dirtyTextureRegion.origin);
                }
                else
                {
                    // Upload whole texture
                    view.UploadGameVisualization(*mGameVisualizationTexture);
                }
            });
    }
    else
    {
        assert(!mGameVisualizationTexture);

        if (view.HasGameVisualization())
        {
            view.RemoveGameVisualization();
        }
    }

    mDirtyGameVisualizationRegions.Reset();

    // Structural

    if (mStructuralLayerVisualizationMode != StructuralLayerVisualizationModeType::None)
    {
        if (!mStructuralLayerVisualizationTexture)
        {
            // Initialize structural visualization
            mStructuralLayerVisualizationTexture = std::make_unique<RgbaImageData>(ImageSize(mModel.GetShipSize().width, mModel.GetShipSize().height));
        }

        if (!mDirtyStructuralLayerVisualizationRegions.IsEmpty()) // Needs only be done once
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
        }

        mDirtyStructuralLayerVisualizationRegions.Apply(
            [&](ShipSpaceRect const & region)
            {
                // Update visualization
                ImageRect const dirtyTextureRegion = UpdateStructuralLayerVisualization(region);

                // Upload visualization
                if (dirtyTextureRegion != ImageRect(mStructuralLayerVisualizationTexture->Size))
                {
                    //
                    // For better performance, we only upload the dirty sub-texture
                    //

                    auto subTexture = RgbaImageData(dirtyTextureRegion.size);
                    subTexture.BlitFromRegion(
                        *mStructuralLayerVisualizationTexture,
                        dirtyTextureRegion,
                        { 0, 0 });

                    view.UpdateStructuralLayerVisualization(
                        subTexture,
                        dirtyTextureRegion.origin);
                }
                else
                {
                    // Upload whole texture
                    view.UploadStructuralLayerVisualization(*mStructuralLayerVisualizationTexture);
                }
            });
    }
    else
    {
        assert(!mStructuralLayerVisualizationTexture);

        if (view.HasStructuralLayerVisualization())
        {
            view.RemoveStructuralLayerVisualization();
        }
    }

    if (mDirtyStructuralLayerVisualizationRegions.GetRegionCount() >= 32)
    {
        LogMessage("ModelController::UpdateVisualizations: # of dirty regions: ", mDirtyStructuralLayerVisualizationRegions.GetRegionCount());
    }

    mDirtyStructuralLayerVisualizationRegions.Reset();

    // Electrical

    if (mElectricalLayerVisualizationMode != ElectricalLayerVisualizationModeType::None)
    {
        if (!mElectricalLayerVisualizationTexture)
        {
            // Initialize electrical visualization
            mElectricalLayerVisualizationTexture = std::make_unique<RgbaImageData>(mModel.GetShipSize().width, mModel.GetShipSize().height);
        }

        mDirtyElectricalLayerVisualizationRegions.Apply(
            [&](ShipSpaceRect const & region)
            {
                // Update visualization
                UpdateElectricalLayerVisualization(region);

                // Upload visualization
                view.UploadElectricalLayerVisualization(*mElectricalLayerVisualizationTexture);
            });
    }
    else
    {
        assert(!mElectricalLayerVisualizationTexture);

        if (view.HasElectricalLayerVisualization())
        {
            view.RemoveElectricalLayerVisualization();
        }
    }

    mDirtyElectricalLayerVisualizationRegions.Reset();

    // Ropes

    if (mRopesLayerVisualizationMode != RopesLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::Ropes));

        // Ropes viz is whole buffer, just do it if we have at least one region
        if (!mDirtyRopesLayerVisualizationRegions.IsEmpty())
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

    mDirtyRopesLayerVisualizationRegions.Reset();

    // Exterior Texture

    if (mExteriorTextureLayerVisualizationMode != ExteriorTextureLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::ExteriorTexture));

        mDirtyExteriorTextureLayerVisualizationRegions.Apply(
            [&](ImageRect const & region)
            {
                // Update visualization
                UpdateExteriorTextureLayerVisualization(); // Dirty region not needed for updating viz in this implementation

                // Upload visualization
                if (region != ImageRect(mModel.GetExteriorTextureLayer().Buffer.Size))
                {
                    //
                    // For better performance, we only upload the dirty sub-texture
                    //

                    auto subTexture = RgbaImageData(region.size);
                    subTexture.BlitFromRegion(
                        mModel.GetExteriorTextureLayer().Buffer,
                        region,
                        { 0, 0 });

                    view.UpdateExteriorTextureLayerVisualization(
                        subTexture,
                        region.origin);
                }
                else
                {
                    // Upload whole texture
                    view.UploadExteriorTextureLayerVisualization(mModel.GetExteriorTextureLayer().Buffer);
                }
            });
    }
    else
    {
        if (view.HasExteriorTextureLayerVisualization())
        {
            view.RemoveExteriorTextureLayerVisualization();
        }
    }

    mDirtyExteriorTextureLayerVisualizationRegions.Reset();

    // Interior Texture

    if (mInteriorTextureLayerVisualizationMode != InteriorTextureLayerVisualizationModeType::None)
    {
        assert(mModel.HasLayer(LayerType::InteriorTexture));

        mDirtyInteriorTextureLayerVisualizationRegions.Apply(
            [&](ImageRect const & region)
            {
                // Update visualization
                UpdateInteriorTextureLayerVisualization(); // Dirty region not needed for updating viz in this implementation

                // Upload visualization
                if (region != ImageRect(mModel.GetInteriorTextureLayer().Buffer.Size))
                {
                    //
                    // For better performance, we only upload the dirty sub-texture
                    //

                    auto subTexture = RgbaImageData(region.size);
                    subTexture.BlitFromRegion(
                        mModel.GetInteriorTextureLayer().Buffer,
                        region,
                        { 0, 0 });

                    view.UpdateInteriorTextureLayerVisualization(
                        subTexture,
                        region.origin);
                }
                else
                {
                    // Upload whole texture
                    view.UploadInteriorTextureLayerVisualization(mModel.GetInteriorTextureLayer().Buffer);
                }
            });
    }
    else
    {
        if (view.HasInteriorTextureLayerVisualization())
        {
            view.RemoveInteriorTextureLayerVisualization();
        }
    }

    mDirtyInteriorTextureLayerVisualizationRegions.Reset();
}

//
// Coords
//

ShipSpaceRect ModelController::ImageRectToContainingShipSpaceRect(
    ImageRect const & imageRect,
    vec2f const & shipToImageFactor) const
{
    // Here we map the texture coords to the ship coords in the same "texturing" fashion as we do for rendering;
    // we do this because the texture for a particle at ship coords (x, y) is sampled at the center of the
    // texture's quad for that particle.
    //
    // Here we want to calculate the ship rect whose tessellation guarantees coverage of this texture rect.
    // By observing that the pixel at coordinate t is covered by the line between ship coordinate s(t) and s(t+1),
    // we create this covering rect using s(t_start) and s(t_end) + 1, where t_end is the last texture coordinate
    // (i.e. it's t_start + t_size - 1).
    //
    // The formula for s(t) is the "texturization" one, i.s. e = (t - o/2) / o, where o is the number of texture
    // pixels in one ship quad.

    ShipSpaceCoordinates const s_start = ShipSpaceCoordinates::FromFloatFloor(
        vec2f(
            (static_cast<float>(imageRect.origin.x) - shipToImageFactor.x / 2.0f) / shipToImageFactor.x,
            (static_cast<float>(imageRect.origin.y) - shipToImageFactor.y / 2.0f) / shipToImageFactor.y));
    ShipSpaceCoordinates const s_end = ShipSpaceCoordinates::FromFloatFloor(
        vec2f(
            (static_cast<float>(imageRect.origin.x + imageRect.size.width - 1) - shipToImageFactor.x / 2.0f) / shipToImageFactor.x + 1.0f,
            (static_cast<float>(imageRect.origin.y + imageRect.size.height - 1) - shipToImageFactor.y / 2.0f) / shipToImageFactor.y + 1.0f));

    ShipSpaceRect containingRect(
        s_start,
        s_end - s_start + ShipSpaceSize(1, 1)); // size = (end - start) + 1

    // Make sure fits
    auto const intersection = containingRect.MakeIntersectionWith(ShipSpaceRect(GetShipSize()));
    assert(intersection.has_value());
    return *intersection;
}

vec2f ModelController::GetShipSpaceToTextureSpaceFactor(
    ShipSpaceSize const & shipSize,
    ImageSize const & textureSize)
{
    return vec2f(
        static_cast<float>(textureSize.width) / static_cast<float>(shipSize.width),
        static_cast<float>(textureSize.height) / static_cast<float>(shipSize.height));
}

vec2f ModelController::GetTextureSpaceToShipSpaceFactor(
    ImageSize const & textureSize,
    ShipSpaceSize const & shipSize)
{
    return vec2f(
        static_cast<float>(shipSize.width) / static_cast<float>(textureSize.width),
        static_cast<float>(shipSize.height) / static_cast<float>(textureSize.height));
}

vec2f ModelController::GetTextureSpaceToTextureSpaceFactor(
    ImageSize const & sourceTextureSize,
    ImageSize const & targetTextureSize)
{
    return vec2f(
        static_cast<float>(targetTextureSize.width) / static_cast<float>(sourceTextureSize.width),
        static_cast<float>(targetTextureSize.height) / static_cast<float>(sourceTextureSize.height));
}

//
// Misc
//

ShipSpaceBodyRegions::ShipSpaceBodyRegion ModelController::MakeShipBodyRegionClip(
    ShipSpaceBodyRegions::ShipSpaceBodyRegion const & source,
    ShipSpaceRect const & clipRegion)
{
    assert(clipRegion.IsContainedInRect(source.OriginatingRegion));
    assert(clipRegion.IsContainedInRect(ShipSpaceRect(source.LayerRegions.Size)));

    // Clip regions

    std::unique_ptr<StructuralLayerData> clippedStructuralLayerData;
    if (source.LayerRegions.StructuralLayer)
    {
        clippedStructuralLayerData = std::make_unique<StructuralLayerData>(source.LayerRegions.StructuralLayer->MakeRegionBackup(clipRegion));
    }

    std::unique_ptr<ElectricalLayerData> clippedElectricalLayerData;
    if (source.LayerRegions.ElectricalLayer)
    {
        clippedElectricalLayerData = std::make_unique<ElectricalLayerData>(source.LayerRegions.ElectricalLayer->MakeRegionBackup(clipRegion)); // Panel is whole
    }

    std::unique_ptr<RopesLayerData> clippedRopesLayerData;
    if (source.LayerRegions.RopesLayer)
    {
        clippedRopesLayerData = std::make_unique<RopesLayerData>(source.LayerRegions.RopesLayer->MakeBackup()); // Whole
    }

    std::unique_ptr<TextureLayerData> clippedExteriorTextureLayerData;
    if (source.LayerRegions.ExteriorTextureLayer)
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(source.GetSize(), source.LayerRegions.ExteriorTextureLayer->Buffer.Size);
        ImageRect const exteriorTextureClipRegion = ImageRect(
            ImageCoordinates::FromFloatFloor(clipRegion.origin.ToFloat().scale(shipToTexture)),
            ImageSize::FromFloatFloor(clipRegion.size.ToFloat<vec2f>().scale(shipToTexture)));

        clippedExteriorTextureLayerData = std::make_unique<TextureLayerData>(source.LayerRegions.ExteriorTextureLayer->MakeRegionBackup(exteriorTextureClipRegion));
    }

    std::unique_ptr<TextureLayerData> clippedInteriorTextureLayerData;
    if (source.LayerRegions.InteriorTextureLayer)
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(source.GetSize(), source.LayerRegions.InteriorTextureLayer->Buffer.Size);
        ImageRect const interiorTextureClipRegion = ImageRect(
            ImageCoordinates::FromFloatFloor(clipRegion.origin.ToFloat().scale(shipToTexture)),
            ImageSize::FromFloatFloor(clipRegion.size.ToFloat<vec2f>().scale(shipToTexture)));

        clippedInteriorTextureLayerData = std::make_unique<TextureLayerData>(source.LayerRegions.InteriorTextureLayer->MakeRegionBackup(interiorTextureClipRegion));
    }

    // Calculate new "absolute" originating region
    ShipSpaceRect newOriginatingRegion = ShipSpaceRect(
        ShipSpaceCoordinates(
            source.OriginatingRegion.origin.x + clipRegion.origin.x,
            source.OriginatingRegion.origin.y + clipRegion.origin.y),
        clipRegion.size); // ClipRegion is smaller anyway

    return ShipSpaceBodyRegions::ShipSpaceBodyRegion(
        newOriginatingRegion,
        std::move(clippedStructuralLayerData),
        std::move(clippedElectricalLayerData),
        std::move(clippedRopesLayerData),
        std::move(clippedExteriorTextureLayerData),
        std::move(clippedExteriorTextureLayerData));
}

ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion ModelController::MakeShipBodyRegionClip(
    ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion const & source,
    ImageRect const & clipRegion)
{
    assert(clipRegion.IsContainedInRect(source.OriginatingRegion));
    assert(clipRegion.IsContainedInRect(ImageRect(source.GetSize())));

    // Clip regions

    std::unique_ptr<TextureLayerData> clippedExteriorTextureLayerData;
    if (source.ExteriorTextureLayerRegion)
    {
        clippedExteriorTextureLayerData = std::make_unique<TextureLayerData>(source.ExteriorTextureLayerRegion->MakeRegionBackup(clipRegion));
    }

    std::unique_ptr<TextureLayerData> clippedInteriorTextureLayerData;
    if (source.InteriorTextureLayerRegion)
    {
        vec2f const externalToInternal = GetTextureSpaceToTextureSpaceFactor(source.GetSize(), source.InteriorTextureLayerRegion->Buffer.Size);
        ImageRect const interiorTextureClipRegion = ImageRect(
            ImageCoordinates::FromFloatFloor(clipRegion.origin.ToFloat().scale(externalToInternal)),
            ImageSize::FromFloatFloor(clipRegion.size.ToFloat<vec2f>().scale(externalToInternal)));

        clippedInteriorTextureLayerData = std::make_unique<TextureLayerData>(source.InteriorTextureLayerRegion->MakeRegionBackup(interiorTextureClipRegion));
    }

    // Calculate new "absolute" originating region
    ImageRect newOriginatingRegion = ImageRect(
        ImageCoordinates(
            source.OriginatingRegion.origin.x + clipRegion.origin.x,
            source.OriginatingRegion.origin.y + clipRegion.origin.y),
        clipRegion.size); // ClipRegion is smaller anyway

    return ExteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
        newOriginatingRegion,
        std::move(clippedExteriorTextureLayerData),
        std::move(clippedInteriorTextureLayerData));
}

InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion ModelController::MakeShipBodyRegionClip(
    InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion const & source,
    ImageRect const & clipRegion)
{
    assert(clipRegion.IsContainedInRect(source.OriginatingRegion));
    assert(clipRegion.IsContainedInRect(ImageRect(source.GetSize())));

    // Clip regions

    std::unique_ptr<TextureLayerData> clippedExteriorTextureLayerData;
    if (source.ExteriorTextureLayerRegion)
    {
        vec2f const internalToExternal = GetTextureSpaceToTextureSpaceFactor(source.GetSize(), source.ExteriorTextureLayerRegion->Buffer.Size);
        ImageRect const exteriorTextureClipRegion = ImageRect(
            ImageCoordinates::FromFloatFloor(clipRegion.origin.ToFloat().scale(internalToExternal)),
            ImageSize::FromFloatFloor(clipRegion.size.ToFloat<vec2f>().scale(internalToExternal)));

        clippedExteriorTextureLayerData = std::make_unique<TextureLayerData>(source.ExteriorTextureLayerRegion->MakeRegionBackup(exteriorTextureClipRegion));
    }

    std::unique_ptr<TextureLayerData> clippedInteriorTextureLayerData;
    if (source.InteriorTextureLayerRegion)
    {
        clippedInteriorTextureLayerData = std::make_unique<TextureLayerData>(source.InteriorTextureLayerRegion->MakeRegionBackup(clipRegion));
    }

    // Calculate new "absolute" originating region
    ImageRect newOriginatingRegion = ImageRect(
        ImageCoordinates(
            source.OriginatingRegion.origin.x + clipRegion.origin.x,
            source.OriginatingRegion.origin.y + clipRegion.origin.y),
        clipRegion.size); // ClipRegion is smaller anyway

    return InteriorTextureSpaceBodyRegions::TextureSpaceBodyRegion(
        newOriginatingRegion,
        std::move(clippedExteriorTextureLayerData),
        std::move(clippedInteriorTextureLayerData));
}

LayerSelector ModelController::IntersectLayerPresence(ShipLayers const & otherSource) const
{
    LayerSelector layers;

    if (otherSource.StructuralLayer && mModel.HasLayer(LayerType::Structural))
    {
        layers.Add(LayerType::Structural);
    }

    if (otherSource.ElectricalLayer && mModel.HasLayer(LayerType::Electrical))
    {
        layers.Add(LayerType::Electrical);
    }

    if (otherSource.RopesLayer && mModel.HasLayer(LayerType::Ropes))
    {
        layers.Add(LayerType::Ropes);
    }

    if (otherSource.ExteriorTextureLayer && mModel.HasLayer(LayerType::ExteriorTexture))
    {
        layers.Add(LayerType::ExteriorTexture);
    }

    if (otherSource.InteriorTextureLayer && mModel.HasLayer(LayerType::InteriorTexture))
    {
        layers.Add(LayerType::InteriorTexture);
    }

    return layers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelController::InitializeStructuralLayerAnalysis()
{
    mMassParticleCount = 0;
    mTotalMass = 0.0f;
    mCenterOfMassSum = vec2f::zero();

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    for (int y = 0; y < structuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayerBuffer.Size.width; ++x)
        {
            ShipSpaceCoordinates const coords{ x, y };
            if (structuralLayerBuffer[coords].Material != nullptr)
            {
                auto const mass = structuralLayerBuffer[coords].Material->GetMass();

                ++mMassParticleCount;
                mTotalMass += mass;
                mCenterOfMassSum += coords.ToFloat() * mass;
            }
        }
    }
}

void ModelController::InitializeElectricalLayerAnalysis()
{
    // Reset factory
    mInstancedElectricalElementSet.Reset();

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
                    mInstancedElectricalElementSet.Register(electricalLayerBuffer.Data[i].InstanceIndex, electricalLayerBuffer.Data[i].Material);
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
    // Write particle and maintain analysis

    auto & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    if (structuralLayerBuffer[coords].Material != nullptr)
    {
        auto const mass = structuralLayerBuffer[coords].Material->GetMass();

        assert(mMassParticleCount > 0);
        --mMassParticleCount;
        if (mMassParticleCount == 0)
        {
            mTotalMass = 0.0f;
        }
        else
        {
            mTotalMass -= mass;
        }

        mCenterOfMassSum -= coords.ToFloat() * mass;
    }

    structuralLayerBuffer[coords] = StructuralElement(material);

    if (material != nullptr)
    {
        auto const mass = material->GetMass();

        ++mMassParticleCount;
        mTotalMass += mass;
        mCenterOfMassSum += coords.ToFloat() * mass;
    }
}

template<bool IsForEphViz>
void ModelController::DoStructuralRectangle(
    ShipSpaceRect const & rect,
    std::uint32_t lineSize,
    StructuralMaterial const * lineMaterial,
    std::optional<StructuralMaterial const *> fillMaterial)
{
    auto & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    int const yLineBottom = rect.origin.y + static_cast<int>(lineSize);
    int const yLineTop = rect.origin.y + rect.size.height - static_cast<int>(lineSize);
    int const yTop = rect.origin.y + rect.size.height;

    int const xLineLeft = rect.origin.x + static_cast<int>(lineSize);
    int const xLineRight = rect.origin.x + rect.size.width - static_cast<int>(lineSize);
    int const xRight = rect.origin.x + rect.size.width;

    for (int y = rect.origin.y; y < yTop; ++y)
    {
        for (int x = rect.origin.x; x < xRight; ++x)
        {
            ShipSpaceCoordinates const coords(x, y);

            if (y >= yLineBottom && y < yLineTop
                && x >= xLineLeft && x < xLineRight)
            {
                // Fill
                if (fillMaterial)
                {
                    if constexpr (IsForEphViz)
                    {
                        structuralLayerBuffer[coords] = StructuralElement(*fillMaterial);
                    }
                    else
                    {
                        WriteParticle(coords, *fillMaterial);
                    }
                }
            }
            else
            {
                // Line
                if constexpr (IsForEphViz)
                {
                    structuralLayerBuffer[coords] = StructuralElement(lineMaterial);
                }
                else
                {
                    WriteParticle(coords, lineMaterial);
                }
            }
        }
    }
}

typename LayerTypeTraits<LayerType::Structural>::buffer_type ModelController::MakeStructureTrace(
    ShipSpaceRect const & shipRect,
    ImageRect const & textureRect,
    StructuralMaterial const * edgeMaterial,
    StructuralMaterial const * fillMaterial,
    std::uint8_t alphaThreshold) const
{
    // Here we map the texture coords to the ship coords in the same "texturing" fashion as we do for rendering;
    // we do this because the texture for a particle at ship coords (x, y) is sampled at the center of the
    // texture's quad for that particle.
    //
    // Here we want to calculate the ship rect whose tessellation guarantees coverage of this texture rect.
    // By observing that the pixel at coordinate t is covered by the line between ship coordinate s(t) and s(t+1),
    // we set edge particles as follows:
    //  Out->In transition: we detect entering texture image at texture coordinates t; assuming more particles will
    //                      follow, we place an edge particle at s(t)
    //  In-Out transition: we detecting leaving texture image at texture coordinates t; assuming there were particles
    //                      earlier (at s(t-1)), we place an edge particle at s(t-1) + 1
    //
    // The formula for s(t) is the "texturization" one, i.e. s = (t - o/2) / o, where o is the number of texture
    // pixels in one ship quad.
    //
    // Note that we do not take into account the ability to cover with just *triangles*, so to not overcomplicate the algorithm.
    // But that would be a nice improvement.

    //
    // Allocate buffer and make it clear (fully "transparent")
    //

    auto newStructureRegionBuffer = LayerTypeTraits<LayerType::Structural>::buffer_type(shipRect.size);
    newStructureRegionBuffer.Fill(StructuralElement(nullptr));

    //
    // Prepare passes
    //

    auto const & exteriorTextureLayerBuffer = mModel.GetExteriorTextureLayer().Buffer;

    vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetExteriorTextureSize());

    auto const writeParticle = [&](ShipSpaceCoordinates s, bool isEdge) -> void
    {
        if (s.IsInSize(shipRect.size))
        {
            if (isEdge)
            {
                // Always wins
                newStructureRegionBuffer[s] = StructuralElement(edgeMaterial);
            }
            else if (newStructureRegionBuffer[s].Material == nullptr)
            {
                // Only if first time
                newStructureRegionBuffer[s] = StructuralElement(fillMaterial);
            }
        }
    };

    //
    // Vertical pass - bottom -> up
    //

    for (int tx = textureRect.origin.x; tx < textureRect.origin.x + textureRect.size.width; ++tx)
    {
        // Calculate s.x once for entire scan line
        int sx = static_cast<int>(std::floorf(static_cast<float>(tx) / shipToTexture.x)) - shipRect.origin.x;

        // Init state at ty = 0
        bool isScanLineFull = exteriorTextureLayerBuffer[{tx, textureRect.origin.y}].a > alphaThreshold;

        for (int ty = textureRect.origin.y + 1; ty < textureRect.origin.y + textureRect.size.height; ++ty)
        {
            bool const isPixelFull = exteriorTextureLayerBuffer[{tx, ty}].a > alphaThreshold;

            if (!isScanLineFull && isPixelFull)
            {
                // Out->In

                // Place at s(t)
                int const sy = static_cast<int>(std::floorf(static_cast<float>(ty) / shipToTexture.y - 0.5f));
                writeParticle({ sx, sy - shipRect.origin.y }, true);
            }
            else if (isScanLineFull && !isPixelFull)
            {
                // In->Out

                // Place at s(t-1) + 1
                int const sy = static_cast<int>(std::floorf(static_cast<float>(ty - 1) / shipToTexture.y - 0.5f)) + 1;
                writeParticle({ sx, sy - shipRect.origin.y }, true);
            }
            else if (isScanLineFull && isPixelFull)
            {
                // Filler - at s(t)
                int const sy = static_cast<int>(std::floorf(static_cast<float>(ty) / shipToTexture.y - 0.5f));
                writeParticle({ sx, sy - shipRect.origin.y }, false);
            }

            isScanLineFull = isPixelFull;
        }
    }

    //
    // Horizontal pass - left->right
    //

    for (int ty = textureRect.origin.y; ty < textureRect.origin.y + textureRect.size.height; ++ty)
    {
        // Calculate s.y once for entire scan line
        int sy = static_cast<int>(std::floorf(static_cast<float>(ty) / shipToTexture.y)) - shipRect.origin.y;

        // Init state at tx = 0
        bool isScanLineFull = exteriorTextureLayerBuffer[{textureRect.origin.x, ty}].a > alphaThreshold;

        for (int tx = textureRect.origin.x + 1; tx < textureRect.origin.x + textureRect.size.width; ++tx)
        {
            bool const isPixelFull = exteriorTextureLayerBuffer[{tx, ty}].a > alphaThreshold;

            if (!isScanLineFull && isPixelFull)
            {
                // Out->In

                // Place at s(t)
                int const sx = static_cast<int>(std::floorf(static_cast<float>(tx) / shipToTexture.x - 0.5f));
                writeParticle({ sx - shipRect.origin.x, sy }, true);
            }
            else if (isScanLineFull && !isPixelFull)
            {
                // In->Out

                // Place at s(t-1) + 1
                int const sx = static_cast<int>(std::floorf(static_cast<float>(tx - 1) / shipToTexture.x - 0.5f)) + 1;
                writeParticle({ sx - shipRect.origin.x, sy }, true);
            }
            else if (isScanLineFull && isPixelFull)
            {
                // Filler - at s(t)
                int const sx = static_cast<int>(std::floorf(static_cast<float>(tx) / shipToTexture.x - 0.5f));
                writeParticle({ sx - shipRect.origin.x, sy }, false);
            }

            isScanLineFull = isPixelFull;
        }
    }

    return newStructureRegionBuffer;
}

void ModelController::WriteParticle(
    ShipSpaceCoordinates const & coords,
    ElectricalMaterial const * material)
{
    auto & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;

    auto const & oldElement = electricalLayerBuffer[coords];

    // Decide instance index and maintain panel
    ElectricalElementInstanceIndex instanceIndex;
    if (oldElement.Material == nullptr
        || !oldElement.Material->IsInstanced)
    {
        if (material != nullptr
            && material->IsInstanced)
        {
            // New instanced element...

            // ...new instance index
            instanceIndex = mInstancedElectricalElementSet.Add(material);
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
            mInstancedElectricalElementSet.Remove(oldElement.InstanceIndex);
            instanceIndex = NoneElectricalElementInstanceIndex;

            // Remove from panel (if there)
            mModel.GetElectricalLayer().Panel.TryRemove(oldElement.InstanceIndex);
        }
        else
        {
            // Both instanced...

            // ...keep old instanceIndex
            instanceIndex = oldElement.InstanceIndex;

            // Update (in case it's different)
            mInstancedElectricalElementSet.Replace(instanceIndex, material);
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

void ModelController::InternalAppendRope(
    ShipSpaceCoordinates const & startCoords,
    ShipSpaceCoordinates const & endCoords,
    StructuralMaterial const * material)
{
    assert(material != nullptr);

    mModel.GetRopesLayer().Buffer.EmplaceBack(
        startCoords,
        endCoords,
        material,
        material->RenderColor);
}

void ModelController::InternalMoveRopeEndpoint(
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

bool ModelController::InternalEraseRopeAt(ShipSpaceCoordinates const & coords)
{
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

        return true;
    }
    else
    {
        return false;
    }
}

template<LayerType TLayer>
std::optional<ShipSpaceRect> ModelController::DoFlood(
    ShipSpaceCoordinates const & start,
    typename LayerTypeTraits<TLayer>::material_type const * material,
    bool doContiguousOnly,
    typename LayerTypeTraits<TLayer>::layer_data_type const & layer)
{
    assert(start.IsInRect(GetWholeShipRect()));

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

std::optional<ImageRect> ModelController::DoTextureMagicWandEraseBackground(
    ImageCoordinates const & start,
    unsigned int tolerance,
    bool isAntiAlias,
    bool doContiguousOnly,
    TextureLayerData & layer)
{
    //
    // For the purposes of this operation, a pixel might exist or not;
    // it exists if and only if its alpha is not zero.
    //

    auto const textureSize = layer.Buffer.Size;

    assert(start.IsInSize(textureSize));

    // Get starting color
    rgbaColor const seedColorRgb = layer.Buffer[start];
    if (seedColorRgb.a == 0)
    {
        // Starting pixel does not exist
        return std::nullopt;
    }

    vec3f const seedColor = seedColorRgb.toVec3f();

    // Our distance function - from https://www.photoshopgurus.com/forum/threads/tolerance.52555/page-2
    auto const distanceFromSeed = [seedColor](vec3f const & sampleColor) -> float
    {
        vec3f const deltaColor = (sampleColor - seedColor).abs();

        return std::max({
            deltaColor.x,
            deltaColor.y,
            deltaColor.z,
            std::abs(deltaColor.x - deltaColor.y),
            std::abs(deltaColor.y - deltaColor.z),
            std::abs(deltaColor.z - deltaColor.x) });
    };

    // Transform tolerance into max distance (included)
    float const maxColorDistance = static_cast<float>(tolerance) / 100.0f;

    // Save original alpha mask for neighbors
    Buffer2D<typename rgbaColor::data_type, ImageSpaceTag> originalAlphaMask = layer.Buffer.Transform<typename rgbaColor::data_type>(
        [](rgbaColor const & color) -> typename rgbaColor::data_type
        {
            return color.a;
        });

    // Initialize affected region
    ImageRect affectedRegion(start); // We're sure we'll erase the start pixel

    // Anti-alias functor
    auto const doAntiAliasNeighbor = [&](ImageCoordinates const & neighborCoordinates) -> void
    {
        layer.Buffer[neighborCoordinates].a = originalAlphaMask[neighborCoordinates] / 3;
        affectedRegion.UnionWith(neighborCoordinates);
    };

    if (doContiguousOnly)
    {
        //
        // Flood from starting point
        //

        std::queue<ImageCoordinates> pixelsToPropagateFrom;

        // Erase this pixel
        layer.Buffer[start].a = 0;
        affectedRegion.UnionWith(start);

        // Visit from here
        pixelsToPropagateFrom.push(start);

        while (!pixelsToPropagateFrom.empty())
        {
            auto const sourceCoords = pixelsToPropagateFrom.front();
            pixelsToPropagateFrom.pop();

            // Check neighbors
            for (int yn = sourceCoords.y - 1; yn <= sourceCoords.y + 1; ++yn)
            {
                for (int xn = sourceCoords.x - 1; xn <= sourceCoords.x + 1; ++xn)
                {
                    ImageCoordinates const neighborCoordinates{ xn, yn };
                    if (neighborCoordinates.IsInSize(textureSize)
                        && layer.Buffer[neighborCoordinates].a != 0)
                    {
                        // Check distance
                        if (distanceFromSeed(layer.Buffer[neighborCoordinates].toVec3f()) <= maxColorDistance)
                        {
                            // Erase this pixel
                            layer.Buffer[neighborCoordinates].a = 0;
                            affectedRegion.UnionWith(neighborCoordinates);

                            // Continue visit from here
                            pixelsToPropagateFrom.push(neighborCoordinates);
                        }
                        else if (isAntiAlias)
                        {
                            // Anti-alias this neighbor
                            doAntiAliasNeighbor(neighborCoordinates);
                        }
                    }
                }
            }
        }
    }
    else
    {
        //
        // Color substitution
        //

        for (int y = 0; y < textureSize.height; ++y)
        {
            for (int x = 0; x < textureSize.width; ++x)
            {
                ImageCoordinates const sampleCoordinates{ x, y };

                if (layer.Buffer[sampleCoordinates].a != 0
                    && distanceFromSeed(layer.Buffer[sampleCoordinates].toVec3f()) <= maxColorDistance)
                {
                    // Erase
                    layer.Buffer[sampleCoordinates].a = 0;
                    affectedRegion.UnionWith(sampleCoordinates);

                    if (isAntiAlias)
                    {
                        //
                        // Do anti-aliasing on neighboring, too-distant pixels
                        //

                        for (int yn = y - 1; yn <= y + 1; ++yn)
                        {
                            for (int xn = x - 1; xn <= x + 1; ++xn)
                            {
                                ImageCoordinates const neighborCoordinates{ xn, yn };
                                if (neighborCoordinates.IsInSize(textureSize)
                                    && layer.Buffer[neighborCoordinates].a != 0
                                    && distanceFromSeed(layer.Buffer[neighborCoordinates].toVec3f()) > maxColorDistance)
                                {
                                    doAntiAliasNeighbor(neighborCoordinates);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return affectedRegion;
}

void ModelController::DoStructuralRegionFill(
    ShipSpaceRect const & region,
    StructuralMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Structural));

    //
    // Update model with just material - no analyses
    //

    auto & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;

    assert(region.IsContainedInRect(ShipSpaceRect(structuralLayerBuffer.Size)));

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
}

void ModelController::DoElectricalRegionFill(
    ShipSpaceRect const & region,
    ElectricalMaterial const * material)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    //
    // Update model just with material - no instance ID, no analyses, no panel
    //

    auto & electricalLayerBuffer = mModel.GetElectricalLayer().Buffer;

    assert(region.IsContainedInRect(ShipSpaceRect(electricalLayerBuffer.Size)));

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
}

void ModelController::DoRopesRegionErase(
    ShipSpaceRect const & region)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    //
    // Update model
    //

    mModel.GetRopesLayer().Buffer.EraseRegion(region);

    // Update visualization
    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
}

void ModelController::DoExteriorTextureRegionErase(
    ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    //
    // Update model
    //

    auto & textureLayerBuffer = mModel.GetExteriorTextureLayer().Buffer;

    assert(region.IsContainedInRect(ImageRect(textureLayerBuffer.Size)));

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            textureLayerBuffer[{x, y}].a = 0;
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(ExteriorImageRectToContainingShipSpaceRect(region));
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(region);
}

void ModelController::DoInteriorTextureRegionErase(
    ImageRect const & region)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    //
    // Update model
    //

    auto & textureLayerBuffer = mModel.GetInteriorTextureLayer().Buffer;

    assert(region.IsContainedInRect(ImageRect(textureLayerBuffer.Size)));

    for (int y = region.origin.y; y < region.origin.y + region.size.height; ++y)
    {
        for (int x = region.origin.x; x < region.origin.x + region.size.width; ++x)
        {
            textureLayerBuffer[{x, y}].a = 0;
        }
    }

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(region);
}

void ModelController::DoStructuralBlit(
    typename LayerTypeTraits<LayerType::Structural>::buffer_type const & sourceBuffer,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetCoordinates,
    bool isTransparent)
{
    assert(mModel.HasLayer(LayerType::Structural));

    // The source region is entirely contained in the source buffer
    assert(sourceRegion.IsContainedInRect(ShipSpaceRect(sourceBuffer.Size)));

    // Rect in the ship that will be affected by this operation
    ShipSpaceRect const affectedRegion(targetCoordinates, sourceRegion.size);

    // The affected region is entirely contained in this ship
    assert(affectedRegion.IsContainedInRect(GetWholeShipRect()));

    auto const elementOperator = isTransparent
        ? [](StructuralElement const & src, StructuralElement const & dst) -> StructuralElement const &
        {
            return src.Material
                ? src
                : dst;
        }
        : [](StructuralElement const & src, StructuralElement const &) -> StructuralElement const &
        {
            return src;
        };

    mModel.GetStructuralLayer().Buffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetCoordinates,
        elementOperator);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(affectedRegion);
    RegisterDirtyVisualization<VisualizationType::StructuralLayer>(affectedRegion);
}

void ModelController::DoElectricalBlit(
    typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & sourceBuffer,
    ShipSpaceRect const & sourceRegion,
    ShipSpaceCoordinates const & targetCoordinates,
    std::function<ElectricalElement(ElectricalElement const &, ElectricalElement const &)> elementOperator)
{
    assert(mModel.HasLayer(LayerType::Electrical));

    // The source region is entirely contained in the source buffer
    assert(sourceRegion.IsContainedInRect(ShipSpaceRect(sourceBuffer.Size)));

    // Rect in the ship that will be affected by this operation
    ShipSpaceRect const affectedRegion(targetCoordinates, sourceRegion.size);

    // The affected region is entirely contained in this ship
    assert(affectedRegion.IsContainedInRect(GetWholeShipRect()));

    mModel.GetElectricalLayer().Buffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetCoordinates,
        elementOperator);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::ElectricalLayer>(affectedRegion);
}

void ModelController::DoRopesBlit(
    typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & sourceBuffer,
    ShipSpaceCoordinates const & targetCoordinates, // Unclipped
    bool isTransparent)
{
    assert(mModel.HasLayer(LayerType::Ropes));

    mModel.GetRopesLayer().Buffer.BlitFromRegion(
        sourceBuffer,
        targetCoordinates,
        isTransparent);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::RopesLayer>(GetWholeShipRect());
}

void ModelController::DoExteriorTextureBlit(
    typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & sourceBuffer,
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetCoordinates,
    bool isTransparent)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    // The source region is entirely contained in the source buffer
    assert(sourceRegion.IsContainedInRect(ImageRect(sourceBuffer.Size)));

    // Rect in the ship that will be affected by this operation
    ImageRect const affectedRegion(targetCoordinates, sourceRegion.size);

    // The target region is entirely contained in this ship
    assert(affectedRegion.IsContainedInRect(GetWholeExteriorTextureRect()));

    auto const elementOperator = isTransparent
        ? [](rgbaColor const & src, rgbaColor const & dst) -> rgbaColor
        {
            return dst.blend(src);
        }
        : [](rgbaColor const & src, rgbaColor const &) -> rgbaColor
        {
            return src;
        };

    mModel.GetExteriorTextureLayer().Buffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetCoordinates,
        elementOperator);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(ExteriorImageRectToContainingShipSpaceRect(affectedRegion));
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(affectedRegion);
}

void ModelController::DoInteriorTextureBlit(
    typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & sourceBuffer,
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetCoordinates,
    bool isTransparent)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    // The source region is entirely contained in the source buffer
    assert(sourceRegion.IsContainedInRect(ImageRect(sourceBuffer.Size)));

    // Rect in the ship that will be affected by this operation
    ImageRect const affectedRegion(targetCoordinates, sourceRegion.size);

    // The target region is entirely contained in this ship
    assert(affectedRegion.IsContainedInRect(GetWholeInteriorTextureRect()));

    auto const elementOperator = isTransparent
        ? [](rgbaColor const & src, rgbaColor const & dst) -> rgbaColor
        {
            return dst.blend(src);
        }
        : [](rgbaColor const & src, rgbaColor const &) -> rgbaColor
        {
            return src;
        };

    mModel.GetInteriorTextureLayer().Buffer.BlitFromRegion(
        sourceBuffer,
        sourceRegion,
        targetCoordinates,
        elementOperator);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(affectedRegion);
}

TextureLayerData ModelController::DoMakeTextureLayerFromImage(
    TextureLayerData const & source,
    ImageRect const & sourceRegion,
    ImageCoordinates const & targetOrigin)
{
    RgbaImageData newTexture(
        source.Buffer.Size,
        rgbaColor(rgbaColor::data_type_max, rgbaColor::data_type_max, rgbaColor::data_type_max, 0));

    newTexture.BlitFromRegion(
        source.Buffer,
        sourceRegion,
        targetOrigin);

    return TextureLayerData(std::move(newTexture));
}

void ModelController::InternalRestoreExteriorTextureLayerBackup(
    TextureLayerData && sourceLayerBackup,
    ImageRect const & originatingRegion)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.GetExteriorTextureLayer().RestoreRegionBackup(
        std::move(sourceLayerBackup),
        originatingRegion.origin);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::Game>(ExteriorImageRectToContainingShipSpaceRect(originatingRegion));
    RegisterDirtyVisualization<VisualizationType::ExteriorTextureLayer>(originatingRegion);
}

void ModelController::InternalRestoreExteriorTextureLayerEphemeralVisualizationRegion(
    typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & backupBuffer,
    ImageRect const & backupBufferRegion,
    ImageCoordinates const & targetOrigin,
    EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState)
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    assert(mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);

    DoExteriorTextureBlit(
        backupBuffer,
        backupBufferRegion,
        targetOrigin,
        false);

    // Remember we are not anymore in temp visualization
    finalEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization = false;
}

void ModelController::InternalRestoreInteriorTextureLayerBackup(
    TextureLayerData && sourceLayerBackup,
    ImageRect const & originatingRegion)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    //
    // Restore model
    //

    mModel.GetInteriorTextureLayer().RestoreRegionBackup(
        std::move(sourceLayerBackup),
        originatingRegion.origin);

    //
    // Update visualization
    //

    RegisterDirtyVisualization<VisualizationType::InteriorTextureLayer>(originatingRegion);
}

void ModelController::InternalRestoreInteriorTextureLayerEphemeralVisualizationRegion(
    typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & backupBuffer,
    ImageRect const & backupBufferRegion,
    ImageCoordinates const & targetOrigin,
    EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState)
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    assert(mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);

    DoInteriorTextureBlit(
        backupBuffer,
        backupBufferRegion,
        targetOrigin,
        false);

    // Remember we are not anymore in temp visualization
    finalEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization = false;
}

//
// Eph viz
//

ShipSpaceEphemeralVisualizationRestoreRegions::payload_type ModelController::MakeShipSpaceEphemeralVisualizationRestoreRegion(
    ShipSpaceRect const & region,
    LayerSelector const & layerSelection) const
{
    // The requested region is entirely within the ship
    assert(region.IsContainedInRect(GetWholeShipRect()));

    std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> structuralLayerBufferRegion;
    std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type> electricalLayerBufferRegion;
    std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> ropesLayerBuffer; // Whole buffer
    std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> exteriorTextureLayerBufferRegion;
    std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> interiorTextureLayerBufferRegion;

    if (layerSelection.IsSet(LayerType::Structural))
    {
        structuralLayerBufferRegion = mModel.GetStructuralLayer().Buffer.CloneRegion(region);
    }

    if (layerSelection.IsSet(LayerType::Electrical))
    {
        electricalLayerBufferRegion = mModel.GetElectricalLayer().Buffer.CloneRegion(region);
    }

    if (layerSelection.IsSet(LayerType::Ropes))
    {
        ropesLayerBuffer = mModel.GetRopesLayer().Buffer.Clone(); // Whole buffer
    }

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        // The requested region is entirely within the ship
        assert(ShipSpaceToExteriorTextureSpace(region).IsContainedInRect(GetWholeExteriorTextureRect()));

        exteriorTextureLayerBufferRegion = mModel.GetExteriorTextureLayer().Buffer.CloneRegion(ShipSpaceToExteriorTextureSpace(region));
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        // The requested region is entirely within the ship
        assert(ShipSpaceToInteriorTextureSpace(region).IsContainedInRect(GetWholeInteriorTextureRect()));

        interiorTextureLayerBufferRegion = mModel.GetInteriorTextureLayer().Buffer.CloneRegion(ShipSpaceToInteriorTextureSpace(region));
    }

    return ShipSpaceEphemeralVisualizationRestoreRegions::payload_type(
        region,
        std::move(structuralLayerBufferRegion),
        std::move(electricalLayerBufferRegion),
        std::move(ropesLayerBuffer),
        std::move(exteriorTextureLayerBufferRegion),
        std::move(interiorTextureLayerBufferRegion));
}

ExteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type ModelController::MakeExteriorTextureSpaceEphemeralVisualizationRestoreRegion(
    ImageRect const & region,
    LayerSelector const & layerSelection) const
{
    // The requested region is entirely within the exterior texture
    assert(region.IsContainedInRect(GetWholeExteriorTextureRect()));

    std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> exteriorTextureLayerBufferRegion;
    std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> interiorTextureLayerBufferRegion;

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        exteriorTextureLayerBufferRegion = mModel.GetExteriorTextureLayer().Buffer.CloneRegion(region);
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        // The requested region is entirely within the ship
        assert(ExteriorTextureSpaceToInteriorTextureSpace(region).IsContainedInRect(GetWholeInteriorTextureRect()));

        interiorTextureLayerBufferRegion = mModel.GetInteriorTextureLayer().Buffer.CloneRegion(ExteriorTextureSpaceToInteriorTextureSpace(region));
    }

    return ExteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type(
        region,
        std::move(exteriorTextureLayerBufferRegion),
        std::move(interiorTextureLayerBufferRegion));
}

InteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type ModelController::MakeInteriorTextureSpaceEphemeralVisualizationRestoreRegion(
    ImageRect const & region,
    LayerSelector const & layerSelection) const
{
    // The requested region is entirely within the interior texture
    assert(region.IsContainedInRect(GetWholeInteriorTextureRect()));

    std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> exteriorTextureLayerBufferRegion;
    std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> interiorTextureLayerBufferRegion;

    if (layerSelection.IsSet(LayerType::ExteriorTexture))
    {
        // The requested region is entirely within the ship
        assert(InteriorTextureSpaceToExteriorTextureSpace(region).IsContainedInRect(GetWholeExteriorTextureRect()));

        exteriorTextureLayerBufferRegion = mModel.GetExteriorTextureLayer().Buffer.CloneRegion(InteriorTextureSpaceToExteriorTextureSpace(region));
    }

    if (layerSelection.IsSet(LayerType::InteriorTexture))
    {
        interiorTextureLayerBufferRegion = mModel.GetInteriorTextureLayer().Buffer.CloneRegion(region);
    }

    return InteriorTextureSpaceEphemeralVisualizationRestoreRegions::payload_type(
        region,
        std::move(exteriorTextureLayerBufferRegion),
        std::move(interiorTextureLayerBufferRegion));
}

//
// Viz
//

template<VisualizationType TVisualization, typename TRect>
void ModelController::RegisterDirtyVisualization(TRect const & region)
{
    if constexpr (TVisualization == VisualizationType::Game)
    {
        mDirtyGameVisualizationRegions.AddRegion(region);
    }
    else if constexpr (TVisualization == VisualizationType::StructuralLayer)
    {
        mDirtyStructuralLayerVisualizationRegions.AddRegion(region);
    }
    else if constexpr (TVisualization == VisualizationType::ElectricalLayer)
    {
        mDirtyElectricalLayerVisualizationRegions.AddRegion(region);
    }
    else if constexpr (TVisualization == VisualizationType::RopesLayer)
    {
        mDirtyRopesLayerVisualizationRegions.AddRegion(region);
    }
    else if constexpr (TVisualization == VisualizationType::ExteriorTextureLayer)
    {
        mDirtyExteriorTextureLayerVisualizationRegions.AddRegion(region);
    }
    else
    {
        static_assert(TVisualization == VisualizationType::InteriorTextureLayer);

        mDirtyInteriorTextureLayerVisualizationRegions.AddRegion(region);
    }
}

ImageRect ModelController::UpdateGameVisualization(
    ShipSpaceRect const & region,
    GameAssetManager const & gameAssetManager)
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
            settings,
            gameAssetManager);

        sourceTexture = mGameVisualizationAutoTexturizationTexture.get();
    }
    else
    {
        assert(mGameVisualizationMode == GameVisualizationModeType::ExteriorTextureMode);

        assert(mModel.HasLayer(LayerType::Structural));
        assert(mModel.HasLayer(LayerType::ExteriorTexture));

        sourceTexture = &mModel.GetExteriorTextureLayer().Buffer;
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
        ImageCoordinates(
            effectiveRegion.origin.x * mGameVisualizationTextureMagnificationFactor,
            effectiveRegion.origin.y * mGameVisualizationTextureMagnificationFactor),
        ImageSize(
            effectiveRegion.size.width * mGameVisualizationTextureMagnificationFactor,
            effectiveRegion.size.height * mGameVisualizationTextureMagnificationFactor));
}

ImageRect ModelController::UpdateStructuralLayerVisualization(ShipSpaceRect const & region)
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
            // Nop
            break;
        }
    }

    return ImageRect(
        ImageCoordinates(
            region.origin.x,
            region.origin.y),
        ImageSize(
            region.size.width,
            region.size.height));
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
                ? structuralMaterial->RenderColor
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

void ModelController::UpdateExteriorTextureLayerVisualization()
{
    switch(mExteriorTextureLayerVisualizationMode)
    {
        case ExteriorTextureLayerVisualizationModeType::MatteMode:
        {
            assert(mModel.HasLayer(LayerType::ExteriorTexture));

            // Nop
            break;
        }

        case ExteriorTextureLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

void ModelController::UpdateInteriorTextureLayerVisualization()
{
    switch (mInteriorTextureLayerVisualizationMode)
    {
        case InteriorTextureLayerVisualizationModeType::MatteMode:
        {
            assert(mModel.HasLayer(LayerType::InteriorTexture));

            // Nop
            break;
        }

        case InteriorTextureLayerVisualizationModeType::None:
        {
            return;
        }
    }
}

void ModelController::VerifyElectricalLayerInvariants() const
{
#ifdef _DEBUG

    assert(HasLayer(LayerType::Electrical));

    auto const & layer = mModel.GetElectricalLayer();

    std::set<ElectricalElementInstanceIndex> presentElectricalElementInstanceIndices;
    size_t electricalParticleCount = 0;
    for (size_t i = 0; i < layer.Buffer.Size.GetLinearSize(); ++i)
    {
        auto const & element = layer.Buffer.Data.get()[i];
        if (element.Material != nullptr)
        {
            assert((element.InstanceIndex != NoneElectricalElementInstanceIndex && element.Material->IsInstanced)
                || (element.InstanceIndex == NoneElectricalElementInstanceIndex && !element.Material->IsInstanced));

            if (element.Material->IsInstanced)
            {
                auto [_, isInserted] = presentElectricalElementInstanceIndices.emplace(element.InstanceIndex);

                // Not dupe
                assert(isInserted);

                // Exists in instance set
                assert(mInstancedElectricalElementSet.IsRegistered(element.InstanceIndex));
            }

            ++electricalParticleCount;
        }
    }

    assert(electricalParticleCount == mElectricalParticleCount);

    // InstanceSet has no extra's
    for (auto const & entry : mInstancedElectricalElementSet.GetElements())
    {
        assert(presentElectricalElementInstanceIndices.find(entry.first) != presentElectricalElementInstanceIndices.end());
    }

    // Panel has no foreigners
    for (auto it = layer.Panel.begin(); it != layer.Panel.end(); ++it)
    {
        assert(presentElectricalElementInstanceIndices.find(it->first) != presentElectricalElementInstanceIndices.end());
    }

#endif
}

}