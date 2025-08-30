/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameShaderSets.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>

namespace GameShaderSets::_detail {

ProgramKind ShaderNameToProgramKind(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "aabbs")
        return ProgramKind::AABBs;
    else if (lstr == "am_bomb_preimplosion")
        return ProgramKind::AMBombPreImplosion;
    else if (lstr == "clouds_basic")
        return ProgramKind::CloudsBasic;
    else if (lstr == "clouds_detailed")
        return ProgramKind::CloudsDetailed;
    else if (lstr == "cross_of_light")
        return ProgramKind::CrossOfLight;
    else if (lstr == "fishes_basic")
        return ProgramKind::FishesBasic;
    else if (lstr == "fishes_detailed")
        return ProgramKind::FishesDetailed;
    else if (lstr == "generic_mipmapped_textures_ndc")
        return ProgramKind::GenericMipMappedTexturesNdc;
    else if (lstr == "interactive_tool_dashed_lines")
        return ProgramKind::InteractiveToolDashedLines;
    else if (lstr == "land_flat_basic")
        return ProgramKind::LandFlatBasic;
    else if (lstr == "land_flat_detailed")
        return ProgramKind::LandFlatDetailed;
    else if (lstr == "land_texture_basic")
        return ProgramKind::LandTextureBasic;
    else if (lstr == "land_texture_detailed")
        return ProgramKind::LandTextureDetailed;
    else if (lstr == "laser_ray")
        return ProgramKind::LaserRay;
    else if (lstr == "lightning")
        return ProgramKind::Lightning;
    else if (lstr == "multi_notification")
        return ProgramKind::MultiNotification;
    else if (lstr == "ocean_depth_basic")
        return ProgramKind::OceanDepthBasic;
    else if (lstr == "ocean_depth_detailed_background_lower")
        return ProgramKind::OceanDepthDetailedBackgroundLower;
    else if (lstr == "ocean_depth_detailed_background_upper")
        return ProgramKind::OceanDepthDetailedBackgroundUpper;
    else if (lstr == "ocean_depth_detailed_foreground_lower")
        return ProgramKind::OceanDepthDetailedForegroundLower;
    else if (lstr == "ocean_depth_detailed_foreground_upper")
        return ProgramKind::OceanDepthDetailedForegroundUpper;
    else if (lstr == "ocean_flat_basic")
        return ProgramKind::OceanFlatBasic;
    else if (lstr == "ocean_flat_detailed_background_lower")
        return ProgramKind::OceanFlatDetailedBackgroundLower;
    else if (lstr == "ocean_flat_detailed_background_upper")
        return ProgramKind::OceanFlatDetailedBackgroundUpper;
    else if (lstr == "ocean_flat_detailed_foreground_lower")
        return ProgramKind::OceanFlatDetailedForegroundLower;
    else if (lstr == "ocean_flat_detailed_foreground_upper")
        return ProgramKind::OceanFlatDetailedForegroundUpper;
    else if (lstr == "ocean_texture_basic")
        return ProgramKind::OceanTextureBasic;
    else if (lstr == "ocean_texture_detailed_background_lower")
        return ProgramKind::OceanTextureDetailedBackgroundLower;
    else if (lstr == "ocean_texture_detailed_background_upper")
        return ProgramKind::OceanTextureDetailedBackgroundUpper;
    else if (lstr == "ocean_texture_detailed_foreground_lower")
        return ProgramKind::OceanTextureDetailedForegroundLower;
    else if (lstr == "ocean_texture_detailed_foreground_upper")
        return ProgramKind::OceanTextureDetailedForegroundUpper;
    else if (lstr == "physics_probe_panel")
        return ProgramKind::PhysicsProbePanel;
    else if (lstr == "rain")
        return ProgramKind::Rain;
    else if (lstr == "rect_selection")
        return ProgramKind::RectSelection;
    else if (lstr == "ship_centers")
        return ProgramKind::ShipCenters;
    else if (lstr == "ship_circle_highlights")
        return ProgramKind::ShipCircleHighlights;
    else if (lstr == "ship_electrical_element_highlights")
        return ProgramKind::ShipElectricalElementHighlights;
    else if (lstr == "ship_electric_sparks")
        return ProgramKind::ShipElectricSparks;
    else if (lstr == "ship_explosions")
        return ProgramKind::ShipExplosions;
    else if (lstr == "ship_flames_background")
        return ProgramKind::ShipFlamesBackground;
    else if (lstr == "ship_flames_foreground")
        return ProgramKind::ShipFlamesForeground;
    else if (lstr == "ship_frontier_edges")
        return ProgramKind::ShipFrontierEdges;
    else if (lstr == "ship_generic_mipmapped_textures")
        return ProgramKind::ShipGenericMipMappedTextures;
    else if (lstr == "ship_jet_engine_flames")
        return ProgramKind::ShipJetEngineFlames;
    else if (lstr == "ship_npcs_quad_flat")
        return ProgramKind::ShipNpcsQuadFlat;
    else if (lstr == "ship_npcs_quad_with_roles")
        return ProgramKind::ShipNpcsQuadWithRoles;
    else if (lstr == "ship_npcs_texture")
        return ProgramKind::ShipNpcsTexture;
    else if (lstr == "ship_point_to_point_arrows")
        return ProgramKind::ShipPointToPointArrows;
    else if (lstr == "ship_points_color")
        return ProgramKind::ShipPointsColor;
    else if (lstr == "ship_points_color_stress")
        return ProgramKind::ShipPointsColorStress;
    else if (lstr == "ship_points_color_heatoverlay")
        return ProgramKind::ShipPointsColorHeatOverlay;
    else if (lstr == "ship_points_color_heatoverlay_stress")
        return ProgramKind::ShipPointsColorHeatOverlayStress;
    else if (lstr == "ship_points_color_incandescence")
        return ProgramKind::ShipPointsColorIncandescence;
    else if (lstr == "ship_points_color_incandescence_stress")
        return ProgramKind::ShipPointsColorIncandescenceStress;
    else if (lstr == "ship_ropes")
        return ProgramKind::ShipRopes;
    else if (lstr == "ship_ropes_stress")
        return ProgramKind::ShipRopesStress;
    else if (lstr == "ship_ropes_heatoverlay")
        return ProgramKind::ShipRopesHeatOverlay;
    else if (lstr == "ship_ropes_heatoverlay_stress")
        return ProgramKind::ShipRopesHeatOverlayStress;
    else if (lstr == "ship_ropes_incandescence")
        return ProgramKind::ShipRopesIncandescence;
    else if (lstr == "ship_ropes_incandescence_stress")
        return ProgramKind::ShipRopesIncandescenceStress;
    else if (lstr == "ship_sparkles")
        return ProgramKind::ShipSparkles;
    else if (lstr == "ship_springs_color")
        return ProgramKind::ShipSpringsColor;
    else if (lstr == "ship_springs_color_stress")
        return ProgramKind::ShipSpringsColorStress;
    else if (lstr == "ship_springs_color_heatoverlay")
        return ProgramKind::ShipSpringsColorHeatOverlay;
    else if (lstr == "ship_springs_color_heatoverlay_stress")
        return ProgramKind::ShipSpringsColorHeatOverlayStress;
    else if (lstr == "ship_springs_color_incandescence")
        return ProgramKind::ShipSpringsColorIncandescence;
    else if (lstr == "ship_springs_color_incandescence_stress")
        return ProgramKind::ShipSpringsColorIncandescenceStress;
    else if (lstr == "ship_springs_decay")
        return ProgramKind::ShipSpringsDecay;
    else if (lstr == "ship_springs_internal_pressure")
        return ProgramKind::ShipSpringsInternalPressure;
    else if (lstr == "ship_springs_strength")
        return ProgramKind::ShipSpringsStrength;
    else if (lstr == "ship_springs_texture")
        return ProgramKind::ShipSpringsTexture;
    else if (lstr == "ship_springs_texture_stress")
        return ProgramKind::ShipSpringsTextureStress;
    else if (lstr == "ship_springs_texture_heatoverlay")
        return ProgramKind::ShipSpringsTextureHeatOverlay;
    else if (lstr == "ship_springs_texture_heatoverlay_stress")
        return ProgramKind::ShipSpringsTextureHeatOverlayStress;
    else if (lstr == "ship_springs_texture_incandescence")
        return ProgramKind::ShipSpringsTextureIncandescence;
    else if (lstr == "ship_springs_texture_incandescence_stress")
        return ProgramKind::ShipSpringsTextureIncandescenceStress;
    else if (lstr == "ship_stressed_springs")
        return ProgramKind::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return ProgramKind::ShipTrianglesColor;
    else if (lstr == "ship_triangles_color_stress")
        return ProgramKind::ShipTrianglesColorStress;
    else if (lstr == "ship_triangles_color_heatoverlay")
        return ProgramKind::ShipTrianglesColorHeatOverlay;
    else if (lstr == "ship_triangles_color_heatoverlay_stress")
        return ProgramKind::ShipTrianglesColorHeatOverlayStress;
    else if (lstr == "ship_triangles_color_incandescence")
        return ProgramKind::ShipTrianglesColorIncandescence;
    else if (lstr == "ship_triangles_color_incandescence_stress")
        return ProgramKind::ShipTrianglesColorIncandescenceStress;
    else if (lstr == "ship_triangles_decay")
        return ProgramKind::ShipTrianglesDecay;
    else if (lstr == "ship_triangles_internal_pressure")
        return ProgramKind::ShipTrianglesInternalPressure;
    else if (lstr == "ship_triangles_strength")
        return ProgramKind::ShipTrianglesStrength;
    else if (lstr == "ship_triangles_texture")
        return ProgramKind::ShipTrianglesTexture;
    else if (lstr == "ship_triangles_texture_stress")
        return ProgramKind::ShipTrianglesTextureStress;
    else if (lstr == "ship_triangles_texture_heatoverlay")
        return ProgramKind::ShipTrianglesTextureHeatOverlay;
    else if (lstr == "ship_triangles_texture_heatoverlay_stress")
        return ProgramKind::ShipTrianglesTextureHeatOverlayStress;
    else if (lstr == "ship_triangles_texture_incandescence")
        return ProgramKind::ShipTrianglesTextureIncandescence;
    else if (lstr == "ship_triangles_texture_incandescence_stress")
        return ProgramKind::ShipTrianglesTextureIncandescenceStress;
    else if (lstr == "ship_vectors")
        return ProgramKind::ShipVectors;
    else if (lstr == "sky")
        return ProgramKind::Sky;
    else if (lstr == "stars")
        return ProgramKind::Stars;
    else if (lstr == "text")
        return ProgramKind::Text;
    else if (lstr == "texture_notifications")
        return ProgramKind::TextureNotifications;
    else if (lstr == "underwater_plant")
        return ProgramKind::UnderwaterPlant;
    else if (lstr == "world_border")
        return ProgramKind::WorldBorder;
    else
        throw GameException("Unrecognized Game program \"" + str + "\"");
}

std::string ProgramKindToStr(ProgramKind program)
{
    switch (program)
    {
        case ProgramKind::AABBs:
            return "AABBs";
        case ProgramKind::AMBombPreImplosion:
            return "AMBombPreImplosion";
        case ProgramKind::CloudsBasic:
            return "CloudsBasic";
        case ProgramKind::CloudsDetailed:
            return "CloudsDetailed";
        case ProgramKind::CrossOfLight:
            return "CrossOfLight";
        case ProgramKind::FishesBasic:
            return "FishesBasic";
        case ProgramKind::FishesDetailed:
            return "FishesDetailed";
        case ProgramKind::GenericMipMappedTexturesNdc:
            return "GenericMipMappedTexturesNdc";
        case ProgramKind::InteractiveToolDashedLines:
            return "InteractiveToolDashedLines";
        case ProgramKind::LandFlatBasic:
            return "LandFlatBasic";
        case ProgramKind::LandFlatDetailed:
            return "LandFlatDetailed";
        case ProgramKind::LandTextureBasic:
            return "LandTextureBasic";
        case ProgramKind::LandTextureDetailed:
            return "LandTextureDetailed";
        case ProgramKind::LaserRay:
            return "LaserRay";
        case ProgramKind::Lightning:
            return "Lightning";
        case ProgramKind::MultiNotification:
            return "MultiNotification";
        case ProgramKind::OceanDepthBasic:
            return "OceanDepthBasic";
        case ProgramKind::OceanDepthDetailedBackgroundLower:
            return "OceanDepthDetailedBackgroundLower";
        case ProgramKind::OceanDepthDetailedBackgroundUpper:
            return "OceanDepthDetailedBackgroundUpper";
        case ProgramKind::OceanDepthDetailedForegroundLower:
            return "OceanDepthDetailedForegroundLower";
        case ProgramKind::OceanDepthDetailedForegroundUpper:
            return "OceanDepthDetailedForegroundUpper";
        case ProgramKind::OceanFlatBasic:
            return "OceanFlatBasic";
        case ProgramKind::OceanFlatDetailedBackgroundLower:
            return "OceanFlatDetailedBackgroundLower";
        case ProgramKind::OceanFlatDetailedBackgroundUpper:
            return "OceanFlatDetailedBackgroundUpper";
        case ProgramKind::OceanFlatDetailedForegroundLower:
            return "OceanFlatDetailedForegroundLower";
        case ProgramKind::OceanFlatDetailedForegroundUpper:
            return "OceanFlatDetailedForegroundUpper";
        case ProgramKind::OceanTextureBasic:
            return "OceanTextureBasic";
        case ProgramKind::OceanTextureDetailedBackgroundLower:
            return "OceanTextureDetailedBackgroundLower";
        case ProgramKind::OceanTextureDetailedBackgroundUpper:
            return "OceanTextureDetailedBackgroundUpper";
        case ProgramKind::OceanTextureDetailedForegroundLower:
            return "OceanTextureDetailedForegroundLower";
        case ProgramKind::OceanTextureDetailedForegroundUpper:
            return "OceanTextureDetailedForegroundUpper";
        case ProgramKind::PhysicsProbePanel:
            return "PhysicsProbePanel";
        case ProgramKind::Rain:
            return "Rain";
        case ProgramKind::RectSelection:
            return "RectSelection";
        case ProgramKind::ShipCenters:
            return "ShipCenters";
        case ProgramKind::ShipCircleHighlights:
            return "ShipCircleHighlights";
        case ProgramKind::ShipElectricalElementHighlights:
            return "ShipElectricalElementHighlights";
        case ProgramKind::ShipElectricSparks:
            return "ShipElectricSparks";
        case ProgramKind::ShipExplosions:
            return "ShipExplosions";
        case ProgramKind::ShipFlamesBackground:
            return "ShipFlamesBackground";
        case ProgramKind::ShipFlamesForeground:
            return "ShipFlamesForeground";
        case ProgramKind::ShipFrontierEdges:
            return "ShipFrontierEdges";
        case ProgramKind::ShipGenericMipMappedTextures:
            return "ShipGenericMipMappedTextures";
        case ProgramKind::ShipJetEngineFlames:
            return "ShipJetEngineFlames";
        case ProgramKind::ShipNpcsQuadFlat:
            return "ShipNpcsQuadFlat";
        case ProgramKind::ShipNpcsQuadWithRoles:
            return "ShipNpcsQuadWithRoles";
        case ProgramKind::ShipNpcsTexture:
            return "ShipNpcsTexture";
        case ProgramKind::ShipPointToPointArrows:
            return "ShipPointToPointArrows";
        case ProgramKind::ShipPointsColor:
            return "ShipPointsColor";
        case ProgramKind::ShipPointsColorStress:
            return "ShipPointsColorStress";
        case ProgramKind::ShipPointsColorHeatOverlay:
            return "ShipPointsColorHeatOverlay";
        case ProgramKind::ShipPointsColorHeatOverlayStress:
            return "ShipPointsColorHeatOverlayStress";
        case ProgramKind::ShipPointsColorIncandescence:
            return "ShipPointsColorIncandescence";
        case ProgramKind::ShipPointsColorIncandescenceStress:
            return "ShipPointsColorIncandescenceStress";
        case ProgramKind::ShipRopes:
            return "ShipRopes";
        case ProgramKind::ShipRopesStress:
            return "ShipRopesStress";
        case ProgramKind::ShipRopesHeatOverlay:
            return "ShipRopesHeatOverlay";
        case ProgramKind::ShipRopesHeatOverlayStress:
            return "ShipRopesHeatOverlayStress";
        case ProgramKind::ShipRopesIncandescence:
            return "ShipRopesIncandescence";
        case ProgramKind::ShipRopesIncandescenceStress:
            return "ShipRopesIncandescenceStress";
        case ProgramKind::ShipSparkles:
            return "ShipSparkles";
        case ProgramKind::ShipSpringsColor:
            return "ShipSpringsColor";
        case ProgramKind::ShipSpringsColorStress:
            return "ShipSpringsColorStress";
        case ProgramKind::ShipSpringsColorHeatOverlay:
            return "ShipSpringsColorHeatOverlay";
        case ProgramKind::ShipSpringsColorHeatOverlayStress:
            return "ShipSpringsColorHeatOverlayStress";
        case ProgramKind::ShipSpringsColorIncandescence:
            return "ShipSpringsColorIncandescence";
        case ProgramKind::ShipSpringsColorIncandescenceStress:
            return "ShipSpringsColorIncandescenceStress";
        case ProgramKind::ShipSpringsDecay:
            return "ShipSpringsDecay";
        case ProgramKind::ShipSpringsInternalPressure:
            return "ShipSpringsInternalPressure";
        case ProgramKind::ShipSpringsStrength:
            return "ShipSpringsStrength";
        case ProgramKind::ShipSpringsTexture:
            return "ShipSpringsTexture";
        case ProgramKind::ShipSpringsTextureStress:
            return "ShipSpringsTextureStress";
        case ProgramKind::ShipSpringsTextureHeatOverlay:
            return "ShipSpringsTextureHeatOverlay";
        case ProgramKind::ShipSpringsTextureHeatOverlayStress:
            return "ShipSpringsTextureHeatOverlayStress";
        case ProgramKind::ShipSpringsTextureIncandescence:
            return "ShipSpringsTextureIncandescence";
        case ProgramKind::ShipSpringsTextureIncandescenceStress:
            return "ShipSpringsTextureIncandescenceStress";
        case ProgramKind::ShipStressedSprings:
            return "ShipStressedSprings";
        case ProgramKind::ShipTrianglesColor:
            return "ShipTrianglesColor";
        case ProgramKind::ShipTrianglesColorStress:
            return "ShipTrianglesColorStress";
        case ProgramKind::ShipTrianglesColorHeatOverlay:
            return "ShipTrianglesColorHeatOverlay";
        case ProgramKind::ShipTrianglesColorHeatOverlayStress:
            return "ShipTrianglesColorHeatOverlayStress";
        case ProgramKind::ShipTrianglesColorIncandescence:
            return "ShipTrianglesColorIncandescence";
        case ProgramKind::ShipTrianglesColorIncandescenceStress:
            return "ShipTrianglesColorIncandescenceStress";
        case ProgramKind::ShipTrianglesDecay:
            return "ShipTrianglesDecay";
        case ProgramKind::ShipTrianglesInternalPressure:
            return "ShipTrianglesInternalPressure";
        case ProgramKind::ShipTrianglesStrength:
            return "ShipTrianglesStrength";
        case ProgramKind::ShipTrianglesTexture:
            return "ShipTrianglesTexture";
        case ProgramKind::ShipTrianglesTextureStress:
            return "ShipTrianglesTextureStress";
        case ProgramKind::ShipTrianglesTextureHeatOverlay:
            return "ShipTrianglesTextureHeatOverlay";
        case ProgramKind::ShipTrianglesTextureHeatOverlayStress:
            return "ShipTrianglesTextureHeatOverlayStress";
        case ProgramKind::ShipTrianglesTextureIncandescence:
            return "ShipTrianglesTextureIncandescence";
        case ProgramKind::ShipTrianglesTextureIncandescenceStress:
            return "ShipTrianglesTextureIncandescenceStress";
        case ProgramKind::ShipVectors:
            return "ShipVectors";
        case ProgramKind::Sky:
            return "Sky";
        case ProgramKind::Stars:
            return "Stars";
        case ProgramKind::Text:
            return "Text";
        case ProgramKind::TextureNotifications:
            return "TextureNotifications";
        case ProgramKind::UnderwaterPlant:
            return "UnderwaterPlant";
        case ProgramKind::WorldBorder:
            return "WorldBorder";
    }

    assert(false);
    throw GameException("Unsupported Game ProgramKind");
}

ProgramParameterKind StrToProgramParameterKind(std::string const & str)
{
    if (str == "AtlasTile1Dx")
        return ProgramParameterKind::AtlasTile1Dx;
    else if (str == "AtlasTile1LeftBottomTextureCoordinates")
        return ProgramParameterKind::AtlasTile1LeftBottomTextureCoordinates;
    else if (str == "AtlasTile1Size")
        return ProgramParameterKind::AtlasTile1Size;
    else if (str == "AtlasTileGeometryIndexed")
        return ProgramParameterKind::AtlasTileGeometryIndexed;
    else if (str == "CrepuscularColor")
        return ProgramParameterKind::CrepuscularColor;
    else if (str == "EffectiveAmbientLightIntensity")
        return ProgramParameterKind::EffectiveAmbientLightIntensity;
    else if (str == "EffectiveMoonlightColor")
        return ProgramParameterKind::EffectiveMoonlightColor;
    else if (str == "FlameProgress")
        return ProgramParameterKind::FlameProgress;
    else if (str == "FlatSkyColor")
        return ProgramParameterKind::FlatSkyColor;
    else if (str == "HeatShift")
        return ProgramParameterKind::HeatShift;
    else if (str == "KaosAdjustment")
        return ProgramParameterKind::KaosAdjustment;
    else if (str == "LampLightColor")
        return ProgramParameterKind::LampLightColor;
    else if (str == "LampToolAttributes")
        return ProgramParameterKind::LampToolAttributes;
    else if (str == "LandFlatColor")
        return ProgramParameterKind::LandFlatColor;
    else if (str == "MatteColor")
        return ProgramParameterKind::MatteColor;
    else if (str == "NoiseStrength")
        return ProgramParameterKind::NoiseStrength;
    else if (str == "NpcQuadFlatColor")
        return ProgramParameterKind::NpcQuadFlatColor;
    else if (str == "OceanTransparency")
        return ProgramParameterKind::OceanTransparency;
    else if (str == "OceanDepthColorStart")
        return ProgramParameterKind::OceanDepthColorStart;
    else if (str == "OceanDepthColorEnd")
        return ProgramParameterKind::OceanDepthColorEnd;
    else if (str == "OceanDepthDarkeningRate")
        return ProgramParameterKind::OceanDepthDarkeningRate;
    else if (str == "OceanFlatColor")
        return ProgramParameterKind::OceanFlatColor;
    else if (str == "OrthoMatrix")
        return ProgramParameterKind::OrthoMatrix;
    else if (str == "RainAngle")
        return ProgramParameterKind::RainAngle;
    else if (str == "RainDensity")
        return ProgramParameterKind::RainDensity;
    else if (str == "ShipDepthDarkeningSensitivity")
        return ProgramParameterKind::ShipDepthDarkeningSensitivity;
    else if (str == "ShipParticleRenderMode")
        return ProgramParameterKind::ShipParticleRenderMode;
    else if (str == "StarTransparency")
        return ProgramParameterKind::StarTransparency;
    else if (str == "StressColorMap")
        return ProgramParameterKind::StressColorMap;
    else if (str == "SunRaysInclination")
        return ProgramParameterKind::SunRaysInclination;
    else if (str == "TextLighteningStrength")
        return ProgramParameterKind::TextLighteningStrength;
    else if (str == "TextureLighteningStrength")
        return ProgramParameterKind::TextureLighteningStrength;
    else if (str == "TextureScaling")
        return ProgramParameterKind::TextureScaling;
    else if (str == "Time")
        return ProgramParameterKind::Time;
    else if (str == "ViewportSize")
        return ProgramParameterKind::ViewportSize;
    else if (str == "WaterColor")
        return ProgramParameterKind::WaterColor;
    else if (str == "WaterContrast")
        return ProgramParameterKind::WaterContrast;
    else if (str == "WaterLevelThreshold")
        return ProgramParameterKind::WaterLevelThreshold;
    else if (str == "WidthNdc")
        return ProgramParameterKind::WidthNdc;
    else if (str == "Zoom")
        return ProgramParameterKind::Zoom;
    // Textures
    else if (str == "SharedTexture")
        return ProgramParameterKind::SharedTexture;
    else if (str == "CloudsAtlasTexture")
        return ProgramParameterKind::CloudsAtlasTexture;
    else if (str == "ExplosionsAtlasTexture")
        return ProgramParameterKind::ExplosionsAtlasTexture;
    else if (str == "FishesAtlasTexture")
        return ProgramParameterKind::FishesAtlasTexture;
    else if (str == "GenericLinearTexturesAtlasTexture")
        return ProgramParameterKind::GenericLinearTexturesAtlasTexture;
    else if (str == "GenericMipMappedTexturesAtlasTexture")
        return ProgramParameterKind::GenericMipMappedTexturesAtlasTexture;
    else if (str == "LandTexture")
        return ProgramParameterKind::LandTexture;
    else if (str == "NoiseTexture")
        return ProgramParameterKind::NoiseTexture;
    else if (str == "NpcAtlasTexture")
        return ProgramParameterKind::NpcAtlasTexture;
    else if (str == "OceanTexture")
        return ProgramParameterKind::OceanTexture;
    else
        throw GameException("Unrecognized Game program parameter \"" + str + "\"");
}

std::string ProgramParameterKindToStr(ProgramParameterKind programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterKind::AtlasTile1Dx:
            return "AtlasTile1Dx";
        case ProgramParameterKind::AtlasTile1LeftBottomTextureCoordinates:
            return "AtlasTile1LeftBottomTextureCoordinates";
        case ProgramParameterKind::AtlasTile1Size:
            return "AtlasTile1Size";
        case ProgramParameterKind::AtlasTileGeometryIndexed:
            return "AtlasTileGeometryIndexed";
        case ProgramParameterKind::CrepuscularColor:
            return "CrepuscularColor";
        case ProgramParameterKind::EffectiveAmbientLightIntensity:
            return "EffectiveAmbientLightIntensity";
        case ProgramParameterKind::EffectiveMoonlightColor:
            return "EffectiveMoonlightColor";
        case ProgramParameterKind::FlameProgress:
            return "FlameProgress";
        case ProgramParameterKind::FlatSkyColor:
            return "FlatSkyColor";
        case ProgramParameterKind::HeatShift:
            return "HeatShift";
        case ProgramParameterKind::KaosAdjustment:
            return "KaosAdjustment";
        case ProgramParameterKind::LampLightColor:
            return "LampLightColor";
        case ProgramParameterKind::LampToolAttributes:
            return "LampToolAttributes";
        case ProgramParameterKind::LandFlatColor:
            return "LandFlatColor";
        case ProgramParameterKind::MatteColor:
            return "MatteColor";
        case ProgramParameterKind::NoiseStrength:
            return "NoiseStrength";
        case ProgramParameterKind::NpcQuadFlatColor:
            return "NpcQuadFlatColor";
        case ProgramParameterKind::OceanDepthColorStart:
            return "OceanDepthColorStart";
        case ProgramParameterKind::OceanDepthColorEnd:
            return "OceanDepthColorEnd";
        case ProgramParameterKind::OceanDepthDarkeningRate:
            return "OceanDepthDarkeningRate";
        case ProgramParameterKind::OceanFlatColor:
            return "OceanFlatColor";
        case ProgramParameterKind::OceanTransparency:
            return "OceanTransparency";
        case ProgramParameterKind::OrthoMatrix:
            return "OrthoMatrix";
        case ProgramParameterKind::RainAngle:
            return "RainAngle";
        case ProgramParameterKind::RainDensity:
            return "RainDensity";
        case ProgramParameterKind::ShipDepthDarkeningSensitivity:
            return "ShipDepthDarkeningSensitivity";
        case ProgramParameterKind::ShipParticleRenderMode:
            return "ShipParticleRenderMode";
        case ProgramParameterKind::StarTransparency:
            return "StarTransparency";
        case ProgramParameterKind::StressColorMap:
            return "StressColorMap";
        case ProgramParameterKind::SunRaysInclination:
            return "SunRaysInclination";
        case ProgramParameterKind::TextLighteningStrength:
            return "TextLighteningStrength";
        case ProgramParameterKind::TextureLighteningStrength:
            return "TextureLighteningStrength";
        case ProgramParameterKind::TextureScaling:
            return "TextureScaling";
        case ProgramParameterKind::Time:
            return "Time";
        case ProgramParameterKind::ViewportSize:
            return "ViewportSize";
        case ProgramParameterKind::WaterColor:
            return "WaterColor";
        case ProgramParameterKind::WaterContrast:
            return "WaterContrast";
        case ProgramParameterKind::WaterLevelThreshold:
            return "WaterLevelThreshold";
        case ProgramParameterKind::WidthNdc:
            return "WidthNdc";
        case ProgramParameterKind::Zoom:
            return "Zoom";
            // Textures
        case ProgramParameterKind::SharedTexture:
            return "SharedTexture";
        case ProgramParameterKind::CloudsAtlasTexture:
            return "CloudsAtlasTexture";
        case ProgramParameterKind::FishesAtlasTexture:
            return "FishesAtlasTexture";
        case ProgramParameterKind::ExplosionsAtlasTexture:
            return "ExplosionsAtlasTexture";
        case ProgramParameterKind::GenericLinearTexturesAtlasTexture:
            return "GenericLinearTexturesAtlasTexture";
        case ProgramParameterKind::GenericMipMappedTexturesAtlasTexture:
            return "GenericMipMappedTexturesAtlasTexture";
        case ProgramParameterKind::LandTexture:
            return "LandTexture";
        case ProgramParameterKind::NoiseTexture:
            return "NoiseTexture";
        case ProgramParameterKind::NpcAtlasTexture:
            return "NpcAtlasTexture";
        case ProgramParameterKind::OceanTexture:
            return "OceanTexture";
    }

    assert(false);
    throw GameException("Unsupported Game ProgramParameterKind");
}

VertexAttributeKind StrToVertexAttributeKind(std::string const & str)
{
    // World
    if (Utils::CaseInsensitiveEquals(str, "Sky"))
        return VertexAttributeKind::Sky;
    else if (Utils::CaseInsensitiveEquals(str, "Star"))
        return VertexAttributeKind::Star;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning1"))
        return VertexAttributeKind::Lightning1;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning2"))
        return VertexAttributeKind::Lightning2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud1"))
        return VertexAttributeKind::Cloud1;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud2"))
        return VertexAttributeKind::Cloud2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud3"))
        return VertexAttributeKind::Cloud3;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return VertexAttributeKind::Land;
    else if (Utils::CaseInsensitiveEquals(str, "OceanBasic"))
        return VertexAttributeKind::OceanBasic;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed1Upper"))
        return VertexAttributeKind::OceanDetailed1Upper;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed2Upper"))
        return VertexAttributeKind::OceanDetailed2Upper;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed1Lower"))
        return VertexAttributeKind::OceanDetailed1Lower;
    else if (Utils::CaseInsensitiveEquals(str, "Fish1"))
        return VertexAttributeKind::Fish1;
    else if (Utils::CaseInsensitiveEquals(str, "Fish2"))
        return VertexAttributeKind::Fish2;
    else if (Utils::CaseInsensitiveEquals(str, "Fish3"))
        return VertexAttributeKind::Fish3;
    else if (Utils::CaseInsensitiveEquals(str, "Fish4"))
        return VertexAttributeKind::Fish4;
    else if (Utils::CaseInsensitiveEquals(str, "UnderwaterPlantStatic1"))
        return VertexAttributeKind::UnderwaterPlantStatic1;
    else if (Utils::CaseInsensitiveEquals(str, "UnderwaterPlantStatic2"))
        return VertexAttributeKind::UnderwaterPlantStatic2;
    else if (Utils::CaseInsensitiveEquals(str, "UnderwaterPlantStatic3"))
        return VertexAttributeKind::UnderwaterPlantStatic3;
    else if (Utils::CaseInsensitiveEquals(str, "UnderwaterPlantDynamic1"))
        return VertexAttributeKind::UnderwaterPlantDynamic1;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion1"))
        return VertexAttributeKind::AMBombPreImplosion1;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion2"))
        return VertexAttributeKind::AMBombPreImplosion2;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight1"))
        return VertexAttributeKind::CrossOfLight1;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight2"))
        return VertexAttributeKind::CrossOfLight2;
    else if (Utils::CaseInsensitiveEquals(str, "AABB1"))
        return VertexAttributeKind::AABB1;
    else if (Utils::CaseInsensitiveEquals(str, "AABB2"))
        return VertexAttributeKind::AABB2;
    else if (Utils::CaseInsensitiveEquals(str, "Rain"))
        return VertexAttributeKind::Rain;
    else if (Utils::CaseInsensitiveEquals(str, "WorldBorder"))
        return VertexAttributeKind::WorldBorder;
    // Ship
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup1"))
        return VertexAttributeKind::ShipPointAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup2"))
        return VertexAttributeKind::ShipPointAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointColor"))
        return VertexAttributeKind::ShipPointColor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointTemperature"))
        return VertexAttributeKind::ShipPointTemperature;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointStress"))
        return VertexAttributeKind::ShipPointStress;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAuxiliaryData"))
        return VertexAttributeKind::ShipPointAuxiliaryData;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointFrontierColor"))
        return VertexAttributeKind::ShipPointFrontierColor;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup1"))
        return VertexAttributeKind::NpcAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup2"))
        return VertexAttributeKind::NpcAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup3"))
        return VertexAttributeKind::NpcAttributeGroup3;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup4"))
        return VertexAttributeKind::NpcAttributeGroup4;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricSpark1"))
        return VertexAttributeKind::ElectricSpark1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion1"))
        return VertexAttributeKind::Explosion1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion2"))
        return VertexAttributeKind::Explosion2;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion3"))
        return VertexAttributeKind::Explosion3;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle1"))
        return VertexAttributeKind::Sparkle1;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle2"))
        return VertexAttributeKind::Sparkle2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture1"))
        return VertexAttributeKind::ShipGenericMipMappedTexture1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture2"))
        return VertexAttributeKind::ShipGenericMipMappedTexture2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture3"))
        return VertexAttributeKind::ShipGenericMipMappedTexture3;
    else if (Utils::CaseInsensitiveEquals(str, "Flame1"))
        return VertexAttributeKind::Flame1;
    else if (Utils::CaseInsensitiveEquals(str, "Flame2"))
        return VertexAttributeKind::Flame2;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame1"))
        return VertexAttributeKind::JetEngineFlame1;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame2"))
        return VertexAttributeKind::JetEngineFlame2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight1"))
        return VertexAttributeKind::Highlight1;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight2"))
        return VertexAttributeKind::Highlight2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight3"))
        return VertexAttributeKind::Highlight3;
    else if (Utils::CaseInsensitiveEquals(str, "VectorArrow"))
        return VertexAttributeKind::VectorArrow;
    else if (Utils::CaseInsensitiveEquals(str, "Center1"))
        return VertexAttributeKind::Center1;
    else if (Utils::CaseInsensitiveEquals(str, "Center2"))
        return VertexAttributeKind::Center2;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow1"))
        return VertexAttributeKind::PointToPointArrow1;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow2"))
        return VertexAttributeKind::PointToPointArrow2;
    // Notifications
    else if (Utils::CaseInsensitiveEquals(str, "Text1"))
        return VertexAttributeKind::Text1;
    else if (Utils::CaseInsensitiveEquals(str, "Text2"))
        return VertexAttributeKind::Text2;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification1"))
        return VertexAttributeKind::TextureNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification2"))
        return VertexAttributeKind::TextureNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel1"))
        return VertexAttributeKind::PhysicsProbePanel1;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel2"))
        return VertexAttributeKind::PhysicsProbePanel2;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification1"))
        return VertexAttributeKind::MultiNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification2"))
        return VertexAttributeKind::MultiNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification3"))
        return VertexAttributeKind::MultiNotification3;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay1"))
        return VertexAttributeKind::LaserRay1;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay2"))
        return VertexAttributeKind::LaserRay2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection1"))
        return VertexAttributeKind::RectSelection1;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection2"))
        return VertexAttributeKind::RectSelection2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection3"))
        return VertexAttributeKind::RectSelection3;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveToolDashedLine1"))
        return VertexAttributeKind::InteractiveToolDashedLine1;
    // Global
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc1"))
        return VertexAttributeKind::GenericMipMappedTextureNdc1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc2"))
        return VertexAttributeKind::GenericMipMappedTextureNdc2;
    else
        throw GameException("Unrecognized Game vertex attribute \"" + str + "\"");
}

}