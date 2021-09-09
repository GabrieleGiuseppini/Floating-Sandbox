/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Model.h"

namespace ShipBuilder {

Model::Model(WorkSpaceSize const & workSpaceSize)
    : mWorkSpaceSize(workSpaceSize)
{
    // TODO: initialize structural layer

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::NewStructuralLayer()
{
    // TODO
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::SetStructuralLayer(/*TODO*/)
{
    // TODO
}

void Model::NewElectricalLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
}

void Model::SetElectricalLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveElectricalLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
}

void Model::NewRopesLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = true;
}

void Model::SetRopesLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveRopesLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = false;
}

void Model::NewTextureLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = true;
}

void Model::SetTextureLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveTextureLayer()
{
    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
}


}