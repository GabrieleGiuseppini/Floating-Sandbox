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

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <memory>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * This class implements operations on the model.
 */
class ModelController
{
public:

    static std::unique_ptr<ModelController> CreateNew(
        ShipSpaceSize const & shipSpaceSize,
        std::string const & shipName,
        View & view);

    static std::unique_ptr<ModelController> CreateForShip(
        ShipDefinition && shipDefinition,
        View & view);

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

    void RestoreDirtyState(Model::DirtyState const & dirtyState)
    {
        mModel.SetDirtyState(dirtyState);
    }

    void ClearIsDirty()
    {
        mModel.ClearIsDirty();
    }

    ModelValidationResults ValidateModel() const;

    void UploadVisualization();

#ifdef _DEBUG
    bool IsInEphemeralVisualization() const
    {
        return mIsStructuralLayerInEphemeralVisualization
            || mIsElectricalLayerInEphemeralVisualization
            || mIsRopesLayerInEphemeralVisualization;
        // TODO: other layers
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

    void RestoreStructuralLayer(
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

    void RestoreElectricalLayer(
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

    //
    // Texture
    //

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();

private:

    ModelController(
        Model && model,
        View & view);

    void InitializeStructuralLayer();

    void InitializeElectricalLayer();

    void InitializeRopesLayer();

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

    void UpdateTextureLayerVisualization(ShipSpaceRect const & region);

private:

    View & mView;

    Model mModel;

    //
    // Auxiliary layers' members
    //


    ElectricalElementInstanceIndexFactory mElectricalElementInstanceIndexFactory;
    size_t mElectricalParticleCount;

    //
    // Visualizations
    //

    std::unique_ptr<RgbaImageData> mStructuralLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyStructuralLayerVisualizationRegion;

    std::unique_ptr<RgbaImageData> mElectricalLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyElectricalLayerVisualizationRegion;

    bool mIsRopesLayerVisualizationDirty;

    // TODO: other layers

    //
    // Debugging
    //

    bool mIsStructuralLayerInEphemeralVisualization;
    bool mIsElectricalLayerInEphemeralVisualization;
    bool mIsRopesLayerInEphemeralVisualization;
};

}