/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Model.h"

#include <algorithm>

namespace ShipBuilder {

Model::Model(ShipSpaceSize const & shipSize)
    : mShipSize(shipSize)
{
    // Initialize structural layer
    MakeNewEmptyStructuralLayer(mShipSize);

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;

    // Initialize dirtiness
    ClearIsDirty();
}

void Model::NewStructuralLayer()
{
    MakeNewEmptyStructuralLayer(mShipSize);

    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Structural)] = true;
    mIsDirty = true;
}

void Model::SetStructuralLayer(/*TODO*/)
{
    // TODO
}

void Model::NewElectricalLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = true;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Electrical)] = true;
    mIsDirty = true;
}

void Model::SetElectricalLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveElectricalLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Electrical)] = false;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Electrical)] = true;
    mIsDirty = true;
}

void Model::NewRopesLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = true;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Ropes)] = true;
    mIsDirty = true;
}

void Model::SetRopesLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveRopesLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Ropes)] = false;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Ropes)] = true;
    mIsDirty = true;
}

void Model::NewTextureLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = true;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Texture)] = true;
    mIsDirty = true;
}

void Model::SetTextureLayer(/*TODO*/)
{
    // TODO
}

void Model::RemoveTextureLayer()
{
    // TODO

    mLayerPresenceMap[static_cast<size_t>(LayerType::Texture)] = false;
    mLayerDirtinessMap[static_cast<size_t>(LayerType::Texture)] = true;
    mIsDirty = true;
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

// TODO: to be invoked by oursevels when we're saved
void Model::ClearIsDirty()
{
    mLayerDirtinessMap.fill(false);
    mIsDirty = false;
}

void Model::MakeNewEmptyStructuralLayer(ShipSpaceSize const & size)
{
    // Material

    mStructuralMaterialMatrix = std::make_unique<MaterialBuffer<StructuralMaterial>>(
        size.width,
        size.height,
        nullptr); // No material

    // Render color

    mStructuralRenderColorTexture = std::make_unique<RgbaImageData>(
        size.width,
        size.height,
        rgbaColor::zero()); // Fully transparent

    // TODOTEST: initialize with checker pattern
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