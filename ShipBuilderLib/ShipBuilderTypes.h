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

//
// Visualization
//

enum class VisualizationType : std::uint32_t
{
    Game = 0,
    StructuralLayer,
    ElectricalLayer,
    RopesLayer,
    TextureLayer
};

size_t constexpr VisualizationCount = static_cast<size_t>(VisualizationType::TextureLayer) + 1;

inline LayerType VisualizationToLayer(VisualizationType visualization)
{
    switch (visualization)
    {
        case VisualizationType::Game:
        case VisualizationType::StructuralLayer:
        {
            return LayerType::Structural;
        }

        case VisualizationType::ElectricalLayer:
        {
            return LayerType::Electrical;
        }

        case VisualizationType::RopesLayer:
        {
            return LayerType::Ropes;
        }

        case VisualizationType::TextureLayer:
        {
            return LayerType::Texture;
        }
    }

    assert(false);
    return LayerType::Structural;
}

enum class GameVisualizationModeType
{
    None,
    AutoTexturizationMode,
    TextureMode
};

enum class StructuralLayerVisualizationModeType
{
    None,
    MeshMode,
    PixelMode
};

enum class ElectricalLayerVisualizationModeType
{
    None,
    PixelMode
    // FUTURE: CircuitMode
};

enum class RopesLayerVisualizationModeType
{
    None,
    LinesMode
};

enum class TextureLayerVisualizationModeType
{
    None,
    MatteMode
};

}