/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Model.h"

#include <Game/MaterialDatabase.h>

#include <algorithm>

namespace ShipBuilder {

Model::Model(ShipSpaceSize const & shipSize)
    : mShipSize(shipSize)
    , mStructuralLayerBuffer()
{
    // Initialize structural layer
    MakeNewStructuralLayer(mShipSize);

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;

    // Initialize dirtiness
    mLayerDirtinessMap.fill(false);
    mIsDirty = false;
}

std::unique_ptr<StructuralLayerBuffer> Model::CloneStructuralLayerBuffer() const
{
    assert(mStructuralLayerBuffer);
    return mStructuralLayerBuffer->MakeCopy();
}

void Model::NewStructuralLayer()
{
    MakeNewStructuralLayer(mShipSize);

    // Update presence map
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
}

void Model::SetStructuralLayer(/*TODO*/)
{
    // TODO
}

std::unique_ptr<ElectricalLayerBuffer> Model::CloneElectricalLayerBuffer() const
{
    // TODO
    return nullptr;
    ////assert(mElectricalLayerBuffer);
    ////return mElectricalLayerBuffer->MakeCopy();
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

void Model::RecalculateGlobalIsDirty()
{
    mIsDirty = std::find(
        mLayerDirtinessMap.cbegin(),
        mLayerDirtinessMap.cend(),
        true) != mLayerDirtinessMap.cend()
        ? true
        : false;
}

void Model::MakeNewStructuralLayer(ShipSpaceSize const & size)
{
    mStructuralLayerBuffer = std::make_unique<StructuralLayerBuffer>(
        size,
        StructuralElement(nullptr)); // No material

    mStructuralRenderColorTexture = std::make_unique<RgbaImageData>(
        size.width,
        size.height,
        rgbaColor(MaterialDatabase::EmptyMaterialColorKey, 255));

    // TODOTEST: initializing with checker pattern
    for (int y = 0; y < size.height; ++y)
    {
        for (int x = 0; x < size.width; ++x)
        {
            rgbaColor color;
            if (x == 0 || x == size.width - 1 || y == 0 || y == size.height - 1)
                color = rgbaColor(0, 0, 255, 255);
            else
                color = rgbaColor(
                    ((x + y) % 2) ? 255 : 0,
                    ((x + y) % 2) ? 0 : 255,
                    0,
                    255);

            (*mStructuralRenderColorTexture)[ImageCoordinates(x, y)] = color;
        }
    }
}

}