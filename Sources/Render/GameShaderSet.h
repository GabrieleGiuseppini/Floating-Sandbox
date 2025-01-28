/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <string>

//
// Shaders
//

enum class ProgramType
{
    AABBs = 0,
    AMBombPreImplosion,
    CloudsBasic,
    CloudsDetailed,
    CrossOfLight,
    FishesBasic,
    FishesDetailed,
    GenericMipMappedTexturesNdc,
    InteractiveToolDashedLines,
    LandFlatBasic,
    LandFlatDetailed,
    LandTextureBasic,
    LandTextureDetailed,
    LaserRay,
    Lightning,
    MultiNotification,
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
    ShipNpcsQuadFlat,
    ShipNpcsQuadWithRoles,
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
    WorldBorder,

    _Last = WorldBorder
};

ProgramType _ShaderNameToProgramType(std::string const & str);

std::string _ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType : std::uint8_t
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
    KaosAdjustment,
    LampLightColor,
    LampToolAttributes,
    LandFlatColor,
    MatteColor,
    NoiseStrength,
    NpcQuadFlatColor,
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
    Zoom,

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

ProgramParameterType _StrToProgramParameterType(std::string const & str);

std::string _ProgramParameterTypeToStr(ProgramParameterType programParameter);

/*
 * This enum serves merely to associate a vertex attribute index to each vertex attribute name.
 */
enum class VertexAttributeType : std::uint32_t
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
    Cloud3 = 2,

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

    NpcAttributeGroup1 = 0,
    NpcAttributeGroup2 = 1,
    NpcAttributeGroup3 = 2,
    NpcAttributeGroup4 = 3,

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

    MultiNotification1 = 0,
    MultiNotification2 = 1,
    MultiNotification3 = 2,

    LaserRay1 = 0,
    LaserRay2 = 1,

    RectSelection1 = 0,
    RectSelection2 = 1,
    RectSelection3 = 2,

    InteractiveToolDashedLine1 = 0,

    //
    // Global
    //

    GenericMipMappedTextureNdc1 = 0,
    GenericMipMappedTextureNdc2 = 1
};

VertexAttributeType _StrToVertexAttributeType(std::string const & str);

struct GameShaderSet
{
    using ProgramType = ProgramType;
    using ProgramParameterType = ProgramParameterType;
    using VertexAttributeType = VertexAttributeType;

    static inline std::string ShaderSetName = "Game";

    static constexpr auto ShaderNameToProgramType = _ShaderNameToProgramType;
    static constexpr auto ProgramTypeToStr = _ProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = _StrToProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = _ProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = _StrToVertexAttributeType;
};
