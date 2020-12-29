/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <cstdint>
#include <string>

namespace Render {

//
// Shaders
//

enum class ProgramType
{
    AABBs = 0,
    AMBombPreImplosion,
    Clouds,
    CrossOfLight,
    FireExtinguisherSpray,
    Fishes,
    HeatBlasterFlameCool,
    HeatBlasterFlameHeat,
    LandFlat,
    LandTexture,
    Lightning,
    OceanDepthBasic,
    OceanDepthDetailed,
    OceanFlatBasic,
    OceanFlatDetailed,
    OceanTextureBasic,
    OceanTextureDetailed,
    Rain,
    ShipCircleHighlights,
    ShipElectricalElementHighlights,
    ShipExplosions,
    ShipFlamesBackground,
    ShipFlamesForeground,
    ShipFrontierEdges,
    ShipGenericMipMappedTextures,
    ShipPointsColor,
    ShipPointsColorWithTemperature,
    ShipRopes,
    ShipRopesWithTemperature,
    ShipSparkles,
    ShipSpringsColor,
    ShipSpringsColorWithTemperature,
    ShipSpringsTexture,
    ShipSpringsTextureWithTemperature,
    ShipStressedSprings,
    ShipTrianglesColor,
    ShipTrianglesColorWithTemperature,
    ShipTrianglesDecay,
    ShipTrianglesTexture,
    ShipTrianglesTextureWithTemperature,
    ShipVectors,
    Stars,
    TextNotifications,
    TextureNotifications,
    WorldBorder,

    _Last = WorldBorder
};

ProgramType ShaderFilenameToProgramType(std::string const & str);

std::string ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType : uint8_t
{
    AtlasTile1Dx = 0,
    AtlasTile1LeftBottomTextureCoordinates,
    AtlasTile1Size,
    EffectiveAmbientLightIntensity,
    FlameSpeed,
    FlameWindRotationAngle,
    HeatOverlayTransparency,
    LampLightColor,
    LandFlatColor,
    MatteColor,
    OceanTransparency,
    OceanDarkeningRate,
    OceanDepthColorStart,
    OceanDepthColorEnd,
    OceanFlatColor,
    OrthoMatrix,
    RainDensity,
    StarTransparency,
    TextLighteningStrength,
    TextureLighteningStrength,
    TextureScaling,
    Time,
    ViewportSize,
    WaterColor,
    WaterContrast,
    WaterLevelThreshold,

    // Textures
    SharedTexture,                          // 0, for programs that don't use a dedicated unit and hence will keep binding different textures
    CloudsAtlasTexture,                     // 1
    ExplosionsAtlasTexture,                 // 2
    FishesAtlasTexture,                     // 3
    GenericLinearTexturesAtlasTexture,      // 4
    GenericMipMappedTexturesAtlasTexture,   // 5
    LandTexture,                            // 6
    NoiseTexture1,                          // 7
    NoiseTexture2,                          // 8
    OceanTexture,                           // 9

    _FirstTexture = SharedTexture,
    _LastTexture = OceanTexture
};

ProgramParameterType StrToProgramParameterType(std::string const & str);

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

/*
 * This enum serves merely to associate a vertex attribute index to each vertex attribute name.
 */
enum class VertexAttributeType : GLuint
{
    //
    // World
    //

    Star = 0,

    Lightning1 = 0,
    Lightning2 = 1,

    Cloud1 = 0,
    Cloud2 = 1,

    Land = 0,

    OceanBasic = 0,

    OceanDetailed1 = 0,
    OceanDetailed2 = 1,

    Fish1 = 0,
    Fish2 = 1,
    Fish3 = 2,
    Fish4 = 3,

    AMBombPreImplosion1 = 0,
    AMBombPreImplosion2 = 1,

    CrossOfLight1 = 0,
    CrossOfLight2 = 1,

    AABB1 = 0,
    AABB2 = 1,

    Rain = 0,

    FireExtinguisherSpray = 0,

    HeatBlasterFlame = 0,

    WorldBorder = 0,

    //
    // Ship
    //

    ShipPointAttributeGroup1 = 0,   // Position, TextureCoordinates
    ShipPointAttributeGroup2 = 1,   // Light, Water, PlaneId, Decay
    ShipPointColor = 2,
    ShipPointTemperature = 3,
    ShipPointFrontierColor = 4,

    Explosion1 = 0,
    Explosion2 = 1,
    Explosion3 = 2,

    Sparkle1 = 0,
    Sparkle2 = 1,

    GenericMipMappedTexture1 = 0,
    GenericMipMappedTexture2 = 1,
    GenericMipMappedTexture3 = 2,

    Flame1 = 0,
    Flame2 = 1,

    Highlight1 = 0,
    Highlight2 = 1,
    Highlight3 = 2,

    VectorArrow = 0,

    //
    // Notifications
    //

    TextNotification1 = 0,
    TextNotification2 = 1,

    TextureNotification1 = 0,
    TextureNotification2 = 1
};

VertexAttributeType StrToVertexAttributeType(std::string const & str);

struct ShaderManagerTraits
{
    using ProgramType = Render::ProgramType;
    using ProgramParameterType = Render::ProgramParameterType;
    using VertexAttributeType = Render::VertexAttributeType;

    static constexpr auto ShaderFilenameToProgramType = Render::ShaderFilenameToProgramType;
    static constexpr auto ProgramTypeToStr = Render::ProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = Render::StrToProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = Render::ProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = Render::StrToVertexAttributeType;
};

}
