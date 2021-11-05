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


    Model const & GetModel() const
    {
        return mModel;
    }

    ShipDefinition MakeShipDefinition() const;

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
        StructuralMaterial const * material);

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

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void TrimElectricalParticlesWithoutSubstratum();

    void ElectricalRegionFill(
        ShipSpaceRect const & region,
        ElectricalMaterial const * material);

    std::optional<ShipSpaceRect> ElectricalFlood(
        ShipSpaceCoordinates const & start,
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

    inline void WriteParticle(
        ShipSpaceCoordinates const & coords,
        StructuralMaterial const * material);

    inline void WriteParticle(
        ShipSpaceCoordinates const & coords,
        ElectricalMaterial const * material);

    template<LayerType TLayer>
    std::optional<ShipSpaceRect> Flood(
        ShipSpaceCoordinates const & start,
        typename LayerTypeTraits<TLayer>::material_type const * material,
        typename LayerTypeTraits<TLayer>::layer_data_type const & layer);

    void UpdateStructuralLayerVisualization(ShipSpaceRect const & region);

    void UpdateElectricalLayerVisualization(ShipSpaceRect const & region);

    void UpdateRopesLayerVisualization(ShipSpaceRect const & region);

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

    // TODO: other layers

    //
    // Debugging
    //

    bool mIsStructuralLayerInEphemeralVisualization;
    bool mIsElectricalLayerInEphemeralVisualization;
};

}