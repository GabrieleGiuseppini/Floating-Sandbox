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

#include <memory>
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

    //
    // Structural
    //

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    void StructuralRegionFill(
        StructuralMaterial const * material,
        ShipSpaceRect const & region);

    void StructuralRegionReplace(
        StructuralLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void ElectricalRegionFill(
        ElectricalMaterial const * material,
        ShipSpaceRect const & region);

    void ElectricalRegionReplace(
        ElectricalLayerBuffer const & layerBufferRegion,
        ShipSpaceCoordinates const & origin);

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

    Model const & GetModel() const
    {
        return mModel;
    }

private:

    ModelController(
        Model && model,
        View & view);

    static void RepopulateDerivedStructuralData(
        Model & model,
        ShipSpaceRect const & region);

    void UploadStructuralLayerToView();

    void UploadStructuralLayerRowToView(
        ShipSpaceCoordinates const & origin,
        int width);

private:

    View & mView;

    Model mModel;
};

}