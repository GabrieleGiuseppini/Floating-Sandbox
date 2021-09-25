/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/LayerBuffers.h>
#include <Game/Materials.h>

#include <GameCore/ImageData.h>

#include <array>
#include <cassert>
#include <memory>

namespace ShipBuilder {

/*
 * This is an anemic object model containing the whole representation of the ship being built.
 *
 * - All data, (almost) no operations (anemic), fully exported
 * - Modified by ModelController
 * - Knows nothing about view
 * - IsDirty storage
 *      - But not maintenance: that is done by Controller, via ModelController methods
 */
class Model
{
public:

    Model(ShipSpaceSize const & shipSize);

    std::unique_ptr<StructuralLayerBuffer> CloneStructuralLayerBuffer() const;
    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    std::unique_ptr<ElectricalLayerBuffer> CloneElectricalLayerBuffer() const;
    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();

    ShipSpaceSize const & GetShipSize() const
    {
        return mShipSize;
    }

    bool HasLayer(LayerType layer) const
    {
        return mLayerPresenceMap[static_cast<size_t>(layer)];
    }

    bool HasExtraLayers() const
    {
        for (size_t iLayer = 0; iLayer < LayerCount; ++iLayer)
        {
            if (mLayerPresenceMap[iLayer])
                return true;
        }

        return false;
    }

    bool GetIsDirty() const
    {
        return mIsDirty;
    }

    bool GetIsDirty(LayerType layer) const
    {
        return mLayerDirtinessMap[static_cast<size_t>(layer)];
    }

    void SetIsDirty(LayerType layer)
    {
        mLayerDirtinessMap[static_cast<size_t>(layer)] = true;
        mIsDirty = true;
    }

    void ClearIsDirty()
    {
        mLayerDirtinessMap.fill(false);
        mIsDirty = false;
    }

    void ClearIsDirty(LayerType layer)
    {
        mLayerDirtinessMap[static_cast<size_t>(layer)] = false;
        RecalculateGlobalIsDirty();
    }

    StructuralLayerBuffer & GetStructuralLayerBuffer()
    {
        assert(mStructuralLayerBuffer);
        return *mStructuralLayerBuffer;
    }

    RgbaImageData const & GetStructuralRenderColorTexture() const
    {
        assert(mStructuralRenderColorTexture);
        return *mStructuralRenderColorTexture;
    }

    RgbaImageData & GetStructuralRenderColorTexture()
    {
        assert(mStructuralRenderColorTexture);
        return *mStructuralRenderColorTexture;
    }

private:

    void RecalculateGlobalIsDirty();

    void MakeNewStructuralLayer(ShipSpaceSize const & size);

private:

    ShipSpaceSize mShipSize;

    //
    // Structural Layer
    //

    std::unique_ptr<StructuralLayerBuffer> mStructuralLayerBuffer;

    // Derived buffers
    std::unique_ptr<RgbaImageData> mStructuralRenderColorTexture;

    //
    // Misc state
    //

    // Layer presence map (cached)
    std::array<bool, LayerCount> mLayerPresenceMap;

    // Dirty map and global flag
    std::array<bool, LayerCount> mLayerDirtinessMap;
    bool mIsDirty; // Cached version of above
};

}