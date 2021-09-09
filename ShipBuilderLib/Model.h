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
        for (size_t iLayer = static_cast<size_t>(LayerType::Structural) + 1; iLayer <= static_cast<uint32_t>(LayerType::_Last); ++iLayer)
        {
            if (mLayerPresenceMap[iLayer])
                return true;
        }

        return false;
    }

private:

    WorkSpaceSize mWorkSpaceSize;

    // Layer presence map (cached)
    std::array<bool, static_cast<size_t>(LayerType::_Last) + 1> mLayerPresenceMap;
};

}