/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "CustomMaterialsPod.h"
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
    CustomMaterialsPod CustomMaterials; // To maintain lifetime of ship's custom materials

    ShipDefinition(
        ShipLayers && layers,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        CustomMaterialsPod && customMaterials)
        : Layers(std::move(layers))
        , Metadata(metadata)
        , PhysicsData(physicsData)
        , AutoTexturizationSettings(autoTexturizationSettings)
        , CustomMaterials(std::move(customMaterials))
    {}
};
