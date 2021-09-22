/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LayerBuffers.h"
#include "ShipMetadata.h"
#include "ShipPhysicsData.h"

#include <memory>
#include <optional>

struct ShipMaterialization
{
    ShipSpaceSize Size;

    StructuralLayerBuffer StructuralLayer;
    std::unique_ptr<ElectricalLayerBuffer> ElectricalLayer;
    std::unique_ptr<RopesLayerBuffer> RopesLayer;
    std::unique_ptr<TextureLayerBuffer> TextureLayer;

    std::optional<ShipAutoTexturizationSettings> const AutoTexturizationSettings;
    ShipMetadata Metadata;
    ShipPhysicsData PhysicsData;

    ShipMaterialization(
        ShipSpaceSize const & size,
        StructuralLayerBuffer && structuralLayer,
        std::unique_ptr<ElectricalLayerBuffer> && electricalLayer,
        std::unique_ptr<RopesLayerBuffer> && ropesLayer,
        std::unique_ptr<TextureLayerBuffer> && textureLayer,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings)
        : Size(size)
        , StructuralLayer(std::move(structuralLayer))
        , ElectricalLayer(std::move(electricalLayer))
        , RopesLayer(std::move(ropesLayer))
        , TextureLayer(std::move(textureLayer))
        , AutoTexturizationSettings(std::move(autoTexturizationSettings))
        , Metadata(std::move(metadata))
        , PhysicsData(std::move(physicsData))
    {}
};
