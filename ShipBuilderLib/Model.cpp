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
    , mShipAutoTexturizationSettings()
    , mStructuralLayer(MakeNewEmptyStructuralLayer(mShipSize))
    , mElectricalLayer() // None
    , mRopesLayer() // None
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
    , mShipAutoTexturizationSettings(shipDefinition.AutoTexturizationSettings)
    , mStructuralLayer(new StructuralLayerData(std::move(shipDefinition.StructuralLayer)))
    , mElectricalLayer(std::move(shipDefinition.ElectricalLayer))
    , mRopesLayer(std::move(shipDefinition.RopesLayer))
    , mTextureLayer(std::move(shipDefinition.TextureLayer))
    , mDirtyState()
{
    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = (mElectricalLayer != nullptr);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = (mRopesLayer != nullptr);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = (mTextureLayer != nullptr);
}

ShipDefinition Model::MakeShipDefinition() const
{
    return ShipDefinition(
        GetShipSize(),
        CloneStructuralLayer(),
        CloneElectricalLayer(),
        CloneRopesLayer(),
        CloneTextureLayer(),
        GetShipMetadata(),
        GetShipPhysicsData(),
        GetShipAutoTexturizationSettings());
}

void Model::NewStructuralLayer()
{
    // Reset layer
    mStructuralLayer = MakeNewEmptyStructuralLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::SetStructuralLayer(StructuralLayerData && structuralLayer)
{
    assert(structuralLayer.Buffer.Size == GetShipSize());

    // Update layer
    mStructuralLayer.reset(new StructuralLayerData(std::move(structuralLayer)));

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

StructuralLayerData Model::CloneStructuralLayer() const
{
    assert(mStructuralLayer);

    return mStructuralLayer->Clone();
}

void Model::RestoreStructuralLayer(StructuralLayerData && structuralLayer)
{
    // Replace layer
    mStructuralLayer = std::make_unique<StructuralLayerData>(std::move(structuralLayer));

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::NewElectricalLayer()
{
    // Reset layer
    mElectricalLayer = MakeNewEmptyElectricalLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::SetElectricalLayer(ElectricalLayerData && electricalLayer)
{
    assert(electricalLayer.Buffer.Size == GetShipSize());

    // Update layer
    mElectricalLayer.reset(new ElectricalLayerData(std::move(electricalLayer)));

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::RemoveElectricalLayer()
{
    // Remove layer
    mElectricalLayer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
}

std::unique_ptr<ElectricalLayerData> Model::CloneElectricalLayer() const
{
    std::unique_ptr<ElectricalLayerData> clonedLayer;

    if (mElectricalLayer)
    {
        clonedLayer.reset(new ElectricalLayerData(mElectricalLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreElectricalLayer(std::unique_ptr<ElectricalLayerData> electricalLayer)
{
    // Replace layer
    mElectricalLayer = std::move(electricalLayer);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = (bool)mElectricalLayer;
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
    ////assert(structuralLayer.Buffer.Size == GetShipSize());

    ////// Update layer
    ////mStructuralLayer.reset(new StructuralLayerData(std::move(structuralLayer)));

    // TODO: make sure also electrical panel is copied (moved) with LayerData

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = true;
}

void Model::RemoveRopesLayer()
{
    // Remove layer
    mRopesLayer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = false;
}

std::unique_ptr<RopesLayerData> Model::CloneRopesLayer() const
{
    std::unique_ptr<RopesLayerData> clonedLayer;

    if (mRopesLayer)
    {
        clonedLayer.reset(new RopesLayerData(mRopesLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreRopesLayer(std::unique_ptr<RopesLayerData> ropesLayer)
{
    // Replace layer
    mRopesLayer = std::move(ropesLayer);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = (bool)mRopesLayer;
}

void Model::NewTextureLayer()
{
    // TODO

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = true;
}

void Model::SetTextureLayer(TextureLayerData && textureLayer)
{
    // Update layer
    mTextureLayer.reset(new TextureLayerData(std::move(textureLayer)));

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = true;
}

void Model::RemoveTextureLayer()
{
    // Remove layer
    mTextureLayer.reset();

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
}

std::unique_ptr<TextureLayerData> Model::CloneTextureLayer() const
{
    std::unique_ptr<TextureLayerData> clonedLayer;

    if (mTextureLayer)
    {
        clonedLayer.reset(new TextureLayerData(mTextureLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreTextureLayer(std::unique_ptr<TextureLayerData> textureLayer)
{
    // Replace layer
    mTextureLayer = std::move(textureLayer);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = (bool)mTextureLayer;
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