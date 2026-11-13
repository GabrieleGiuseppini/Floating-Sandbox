/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GenericEphemeralVisualizationRestorePayload.h"
#include "GenericUndoPayload.h"
#include "IModelObservable.h"
#include "InstancedElectricalElementSet.h"
#include "LayerSelector.h"
#include "Model.h"
#include "ModelValidationSession.h"
#include "CombinedRegions.h"
#include "ShipBuilderTypes.h"
#include "View.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>
#include <Simulation/ShipDefinition.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/Finalizer.h>
#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace ShipBuilder {

/*
 * This class implements operations on the model.
 *
 * The ModelController does not maintain the model's dirtyness; that is managed by the Controller.
 */
class ModelController : public IModelObservable
{
public:

    static std::unique_ptr<ModelController> CreateNew(
        ShipSpaceSize const & shipSpaceSize,
        std::string const & shipName,
        ShipTexturizer const & shipTexturizer);

    static std::unique_ptr<ModelController> CreateForShip(
        ShipDefinition && shipDefinition,
        ShipTexturizer const & shipTexturizer);

    ShipDefinition MakeShipDefinition() const;

    ShipSpaceSize const & GetShipSize() const override
    {
        return mModel.GetShipSize();
    }

    void SetShipSize(ShipSpaceSize const & shipSize)
    {
        mModel.SetShipSize(shipSize);
    }

    ModelMacroProperties GetModelMacroProperties() const override
    {
        assert(mMassParticleCount == 0 || mTotalMass != 0.0f);

        return ModelMacroProperties(
            mMassParticleCount,
            mTotalMass,
            mMassParticleCount != 0 ? mCenterOfMassSum / mTotalMass : std::optional<vec2f>());
    }

    std::unique_ptr<RgbaImageData> MakePreview() const;

    std::optional<ShipSpaceRect> CalculateBoundingBox() const;

    bool HasLayer(LayerType layer) const override
    {
        return mModel.HasLayer(layer);
    }

    LayerSelector GetAllPresentLayers() const
    {
        return mModel.GetAllPresentLayers();
    }

    bool IsDirty() const override
    {
        return mModel.GetIsDirty();
    }

    bool IsLayerDirty(LayerType layer) const override
    {
        return mModel.GetIsDirty(layer);
    }

    ModelDirtyState GetDirtyState() const
    {
        return mModel.GetDirtyState();
    }

    void SetLayerDirty(LayerType layer)
    {
        mModel.SetIsDirty(layer);
    }

    void RestoreDirtyState(ModelDirtyState const & dirtyState)
    {
        mModel.SetDirtyState(dirtyState);
    }

    void ClearIsDirty()
    {
        mModel.ClearIsDirty();
    }

    ModelValidationSession StartValidation(Finalizer && finalizer) const;

#ifdef _DEBUG
    bool IsInEphemeralVisualization() const
    {
        return mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization
            || mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization
            || mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization
            || mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization
            || mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization;
    }
#endif

    ShipMetadata const & GetShipMetadata() const override
    {
        return mModel.GetShipMetadata();
    }

    void SetShipMetadata(ShipMetadata && shipMetadata)
    {
        mModel.SetShipMetadata(std::move(shipMetadata));
    }

    ShipPhysicsData const & GetShipPhysicsData() const override
    {
        return mModel.GetShipPhysicsData();
    }

    void SetShipPhysicsData(ShipPhysicsData && shipPhysicsData)
    {
        mModel.SetShipPhysicsData(std::move(shipPhysicsData));
    }

    std::optional<ShipAutoTexturizationSettings> const & GetShipAutoTexturizationSettings() const override
    {
        return mModel.GetShipAutoTexturizationSettings();
    }

    void SetShipAutoTexturizationSettings(std::optional<ShipAutoTexturizationSettings> && shipAutoTexturizationSettings)
    {
        mModel.SetShipAutoTexturizationSettings(std::move(shipAutoTexturizationSettings));
    }

    InstancedElectricalElementSet const & GetInstancedElectricalElementSet() const
    {
        return mInstancedElectricalElementSet;
    }

    void SetElectricalPanel(ElectricalPanel && electricalPanel)
    {
        mModel.SetElectricalPanel(std::move(electricalPanel));
    }

    std::optional<SampledInformation> SampleInformationAt(ShipSpaceCoordinates const & coordinates, LayerType layer) const;

    void Flip(DirectionType direction);

    void Rotate90(RotationDirectionType direction);

    void ResizeShip(
        ShipSpaceSize const & newSize,
        ShipSpaceCoordinates const & originOffset);

    template<LayerType TLayer>
    typename LayerTypeTraits<TLayer>::layer_data_type CloneExistingLayer() const
    {
        switch (TLayer)
        {
            case LayerType::Electrical:
            {
                assert(!mEphemeralVisualizationLayerState.IsElectricalLayerInEphemeralVisualization);
                break;
            }

            case LayerType::Ropes:
            {
                assert(!mEphemeralVisualizationLayerState.IsRopesLayerInEphemeralVisualization);
                break;
            }

            case LayerType::Structural:
            {
                assert(!mEphemeralVisualizationLayerState.IsStructuralLayerInEphemeralVisualization);
                break;
            }

            case LayerType::ExteriorTexture:
            {
                assert(!mEphemeralVisualizationLayerState.IsExteriorTextureLayerInEphemeralVisualization);
                break;
            }

            case LayerType::InteriorTexture:
            {
                assert(!mEphemeralVisualizationLayerState.IsInteriorTextureLayerInEphemeralVisualization);
                break;
            }

        }

        return mModel.CloneExistingLayer<TLayer>();
    }

    ShipLayers Copy(
        ShipSpaceRect const & region,
        LayerSelector const & layerSelection) const;

    GenericShipSpaceUndoPayload EraseRegion(
        ShipSpaceRect const & region,
        LayerSelector const & layerSelection);

    GenericShipSpaceUndoPayload Paste(
        ShipLayers const & sourcePayload,
        ShipSpaceCoordinates const & pasteOrigin,
        bool isTransparent);

    GenericShipSpaceEphemeralVisualizationRestorePayload PasteForEphemeralVisualization(
        ShipLayers const & sourcePayload,
        ShipSpaceCoordinates const & pasteOrigin,
        bool isTransparent);

    // TODO: committed transfer here, X 3

    GenericShipSpaceEphemeralVisualizationRestorePayload ShipSpaceRegionTransferForEphemeralVisualization(
        ShipSpaceRect const & sourceRegion, // In current model
        ShipSpaceCoordinates const & destinationPos,
        LayerSelector const & layerSelection,
        bool isTransparent);
    GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload ExteriorTextureSpaceRegionTransferForEphemeralVisualization(
        ImageRect const & sourceRegion, // In current model
        ImageCoordinates const & destinationPos,
        LayerSelector const & layerSelection,
        bool isTransparent);
    GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload InteriorTextureSpaceRegionTransferForEphemeralVisualization(
        ImageRect const & sourceRegion, // In current model
        ImageCoordinates const & destinationPos,
        LayerSelector const & layerSelection,
        bool isTransparent);

    void Restore(GenericShipSpaceUndoPayload && undoPayload);
    void Restore(GenericExteriorTextureSpaceUndoPayload && undoPayload);
    void Restore(GenericInteriorTextureSpaceUndoPayload && undoPayload);

    void RestoreEphemeralVisualization(GenericShipSpaceEphemeralVisualizationRestorePayload && restorePayload);
    void RestoreEphemeralVisualization(GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload && restorePayload);
    void RestoreEphemeralVisualization(GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload && restorePayload);

    LayerSelector IntersectLayerPresence(ShipLayers const & otherSource) const;

    //
    // Structural
    //

    StructuralLayerData const & GetStructuralLayer() const override;

    void SetStructuralLayer(StructuralLayerData && structuralLayer);

    std::unique_ptr<StructuralLayerData> CloneStructuralLayer() const;

    StructuralMaterial const * SampleStructuralMaterialAt(ShipSpaceCoordinates const & coords) const;

    std::set<StructuralMaterial const *> GetStructuralMaterialsInUse(std::vector<StructuralMaterial const *> const & candidates) const;

    void StructuralRegionFill(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

    GenericShipSpaceUndoPayload StructuralRectangle(
        ShipSpaceRect const & rect,
        std::uint32_t lineSize,
        StructuralMaterial const * lineMaterial,
        std::optional<StructuralMaterial const *> fillMaterial);

    GenericShipSpaceUndoPayload StructureTrace(
        ImageRect const & textureRect,
        StructuralMaterial const * edgeMaterial,
        StructuralMaterial const * fillMaterial,
        std::uint8_t alphaThreshold);

    std::optional<ShipSpaceRect> StructuralFlood(
        ShipSpaceCoordinates const & start,
        StructuralMaterial const * material,
        bool doContiguousOnly);

    void RestoreStructuralLayerRegionBackup(
        StructuralLayerData && sourceLayerRegionBackup,
        ShipSpaceCoordinates const & targetOrigin);

    void RestoreStructuralLayer(std::unique_ptr<StructuralLayerData> sourceLayer);

    void StructuralRegionFillForEphemeralVisualization(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

    GenericShipSpaceEphemeralVisualizationRestorePayload StructuralRectangleForEphemeralVisualization(
        ShipSpaceRect const & rect,
        std::uint32_t lineSize,
        StructuralMaterial const * lineMaterial,
        std::optional<StructuralMaterial const *> fillMaterial);

    void RestoreStructuralLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & backupBuffer,
        ShipSpaceRect const & backupBufferRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void RestoreStructuralLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & backupBuffer,
        CombinedShipSpaceRegions const & regions);

    //
    // Electrical
    //

    ElectricalPanel const & GetElectricalPanel() const;

    void SetElectricalLayer(ElectricalLayerData && electricalLayer);

    void RemoveElectricalLayer();

    std::unique_ptr<ElectricalLayerData> CloneElectricalLayer() const;

    ElectricalMaterial const * SampleElectricalMaterialAt(ShipSpaceCoordinates const & coords) const;

    std::set<ElectricalMaterial const *> GetElectricalMaterialsInUse(std::vector<ElectricalMaterial const *> const & candidates) const;

    bool IsElectricalParticleAllowedAt(ShipSpaceCoordinates const & coords) const;

    std::optional<ShipSpaceRect> TrimElectricalParticlesWithoutSubstratum();

    void ElectricalRegionFill(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    void RestoreElectricalLayerRegionBackup(
        ElectricalLayerData && sourceLayerRegionBackup,
        ShipSpaceCoordinates const & targetOrigin);

    void RestoreElectricalLayer(std::unique_ptr<ElectricalLayerData> sourceLayer);

    void ElectricalRegionFillForEphemeralVisualization(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    void RestoreElectricalLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & backupBuffer,
        ShipSpaceRect const & backupBufferRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void RestoreElectricalLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & backupBuffer,
        CombinedShipSpaceRegions const & regions);

    //
    // Ropes
    //

    void SetRopesLayer(RopesLayerData && ropesLayer);

    void RemoveRopesLayer();

    std::unique_ptr<RopesLayerData> CloneRopesLayer() const;

    StructuralMaterial const * SampleRopesMaterialAt(ShipSpaceCoordinates const & coords) const;

    std::set<StructuralMaterial const *> GetRopesMaterialsInUse(std::vector<StructuralMaterial const *> const & candidates) const;

    std::optional<size_t> GetRopeElementIndexAt(ShipSpaceCoordinates const & coords) const;

    void AddRope(
        ShipSpaceCoordinates const & startCoords,
        ShipSpaceCoordinates const & endCoords,
        StructuralMaterial const * material);

    void MoveRopeEndpoint(
        size_t ropeElementIndex,
        ShipSpaceCoordinates const & oldCoords,
        ShipSpaceCoordinates const & newCoords);

    bool EraseRopeAt(ShipSpaceCoordinates const & coords);

    void EraseRopesRegion(ShipSpaceRect const & region);

    void RestoreRopesLayerRegionBackup(
        RopesLayerData && sourceLayerRegionBackup,
        ShipSpaceCoordinates const & targetOrigin);

    void RestoreRopesLayer(std::unique_ptr<RopesLayerData> sourceLayer);

    void AddRopeForEphemeralVisualization(
        ShipSpaceCoordinates const & startCoords,
        ShipSpaceCoordinates const & endCoords,
        StructuralMaterial const * material);

    void MoveRopeEndpointForEphemeralVisualization(
        size_t ropeElementIndex,
        ShipSpaceCoordinates const & oldCoords,
        ShipSpaceCoordinates const & newCoords);

    void RestoreRopesLayerEphemeralVisualization(typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & backupBuffer);

    //
    // Exterior Texture
    //

    TextureLayerData const & GetExteriorTextureLayer() const;

    ImageSize const & GetExteriorTextureSize() const
    {
        assert(mModel.HasLayer(LayerType::ExteriorTexture));

        return mModel.GetExteriorTextureLayer().Buffer.Size;
    }

    ImageCoordinates ShipSpaceToExteriorTextureSpace(ShipSpaceCoordinates const & shipCoordinates) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetExteriorTextureSize());

        return ImageCoordinates::FromFloatFloor(shipCoordinates.ToFloat().scale(shipToTexture));
    }

    ImageRect ShipSpaceToExteriorTextureSpace(ShipSpaceRect const & shipRect) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetExteriorTextureSize());

        return ImageRect(
            ImageCoordinates::FromFloatFloor(shipRect.origin.ToFloat().scale(shipToTexture)),
            ImageSize::FromFloatFloor(shipRect.size.ToFloat<vec2f>().scale(shipToTexture)));
    }

    ShipSpaceCoordinates ExteriorTextureSpaceToShipSpace(ImageCoordinates const & imageCoordinates) const
    {
        vec2f const textureToShip = GetTextureSpaceToShipSpaceFactor(GetExteriorTextureSize(), GetShipSize());

        return ShipSpaceCoordinates::FromFloatFloor(imageCoordinates.ToFloat().scale(textureToShip));
    }

    ImageCoordinates ExteriorTextureSpaceToInteriorTextureSpace(ImageCoordinates const & imageCoordinates) const
    {
        vec2f const externalToInteral = GetTextureSpaceToTextureSpaceFactor(GetExteriorTextureSize(), GetInteriorTextureSize());

        return ImageCoordinates::FromFloatFloor(imageCoordinates.ToFloat().scale(externalToInteral));
    }

    ImageRect ExteriorTextureSpaceToInteriorTextureSpace(ImageRect const & imageRect) const
    {
        vec2f const externalToInteral = GetTextureSpaceToTextureSpaceFactor(GetExteriorTextureSize(), GetInteriorTextureSize());

        return ImageRect(
            ImageCoordinates::FromFloatFloor(imageRect.origin.ToFloat().scale(externalToInteral)),
            ImageSize::FromFloatFloor(imageRect.size.ToFloat<vec2f>().scale(externalToInteral)));
    }

    ShipSpaceRect ExteriorImageRectToContainingShipSpaceRect(ImageRect const & imageRect) const
    {
        return ImageRectToContainingShipSpaceRect(
            imageRect,
            GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetExteriorTextureSize()));
    }

    void SetExteriorTextureLayer(
        TextureLayerData && exteriorTextureLayer,
        std::optional<std::string> originalTextureArtCredits);
    void RemoveExteriorTextureLayer();

    std::unique_ptr<TextureLayerData> CloneExteriorTextureLayer() const;

    void EraseExteriorTextureRegion(ImageRect const & region);

    std::optional<ImageRect> ExteriorTextureMagicWandEraseBackground(
        ImageCoordinates const & start,
        unsigned int tolerance,
        bool isAntiAlias,
        bool doContiguousOnly);

    void RestoreExteriorTextureLayerRegionBackup(
        TextureLayerData && sourceLayerRegionBackup,
        ImageCoordinates const & targetOrigin);

    void RestoreExteriorTextureLayer(
        std::unique_ptr<TextureLayerData> exteriorTextureLayer,
        std::optional<std::string> originalTextureArtCredits);

    void ExteriorTextureRegionEraseForEphemeralVisualization(ImageRect const & region);

    void MakeExteriorLayerFromImage(
        TextureLayerData const & source,
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetOrigin);

    void RestoreExteriorTextureLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & backupBuffer,
        ImageRect const & backupBufferRegion,
        ImageCoordinates const & targetOrigin);

    //
    // Interior Texture
    //

    TextureLayerData const & GetInteriorTextureLayer() const;

    ImageSize const & GetInteriorTextureSize() const
    {
        assert(mModel.HasLayer(LayerType::InteriorTexture));

        return mModel.GetInteriorTextureLayer().Buffer.Size;
    }

    ImageCoordinates ShipSpaceToInteriorTextureSpace(ShipSpaceCoordinates const & shipCoordinates) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetInteriorTextureSize());

        return ImageCoordinates::FromFloatFloor(shipCoordinates.ToFloat().scale(shipToTexture));
    }

    ImageRect ShipSpaceToInteriorTextureSpace(ShipSpaceRect const & shipRect) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetInteriorTextureSize());

        return ImageRect(
            ImageCoordinates::FromFloatFloor(shipRect.origin.ToFloat().scale(shipToTexture)),
            ImageSize::FromFloatFloor(shipRect.size.ToFloat<vec2f>().scale(shipToTexture)));
    }

    ImageCoordinates InteriorTextureSpaceToExteriorTextureSpace(ImageCoordinates const & imageCoordinates) const
    {
        vec2f const internalToExteral = GetTextureSpaceToTextureSpaceFactor(GetInteriorTextureSize(), GetExteriorTextureSize());

        return ImageCoordinates::FromFloatFloor(imageCoordinates.ToFloat().scale(internalToExteral));
    }

    ImageRect InteriorTextureSpaceToExteriorTextureSpace(ImageRect const & imageRect) const
    {
        vec2f const internalToExteral = GetTextureSpaceToTextureSpaceFactor(GetInteriorTextureSize(), GetExteriorTextureSize());

        return ImageRect(
            ImageCoordinates::FromFloatFloor(imageRect.origin.ToFloat().scale(internalToExteral)),
            ImageSize::FromFloatFloor(imageRect.size.ToFloat<vec2f>().scale(internalToExteral)));
    }

    ShipSpaceRect InteriorImageRectToContainingShipSpaceRect(ImageRect const & imageRect) const
    {
        return ImageRectToContainingShipSpaceRect(
            imageRect,
            GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetInteriorTextureSize()));
    }

    void SetInteriorTextureLayer(TextureLayerData && interiorTextureLayer);
    void RemoveInteriorTextureLayer();

    std::unique_ptr<TextureLayerData> CloneInteriorTextureLayer() const;

    void EraseInteriorTextureRegion(ImageRect const & region);

    std::optional<ImageRect> InteriorTextureMagicWandEraseBackground(
        ImageCoordinates const & start,
        unsigned int tolerance,
        bool isAntiAlias,
        bool doContiguousOnly);

    void RestoreInteriorTextureLayerRegionBackup(
        TextureLayerData && sourceLayerRegionBackup,
        ImageCoordinates const & targetOrigin);

    void RestoreInteriorTextureLayer(std::unique_ptr<TextureLayerData> interiorTextureLayer);

    void InteriorTextureRegionEraseForEphemeralVisualization(ImageRect const & region);

    void MakeInteriorLayerFromImage(
        TextureLayerData const & source,
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetOrigin);

    void RestoreInteriorTextureLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & backupBuffer,
        ImageRect const & backupBufferRegion,
        ImageCoordinates const & targetOrigin);

    //
    // Templated
    //

    template<LayerType TLayerType>
    std::optional<ShipSpaceRect> ReplaceCustomMaterials(std::map<typename LayerTypeTraits<TLayerType>::material_type const *, typename LayerTypeTraits<TLayerType>::material_type const *> const & replacementMap);

    //
    // Visualizations
    //

    void SetGameVisualizationMode(GameVisualizationModeType mode);
    void ForceWholeGameVisualizationRefresh();

    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode);

    void SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode);

    void SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode);

    void SetExteriorTextureLayerVisualizationMode(ExteriorTextureLayerVisualizationModeType mode);

    void SetInteriorTextureLayerVisualizationMode(InteriorTextureLayerVisualizationModeType mode);

    void UpdateVisualizations(
        View & view,
        GameAssetManager const & gameAssetManager);

    //
    // Coords
    //

    ShipSpaceRect ImageRectToContainingShipSpaceRect(
        ImageRect const & imageRect,
        vec2f const & shipToImageFactor) const;

    static vec2f GetShipSpaceToTextureSpaceFactor(
        ShipSpaceSize const & shipSize,
        ImageSize const & textureSize);

    static vec2f GetTextureSpaceToShipSpaceFactor(
        ImageSize const & textureSize,
        ShipSpaceSize const & shipSize);

    static vec2f GetTextureSpaceToTextureSpaceFactor(
        ImageSize const & sourceTextureSize,
        ImageSize const & targetTextureSize);

private:

    ModelController(
        Model && model,
        ShipTexturizer const & shipTexturizer);

    inline ShipSpaceRect GetWholeShipRect() const
    {
        return ShipSpaceRect(mModel.GetShipSize());
    }

    inline ImageRect GetWholeExteriorTextureRect() const
    {
        assert(mModel.HasLayer(LayerType::ExteriorTexture));

        return ImageRect(GetExteriorTextureSize());
    }

    inline ImageRect GetWholeInteriorTextureRect() const
    {
        assert(mModel.HasLayer(LayerType::InteriorTexture));

        return ImageRect(GetInteriorTextureSize());
    }

    void InitializeStructuralLayerAnalysis();

    void InitializeElectricalLayerAnalysis();

    void InitializeRopesLayerAnalysis();

    inline void WriteParticle(
        ShipSpaceCoordinates const & coords,
        StructuralMaterial const * material);

    template<bool IsForEphViz>
    void DoStructuralRectangle(
        ShipSpaceRect const & rect,
        std::uint32_t lineSize,
        StructuralMaterial const * lineMaterial,
        std::optional<StructuralMaterial const *> fillMaterial);

    typename LayerTypeTraits<LayerType::Structural>::buffer_type MakeStructureTrace(
        ShipSpaceRect const & shipRect,
        ImageRect const & textureRect,
        StructuralMaterial const * edgeMaterial,
        StructuralMaterial const * fillMaterial,
        std::uint8_t alphaThreshold) const;

    inline void WriteParticle(
        ShipSpaceCoordinates const & coords,
        ElectricalMaterial const * material);

    inline void AppendRope(
        ShipSpaceCoordinates const & startCoords,
        ShipSpaceCoordinates const & endCoords,
        StructuralMaterial const * material);

    void MoveRopeEndpoint(
        RopeElement & ropeElement,
        ShipSpaceCoordinates const & oldCoords,
        ShipSpaceCoordinates const & newCoords);

    bool InternalEraseRopeAt(ShipSpaceCoordinates const & coords);

    template<LayerType TLayer>
    std::optional<ShipSpaceRect> DoFlood(
        ShipSpaceCoordinates const & start,
        typename LayerTypeTraits<TLayer>::material_type const * material,
        bool doContiguousOnly,
        typename LayerTypeTraits<TLayer>::layer_data_type const & layer);

    std::optional<ImageRect> DoTextureMagicWandEraseBackground(
        ImageCoordinates const & start,
        unsigned int tolerance,
        bool isAntiAlias,
        bool doContiguousOnly,
        TextureLayerData & layer);

    GenericShipSpaceUndoPayload MakeGenericShipSpaceUndoPayload(
        std::optional<ShipSpaceRect> const & region,
        bool doStructuralLayer,
        bool doElectricalLayer,
        bool doRopesLayer,
        bool doExteriorTextureLayer,
        bool doInteriorTextureLayer) const;

    GenericShipSpaceEphemeralVisualizationRestorePayload::payload_type MakeGenericShipSpaceEphemeralVisualizationRestorePayload(
        ShipSpaceRect const & region,
        LayerSelector const & layerSelection) const;
    GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload::payload_type MakeGenericExteriorTextureSpaceEphemeralVisualizationRestorePayload(
        ImageRect const & region,
        LayerSelector const & layerSelection) const;
    GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload::payload_type MakeGenericInteriorTextureSpaceEphemeralVisualizationRestorePayload(
        ImageRect const & region,
        LayerSelector const & layerSelection) const;

    struct EphemeralVisualizationLayerState;

    void InternalRestoreStructuralLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & backupBuffer,
        ShipSpaceRect const & backupBufferRegion,
        ShipSpaceCoordinates const & targetOrigin,
        EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState);

    void InternalRestoreElectricalLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & backupBuffer,
        ShipSpaceRect const & backupBufferRegion,
        ShipSpaceCoordinates const & targetOrigin,
        EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState);

    void InternalRestoreRopesLayerEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & backupBuffer,
        EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState);

    void InternalRestoreExteriorTextureLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & backupBuffer,
        ImageRect const & backupBufferRegion,
        ImageCoordinates const & targetOrigin,
        EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState);

    void InternalRestoreInteriorTextureLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & backupBuffer,
        ImageRect const & backupBufferRegion,
        ImageCoordinates const & targetOrigin,
        EphemeralVisualizationLayerState & finalEphemeralVisualizationLayerState);

    void DoStructuralRegionFill(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

    void DoElectricalRegionFill(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    void DoRopesRegionErase(
        ShipSpaceRect const & region);

    void DoExteriorTextureRegionErase(
        ImageRect const & region);

    void DoInteriorTextureRegionErase(
        ImageRect const & region);

    void DoStructuralRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        bool isTransparent);

    void DoElectricalRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        std::function<ElectricalElement(ElectricalElement const &, ElectricalElement const &)> elementOperator);

    void DoRopesRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        bool isTransparent);

    void DoExteriorTextureRegionBufferPaste(
        typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type const & sourceBuffer,
        ImageRect const & sourceRegion,
        ImageRect const & targetRegion,
        bool isTransparent);

    void DoInteriorTextureRegionBufferPaste(
        typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type const & sourceBuffer,
        ImageRect const & sourceRegion,
        ImageRect const & targetRegion,
        bool isTransparent);

    TextureLayerData MakeTextureLayerFromImage(
        TextureLayerData const & source,
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetOrigin);

    // Viz

    template<VisualizationType TVisualization, typename TRect>
    void RegisterDirtyVisualization(TRect const & region);

    ImageRect UpdateGameVisualization(
        ShipSpaceRect const & region,
        GameAssetManager const & gameAssetManager);

    ImageRect UpdateStructuralLayerVisualization(ShipSpaceRect const & region);

    void RenderStructureInto(
        ShipSpaceRect const & structureRegion,
        RgbaImageData & texture) const;

    void UpdateElectricalLayerVisualization(ShipSpaceRect const & region);

    void UpdateRopesLayerVisualization();

    void UpdateExteriorTextureLayerVisualization();

    void UpdateInteriorTextureLayerVisualization();

private:

    Model mModel;

    ShipTexturizer const & mShipTexturizer;

    //
    // Auxiliary layers' members
    //

    size_t mMassParticleCount;
    float mTotalMass;
    vec2f mCenterOfMassSum;

    InstancedElectricalElementSet mInstancedElectricalElementSet;
    size_t mElectricalParticleCount;

    //
    // Visualizations
    //

    GameVisualizationModeType mGameVisualizationMode;
    std::unique_ptr<RgbaImageData> mGameVisualizationAutoTexturizationTexture;
    std::unique_ptr<RgbaImageData> mGameVisualizationTexture;
    int mGameVisualizationTextureMagnificationFactor;

    StructuralLayerVisualizationModeType mStructuralLayerVisualizationMode;
    std::unique_ptr<RgbaImageData> mStructuralLayerVisualizationTexture;

    ElectricalLayerVisualizationModeType mElectricalLayerVisualizationMode;
    std::unique_ptr<RgbaImageData> mElectricalLayerVisualizationTexture;

    RopesLayerVisualizationModeType mRopesLayerVisualizationMode;

    ExteriorTextureLayerVisualizationModeType mExteriorTextureLayerVisualizationMode;

    InteriorTextureLayerVisualizationModeType mInteriorTextureLayerVisualizationMode;

    // Regions whose visualizations need to be *updated* and *uploaded*
    CombinedShipSpaceRegions mDirtyGameVisualizationRegions;
    CombinedShipSpaceRegions mDirtyStructuralLayerVisualizationRegions;
    CombinedShipSpaceRegions mDirtyElectricalLayerVisualizationRegions;
    CombinedShipSpaceRegions mDirtyRopesLayerVisualizationRegions;
    CombinedTextureSpaceRegions mDirtyExteriorTextureLayerVisualizationRegions;
    CombinedTextureSpaceRegions mDirtyInteriorTextureLayerVisualizationRegions;

    //
    // Debugging
    //

    struct EphemeralVisualizationLayerState
    {
        bool IsStructuralLayerInEphemeralVisualization;
        bool IsElectricalLayerInEphemeralVisualization;
        bool IsRopesLayerInEphemeralVisualization;
        bool IsExteriorTextureLayerInEphemeralVisualization;
        bool IsInteriorTextureLayerInEphemeralVisualization;

        EphemeralVisualizationLayerState()
            : IsStructuralLayerInEphemeralVisualization(false)
            , IsElectricalLayerInEphemeralVisualization(false)
            , IsRopesLayerInEphemeralVisualization(false)
            , IsExteriorTextureLayerInEphemeralVisualization(false)
            , IsInteriorTextureLayerInEphemeralVisualization(false)
        { }
    };

    EphemeralVisualizationLayerState mEphemeralVisualizationLayerState;
};

}