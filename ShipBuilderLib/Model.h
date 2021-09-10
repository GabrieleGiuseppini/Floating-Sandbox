/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <array>

namespace ShipBuilder {

/*
 * This is an anemic object model containing the whole representation of the ship being built.
 */
class Model
{
public:

    Model(WorkSpaceSize const & workSpaceSize);

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();

    WorkSpaceSize const & GetWorkSpaceSize() const
    {
        return mWorkSpaceSize;
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

private:

    void RecalculateGlobalIsDirty();

    void ClearIsDirty();

private:

    WorkSpaceSize mWorkSpaceSize;

    // Layer presence map (cached)
    std::array<bool, LayerCount> mLayerPresenceMap;

    // Dirty map and global flag
    std::array<bool, LayerCount> mLayerDirtinessMap;
    bool mIsDirty; // Cached version of above
};

}