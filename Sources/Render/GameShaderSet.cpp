/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameShaderSet.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>

namespace _detail {

GameShaderProgramType ShaderNameToGameShaderProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "aabbs")
        return GameShaderProgramType::AABBs;
    else if (lstr == "am_bomb_preimplosion")
        return GameShaderProgramType::AMBombPreImplosion;
    else if (lstr == "clouds_basic")
        return GameShaderProgramType::CloudsBasic;
    else if (lstr == "clouds_detailed")
        return GameShaderProgramType::CloudsDetailed;
    else if (lstr == "cross_of_light")
        return GameShaderProgramType::CrossOfLight;
    else if (lstr == "fishes_basic")
        return GameShaderProgramType::FishesBasic;
    else if (lstr == "fishes_detailed")
        return GameShaderProgramType::FishesDetailed;
    else if (lstr == "generic_mipmapped_textures_ndc")
        return GameShaderProgramType::GenericMipMappedTexturesNdc;
    else if (lstr == "interactive_tool_dashed_lines")
        return GameShaderProgramType::InteractiveToolDashedLines;
    else if (lstr == "land_flat_basic")
        return GameShaderProgramType::LandFlatBasic;
    else if (lstr == "land_flat_detailed")
        return GameShaderProgramType::LandFlatDetailed;
    else if (lstr == "land_texture_basic")
        return GameShaderProgramType::LandTextureBasic;
    else if (lstr == "land_texture_detailed")
        return GameShaderProgramType::LandTextureDetailed;
    else if (lstr == "laser_ray")
        return GameShaderProgramType::LaserRay;
    else if (lstr == "lightning")
        return GameShaderProgramType::Lightning;
    else if (lstr == "multi_notification")
        return GameShaderProgramType::MultiNotification;
    else if (lstr == "ocean_depth_basic")
        return GameShaderProgramType::OceanDepthBasic;
    else if (lstr == "ocean_depth_detailed_background")
        return GameShaderProgramType::OceanDepthDetailedBackground;
    else if (lstr == "ocean_depth_detailed_foreground")
        return GameShaderProgramType::OceanDepthDetailedForeground;
    else if (lstr == "ocean_flat_basic")
        return GameShaderProgramType::OceanFlatBasic;
    else if (lstr == "ocean_flat_detailed_background")
        return GameShaderProgramType::OceanFlatDetailedBackground;
    else if (lstr == "ocean_flat_detailed_foreground")
        return GameShaderProgramType::OceanFlatDetailedForeground;
    else if (lstr == "ocean_texture_basic")
        return GameShaderProgramType::OceanTextureBasic;
    else if (lstr == "ocean_texture_detailed_background")
        return GameShaderProgramType::OceanTextureDetailedBackground;
    else if (lstr == "ocean_texture_detailed_foreground")
        return GameShaderProgramType::OceanTextureDetailedForeground;
    else if (lstr == "physics_probe_panel")
        return GameShaderProgramType::PhysicsProbePanel;
    else if (lstr == "rain")
        return GameShaderProgramType::Rain;
    else if (lstr == "rect_selection")
        return GameShaderProgramType::RectSelection;
    else if (lstr == "ship_centers")
        return GameShaderProgramType::ShipCenters;
    else if (lstr == "ship_circle_highlights")
        return GameShaderProgramType::ShipCircleHighlights;
    else if (lstr == "ship_electrical_element_highlights")
        return GameShaderProgramType::ShipElectricalElementHighlights;
    else if (lstr == "ship_electric_sparks")
        return GameShaderProgramType::ShipElectricSparks;
    else if (lstr == "ship_explosions")
        return GameShaderProgramType::ShipExplosions;
    else if (lstr == "ship_flames_background")
        return GameShaderProgramType::ShipFlamesBackground;
    else if (lstr == "ship_flames_foreground")
        return GameShaderProgramType::ShipFlamesForeground;
    else if (lstr == "ship_frontier_edges")
        return GameShaderProgramType::ShipFrontierEdges;
    else if (lstr == "ship_generic_mipmapped_textures")
        return GameShaderProgramType::ShipGenericMipMappedTextures;
    else if (lstr == "ship_jet_engine_flames")
        return GameShaderProgramType::ShipJetEngineFlames;
    else if (lstr == "ship_npcs_quad_flat")
        return GameShaderProgramType::ShipNpcsQuadFlat;
    else if (lstr == "ship_npcs_quad_with_roles")
        return GameShaderProgramType::ShipNpcsQuadWithRoles;
    else if (lstr == "ship_npcs_texture")
        return GameShaderProgramType::ShipNpcsTexture;
    else if (lstr == "ship_point_to_point_arrows")
        return GameShaderProgramType::ShipPointToPointArrows;
    else if (lstr == "ship_points_color")
        return GameShaderProgramType::ShipPointsColor;
    else if (lstr == "ship_points_color_stress")
        return GameShaderProgramType::ShipPointsColorStress;
    else if (lstr == "ship_points_color_heatoverlay")
        return GameShaderProgramType::ShipPointsColorHeatOverlay;
    else if (lstr == "ship_points_color_heatoverlay_stress")
        return GameShaderProgramType::ShipPointsColorHeatOverlayStress;
    else if (lstr == "ship_points_color_incandescence")
        return GameShaderProgramType::ShipPointsColorIncandescence;
    else if (lstr == "ship_points_color_incandescence_stress")
        return GameShaderProgramType::ShipPointsColorIncandescenceStress;
    else if (lstr == "ship_ropes")
        return GameShaderProgramType::ShipRopes;
    else if (lstr == "ship_ropes_stress")
        return GameShaderProgramType::ShipRopesStress;
    else if (lstr == "ship_ropes_heatoverlay")
        return GameShaderProgramType::ShipRopesHeatOverlay;
    else if (lstr == "ship_ropes_heatoverlay_stress")
        return GameShaderProgramType::ShipRopesHeatOverlayStress;
    else if (lstr == "ship_ropes_incandescence")
        return GameShaderProgramType::ShipRopesIncandescence;
    else if (lstr == "ship_ropes_incandescence_stress")
        return GameShaderProgramType::ShipRopesIncandescenceStress;
    else if (lstr == "ship_sparkles")
        return GameShaderProgramType::ShipSparkles;
    else if (lstr == "ship_springs_color")
        return GameShaderProgramType::ShipSpringsColor;
    else if (lstr == "ship_springs_color_stress")
        return GameShaderProgramType::ShipSpringsColorStress;
    else if (lstr == "ship_springs_color_heatoverlay")
        return GameShaderProgramType::ShipSpringsColorHeatOverlay;
    else if (lstr == "ship_springs_color_heatoverlay_stress")
        return GameShaderProgramType::ShipSpringsColorHeatOverlayStress;
    else if (lstr == "ship_springs_color_incandescence")
        return GameShaderProgramType::ShipSpringsColorIncandescence;
    else if (lstr == "ship_springs_color_incandescence_stress")
        return GameShaderProgramType::ShipSpringsColorIncandescenceStress;
    else if (lstr == "ship_springs_decay")
        return GameShaderProgramType::ShipSpringsDecay;
    else if (lstr == "ship_springs_internal_pressure")
        return GameShaderProgramType::ShipSpringsInternalPressure;
    else if (lstr == "ship_springs_strength")
        return GameShaderProgramType::ShipSpringsStrength;
    else if (lstr == "ship_springs_texture")
        return GameShaderProgramType::ShipSpringsTexture;
    else if (lstr == "ship_springs_texture_stress")
        return GameShaderProgramType::ShipSpringsTextureStress;
    else if (lstr == "ship_springs_texture_heatoverlay")
        return GameShaderProgramType::ShipSpringsTextureHeatOverlay;
    else if (lstr == "ship_springs_texture_heatoverlay_stress")
        return GameShaderProgramType::ShipSpringsTextureHeatOverlayStress;
    else if (lstr == "ship_springs_texture_incandescence")
        return GameShaderProgramType::ShipSpringsTextureIncandescence;
    else if (lstr == "ship_springs_texture_incandescence_stress")
        return GameShaderProgramType::ShipSpringsTextureIncandescenceStress;
    else if (lstr == "ship_stressed_springs")
        return GameShaderProgramType::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return GameShaderProgramType::ShipTrianglesColor;
    else if (lstr == "ship_triangles_color_stress")
        return GameShaderProgramType::ShipTrianglesColorStress;
    else if (lstr == "ship_triangles_color_heatoverlay")
        return GameShaderProgramType::ShipTrianglesColorHeatOverlay;
    else if (lstr == "ship_triangles_color_heatoverlay_stress")
        return GameShaderProgramType::ShipTrianglesColorHeatOverlayStress;
    else if (lstr == "ship_triangles_color_incandescence")
        return GameShaderProgramType::ShipTrianglesColorIncandescence;
    else if (lstr == "ship_triangles_color_incandescence_stress")
        return GameShaderProgramType::ShipTrianglesColorIncandescenceStress;
    else if (lstr == "ship_triangles_decay")
        return GameShaderProgramType::ShipTrianglesDecay;
    else if (lstr == "ship_triangles_internal_pressure")
        return GameShaderProgramType::ShipTrianglesInternalPressure;
    else if (lstr == "ship_triangles_strength")
        return GameShaderProgramType::ShipTrianglesStrength;
    else if (lstr == "ship_triangles_texture")
        return GameShaderProgramType::ShipTrianglesTexture;
    else if (lstr == "ship_triangles_texture_stress")
        return GameShaderProgramType::ShipTrianglesTextureStress;
    else if (lstr == "ship_triangles_texture_heatoverlay")
        return GameShaderProgramType::ShipTrianglesTextureHeatOverlay;
    else if (lstr == "ship_triangles_texture_heatoverlay_stress")
        return GameShaderProgramType::ShipTrianglesTextureHeatOverlayStress;
    else if (lstr == "ship_triangles_texture_incandescence")
        return GameShaderProgramType::ShipTrianglesTextureIncandescence;
    else if (lstr == "ship_triangles_texture_incandescence_stress")
        return GameShaderProgramType::ShipTrianglesTextureIncandescenceStress;
    else if (lstr == "ship_vectors")
        return GameShaderProgramType::ShipVectors;
    else if (lstr == "sky")
        return GameShaderProgramType::Sky;
    else if (lstr == "stars")
        return GameShaderProgramType::Stars;
    else if (lstr == "text")
        return GameShaderProgramType::Text;
    else if (lstr == "texture_notifications")
        return GameShaderProgramType::TextureNotifications;
    else if (lstr == "world_border")
        return GameShaderProgramType::WorldBorder;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string GameShaderProgramTypeToStr(GameShaderProgramType program)
{
    switch (program)
    {
        case GameShaderProgramType::AABBs:
            return "AABBs";
        case GameShaderProgramType::AMBombPreImplosion:
            return "AMBombPreImplosion";
        case GameShaderProgramType::CloudsBasic:
            return "CloudsBasic";
        case GameShaderProgramType::CloudsDetailed:
            return "CloudsDetailed";
        case GameShaderProgramType::CrossOfLight:
            return "CrossOfLight";
        case GameShaderProgramType::FishesBasic:
            return "FishesBasic";
        case GameShaderProgramType::FishesDetailed:
            return "FishesDetailed";
        case GameShaderProgramType::GenericMipMappedTexturesNdc:
            return "GenericMipMappedTexturesNdc";
        case GameShaderProgramType::InteractiveToolDashedLines:
            return "InteractiveToolDashedLines";
        case GameShaderProgramType::LandFlatBasic:
            return "LandFlatBasic";
        case GameShaderProgramType::LandFlatDetailed:
            return "LandFlatDetailed";
        case GameShaderProgramType::LandTextureBasic:
            return "LandTextureBasic";
        case GameShaderProgramType::LandTextureDetailed:
            return "LandTextureDetailed";
        case GameShaderProgramType::LaserRay:
            return "LaserRay";
        case GameShaderProgramType::Lightning:
            return "Lightning";
        case GameShaderProgramType::MultiNotification:
            return "MultiNotification";
        case GameShaderProgramType::OceanDepthBasic:
            return "OceanDepthBasic";
        case GameShaderProgramType::OceanDepthDetailedBackground:
            return "OceanDepthDetailedBackground";
        case GameShaderProgramType::OceanDepthDetailedForeground:
            return "OceanDepthDetailedForeground";
        case GameShaderProgramType::OceanFlatBasic:
            return "OceanFlatBasic";
        case GameShaderProgramType::OceanFlatDetailedBackground:
            return "OceanFlatDetailedBackground";
        case GameShaderProgramType::OceanFlatDetailedForeground:
            return "OceanFlatDetailedForeground";
        case GameShaderProgramType::OceanTextureBasic:
            return "OceanTextureBasic";
        case GameShaderProgramType::OceanTextureDetailedBackground:
            return "OceanTextureDetailedBackground";
        case GameShaderProgramType::OceanTextureDetailedForeground:
            return "OceanTextureDetailedForeground";
        case GameShaderProgramType::PhysicsProbePanel:
            return "PhysicsProbePanel";
        case GameShaderProgramType::Rain:
            return "Rain";
        case GameShaderProgramType::RectSelection:
            return "RectSelection";
        case GameShaderProgramType::ShipCenters:
            return "ShipCenters";
        case GameShaderProgramType::ShipCircleHighlights:
            return "ShipCircleHighlights";
        case GameShaderProgramType::ShipElectricalElementHighlights:
            return "ShipElectricalElementHighlights";
        case GameShaderProgramType::ShipElectricSparks:
            return "ShipElectricSparks";
        case GameShaderProgramType::ShipExplosions:
            return "ShipExplosions";
        case GameShaderProgramType::ShipFlamesBackground:
            return "ShipFlamesBackground";
        case GameShaderProgramType::ShipFlamesForeground:
            return "ShipFlamesForeground";
        case GameShaderProgramType::ShipFrontierEdges:
            return "ShipFrontierEdges";
        case GameShaderProgramType::ShipGenericMipMappedTextures:
            return "ShipGenericMipMappedTextures";
        case GameShaderProgramType::ShipJetEngineFlames:
            return "ShipJetEngineFlames";
        case GameShaderProgramType::ShipNpcsQuadFlat:
            return "ShipNpcsQuadFlat";
        case GameShaderProgramType::ShipNpcsQuadWithRoles:
            return "ShipNpcsQuadWithRoles";
        case GameShaderProgramType::ShipNpcsTexture:
            return "ShipNpcsTexture";
        case GameShaderProgramType::ShipPointToPointArrows:
            return "ShipPointToPointArrows";
        case GameShaderProgramType::ShipPointsColor:
            return "ShipPointsColor";
        case GameShaderProgramType::ShipPointsColorStress:
            return "ShipPointsColorStress";
        case GameShaderProgramType::ShipPointsColorHeatOverlay:
            return "ShipPointsColorHeatOverlay";
        case GameShaderProgramType::ShipPointsColorHeatOverlayStress:
            return "ShipPointsColorHeatOverlayStress";
        case GameShaderProgramType::ShipPointsColorIncandescence:
            return "ShipPointsColorIncandescence";
        case GameShaderProgramType::ShipPointsColorIncandescenceStress:
            return "ShipPointsColorIncandescenceStress";
        case GameShaderProgramType::ShipRopes:
            return "ShipRopes";
        case GameShaderProgramType::ShipRopesStress:
            return "ShipRopesStress";
        case GameShaderProgramType::ShipRopesHeatOverlay:
            return "ShipRopesHeatOverlay";
        case GameShaderProgramType::ShipRopesHeatOverlayStress:
            return "ShipRopesHeatOverlayStress";
        case GameShaderProgramType::ShipRopesIncandescence:
            return "ShipRopesIncandescence";
        case GameShaderProgramType::ShipRopesIncandescenceStress:
            return "ShipRopesIncandescenceStress";
        case GameShaderProgramType::ShipSparkles:
            return "ShipSparkles";
        case GameShaderProgramType::ShipSpringsColor:
            return "ShipSpringsColor";
        case GameShaderProgramType::ShipSpringsColorStress:
            return "ShipSpringsColorStress";
        case GameShaderProgramType::ShipSpringsColorHeatOverlay:
            return "ShipSpringsColorHeatOverlay";
        case GameShaderProgramType::ShipSpringsColorHeatOverlayStress:
            return "ShipSpringsColorHeatOverlayStress";
        case GameShaderProgramType::ShipSpringsColorIncandescence:
            return "ShipSpringsColorIncandescence";
        case GameShaderProgramType::ShipSpringsColorIncandescenceStress:
            return "ShipSpringsColorIncandescenceStress";
        case GameShaderProgramType::ShipSpringsDecay:
            return "ShipSpringsDecay";
        case GameShaderProgramType::ShipSpringsInternalPressure:
            return "ShipSpringsInternalPressure";
        case GameShaderProgramType::ShipSpringsStrength:
            return "ShipSpringsStrength";
        case GameShaderProgramType::ShipSpringsTexture:
            return "ShipSpringsTexture";
        case GameShaderProgramType::ShipSpringsTextureStress:
            return "ShipSpringsTextureStress";
        case GameShaderProgramType::ShipSpringsTextureHeatOverlay:
            return "ShipSpringsTextureHeatOverlay";
        case GameShaderProgramType::ShipSpringsTextureHeatOverlayStress:
            return "ShipSpringsTextureHeatOverlayStress";
        case GameShaderProgramType::ShipSpringsTextureIncandescence:
            return "ShipSpringsTextureIncandescence";
        case GameShaderProgramType::ShipSpringsTextureIncandescenceStress:
            return "ShipSpringsTextureIncandescenceStress";
        case GameShaderProgramType::ShipStressedSprings:
            return "ShipStressedSprings";
        case GameShaderProgramType::ShipTrianglesColor:
            return "ShipTrianglesColor";
        case GameShaderProgramType::ShipTrianglesColorStress:
            return "ShipTrianglesColorStress";
        case GameShaderProgramType::ShipTrianglesColorHeatOverlay:
            return "ShipTrianglesColorHeatOverlay";
        case GameShaderProgramType::ShipTrianglesColorHeatOverlayStress:
            return "ShipTrianglesColorHeatOverlayStress";
        case GameShaderProgramType::ShipTrianglesColorIncandescence:
            return "ShipTrianglesColorIncandescence";
        case GameShaderProgramType::ShipTrianglesColorIncandescenceStress:
            return "ShipTrianglesColorIncandescenceStress";
        case GameShaderProgramType::ShipTrianglesDecay:
            return "ShipTrianglesDecay";
        case GameShaderProgramType::ShipTrianglesInternalPressure:
            return "ShipTrianglesInternalPressure";
        case GameShaderProgramType::ShipTrianglesStrength:
            return "ShipTrianglesStrength";
        case GameShaderProgramType::ShipTrianglesTexture:
            return "ShipTrianglesTexture";
        case GameShaderProgramType::ShipTrianglesTextureStress:
            return "ShipTrianglesTextureStress";
        case GameShaderProgramType::ShipTrianglesTextureHeatOverlay:
            return "ShipTrianglesTextureHeatOverlay";
        case GameShaderProgramType::ShipTrianglesTextureHeatOverlayStress:
            return "ShipTrianglesTextureHeatOverlayStress";
        case GameShaderProgramType::ShipTrianglesTextureIncandescence:
            return "ShipTrianglesTextureIncandescence";
        case GameShaderProgramType::ShipTrianglesTextureIncandescenceStress:
            return "ShipTrianglesTextureIncandescenceStress";
        case GameShaderProgramType::ShipVectors:
            return "ShipVectors";
        case GameShaderProgramType::Sky:
            return "Sky";
        case GameShaderProgramType::Stars:
            return "Stars";
        case GameShaderProgramType::Text:
            return "Text";
        case GameShaderProgramType::TextureNotifications:
            return "TextureNotifications";
        case GameShaderProgramType::WorldBorder:
            return "WorldBorder";
    }

    assert(false);
    throw GameException("Unsupported GameShaderProgramType");
}

GameShaderProgramParameterType StrToGameShaderProgramParameterType(std::string const & str)
{
    if (str == "AtlasTile1Dx")
        return GameShaderProgramParameterType::AtlasTile1Dx;
    else if (str == "AtlasTile1LeftBottomTextureCoordinates")
        return GameShaderProgramParameterType::AtlasTile1LeftBottomTextureCoordinates;
    else if (str == "AtlasTile1Size")
        return GameShaderProgramParameterType::AtlasTile1Size;
    else if (str == "CrepuscularColor")
        return GameShaderProgramParameterType::CrepuscularColor;
    else if (str == "EffectiveAmbientLightIntensity")
        return GameShaderProgramParameterType::EffectiveAmbientLightIntensity;
    else if (str == "EffectiveMoonlightColor")
        return GameShaderProgramParameterType::EffectiveMoonlightColor;
    else if (str == "FlameProgress")
        return GameShaderProgramParameterType::FlameProgress;
    else if (str == "FlatSkyColor")
        return GameShaderProgramParameterType::FlatSkyColor;
    else if (str == "HeatShift")
        return GameShaderProgramParameterType::HeatShift;
    else if (str == "KaosAdjustment")
        return GameShaderProgramParameterType::KaosAdjustment;
    else if (str == "LampLightColor")
        return GameShaderProgramParameterType::LampLightColor;
    else if (str == "LampToolAttributes")
        return GameShaderProgramParameterType::LampToolAttributes;
    else if (str == "LandFlatColor")
        return GameShaderProgramParameterType::LandFlatColor;
    else if (str == "MatteColor")
        return GameShaderProgramParameterType::MatteColor;
    else if (str == "NoiseStrength")
        return GameShaderProgramParameterType::NoiseStrength;
    else if (str == "NpcQuadFlatColor")
        return GameShaderProgramParameterType::NpcQuadFlatColor;
    else if (str == "OceanTransparency")
        return GameShaderProgramParameterType::OceanTransparency;
    else if (str == "OceanDepthColorStart")
        return GameShaderProgramParameterType::OceanDepthColorStart;
    else if (str == "OceanDepthColorEnd")
        return GameShaderProgramParameterType::OceanDepthColorEnd;
    else if (str == "OceanDepthDarkeningRate")
        return GameShaderProgramParameterType::OceanDepthDarkeningRate;
    else if (str == "OceanFlatColor")
        return GameShaderProgramParameterType::OceanFlatColor;
    else if (str == "OrthoMatrix")
        return GameShaderProgramParameterType::OrthoMatrix;
    else if (str == "RainAngle")
        return GameShaderProgramParameterType::RainAngle;
    else if (str == "RainDensity")
        return GameShaderProgramParameterType::RainDensity;
    else if (str == "ShipDepthDarkeningSensitivity")
        return GameShaderProgramParameterType::ShipDepthDarkeningSensitivity;
    else if (str == "StarTransparency")
        return GameShaderProgramParameterType::StarTransparency;
    else if (str == "StressColorMap")
        return GameShaderProgramParameterType::StressColorMap;
    else if (str == "SunRaysInclination")
        return GameShaderProgramParameterType::SunRaysInclination;
    else if (str == "TextLighteningStrength")
        return GameShaderProgramParameterType::TextLighteningStrength;
    else if (str == "TextureLighteningStrength")
        return GameShaderProgramParameterType::TextureLighteningStrength;
    else if (str == "TextureScaling")
        return GameShaderProgramParameterType::TextureScaling;
    else if (str == "Time")
        return GameShaderProgramParameterType::Time;
    else if (str == "ViewportSize")
        return GameShaderProgramParameterType::ViewportSize;
    else if (str == "WaterColor")
        return GameShaderProgramParameterType::WaterColor;
    else if (str == "WaterContrast")
        return GameShaderProgramParameterType::WaterContrast;
    else if (str == "WaterLevelThreshold")
        return GameShaderProgramParameterType::WaterLevelThreshold;
    else if (str == "WidthNdc")
        return GameShaderProgramParameterType::WidthNdc;
    else if (str == "Zoom")
        return GameShaderProgramParameterType::Zoom;
    // Textures
    else if (str == "SharedTexture")
        return GameShaderProgramParameterType::SharedTexture;
    else if (str == "CloudsAtlasTexture")
        return GameShaderProgramParameterType::CloudsAtlasTexture;
    else if (str == "ExplosionsAtlasTexture")
        return GameShaderProgramParameterType::ExplosionsAtlasTexture;
    else if (str == "FishesAtlasTexture")
        return GameShaderProgramParameterType::FishesAtlasTexture;
    else if (str == "GenericLinearTexturesAtlasTexture")
        return GameShaderProgramParameterType::GenericLinearTexturesAtlasTexture;
    else if (str == "GenericMipMappedTexturesAtlasTexture")
        return GameShaderProgramParameterType::GenericMipMappedTexturesAtlasTexture;
    else if (str == "LandTexture")
        return GameShaderProgramParameterType::LandTexture;
    else if (str == "NoiseTexture")
        return GameShaderProgramParameterType::NoiseTexture;
    else if (str == "NpcAtlasTexture")
        return GameShaderProgramParameterType::NpcAtlasTexture;
    else if (str == "OceanTexture")
        return GameShaderProgramParameterType::OceanTexture;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string GameShaderProgramParameterTypeToStr(GameShaderProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case GameShaderProgramParameterType::AtlasTile1Dx:
            return "AtlasTile1Dx";
        case GameShaderProgramParameterType::AtlasTile1LeftBottomTextureCoordinates:
            return "AtlasTile1LeftBottomTextureCoordinates";
        case GameShaderProgramParameterType::AtlasTile1Size:
            return "AtlasTile1Size";
        case GameShaderProgramParameterType::CrepuscularColor:
            return "CrepuscularColor";
        case GameShaderProgramParameterType::EffectiveAmbientLightIntensity:
            return "EffectiveAmbientLightIntensity";
        case GameShaderProgramParameterType::EffectiveMoonlightColor:
            return "EffectiveMoonlightColor";
        case GameShaderProgramParameterType::FlameProgress:
            return "FlameProgress";
        case GameShaderProgramParameterType::FlatSkyColor:
            return "FlatSkyColor";
        case GameShaderProgramParameterType::HeatShift:
            return "HeatShift";
        case GameShaderProgramParameterType::KaosAdjustment:
            return "KaosAdjustment";
        case GameShaderProgramParameterType::LampLightColor:
            return "LampLightColor";
        case GameShaderProgramParameterType::LampToolAttributes:
            return "LampToolAttributes";
        case GameShaderProgramParameterType::LandFlatColor:
            return "LandFlatColor";
        case GameShaderProgramParameterType::MatteColor:
            return "MatteColor";
        case GameShaderProgramParameterType::NoiseStrength:
            return "NoiseStrength";
        case GameShaderProgramParameterType::NpcQuadFlatColor:
            return "NpcQuadFlatColor";
        case GameShaderProgramParameterType::OceanDepthColorStart:
            return "OceanDepthColorStart";
        case GameShaderProgramParameterType::OceanDepthColorEnd:
            return "OceanDepthColorEnd";
        case GameShaderProgramParameterType::OceanDepthDarkeningRate:
            return "OceanDepthDarkeningRate";
        case GameShaderProgramParameterType::OceanFlatColor:
            return "OceanFlatColor";
        case GameShaderProgramParameterType::OceanTransparency:
            return "OceanTransparency";
        case GameShaderProgramParameterType::OrthoMatrix:
            return "OrthoMatrix";
        case GameShaderProgramParameterType::RainAngle:
            return "RainAngle";
        case GameShaderProgramParameterType::RainDensity:
            return "RainDensity";
        case GameShaderProgramParameterType::ShipDepthDarkeningSensitivity:
            return "ShipDepthDarkeningSensitivity";
        case GameShaderProgramParameterType::StarTransparency:
            return "StarTransparency";
        case GameShaderProgramParameterType::StressColorMap:
            return "StressColorMap";
        case GameShaderProgramParameterType::SunRaysInclination:
            return "SunRaysInclination";
        case GameShaderProgramParameterType::TextLighteningStrength:
            return "TextLighteningStrength";
        case GameShaderProgramParameterType::TextureLighteningStrength:
            return "TextureLighteningStrength";
        case GameShaderProgramParameterType::TextureScaling:
            return "TextureScaling";
        case GameShaderProgramParameterType::Time:
            return "Time";
        case GameShaderProgramParameterType::ViewportSize:
            return "ViewportSize";
        case GameShaderProgramParameterType::WaterColor:
            return "WaterColor";
        case GameShaderProgramParameterType::WaterContrast:
            return "WaterContrast";
        case GameShaderProgramParameterType::WaterLevelThreshold:
            return "WaterLevelThreshold";
        case GameShaderProgramParameterType::WidthNdc:
            return "WidthNdc";
        case GameShaderProgramParameterType::Zoom:
            return "Zoom";
            // Textures
        case GameShaderProgramParameterType::SharedTexture:
            return "SharedTexture";
        case GameShaderProgramParameterType::CloudsAtlasTexture:
            return "CloudsAtlasTexture";
        case GameShaderProgramParameterType::FishesAtlasTexture:
            return "FishesAtlasTexture";
        case GameShaderProgramParameterType::ExplosionsAtlasTexture:
            return "ExplosionsAtlasTexture";
        case GameShaderProgramParameterType::GenericLinearTexturesAtlasTexture:
            return "GenericLinearTexturesAtlasTexture";
        case GameShaderProgramParameterType::GenericMipMappedTexturesAtlasTexture:
            return "GenericMipMappedTexturesAtlasTexture";
        case GameShaderProgramParameterType::LandTexture:
            return "LandTexture";
        case GameShaderProgramParameterType::NoiseTexture:
            return "NoiseTexture";
        case GameShaderProgramParameterType::NpcAtlasTexture:
            return "NpcAtlasTexture";
        case GameShaderProgramParameterType::OceanTexture:
            return "OceanTexture";
    }

    assert(false);
    throw GameException("Unsupported GameShaderProgramParameterType");
}

GameShaderVertexAttributeType StrToGameShaderVertexAttributeType(std::string const & str)
{
    // World
    if (Utils::CaseInsensitiveEquals(str, "Sky"))
        return GameShaderVertexAttributeType::Sky;
    else if (Utils::CaseInsensitiveEquals(str, "Star"))
        return GameShaderVertexAttributeType::Star;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning1"))
        return GameShaderVertexAttributeType::Lightning1;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning2"))
        return GameShaderVertexAttributeType::Lightning2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud1"))
        return GameShaderVertexAttributeType::Cloud1;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud2"))
        return GameShaderVertexAttributeType::Cloud2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud3"))
        return GameShaderVertexAttributeType::Cloud3;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return GameShaderVertexAttributeType::Land;
    else if (Utils::CaseInsensitiveEquals(str, "OceanBasic"))
        return GameShaderVertexAttributeType::OceanBasic;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed1"))
        return GameShaderVertexAttributeType::OceanDetailed1;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed2"))
        return GameShaderVertexAttributeType::OceanDetailed2;
    else if (Utils::CaseInsensitiveEquals(str, "Fish1"))
        return GameShaderVertexAttributeType::Fish1;
    else if (Utils::CaseInsensitiveEquals(str, "Fish2"))
        return GameShaderVertexAttributeType::Fish2;
    else if (Utils::CaseInsensitiveEquals(str, "Fish3"))
        return GameShaderVertexAttributeType::Fish3;
    else if (Utils::CaseInsensitiveEquals(str, "Fish4"))
        return GameShaderVertexAttributeType::Fish4;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion1"))
        return GameShaderVertexAttributeType::AMBombPreImplosion1;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion2"))
        return GameShaderVertexAttributeType::AMBombPreImplosion2;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight1"))
        return GameShaderVertexAttributeType::CrossOfLight1;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight2"))
        return GameShaderVertexAttributeType::CrossOfLight2;
    else if (Utils::CaseInsensitiveEquals(str, "AABB1"))
        return GameShaderVertexAttributeType::AABB1;
    else if (Utils::CaseInsensitiveEquals(str, "AABB2"))
        return GameShaderVertexAttributeType::AABB2;
    else if (Utils::CaseInsensitiveEquals(str, "Rain"))
        return GameShaderVertexAttributeType::Rain;
    else if (Utils::CaseInsensitiveEquals(str, "WorldBorder"))
        return GameShaderVertexAttributeType::WorldBorder;
    // Ship
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup1"))
        return GameShaderVertexAttributeType::ShipPointAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup2"))
        return GameShaderVertexAttributeType::ShipPointAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointColor"))
        return GameShaderVertexAttributeType::ShipPointColor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointTemperature"))
        return GameShaderVertexAttributeType::ShipPointTemperature;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointStress"))
        return GameShaderVertexAttributeType::ShipPointStress;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAuxiliaryData"))
        return GameShaderVertexAttributeType::ShipPointAuxiliaryData;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointFrontierColor"))
        return GameShaderVertexAttributeType::ShipPointFrontierColor;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup1"))
        return GameShaderVertexAttributeType::NpcAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup2"))
        return GameShaderVertexAttributeType::NpcAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup3"))
        return GameShaderVertexAttributeType::NpcAttributeGroup3;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup4"))
        return GameShaderVertexAttributeType::NpcAttributeGroup4;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricSpark1"))
        return GameShaderVertexAttributeType::ElectricSpark1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion1"))
        return GameShaderVertexAttributeType::Explosion1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion2"))
        return GameShaderVertexAttributeType::Explosion2;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion3"))
        return GameShaderVertexAttributeType::Explosion3;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle1"))
        return GameShaderVertexAttributeType::Sparkle1;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle2"))
        return GameShaderVertexAttributeType::Sparkle2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture1"))
        return GameShaderVertexAttributeType::ShipGenericMipMappedTexture1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture2"))
        return GameShaderVertexAttributeType::ShipGenericMipMappedTexture2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture3"))
        return GameShaderVertexAttributeType::ShipGenericMipMappedTexture3;
    else if (Utils::CaseInsensitiveEquals(str, "Flame1"))
        return GameShaderVertexAttributeType::Flame1;
    else if (Utils::CaseInsensitiveEquals(str, "Flame2"))
        return GameShaderVertexAttributeType::Flame2;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame1"))
        return GameShaderVertexAttributeType::JetEngineFlame1;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame2"))
        return GameShaderVertexAttributeType::JetEngineFlame2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight1"))
        return GameShaderVertexAttributeType::Highlight1;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight2"))
        return GameShaderVertexAttributeType::Highlight2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight3"))
        return GameShaderVertexAttributeType::Highlight3;
    else if (Utils::CaseInsensitiveEquals(str, "VectorArrow"))
        return GameShaderVertexAttributeType::VectorArrow;
    else if (Utils::CaseInsensitiveEquals(str, "Center1"))
        return GameShaderVertexAttributeType::Center1;
    else if (Utils::CaseInsensitiveEquals(str, "Center2"))
        return GameShaderVertexAttributeType::Center2;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow1"))
        return GameShaderVertexAttributeType::PointToPointArrow1;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow2"))
        return GameShaderVertexAttributeType::PointToPointArrow2;
    // Notifications
    else if (Utils::CaseInsensitiveEquals(str, "Text1"))
        return GameShaderVertexAttributeType::Text1;
    else if (Utils::CaseInsensitiveEquals(str, "Text2"))
        return GameShaderVertexAttributeType::Text2;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification1"))
        return GameShaderVertexAttributeType::TextureNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification2"))
        return GameShaderVertexAttributeType::TextureNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel1"))
        return GameShaderVertexAttributeType::PhysicsProbePanel1;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel2"))
        return GameShaderVertexAttributeType::PhysicsProbePanel2;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification1"))
        return GameShaderVertexAttributeType::MultiNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification2"))
        return GameShaderVertexAttributeType::MultiNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification3"))
        return GameShaderVertexAttributeType::MultiNotification3;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay1"))
        return GameShaderVertexAttributeType::LaserRay1;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay2"))
        return GameShaderVertexAttributeType::LaserRay2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection1"))
        return GameShaderVertexAttributeType::RectSelection1;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection2"))
        return GameShaderVertexAttributeType::RectSelection2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection3"))
        return GameShaderVertexAttributeType::RectSelection3;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveToolDashedLine1"))
        return GameShaderVertexAttributeType::InteractiveToolDashedLine1;
    // Global
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc1"))
        return GameShaderVertexAttributeType::GenericMipMappedTextureNdc1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc2"))
        return GameShaderVertexAttributeType::GenericMipMappedTextureNdc2;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}