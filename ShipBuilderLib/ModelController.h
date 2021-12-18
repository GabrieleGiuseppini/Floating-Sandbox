/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElectricalElementInstanceIndexFactory.h"
#include "Model.h"
#include "ModelValidationResults.h"
#include "ShipBuilderTypes.h"
#include "View.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ShipDefinition.h>
#include <Game/ShipTexturizer.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <memory>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * This class implements operations on the model.
 *
 * The ModelController does not maintain the model's dirtyness; that is managed by the Controller.
 */
class ModelController
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

    Model const & GetModel() const
    {
        return mModel;
    }

    std::optional<ShipSpaceRect> CalculateBoundingBox() const;

    void SetLayerDirty(LayerType layer)
    {
        mModel.SetIsDirty(layer);
    }

    void SetAllLayersDirty()
    {
        mModel.SetAllLayersDirty();
    }

    void RestoreDirtyState(Model::DirtyState const & dirtyState)
    {
        mModel.SetDirtyState(dirtyState);
    }

    void ClearIsDirty()
    {
        mModel.ClearIsDirty();
    }

    ModelValidationResults ValidateModel() const;

    void UploadVisualizations(View & view);

#ifdef _DEBUG
    bool IsInEphemeralVisualization() const
    {
        return mIsStructuralLayerInEphemeralVisualization
            || mIsElectricalLayerInEphemeralVisualization
            || mIsRopesLayerInEphemeralVisualization;
    }
#endif

    ShipMetadata const & GetShipMetadata() const
    {
        return mModel.GetShipMetadata();
    }

    void SetShipMetadata(ShipMetadata && shipMetadata)
    {
        mModel.SetShipMetadata(std::move(shipMetadata));
    }

    ShipPhysicsData const & GetShipPhysicsData() const
    {
        return mModel.GetShipPhysicsData();
    }

    void SetShipPhysicsData(ShipPhysicsData && shipPhysicsData)
    {
        mModel.SetShipPhysicsData(std::move(shipPhysicsData));
    }

    std::optional<ShipAutoTexturizationSettings> const & GetShipAutoTexturizationSettings() const
    {
        return mModel.GetShipAutoTexturizationSettings();
    }

    void SetShipAutoTexturizationSettings(std::optional<ShipAutoTexturizationSettings> && shipAutoTexturizationSettings)
    {
        mModel.SetShipAutoTexturizationSettings(std::move(shipAutoTexturizationSettings));
    }

    void Flip(DirectionType direction);

    //
    // Structural
    //

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    void StructuralRegionFill(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

    std::optional<ShipSpaceRect> StructuralFlood(
        ShipSpaceCoordinates const & start,
        StructuralMaterial const * material,
        bool doContiguousOnly);

    void RestoreStructuralLayerRegion(
        StructuralLayerData && sourceLayerRegion,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void StructuralRegionFillForEphemeralVisualization(
        ShipSpaceRect const & region,
        StructuralMaterial const * material);

    void RestoreStructuralLayerRegionForEphemeralVisualization(
        StructuralLayerData const & sourceLayerRegion,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode);

    RgbaImageData const & GetStructuralLayerVisualization() const;

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    bool IsElectricalParticleAllowedAt(ShipSpaceCoordinates const & coords) const;

    std::optional<ShipSpaceRect> TrimElectricalParticlesWithoutSubstratum();

    void ElectricalRegionFill(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    void RestoreElectricalLayerRegion(
        ElectricalLayerData && sourceLayerRegion,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void ElectricalRegionFillForEphemeralVisualization(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    void RestoreElectricalLayerRegionForEphemeralVisualization(
        ElectricalLayerData const & sourceLayerRegion,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetOrigin);

    void SetElectricalLayerVisualizationMode(std::optional<ElectricalLayerVisualizationModeType> mode);

    //
    // Ropes
    //

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

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

    void RestoreRopesLayer(RopesLayerData && sourceLayer);

    void AddRopeForEphemeralVisualization(
        ShipSpaceCoordinates const & startCoords,
        ShipSpaceCoordinates const & endCoords,
        StructuralMaterial const * material);

    void MoveRopeEndpointForEphemeralVisualization(
        size_t ropeElementIndex,
        ShipSpaceCoordinates const & oldCoords,
        ShipSpaceCoordinates const & newCoords);

    void RestoreRopesLayerForEphemeralVisualization(RopesLayerData const & sourceLayer);

    void SetRopesLayerVisualizationMode(std::optional<RopesLayerVisualizationModeType> mode);

    //
    // Texture
    //

    void NewTextureLayer();
    void SetTextureLayer(
        TextureLayerData && textureLayer,
        std::optional<std::string> originalTextureArtCredits);
    void RemoveTextureLayer();

    std::unique_ptr<TextureLayerData> CloneTextureLayer() const;

    void RestoreTextureLayer(
        std::unique_ptr<TextureLayerData> textureLayer,
        std::optional<std::string> originalTextureArtCredits);

    void SetTextureLayerVisualizationMode(std::optional<TextureLayerVisualizationModeType> mode);

private:

    ModelController(
        Model && model,
        ShipTexturizer const & shipTexturizer);

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

    template<LayerType TLayer>
    std::optional<ShipSpaceRect> Flood(
        ShipSpaceCoordinates const & start,
        typename LayerTypeTraits<TLayer>::material_type const * material,
        bool doContiguousOnly,
        typename LayerTypeTraits<TLayer>::layer_data_type const & layer);

    void UpdateStructuralLayerVisualization(ShipSpaceRect const & region);

    void UpdateElectricalLayerVisualization(ShipSpaceRect const & region);

    void UpdateRopesLayerVisualization();

    void UpdateTextureLayerVisualization();

    inline ShipSpaceRect GetWholeShipRect() const
    {
        return { ShipSpaceCoordinates(0, 0), mModel.GetShipSize() };
    }

private:

    Model mModel;

    ShipTexturizer const & mShipTexturizer;

    //
    // Auxiliary layers' members
    //


    ElectricalElementInstanceIndexFactory mElectricalElementInstanceIndexFactory;
    size_t mElectricalParticleCount;

    //
    // Visualizations
    //

    std::optional<StructuralLayerVisualizationModeType> mStructuralLayerVisualizationMode;
    std::unique_ptr<RgbaImageData> mStructuralLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyStructuralLayerVisualizationRegion;

    std::optional<ElectricalLayerVisualizationModeType> mElectricalLayerVisualizationMode;
    std::unique_ptr<RgbaImageData> mElectricalLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyElectricalLayerVisualizationRegion;

    std::optional<RopesLayerVisualizationModeType> mRopesLayerVisualizationMode;
    bool mIsRopesLayerVisualizationDirty;

    std::optional<TextureLayerVisualizationModeType> mTextureLayerVisualizationMode;
    bool mIsTextureLayerVisualizationDirty;

    //
    // Debugging
    //

    bool mIsStructuralLayerInEphemeralVisualization;
    bool mIsElectricalLayerInEphemeralVisualization;
    bool mIsRopesLayerInEphemeralVisualization;
};

}