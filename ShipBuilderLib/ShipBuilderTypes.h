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
    StructuralLine,
    StructuralFlood,
    ElectricalPencil,
    ElectricalEraser,
    ElectricalLine,
    RopePencil,
    RopeEraser,

    _Last = RopeEraser
};

size_t constexpr LayerCount = static_cast<size_t>(LayerType::Texture) + 1;

enum class MaterialPlaneType
{
    Foreground,
    Background
};

enum class StructuralLayerVisualizationModeType
{
    ParticleMode,
    AutoTexturizationMode,
    TextureMode
};

enum class ElectricalLayerVisualizationModeType
{
    ParticleMode
    // FUTURE: CircuitMode
};

enum class RopesLayerVisualizationModeType
{
    LinesMode
};

enum class TextureLayerVisualizationModeType
{
    MatteMode
};

}