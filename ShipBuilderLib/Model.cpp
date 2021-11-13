/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Model.h"

#include <Game/MaterialDatabase.h>

namespace ShipBuilder {

Model::Model(
    ShipSpaceSize const & shipSize,
    std::string const & shipName)
    : mShipSize(shipSize)
    , mShipMetadata(shipName)
    , mShipPhysicsData()
    , mStructuralLayer(MakeNewEmptyStructuralLayer(mShipSize))
    , mElectricalLayer()
    // TODO: ropes layer
    , mTextureLayer() // None
    , mDirtyState()
{
    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

Model::Model(ShipDefinition && shipDefinition)
    : mShipSize(shipDefinition.Size)
    , mShipMetadata(shipDefinition.Metadata)
    , mShipPhysicsData(shipDefinition.PhysicsData)
    , mStructuralLayer(new StructuralLayerData(std::move(shipDefinition.StructuralLayer)))
    , mElectricalLayer(std::move(shipDefinition.ElectricalLayer))
    // TODO: other layers
    , mTextureLayer(std::move(shipDefinition.TextureLayer))
    , mDirtyState()
{
    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = (mElectricalLayer != nullptr);
    // TODO: other layers
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = (mTextureLayer != nullptr);
}

void Model::NewStructuralLayer()
{
    // Reset layer
    mStructuralLayer = MakeNewEmptyStructuralLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::SetStructuralLayer(/*TODO*/)
{
    // TODO
}

void Model::NewElectricalLayer()
{
    // Reset layer
    mElectricalLayer = MakeNewEmptyElectricalLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::SetElectricalLayer(/*TODO*/)
{
    // TODO
    // TODO: make sure also electrical panel is copied (moved) with LayerData
}

void Model::RemoveElectricalLayer()
{
    // Remove layer
    mElectricalLayer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
}

void Model::NewRopesLayer()
{
    // Reset layer
    mRopesLayer = MakeNewEmptyRopesLayer();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = true;
}

void Model::SetRopesLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveRopesLayer()
{
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = false;
}

void Model::NewTextureLayer()
{
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = true;
}

void Model::SetTextureLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveTextureLayer()
{
    // Remove layer
    mTextureLayer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<StructuralLayerData> Model::MakeNewEmptyStructuralLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<StructuralLayerData>(
        shipSize,
        StructuralElement(nullptr)); // No material
}

std::unique_ptr<ElectricalLayerData> Model::MakeNewEmptyElectricalLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<ElectricalLayerData>(
        shipSize,
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex)); // No material
}

std::unique_ptr<RopesLayerData> Model::MakeNewEmptyRopesLayer()
{
    return std::make_unique<RopesLayerData>();
}

}