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
    , mStructuralLayerBuffer(MakeNewStructuralLayer(mShipSize))
    // TODO: other layers
    , mDirtyState()
{
    // Initialize derived structural data
    InitializeDerivedStructuralData();

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

Model::Model(ShipDefinition && shipDefinition)
    : mShipSize(shipDefinition.Size)
    , mShipMetadata(shipDefinition.Metadata)
    , mStructuralLayerBuffer(new StructuralLayerBuffer(std::move(shipDefinition.StructuralLayer)))
    // TODO: other layers
    , mDirtyState()
{
    // Initialize derived structural data
    InitializeDerivedStructuralData();

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::NewStructuralLayer()
{
    // Reset layer
    mStructuralLayerBuffer = MakeNewStructuralLayer(mShipSize);
    InitializeDerivedStructuralData();

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
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::SetElectricalLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveElectricalLayer()
{
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
}

std::unique_ptr<ElectricalLayerBuffer> Model::CloneElectricalLayerBuffer() const
{
    // TODO
    return nullptr;
    ////assert(mElectricalLayerBuffer);
    ////return mElectricalLayerBuffer->MakeCopy();
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
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<StructuralLayerBuffer> Model::MakeNewStructuralLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<StructuralLayerBuffer>(
        shipSize,
        StructuralElement(nullptr)); // No material
}

void Model::InitializeDerivedStructuralData()
{
    mStructuralRenderColorTexture = std::make_unique<RgbaImageData>(mShipSize.width, mShipSize.height);
}

}