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
#include "Model.h"
#include "ModelValidationSession.h"
#include "ShipBuilderTypes.h"
#include "View.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ShipDefinition.h>
#include <Game/ShipTexturizer.h>

#include <GameCore/Finalizer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
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

    void SetAllPresentLayersDirty()
    {
        mModel.SetAllPresentLayersDirty();
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
        return mIsStructuralLayerInEphemeralVisualization
            || mIsElectricalLayerInEphemeralVisualization
            || mIsRopesLayerInEphemeralVisualization
            || mIsTextureLayerInEphemeralVisualization;
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
                assert(!mIsElectricalLayerInEphemeralVisualization);
                break;
            }

            case LayerType::Ropes:
            {
                assert(!mIsRopesLayerInEphemeralVisualization);
                break;
            }

            case LayerType::Structural:
            {
                assert(!mIsStructuralLayerInEphemeralVisualization);
                break;
            }

            case LayerType::Texture:
            {
                assert(!mIsTextureLayerInEphemeralVisualization);
                break;
            }
        }

        return mModel.CloneExistingLayer<TLayer>();
    }

    ShipLayers Copy(
        ShipSpaceRect const & region,
        std::optional<LayerType> layerSelection) const;

    GenericUndoPayload EraseRegion(
        ShipSpaceRect const & region,
        std::optional<LayerType> const & layerSelection);

    GenericUndoPayload Paste(
        ShipLayers const & sourcePayload,
        ShipSpaceCoordinates const & pasteOrigin,
        bool isTransparent);

    void Restore(GenericUndoPayload && undoPayload);

    GenericEphemeralVisualizationRestorePayload PasteForEphemeralVisualization(
        ShipLayers const & sourcePayload,
        ShipSpaceCoordinates const & pasteOrigin,
        bool isTransparent);

    void RestoreEphemeralVisualization(GenericEphemeralVisualizationRestorePayload && restorePayload);

    std::vector<LayerType> CalculateAffectedLayers(ShipLayers const & otherSource) const;

    //
    // Structural
    //

    StructuralLayerData const & GetStructuralLayer() const override;

    void SetStructuralLayer(StructuralLayerData && structuralLayer);

    std::unique_ptr<StructuralLayerData> CloneStructuralLayer() const;

    StructuralMaterial const * SampleStructuralMaterialAt(ShipSpaceCoordinates const & coords) const;

    void StructuralRegionFill(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

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

    void RestoreStructuralLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & backupBuffer,
        ShipSpaceRect const & backupBufferRegion,
        ShipSpaceCoordinates const & targetOrigin);

    //
    // Electrical
    //

    ElectricalPanel const & GetElectricalPanel() const;

    void SetElectricalLayer(ElectricalLayerData && electricalLayer);

    void RemoveElectricalLayer();

    std::unique_ptr<ElectricalLayerData> CloneElectricalLayer() const;

    ElectricalMaterial const * SampleElectricalMaterialAt(ShipSpaceCoordinates const & coords) const;

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

    //
    // Ropes
    //

    void SetRopesLayer(RopesLayerData && ropesLayer);

    void RemoveRopesLayer();

    std::unique_ptr<RopesLayerData> CloneRopesLayer() const;

    StructuralMaterial const * SampleRopesMaterialAt(ShipSpaceCoordinates const & coords) const;

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
    // Texture
    //

    TextureLayerData const & GetTextureLayer() const;

    ImageSize const & GetTextureSize() const
    {
        assert(mModel.HasLayer(LayerType::Texture));

        return mModel.GetTextureLayer().Buffer.Size;
    }

    static vec2f GetShipSpaceToTextureSpaceFactor(
        ShipSpaceSize const & shipSize,
        ImageSize const & textureSize)
    {
        return vec2f(
            static_cast<float>(textureSize.width) / static_cast<float>(shipSize.width),
            static_cast<float>(textureSize.height) / static_cast<float>(shipSize.height));
    }

    static vec2f GetTextureSpaceToShipSpaceFactor(
        ImageSize const & textureSize,
        ShipSpaceSize const & shipSize)
    {
        return vec2f(
            static_cast<float>(shipSize.width) / static_cast<float>(textureSize.width),
            static_cast<float>(shipSize.height) / static_cast<float>(textureSize.height));
    }

    ImageCoordinates ShipSpaceToTextureSpace(ShipSpaceCoordinates const & shipCoordinates) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetTextureSize());

        return ImageCoordinates::FromFloatFloor(shipCoordinates.ToFloat().scale(shipToTexture));
    }

    ImageRect ShipSpaceToTextureSpace(ShipSpaceRect const & shipRect) const
    {
        vec2f const shipToTexture = GetShipSpaceToTextureSpaceFactor(GetShipSize(), GetTextureSize());

        return ImageRect(
            ImageCoordinates::FromFloatFloor(shipRect.origin.ToFloat().scale(shipToTexture)),
            ImageSize::FromFloatFloor(shipRect.size.ToFloat().scale(shipToTexture)));
    }

    ShipSpaceRect ImageRectToContainingShipSpaceRect(ImageRect const & imageRect) const;

    void SetTextureLayer(
        TextureLayerData && textureLayer,
        std::optional<std::string> originalTextureArtCredits);
    void RemoveTextureLayer();

    std::unique_ptr<TextureLayerData> CloneTextureLayer() const;

    void EraseTextureRegion(ImageRect const & region);

    std::optional<ImageRect> TextureMagicWandEraseBackground(
        ImageCoordinates const & start,
        unsigned int tolerance,
        bool isAntiAlias,
        bool doContiguousOnly);

    void RestoreTextureLayerRegionBackup(
        TextureLayerData && sourceLayerRegionBackup,
        ImageCoordinates const & targetOrigin);

    void RestoreTextureLayer(
        std::unique_ptr<TextureLayerData> textureLayer,
        std::optional<std::string> originalTextureArtCredits);

    void TextureRegionEraseForEphemeralVisualization(ImageRect const & region);

    void RestoreTextureLayerRegionEphemeralVisualization(
        typename LayerTypeTraits<LayerType::Texture>::buffer_type const & backupBuffer,
        ImageRect const & backupBufferRegion,
        ImageCoordinates const & targetOrigin);

    //
    // Visualizations
    //

    void SetGameVisualizationMode(GameVisualizationModeType mode);
    void ForceWholeGameVisualizationRefresh();

    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode);

    void SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode);

    void SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode);

    void SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode);

    void UpdateVisualizations(View & view);

private:

    ModelController(
        Model && model,
        ShipTexturizer const & shipTexturizer);

    inline ShipSpaceRect GetWholeShipRect() const
    {
        return ShipSpaceRect(mModel.GetShipSize());
    }

    inline ImageRect GetWholeTextureRect() const
    {
        assert(mModel.HasLayer(LayerType::Texture));

        return ImageRect(GetTextureSize());
    }

    void InitializeStructuralLayerAnalysis();

    void InitializeElectricalLayerAnalysis();

    void InitializeRopesLayerAnalysis();

    inline void WriteParticle(
        ShipSpaceCoordinates const & coords,
        StructuralMaterial const * material);

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

    GenericUndoPayload MakeGenericUndoPayload(
        ShipSpaceRect const & region,
        bool doStructuralLayer,
        bool doElectricalLayer,
        bool doRopesLayer,
        bool doTextureLayer) const;

    GenericEphemeralVisualizationRestorePayload MakeGenericEphemeralVisualizationRestorePayload(
        ShipSpaceRect const & region,
        bool doStructuralLayer,
        bool doElectricalLayer,
        bool doRopesLayer,
        bool doTextureLayer) const;

    bool CheckLayerSelectionApplicability(
        std::optional<LayerType> layerSelection,
        LayerType layer) const
    {
        return (!layerSelection.has_value() || layerSelection == layer)
            && HasLayer(layer);
    }

    void DoStructuralRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Structural>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        bool isTransparent);

    void DoElectricalRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Electrical>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        std::function<ElectricalElement const &(ElectricalElement const &, ElectricalElement const &)> elementOperator);

    void DoRopesRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Ropes>::buffer_type const & sourceBuffer,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetCoordinates,
        bool isTransparent);

    void DoTextureRegionBufferPaste(
        typename LayerTypeTraits<LayerType::Texture>::buffer_type const & sourceBuffer,
        ImageRect const & sourceRegion,
        ImageRect const & targetRegion,
        ImageCoordinates const & targetCoordinates,
        bool isTransparent);

    // Viz

    template<VisualizationType TVisualization, typename TRect>
    void RegisterDirtyVisualization(TRect const & region);

    ImageRect UpdateGameVisualization(ShipSpaceRect const & region);

    ImageRect UpdateStructuralLayerVisualization(ShipSpaceRect const & region);

    void RenderStructureInto(
        ShipSpaceRect const & structureRegion,
        RgbaImageData & texture) const;

    void UpdateElectricalLayerVisualization(ShipSpaceRect const & region);

    void UpdateRopesLayerVisualization();

    void UpdateTextureLayerVisualization();

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

    TextureLayerVisualizationModeType mTextureLayerVisualizationMode;

    // Regions whose visualization needs to be *updated* and uploaded
    std::optional<ShipSpaceRect> mDirtyGameVisualizationRegion;
    std::optional<ShipSpaceRect> mDirtyStructuralLayerVisualizationRegion;
    std::optional<ShipSpaceRect> mDirtyElectricalLayerVisualizationRegion;
    std::optional<ShipSpaceRect> mDirtyRopesLayerVisualizationRegion;
    std::optional<ImageRect> mDirtyTextureLayerVisualizationRegion;

    //
    // Debugging
    //

    bool mIsStructuralLayerInEphemeralVisualization;
    bool mIsElectricalLayerInEphemeralVisualization;
    bool mIsRopesLayerInEphemeralVisualization;
    bool mIsTextureLayerInEphemeralVisualization;
};

}