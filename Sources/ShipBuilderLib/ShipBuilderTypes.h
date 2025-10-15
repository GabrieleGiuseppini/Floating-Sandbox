/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <sstream>

namespace ShipBuilder {

enum class ToolType : std::uint32_t
{
    StructuralPencil,
    StructuralEraser,
    StructuralLine,
    StructuralFlood,
    StructuralSampler,
    StructuralMeasuringTape,
    ElectricalPencil,
    ElectricalEraser,
    ElectricalLine,
    ElectricalSampler,
    RopePencil,
    RopeEraser,
    RopeSampler,
    ExteriorTextureMagicWand,
    ExteriorTextureEraser,
    ExteriorTextureTranslate,
    InteriorTextureMagicWand,
    InteriorTextureEraser,
    InteriorTextureTranslate,
    StructuralSelection,
    ElectricalSelection,
    RopeSelection,
    ExteriorTextureSelection,
    InteriorTextureSelection,
    StructuralPaste,
    ElectricalPaste,
    RopePaste,
    ExteriorTexturePaste,
    InteriorTexturePaste,
    StructuralRectangle,
    StructureTracer,

    _Last = StructureTracer
};

enum class ToolClass
{
    Selection,
    Paste,
    Other
};

size_t constexpr LayerCount = static_cast<size_t>(LayerType::InteriorTexture) + 1;

enum class MaterialPlaneType
{
    Foreground,
    Background
};

enum class FillMode
{
    FillWithForeground,
    FillWithBackground,
    NoFill
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
    ExteriorTextureLayer,
    InteriorTextureLayer
};

size_t constexpr VisualizationCount = static_cast<size_t>(VisualizationType::InteriorTextureLayer) + 1;

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

        case VisualizationType::ExteriorTextureLayer:
        {
            return LayerType::ExteriorTexture;
        }

        case VisualizationType::InteriorTextureLayer:
        {
            return LayerType::InteriorTexture;
        }
    }

    assert(false);
    return LayerType::Structural;
}

enum class GameVisualizationModeType
{
    None,
    AutoTexturizationMode,
    ExteriorTextureMode
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

enum class InteriorTextureLayerVisualizationModeType
{
    None,
    MatteMode
};

enum class ExteriorTextureLayerVisualizationModeType
{
    None,
    MatteMode
};

struct ModelMacroProperties
{
    size_t MassParticleCount;
    float TotalMass;
    std::optional<vec2f> CenterOfMass;

    ModelMacroProperties(
        size_t massParticleCount,
        float totalMass,
        std::optional<vec2f> centerOfMass)
        : MassParticleCount(massParticleCount)
        , TotalMass(totalMass)
        , CenterOfMass(centerOfMass)
    {}
};

struct SampledInformation
{
    std::string MaterialName;
    std::optional<ElectricalElementInstanceIndex> InstanceIndex;

    SampledInformation(
        std::string materialName,
        std::optional<ElectricalElementInstanceIndex> instanceIndex)
        : MaterialName(materialName)
        , InstanceIndex(instanceIndex)
    {}

    bool operator==(SampledInformation const & other) const
    {
        return MaterialName == other.MaterialName
            && InstanceIndex == other.InstanceIndex;
    }
};

struct ModelDirtyState
{
    std::array<bool, LayerCount> IsLayerDirtyMap;
    bool IsMetadataDirty;
    bool IsPhysicsDataDirty;
    bool IsAutoTexturizationSettingsDirty;

    bool GlobalIsDirty;

    ModelDirtyState()
        : IsLayerDirtyMap()
        , IsMetadataDirty(false)
        , IsPhysicsDataDirty(false)
        , IsAutoTexturizationSettingsDirty(false)
        , GlobalIsDirty(false)
    {
        IsLayerDirtyMap.fill(false);
    }

    ModelDirtyState & operator=(ModelDirtyState const & other) = default;

    void RecalculateGlobalIsDirty()
    {
        GlobalIsDirty = std::find(
            IsLayerDirtyMap.cbegin(),
            IsLayerDirtyMap.cend(),
            true) != IsLayerDirtyMap.cend()
            ? true
            : false;

        GlobalIsDirty |=
            IsMetadataDirty
            | IsPhysicsDataDirty
            | IsAutoTexturizationSettingsDirty;
    }
};

}