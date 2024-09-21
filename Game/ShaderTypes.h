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
    BlastToolHalo,
    CloudsBasic,
    CloudsDetailed,
    CrossOfLight,
    FireExtinguisherSpray,
    FishesBasic,
    FishesDetailed,
    GenericMipMappedTexturesNdc,
    HeatBlasterFlameCool,
    HeatBlasterFlameHeat,
    LandFlatBasic,
    LandFlatDetailed,
    LandTextureBasic,
    LandTextureDetailed,
    LaserRay,
    Lightning,
    LineGuide,
    OceanDepthBasic,
    OceanDepthDetailedBackground,
    OceanDepthDetailedForeground,
    OceanFlatBasic,
    OceanFlatDetailedBackground,
    OceanFlatDetailedForeground,
    OceanTextureBasic,
    OceanTextureDetailedBackground,
    OceanTextureDetailedForeground,
    PhysicsProbePanel,
    PressureInjectionHalo,
    Rain,
    RectSelection,
    ShipCenters,
    ShipCircleHighlights,
    ShipElectricalElementHighlights,
    ShipElectricSparks,
    ShipExplosions,
    ShipFlamesBackground,
    ShipFlamesForeground,
    ShipFrontierEdges,
    ShipGenericMipMappedTextures,
    ShipJetEngineFlames,
    ShipNpcsTexture,
    ShipPointToPointArrows,
    ShipPointsColor,
    ShipPointsColorStress,
    ShipPointsColorHeatOverlay,
    ShipPointsColorHeatOverlayStress,
    ShipPointsColorIncandescence,
    ShipPointsColorIncandescenceStress,
    ShipRopes,
    ShipRopesStress,
    ShipRopesHeatOverlay,
    ShipRopesHeatOverlayStress,
    ShipRopesIncandescence,
    ShipRopesIncandescenceStress,
    ShipSparkles,
    ShipSpringsColor,
    ShipSpringsColorStress,
    ShipSpringsColorHeatOverlay,
    ShipSpringsColorHeatOverlayStress,
    ShipSpringsColorIncandescence,
    ShipSpringsColorIncandescenceStress,
    ShipSpringsDecay,
    ShipSpringsInternalPressure,
    ShipSpringsStrength,
    ShipSpringsTexture,
    ShipSpringsTextureStress,
    ShipSpringsTextureHeatOverlay,
    ShipSpringsTextureHeatOverlayStress,
    ShipSpringsTextureIncandescence,
    ShipSpringsTextureIncandescenceStress,
    ShipStressedSprings,
    ShipTrianglesColor,
    ShipTrianglesColorStress,
    ShipTrianglesColorHeatOverlay,
    ShipTrianglesColorHeatOverlayStress,
    ShipTrianglesColorIncandescence,
    ShipTrianglesColorIncandescenceStress,
    ShipTrianglesDecay,
    ShipTrianglesInternalPressure,
    ShipTrianglesStrength,
    ShipTrianglesTexture,
    ShipTrianglesTextureStress,
    ShipTrianglesTextureHeatOverlay,
    ShipTrianglesTextureHeatOverlayStress,
    ShipTrianglesTextureIncandescence,
    ShipTrianglesTextureIncandescenceStress,
    ShipVectors,
    Sky,
    Stars,
    Text,
    TextureNotifications,
    WindSphere,
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
    CrepuscularColor,
    EffectiveAmbientLightIntensity,
    EffectiveMoonlightColor,
    FlameProgress,
    FlatSkyColor,
    HeatShift,
    LampLightColor,
    LampToolAttributes,
    LandFlatColor,
    MatteColor,
    NoiseStrength,    
    OceanDepthColorStart,
    OceanDepthColorEnd,
    OceanDepthDarkeningRate,
    OceanFlatColor,
    OceanTransparency,
    OrthoMatrix,
    RainAngle,
    RainDensity,
    ShipDepthDarkeningSensitivity,
    StarTransparency,
    StressColorMap,
    SunRaysInclination,
    TextLighteningStrength,
    TextureLighteningStrength,
    TextureScaling,
    Time,
    ViewportSize,
    WaterColor,
    WaterContrast,
    WaterLevelThreshold,
    WidthNdc,

    // Textures
    SharedTexture,                          // 0, for programs that don't use a dedicated unit and hence will keep binding different textures (font, ship texture, stressed ship texture, cloud shadows)
    CloudsAtlasTexture,                     // 1
    ExplosionsAtlasTexture,                 // 2
    FishesAtlasTexture,                     // 3
    GenericLinearTexturesAtlasTexture,      // 4
    GenericMipMappedTexturesAtlasTexture,   // 5
    LandTexture,                            // 6
    NoiseTexture,                           // 7
    OceanTexture,                           // 8
    NpcAtlasTexture,                        // 9

    _FirstTexture = SharedTexture,
    _LastTexture = NpcAtlasTexture
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

    Sky = 0,

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

    WorldBorder = 0,

    //
    // Ship
    //

    ShipPointAttributeGroup1 = 0,   // Position, TextureCoordinates
    ShipPointAttributeGroup2 = 1,   // Light, Water, PlaneId, Decay
    ShipPointColor = 2,
    ShipPointTemperature = 3,
    ShipPointStress = 4,
    ShipPointAuxiliaryData = 5,
    ShipPointFrontierColor = 6,

    NpcTextureStaticAttributeGroup1 = 0,
    NpcTextureAttributeGroup1 = 0,
    NpcTextureAttributeGroup2 = 1,
    NpcTextureAttributeGroup3 = 2,

    ElectricSpark1 = 0,

    Explosion1 = 0,
    Explosion2 = 1,
    Explosion3 = 2,

    Sparkle1 = 0,
    Sparkle2 = 1,

    ShipGenericMipMappedTexture1 = 0,
    ShipGenericMipMappedTexture2 = 1,
    ShipGenericMipMappedTexture3 = 2,

    Flame1 = 0,
    Flame2 = 1,

    JetEngineFlame1 = 0,
    JetEngineFlame2 = 1,

    Highlight1 = 0,
    Highlight2 = 1,
    Highlight3 = 2,

    VectorArrow = 0,

    Center1 = 0,
    Center2 = 1,

    PointToPointArrow1 = 0,
    PointToPointArrow2 = 1,

    //
    // Notifications
    //

    Text1 = 0,
    Text2 = 1,

    TextureNotification1 = 0,
    TextureNotification2 = 1,

    PhysicsProbePanel1 = 0,
    PhysicsProbePanel2 = 1,

    FireExtinguisherSpray = 0,

    HeatBlasterFlame = 0,

    BlastToolHalo1 = 0,
    BlastToolHalo2 = 1,

    PressureInjectionHalo1 = 0,
    PressureInjectionHalo2 = 1,

    WindSphere1 = 0,
    WindSphere2 = 1,

    LaserRay1 = 0,
    LaserRay2 = 1,

    RectSelection1 = 0,
    RectSelection2 = 1,
    RectSelection3 = 2,

    LineGuide1 = 0,

    //
    // Global
    //

    GenericMipMappedTextureNdc1 = 0,
    GenericMipMappedTextureNdc2 = 1
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
