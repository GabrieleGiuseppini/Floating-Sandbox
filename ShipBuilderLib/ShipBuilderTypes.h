/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Buffer2D.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <cmath>
#include <cstdint>
#include <sstream>

namespace ShipBuilder {

enum class ToolType : std::uint32_t
{
    StructuralPencil,
    StructuralEraser,
    StructuralFlood,
    ElectricalPencil,
    ElectricalEraser,
    ElectricalFlood,

    _Last = ElectricalFlood
};

size_t constexpr LayerCount = static_cast<size_t>(LayerType::Ropes) + 1;

enum class MaterialPlaneType
{
    Foreground,
    Background
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Model
////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TMaterial>
using MaterialBuffer = Buffer2D<TMaterial const *, ShipSpaceTag>;

}