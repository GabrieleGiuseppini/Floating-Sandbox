/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EditAction.h"
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
        WorkSpaceSize const & workSpaceSize,
        View & view);

    static std::unique_ptr<ModelController> CreateForShip(
        /* TODO: loaded ship ,*/
        View & view);

    std::unique_ptr<UndoEntry> Edit(EditAction const & action);

    void Apply(EditAction const & action);

    void Apply(std::vector<EditAction> const & actions);

    //
    // Structural
    //

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    //
    // Electrical
    //

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

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
        WorkSpaceSize const & workSpaceSize,
        View & view);

    //
    // Structural
    //

// TODOTEST
public:

    std::unique_ptr<EditAction> StructuralRegionFill(
        StructuralMaterial const * material,
        WorkSpaceCoordinates const & origin,
        WorkSpaceSize const & size);

    void UploadStructuralLayerToView();

    //
    // Electrical
    //

    std::unique_ptr<EditAction> ElectricalRegionFill(
        ElectricalMaterial const * material,
        WorkSpaceCoordinates const & origin,
        WorkSpaceSize const & size);

    //
    // Ropes
    //

    //
    // Texture
    //

private:

    View & mView;

    Model mModel;
};

}