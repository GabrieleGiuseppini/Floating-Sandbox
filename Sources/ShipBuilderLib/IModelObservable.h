/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-03-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>
#include <Simulation/ShipAutoTexturizationSettings.h>
#include <Simulation/ShipMetadata.h>
#include <Simulation/ShipPhysicsData.h>

#include <Core/GameTypes.h>

#include <optional>

namespace ShipBuilder {

/*
 * Read-only model interface.
 */
struct IModelObservable
{
    virtual ShipSpaceSize const & GetShipSize() const = 0;

    virtual bool HasLayer(LayerType layer) const = 0;

    virtual bool IsDirty() const = 0;

    virtual bool IsLayerDirty(LayerType layer) const = 0;

    virtual ShipMetadata const & GetShipMetadata() const = 0;

    virtual ShipPhysicsData const & GetShipPhysicsData() const = 0;

    virtual std::optional<ShipAutoTexturizationSettings> const & GetShipAutoTexturizationSettings() const = 0;

    virtual ModelMacroProperties GetModelMacroProperties() const = 0;

    // Layers

    virtual StructuralLayerData const & GetStructuralLayer() const = 0;
};

}