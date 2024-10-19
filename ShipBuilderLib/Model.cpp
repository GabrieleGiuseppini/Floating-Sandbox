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
    : mShipMetadata(shipName)
    , mShipPhysicsData()
    , mShipAutoTexturizationSettings()
    , mLayers(
        shipSize,
        MakeNewEmptyStructuralLayer(shipSize),
        nullptr,
        nullptr,
        nullptr,
        nullptr)
    , mDirtyState()
{
}

Model::Model(ShipDefinition && shipDefinition)
    : mShipMetadata(shipDefinition.Metadata)
    , mShipPhysicsData(shipDefinition.PhysicsData)
    , mShipAutoTexturizationSettings(shipDefinition.AutoTexturizationSettings)
    , mLayers(std::move(shipDefinition.Layers))
    , mDirtyState()
{
}

ShipDefinition Model::MakeShipDefinition() const
{
    return ShipDefinition(
        ShipLayers(
            GetShipSize(),
            CloneStructuralLayer(),
            CloneElectricalLayer(),
            CloneRopesLayer(),
            CloneExteriorTextureLayer(),
            CloneInteriorTextureLayer()),
        GetShipMetadata(),
        GetShipPhysicsData(),
        GetShipAutoTexturizationSettings());
}

void Model::SetStructuralLayer(StructuralLayerData && structuralLayer)
{
    assert(structuralLayer.Buffer.Size == GetShipSize());

    // Update layer
    mLayers.StructuralLayer.reset(new StructuralLayerData(std::move(structuralLayer)));
}

std::unique_ptr<StructuralLayerData> Model::CloneStructuralLayer() const
{
    std::unique_ptr<StructuralLayerData> clonedLayer;

    if (mLayers.StructuralLayer)
    {
        clonedLayer.reset(new StructuralLayerData(mLayers.StructuralLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreStructuralLayer(std::unique_ptr<StructuralLayerData> structuralLayer)
{
    // Replace layer
    mLayers.StructuralLayer = std::move(structuralLayer);
}

void Model::SetElectricalLayer(ElectricalLayerData && electricalLayer)
{
    assert(electricalLayer.Buffer.Size == GetShipSize());

    // Update layer
    mLayers.ElectricalLayer.reset(new ElectricalLayerData(std::move(electricalLayer)));
}

void Model::RemoveElectricalLayer()
{
    // Remove layer
    mLayers.ElectricalLayer.reset();
}

std::unique_ptr<ElectricalLayerData> Model::CloneElectricalLayer() const
{
    std::unique_ptr<ElectricalLayerData> clonedLayer;

    if (mLayers.ElectricalLayer)
    {
        clonedLayer.reset(new ElectricalLayerData(mLayers.ElectricalLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreElectricalLayer(std::unique_ptr<ElectricalLayerData> electricalLayer)
{
    // Replace layer
    mLayers.ElectricalLayer = std::move(electricalLayer);
}

void Model::SetRopesLayer(RopesLayerData && ropesLayer)
{
    // Update layer
    mLayers.RopesLayer.reset(new RopesLayerData(std::move(ropesLayer)));
}

void Model::RemoveRopesLayer()
{
    // Remove layer
    mLayers.RopesLayer.reset();
}

std::unique_ptr<RopesLayerData> Model::CloneRopesLayer() const
{
    std::unique_ptr<RopesLayerData> clonedLayer;

    if (mLayers.RopesLayer)
    {
        clonedLayer.reset(new RopesLayerData(mLayers.RopesLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreRopesLayer(std::unique_ptr<RopesLayerData> ropesLayer)
{
    // Replace layer
    mLayers.RopesLayer = std::move(ropesLayer);
}

void Model::SetExteriorTextureLayer(TextureLayerData && exteriorTextureLayer)
{
    // Update layer
    mLayers.ExteriorTextureLayer.reset(new TextureLayerData(std::move(exteriorTextureLayer)));
}

void Model::RemoveExteriorTextureLayer()
{
    // Remove layer
    mLayers.ExteriorTextureLayer.reset();
}

std::unique_ptr<TextureLayerData> Model::CloneExteriorTextureLayer() const
{
    std::unique_ptr<TextureLayerData> clonedLayer;

    if (mLayers.ExteriorTextureLayer)
    {
        clonedLayer.reset(new TextureLayerData(mLayers.ExteriorTextureLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreExteriorTextureLayer(std::unique_ptr<TextureLayerData> exteriorTextureLayer)
{
    // Replace layer
    mLayers.ExteriorTextureLayer = std::move(exteriorTextureLayer);
}

void Model::SetInteriorTextureLayer(TextureLayerData && interiorTextureLayer)
{
    // Update layer
    mLayers.InteriorTextureLayer.reset(new TextureLayerData(std::move(interiorTextureLayer)));
}

void Model::RemoveInteriorTextureLayer()
{
    // Remove layer
    mLayers.InteriorTextureLayer.reset();
}

std::unique_ptr<TextureLayerData> Model::CloneInteriorTextureLayer() const
{
    std::unique_ptr<TextureLayerData> clonedLayer;

    if (mLayers.InteriorTextureLayer)
    {
        clonedLayer.reset(new TextureLayerData(mLayers.InteriorTextureLayer->Clone()));
    }

    return clonedLayer;
}

void Model::RestoreInteriorTextureLayer(std::unique_ptr<TextureLayerData> interiorTextureLayer)
{
    // Replace layer
    mLayers.InteriorTextureLayer = std::move(interiorTextureLayer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<StructuralLayerData> Model::MakeNewEmptyStructuralLayer(ShipSpaceSize const & shipSize)
{
    return std::make_unique<StructuralLayerData>(
        shipSize,
        StructuralElement(nullptr)); // No material
}

}