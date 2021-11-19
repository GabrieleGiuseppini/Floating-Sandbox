/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ShipDefinition.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>
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
        bool IsPhysicsDataDirty;
        bool IsAutoTexturizationSettingsDirty;

        bool GlobalIsDirty;

        DirtyState()
            : IsLayerDirtyMap()
            , IsMetadataDirty(false)
            , IsPhysicsDataDirty(false)
            , IsAutoTexturizationSettingsDirty(false)
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

            GlobalIsDirty |=
                IsMetadataDirty
                | IsPhysicsDataDirty
                | IsAutoTexturizationSettingsDirty;
        }
    };

public:

    explicit Model(
        ShipSpaceSize const & shipSize,
        std::string const & shipName);

    explicit Model(ShipDefinition && shipDefinition);

    ShipDefinition MakeShipDefinition() const;

    ShipSpaceSize const & GetShipSize() const
    {
        return mShipSize;
    }

    ShipMetadata const & GetShipMetadata() const
    {
        return mShipMetadata;
    }

    ShipMetadata & GetShipMetadata()
    {
        return mShipMetadata;
    }

    void SetShipMetadata(ShipMetadata && shipMetadata)
    {
        mShipMetadata = std::move(shipMetadata);

        mDirtyState.IsMetadataDirty = true;
        mDirtyState.GlobalIsDirty = true;
    }

    ShipPhysicsData const & GetShipPhysicsData() const
    {
        return mShipPhysicsData;
    }

    void SetShipPhysicsData(ShipPhysicsData && shipPhysicsData)
    {
        mShipPhysicsData = std::move(shipPhysicsData);

        mDirtyState.IsPhysicsDataDirty = true;
        mDirtyState.GlobalIsDirty = true;
    }

    std::optional<ShipAutoTexturizationSettings> const & GetShipAutoTexturizationSettings() const
    {
        return mShipAutoTexturizationSettings;
    }

    void SetShipAutoTexturizationSettings(std::optional<ShipAutoTexturizationSettings> && shipAutoTexturizationSettings)
    {
        mShipAutoTexturizationSettings = std::move(shipAutoTexturizationSettings);

        mDirtyState.IsAutoTexturizationSettingsDirty = true;
        mDirtyState.GlobalIsDirty = true;
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
    
    template<LayerType TLayer>
    typename LayerTypeTraits<TLayer>::layer_data_type CloneLayer() const
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            assert(mStructuralLayer);
            return mStructuralLayer->Clone();
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            assert(mElectricalLayer);
            return mElectricalLayer->Clone();
        }
        else if constexpr (TLayer == LayerType::Ropes)
        {
            assert(mRopesLayer);
            return mRopesLayer->Clone();
        }
        else
        {
            static_assert(TLayer == LayerType::Texture);

            assert(mTextureLayer);
            return mTextureLayer->Clone();
        }
    }

    StructuralLayerData const & GetStructuralLayer() const
    {
        assert(mStructuralLayer);
        return *mStructuralLayer;
    }

    StructuralLayerData & GetStructuralLayer()
    {
        assert(mStructuralLayer);
        return *mStructuralLayer;
    }

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    ElectricalLayerData const & GetElectricalLayer() const
    {
        assert(mElectricalLayer);
        return *mElectricalLayer;
    }

    ElectricalLayerData & GetElectricalLayer()
    {
        assert(mElectricalLayer);
        return *mElectricalLayer;
    }

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    RopesLayerData const & GetRopesLayer() const
    {
        assert(mRopesLayer);
        return *mRopesLayer;
    }

    RopesLayerData & GetRopesLayer()
    {
        assert(mRopesLayer);
        return *mRopesLayer;
    }

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

    TextureLayerData const & GetTextureLayer() const
    {
        assert(mTextureLayer);
        return *mTextureLayer;
    }

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();

private:

    static std::unique_ptr<StructuralLayerData> MakeNewEmptyStructuralLayer(ShipSpaceSize const & shipSize);

    static std::unique_ptr<ElectricalLayerData> MakeNewEmptyElectricalLayer(ShipSpaceSize const & shipSize);

    static std::unique_ptr<RopesLayerData> MakeNewEmptyRopesLayer();

private:

    ShipSpaceSize mShipSize;

    ShipMetadata mShipMetadata;
    ShipPhysicsData mShipPhysicsData;
    std::optional<ShipAutoTexturizationSettings> mShipAutoTexturizationSettings;

    //
    // Layers
    //

    std::unique_ptr<StructuralLayerData> mStructuralLayer;

    std::unique_ptr<ElectricalLayerData> mElectricalLayer;

    std::unique_ptr<RopesLayerData> mRopesLayer;

    std::unique_ptr<TextureLayerData> mTextureLayer;

    //
    // Misc state
    //

    // Layer presence map (cached)
    std::array<bool, LayerCount> mLayerPresenceMap;

    // Dirty state
    DirtyState mDirtyState;
};

}