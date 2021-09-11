/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/Materials.h>

#include <GameCore/Buffer2D.h>
#include <GameCore/ImageData.h>

#include <array>
#include <cassert>
#include <memory>

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

    RgbaImageData const & GetStructuralRenderColorTexture() const
    {
        assert(!!mStructuralRenderColorTexture);
        return *mStructuralRenderColorTexture;
    }

private:

    void RecalculateGlobalIsDirty();

    void ClearIsDirty();

    void MakeNewEmptyStructuralLayer(WorkSpaceSize const & size);

private:

    WorkSpaceSize mWorkSpaceSize;

    //
    // Structural Layer
    //

    std::unique_ptr<Buffer2D<StructuralMaterial const *>> mStructuralMaterialMatrix;
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