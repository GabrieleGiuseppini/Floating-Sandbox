/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ShipBuilderTypes.h"
#include "View.h"

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

    void UploadVisualization();

    //
    // Structural
    //

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    void StructuralRegionFill(
        StructuralElement const & element,
        ShipSpaceRect const & region);

    void StructuralRegionReplace(
        StructuralLayerBuffer const & sourceLayerBufferRegion,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetOrigin);

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void ElectricalRegionFill(
        ElectricalElement const & element,
        ShipSpaceRect const & region);

    void ElectricalRegionReplace(
        ElectricalLayerBuffer const & sourceLayerBufferRegion,
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

    void UpdateStructuralLayerVisualization(ShipSpaceRect const & region);

    void UpdateElectricalLayerVisualization(ShipSpaceRect const & region);

    void UpdateRopesLayerVisualization(ShipSpaceRect const & region);

    void UpdateTextureLayerVisualization(ShipSpaceRect const & region);

private:

    View & mView;

    Model mModel;

    //
    // Visualizations
    //

    std::unique_ptr<RgbaImageData> mStructuralLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyStructuralLayerVisualizationRegion;

    std::unique_ptr<RgbaImageData> mElectricalLayerVisualizationTexture;
    std::optional<ImageRect> mDirtyElectricalLayerVisualizationRegion;

    // TODO: other layers
};

}