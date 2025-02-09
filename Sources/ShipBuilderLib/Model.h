/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>
#include <Simulation/Materials.h>
#include <Simulation/ShipDefinition.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

    explicit Model(
        ShipSpaceSize const & shipSize,
        std::string const & shipName);

    explicit Model(ShipDefinition && shipDefinition);

    ShipDefinition MakeShipDefinition() const;

    ShipSpaceSize const & GetShipSize() const
    {
        return mLayers.Size;
    }

    void SetShipSize(ShipSpaceSize const & size)
    {
        mLayers.Size = size;
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

    void SetElectricalPanel(ElectricalPanel && electricalPanel)
    {
        assert(mLayers.ElectricalLayer);
        mLayers.ElectricalLayer->Panel = std::move(electricalPanel);
    }

    bool HasLayer(LayerType layer) const
    {
        switch (layer)
        {
            case LayerType::Structural:
                return (bool)mLayers.StructuralLayer;

            case LayerType::Electrical:
                return (bool)mLayers.ElectricalLayer;

            case LayerType::Ropes:
                return (bool)mLayers.RopesLayer;

            case LayerType::ExteriorTexture:
                return (bool)mLayers.ExteriorTextureLayer;

            case LayerType::InteriorTexture:
                return (bool)mLayers.InteriorTextureLayer;
        }

        assert(false);
        return false;
    }

    std::vector<LayerType> GetAllPresentLayers() const
    {
        std::vector<LayerType> layers;

        for (size_t l = 0; l < LayerCount; ++l)
        {
            if (HasLayer(static_cast<LayerType>(l)))
            {
                layers.push_back(static_cast<LayerType>(l));
            }
        }

        return layers;
    }

    ModelDirtyState const & GetDirtyState() const
    {
        return mDirtyState;
    }

    void SetDirtyState(ModelDirtyState const & dirtyState)
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
        mDirtyState = ModelDirtyState();
    }

    void ClearIsDirty(LayerType layer)
    {
        mDirtyState.IsLayerDirtyMap[static_cast<size_t>(layer)] = false;
        mDirtyState.RecalculateGlobalIsDirty();
    }

    template<LayerType TLayer>
    typename LayerTypeTraits<TLayer>::layer_data_type CloneExistingLayer() const
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            return mLayers.StructuralLayer->Clone();
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            assert(mLayers.ElectricalLayer);
            return mLayers.ElectricalLayer->Clone();
        }
        else if constexpr (TLayer == LayerType::Ropes)
        {
            assert(mLayers.RopesLayer);
            return mLayers.RopesLayer->Clone();
        }
        else if constexpr (TLayer == LayerType::ExteriorTexture)
        {
            assert(mLayers.ExteriorTextureLayer);
            return mLayers.ExteriorTextureLayer->Clone();
        }
        else
        {
            static_assert(TLayer == LayerType::InteriorTexture);

            assert(mLayers.InteriorTextureLayer);
            return mLayers.InteriorTextureLayer->Clone();
        }
    }

    StructuralLayerData const & GetStructuralLayer() const
    {
        assert(mLayers.StructuralLayer);
        return *mLayers.StructuralLayer;
    }

    StructuralLayerData & GetStructuralLayer()
    {
        assert(mLayers.StructuralLayer);
        return *mLayers.StructuralLayer;
    }

    void SetStructuralLayer(StructuralLayerData && structuralLayer);

    std::unique_ptr<StructuralLayerData> CloneStructuralLayer() const;
    void RestoreStructuralLayer(std::unique_ptr<StructuralLayerData> structuralLayer);

    ElectricalLayerData const & GetElectricalLayer() const
    {
        assert(mLayers.ElectricalLayer);
        return *mLayers.ElectricalLayer;
    }

    ElectricalLayerData & GetElectricalLayer()
    {
        assert(mLayers.ElectricalLayer);
        return *mLayers.ElectricalLayer;
    }

    void SetElectricalLayer(ElectricalLayerData && electricalLayer);
    void RemoveElectricalLayer();

    std::unique_ptr<ElectricalLayerData> CloneElectricalLayer() const;
    void RestoreElectricalLayer(std::unique_ptr<ElectricalLayerData> electricalLayer);

    RopesLayerData const & GetRopesLayer() const
    {
        assert(mLayers.RopesLayer);
        return *mLayers.RopesLayer;
    }

    RopesLayerData & GetRopesLayer()
    {
        assert(mLayers.RopesLayer);
        return *mLayers.RopesLayer;
    }

    void SetRopesLayer(RopesLayerData && ropesLayer);
    void RemoveRopesLayer();

    std::unique_ptr<RopesLayerData> CloneRopesLayer() const;
    void RestoreRopesLayer(std::unique_ptr<RopesLayerData> ropesLayer);

    TextureLayerData const & GetExteriorTextureLayer() const
    {
        assert(mLayers.ExteriorTextureLayer);
        return *mLayers.ExteriorTextureLayer;
    }

    TextureLayerData & GetExteriorTextureLayer()
    {
        assert(mLayers.ExteriorTextureLayer);
        return *mLayers.ExteriorTextureLayer;
    }

    void SetExteriorTextureLayer(TextureLayerData && exteriorTextureLayer);
    void RemoveExteriorTextureLayer();

    std::unique_ptr<TextureLayerData> CloneExteriorTextureLayer() const;
    void RestoreExteriorTextureLayer(std::unique_ptr<TextureLayerData> exteriorTextureLayer);

    TextureLayerData const & GetInteriorTextureLayer() const
    {
        assert(mLayers.InteriorTextureLayer);
        return *mLayers.InteriorTextureLayer;
    }

    TextureLayerData & GetInteriorTextureLayer()
    {
        assert(mLayers.InteriorTextureLayer);
        return *mLayers.InteriorTextureLayer;
    }

    void SetInteriorTextureLayer(TextureLayerData && interiorTextureLayer);
    void RemoveInteriorTextureLayer();

    std::unique_ptr<TextureLayerData> CloneInteriorTextureLayer() const;
    void RestoreInteriorTextureLayer(std::unique_ptr<TextureLayerData> interiorTextureLayer);

private:

    static std::unique_ptr<StructuralLayerData> MakeNewEmptyStructuralLayer(ShipSpaceSize const & shipSize);

private:

    ShipMetadata mShipMetadata;
    ShipPhysicsData mShipPhysicsData;
    std::optional<ShipAutoTexturizationSettings> mShipAutoTexturizationSettings;

    ShipLayers mLayers;

    //
    // Misc state
    //

    // Dirty state
    ModelDirtyState mDirtyState;
};

}