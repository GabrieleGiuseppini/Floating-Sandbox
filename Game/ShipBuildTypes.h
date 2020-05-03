/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

/*
 * Definitions of data structures related to ship building.
 *
 * These structures are shared between the ship builder and the ship texturizer.
 */

using ShipBuildPointMatrix = std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]>;


