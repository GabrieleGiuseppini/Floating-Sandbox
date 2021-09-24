/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ShipBuilderTypes.h"
#include "UndoStack.h"
#include "View.h"

#include <Game/Materials.h>

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
        View & view);

    static std::unique_ptr<ModelController> CreateForShip(
        /* TODO: loaded ship ,*/
        View & view);

    //
    // Structural
    //

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    std::unique_ptr<UndoEntry> StructuralRegionFill(
        StructuralMaterial const * material,
        ShipSpaceCoordinates const & origin,
        ShipSpaceSize const & size);

    std::unique_ptr<UndoEntry> StructuralRegionReplace(
        MaterialBuffer<StructuralMaterial> const & region,
        ShipSpaceCoordinates const & origin);

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    std::unique_ptr<UndoEntry> ElectricalRegionFill(
        ElectricalMaterial const * material,
        ShipSpaceCoordinates const & origin,
        ShipSpaceSize const & size);

    std::unique_ptr<UndoEntry> ElectricalRegionReplace(
        MaterialBuffer<ElectricalMaterial> const & region,
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
        ShipSpaceSize const & shipSpaceSize,
        View & view);

    void UploadStructuralLayerToView();

    void UploadStructuralLayerRowToView(
        ShipSpaceCoordinates const & origin,
        int width);

private:

    View & mView;

    Model mModel;
};

}