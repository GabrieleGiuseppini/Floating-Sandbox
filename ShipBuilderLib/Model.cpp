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
    , mStructuralLayerBuffer(MakeNewEmptyStructuralLayer(mShipSize))
    , mElectricalLayerBuffer()
    // TODO: rope layer
    , mTextureLayerBuffer() // None
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
    , mStructuralLayerBuffer(new StructuralLayerBuffer(std::move(shipDefinition.StructuralLayer)))
    , mElectricalLayerBuffer(std::move(shipDefinition.ElectricalLayer))
    // TODO: other layers
    , mTextureLayerBuffer(std::move(shipDefinition.TextureLayer))
    , mDirtyState()
{
    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = (mElectricalLayerBuffer != nullptr);
    // TODO: other layers
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = (mTextureLayerBuffer != nullptr);
}

void Model::NewStructuralLayer()
{
    // Reset layer
    mStructuralLayerBuffer = MakeNewEmptyStructuralLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::SetStructuralLayer(/*TODO*/)
{
    // TODO
}

std::unique_ptr<StructuralLayerBuffer> Model::CloneStructuralLayerBuffer() const
{
    assert(mStructuralLayerBuffer);
    return mStructuralLayerBuffer->MakeCopy();
}

void Model::NewElectricalLayer()
{
    // Reset layer
    mElectricalLayerBuffer = MakeNewEmptyElectricalLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::SetElectricalLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveElectricalLayer()
{
    // Remove layer
    mElectricalLayerBuffer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
}

std::unique_ptr<ElectricalLayerBuffer> Model::CloneElectricalLayerBuffer() const
{
    assert(mElectricalLayerBuffer);
    return mElectricalLayerBuffer->MakeCopy();
}

void Model::NewRopesLayer()
{
    // TODO

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
    mTextureLayerBuffer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
}

std::unique_ptr<TextureLayerBuffer> Model::CloneTextureLayerBuffer() const
{
    assert(mTextureLayerBuffer);
    return mTextureLayerBuffer->MakeCopy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<StructuralLayerBuffer> Model::MakeNewEmptyStructuralLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<StructuralLayerBuffer>(
        shipSize,
        StructuralElement(nullptr)); // No material
}

std::unique_ptr<ElectricalLayerBuffer> Model::MakeNewEmptyElectricalLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<ElectricalLayerBuffer>(
        shipSize,
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex)); // No material
}

}