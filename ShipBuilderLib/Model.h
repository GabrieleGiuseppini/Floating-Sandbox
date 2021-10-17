/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/LayerBuffers.h>
#include <Game/Materials.h>
#include <Game/ShipDefinition.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <string>

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

    struct DirtyState
    {
        std::array<bool, LayerCount> IsLayerDirtyMap;
        bool IsMetadataDirty;

        bool GlobalIsDirty;

        DirtyState()
            : IsLayerDirtyMap()
            , IsMetadataDirty(false)
            , GlobalIsDirty(false)
        {
            IsLayerDirtyMap.fill(false);
        }

        DirtyState & operator=(DirtyState const & other) = default;

        void RecalculateGlobalIsDirty()
        {
            GlobalIsDirty = std::find(
                IsLayerDirtyMap.cbegin(),
                IsLayerDirtyMap.cend(),
                true) != IsLayerDirtyMap.cend()
                ? true
                : false;
        }
    };

public:

    explicit Model(
        ShipSpaceSize const & shipSize,
        std::string const & shipName);

    explicit Model(ShipDefinition && shipDefinition);

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);
    std::unique_ptr<StructuralLayerBuffer> CloneStructuralLayerBuffer() const;

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();
    std::unique_ptr<ElectricalLayerBuffer> CloneElectricalLayerBuffer() const;

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();
    std::unique_ptr<TextureLayerBuffer> CloneTextureLayerBuffer() const;

    ShipSpaceSize const & GetShipSize() const
    {
        return mShipSize;
    }

    ShipMetadata const & GetShipMetadata() const
    {
        return mShipMetadata;
    }

    void SetShipMetadata(ShipMetadata const & shipMetadata)
    {
        mShipMetadata = shipMetadata;
    }

    bool HasLayer(LayerType layer) const
    {
        return mLayerPresenceMap[static_cast<size_t>(layer)];
    }

    bool HasExtraLayers() const
    {
        for (size_t iLayer = 1; iLayer < LayerCount; ++iLayer)
        {
            if (mLayerPresenceMap[iLayer])
                return true;
        }

        return false;
    }

    DirtyState const & GetDirtyState() const
    {
        return mDirtyState;
    }

    void SetDirtyState(DirtyState const & dirtyState)
    {
        mDirtyState = dirtyState;
    }

    bool GetIsDirty() const
    {
        return mDirtyState.GlobalIsDirty;
    }

    bool GetIsDirty(LayerType layer) const
    {
        return mDirtyState.IsLayerDirtyMap[static_cast<size_t>(layer)];
    }

    void SetIsDirty(LayerType layer)
    {
        mDirtyState.IsLayerDirtyMap[static_cast<size_t>(layer)] = true;
        mDirtyState.GlobalIsDirty = true;
    }

    void ClearIsDirty()
    {
        mDirtyState = DirtyState();
    }

    void ClearIsDirty(LayerType layer)
    {
        mDirtyState.IsLayerDirtyMap[static_cast<size_t>(layer)] = false;
        mDirtyState.RecalculateGlobalIsDirty();
    }

    StructuralLayerBuffer & GetStructuralLayerBuffer()
    {
        assert(mStructuralLayerBuffer);
        return *mStructuralLayerBuffer;
    }

    TextureLayerBuffer const & GetTextureLayerBuffer() const
    {
        assert(mTextureLayerBuffer);
        return *mTextureLayerBuffer;
    }

private:

    static std::unique_ptr<StructuralLayerBuffer> MakeNewStructuralLayer(ShipSpaceSize const & shipSize);

private:

    ShipSpaceSize mShipSize;

    ShipMetadata mShipMetadata;

    //
    // Layers
    //

    std::unique_ptr<StructuralLayerBuffer> mStructuralLayerBuffer;

    std::unique_ptr<TextureLayerBuffer> mTextureLayerBuffer;

    //
    // Misc state
    //

    // Layer presence map (cached)
    std::array<bool, LayerCount> mLayerPresenceMap;

    // Dirty state
    DirtyState mDirtyState;
};

}