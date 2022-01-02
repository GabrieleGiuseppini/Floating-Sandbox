/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Layers.h"
#include "ShipAutoTexturizationSettings.h"
#include "ShipMetadata.h"
#include "ShipPhysicsData.h"

#include <memory>
#include <optional>

struct ShipDefinition
{
    ShipSpaceSize Size;

    StructuralLayerData StructuralLayer;
    std::unique_ptr<ElectricalLayerData> ElectricalLayer;
    std::unique_ptr<RopesLayerData> RopesLayer;
    std::unique_ptr<TextureLayerData> TextureLayer;

    ShipMetadata Metadata;
    ShipPhysicsData PhysicsData;
    std::optional<ShipAutoTexturizationSettings> const AutoTexturizationSettings;

    ShipDefinition(
        ShipSpaceSize const & size,
        StructuralLayerData && structuralLayer,
        std::unique_ptr<ElectricalLayerData> && electricalLayer,
        std::unique_ptr<RopesLayerData> && ropesLayer,
        std::unique_ptr<TextureLayerData> && textureLayer,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings)
        : Size(size)
        , StructuralLayer(std::move(structuralLayer))
        , ElectricalLayer(std::move(electricalLayer))
        , RopesLayer(std::move(ropesLayer))
        , TextureLayer(std::move(textureLayer))
        , Metadata(metadata)
        , PhysicsData(physicsData)
        , AutoTexturizationSettings(autoTexturizationSettings)
    {}
};
