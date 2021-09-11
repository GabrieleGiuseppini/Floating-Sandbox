/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Model.h"

#include <algorithm>

namespace ShipBuilder {

Model::Model(WorkSpaceSize const & workSpaceSize)
    : mWorkSpaceSize(workSpaceSize)
{
    // Initialize structural layer
    MakeNewEmptyStructuralLayer(mWorkSpaceSize);

    // Initialize presence map
    mLayerPresenceMap.fill(false);
    mLayerPresenceMap[static_cast<size_t>(LayerType::Structural)] = true;

    // Initialize dirtiness
    ClearIsDirty();
}

void Model::NewStructuralLayer()
{
    MakeNewEmptyStructuralLayer(mWorkSpaceSize);

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

void Model::MakeNewEmptyStructuralLayer(WorkSpaceSize const & size)
{
    mStructuralMaterialMatrix = std::make_unique<Buffer2D<StructuralMaterial const *>>(
        size.width,
        size.height,
        nullptr);

    std::unique_ptr<rgbaColor[]> renderColorData = std::make_unique<rgbaColor[]>(size.width * size.height);
    std::fill(
        renderColorData.get(),
        renderColorData.get() + (size.width * size.height),
        rgbaColor(0, 0, 0, 0));
    mStructuralRenderColorTexture = std::make_unique<RgbaImageData>(
        size.width,
        size.height,
        std::move(renderColorData));
}
}