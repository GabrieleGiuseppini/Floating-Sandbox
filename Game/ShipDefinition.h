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

#include <optional>

struct ShipDefinition
{
    ShipLayers Layers;
    ShipMetadata Metadata;
    ShipPhysicsData PhysicsData;
    std::optional<ShipAutoTexturizationSettings> const AutoTexturizationSettings;

    ShipDefinition(
        ShipLayers && layers,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings)
        : Layers(std::move(layers))
        , Metadata(metadata)
        , PhysicsData(physicsData)
        , AutoTexturizationSettings(autoTexturizationSettings)
    {}
};
