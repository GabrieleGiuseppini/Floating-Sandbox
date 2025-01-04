/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderTypes.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

namespace Render {

ProgramType ShaderFilenameToProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "aabbs")
        return ProgramType::AABBs;
    else if (lstr == "am_bomb_preimplosion")
        return ProgramType::AMBombPreImplosion;
    else if (lstr == "blast_tool_halo")
        return ProgramType::BlastToolHalo;
    else if (lstr == "clouds_basic")
        return ProgramType::CloudsBasic;
    else if (lstr == "clouds_detailed")
        return ProgramType::CloudsDetailed;
    else if (lstr == "cross_of_light")
        return ProgramType::CrossOfLight;
    else if (lstr == "fire_extinguisher_spray")
        return ProgramType::FireExtinguisherSpray;
    else if (lstr == "fishes_basic")
        return ProgramType::FishesBasic;
    else if (lstr == "fishes_detailed")
        return ProgramType::FishesDetailed;
    else if (lstr == "generic_mipmapped_textures_ndc")
        return ProgramType::GenericMipMappedTexturesNdc;
    else if (lstr == "heat_blaster_flame_cool")
        return ProgramType::HeatBlasterFlameCool;
    else if (lstr == "heat_blaster_flame_heat")
        return ProgramType::HeatBlasterFlameHeat;
    else if (lstr == "interactive_tool_dashed_lines")
        return ProgramType::InteractiveToolDashedLines;
    else if (lstr == "land_flat_basic")
        return ProgramType::LandFlatBasic;
    else if (lstr == "land_flat_detailed")
        return ProgramType::LandFlatDetailed;
    else if (lstr == "land_texture_basic")
        return ProgramType::LandTextureBasic;
    else if (lstr == "land_texture_detailed")
        return ProgramType::LandTextureDetailed;
    else if (lstr == "laser_ray")
        return ProgramType::LaserRay;
    else if (lstr == "lightning")
        return ProgramType::Lightning;
    else if (lstr == "multi_notification")
        return ProgramType::MultiNotification;
    else if (lstr == "ocean_depth_basic")
        return ProgramType::OceanDepthBasic;
    else if (lstr == "ocean_depth_detailed_background")
        return ProgramType::OceanDepthDetailedBackground;
    else if (lstr == "ocean_depth_detailed_foreground")
        return ProgramType::OceanDepthDetailedForeground;
    else if (lstr == "ocean_flat_basic")
        return ProgramType::OceanFlatBasic;
    else if (lstr == "ocean_flat_detailed_background")
        return ProgramType::OceanFlatDetailedBackground;
    else if (lstr == "ocean_flat_detailed_foreground")
        return ProgramType::OceanFlatDetailedForeground;
    else if (lstr == "ocean_texture_basic")
        return ProgramType::OceanTextureBasic;
    else if (lstr == "ocean_texture_detailed_background")
        return ProgramType::OceanTextureDetailedBackground;
    else if (lstr == "ocean_texture_detailed_foreground")
        return ProgramType::OceanTextureDetailedForeground;
    else if (lstr == "physics_probe_panel")
        return ProgramType::PhysicsProbePanel;
    else if (lstr == "rain")
        return ProgramType::Rain;
    else if (lstr == "rect_selection")
        return ProgramType::RectSelection;
    else if (lstr == "ship_centers")
        return ProgramType::ShipCenters;
    else if (lstr == "ship_circle_highlights")
        return ProgramType::ShipCircleHighlights;
    else if (lstr == "ship_electrical_element_highlights")
        return ProgramType::ShipElectricalElementHighlights;
    else if (lstr == "ship_electric_sparks")
        return ProgramType::ShipElectricSparks;
    else if (lstr == "ship_explosions")
        return ProgramType::ShipExplosions;
    else if (lstr == "ship_flames_background")
        return ProgramType::ShipFlamesBackground;
    else if (lstr == "ship_flames_foreground")
        return ProgramType::ShipFlamesForeground;
    else if (lstr == "ship_frontier_edges")
        return ProgramType::ShipFrontierEdges;
    else if (lstr == "ship_generic_mipmapped_textures")
        return ProgramType::ShipGenericMipMappedTextures;
    else if (lstr == "ship_jet_engine_flames")
        return ProgramType::ShipJetEngineFlames;
    else if (lstr == "ship_npcs_quad_flat")
        return ProgramType::ShipNpcsQuadFlat;
    else if (lstr == "ship_npcs_quad_with_roles")
        return ProgramType::ShipNpcsQuadWithRoles;
    else if (lstr == "ship_npcs_texture")
        return ProgramType::ShipNpcsTexture;
    else if (lstr == "ship_point_to_point_arrows")
        return ProgramType::ShipPointToPointArrows;
    else if (lstr == "ship_points_color")
        return ProgramType::ShipPointsColor;
    else if (lstr == "ship_points_color_stress")
        return ProgramType::ShipPointsColorStress;
    else if (lstr == "ship_points_color_heatoverlay")
        return ProgramType::ShipPointsColorHeatOverlay;
    else if (lstr == "ship_points_color_heatoverlay_stress")
        return ProgramType::ShipPointsColorHeatOverlayStress;
    else if (lstr == "ship_points_color_incandescence")
        return ProgramType::ShipPointsColorIncandescence;
    else if (lstr == "ship_points_color_incandescence_stress")
        return ProgramType::ShipPointsColorIncandescenceStress;
    else if (lstr == "ship_ropes")
        return ProgramType::ShipRopes;
    else if (lstr == "ship_ropes_stress")
        return ProgramType::ShipRopesStress;
    else if (lstr == "ship_ropes_heatoverlay")
        return ProgramType::ShipRopesHeatOverlay;
    else if (lstr == "ship_ropes_heatoverlay_stress")
        return ProgramType::ShipRopesHeatOverlayStress;
    else if (lstr == "ship_ropes_incandescence")
        return ProgramType::ShipRopesIncandescence;
    else if (lstr == "ship_ropes_incandescence_stress")
        return ProgramType::ShipRopesIncandescenceStress;
    else if (lstr == "ship_sparkles")
        return ProgramType::ShipSparkles;
    else if (lstr == "ship_springs_color")
        return ProgramType::ShipSpringsColor;
    else if (lstr == "ship_springs_color_stress")
        return ProgramType::ShipSpringsColorStress;
    else if (lstr == "ship_springs_color_heatoverlay")
        return ProgramType::ShipSpringsColorHeatOverlay;
    else if (lstr == "ship_springs_color_heatoverlay_stress")
        return ProgramType::ShipSpringsColorHeatOverlayStress;
    else if (lstr == "ship_springs_color_incandescence")
        return ProgramType::ShipSpringsColorIncandescence;
    else if (lstr == "ship_springs_color_incandescence_stress")
        return ProgramType::ShipSpringsColorIncandescenceStress;
    else if (lstr == "ship_springs_decay")
        return ProgramType::ShipSpringsDecay;
    else if (lstr == "ship_springs_internal_pressure")
        return ProgramType::ShipSpringsInternalPressure;
    else if (lstr == "ship_springs_strength")
        return ProgramType::ShipSpringsStrength;
    else if (lstr == "ship_springs_texture")
        return ProgramType::ShipSpringsTexture;
    else if (lstr == "ship_springs_texture_stress")
        return ProgramType::ShipSpringsTextureStress;
    else if (lstr == "ship_springs_texture_heatoverlay")
        return ProgramType::ShipSpringsTextureHeatOverlay;
    else if (lstr == "ship_springs_texture_heatoverlay_stress")
        return ProgramType::ShipSpringsTextureHeatOverlayStress;
    else if (lstr == "ship_springs_texture_incandescence")
        return ProgramType::ShipSpringsTextureIncandescence;
    else if (lstr == "ship_springs_texture_incandescence_stress")
        return ProgramType::ShipSpringsTextureIncandescenceStress;
    else if (lstr == "ship_stressed_springs")
        return ProgramType::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return ProgramType::ShipTrianglesColor;
    else if (lstr == "ship_triangles_color_stress")
        return ProgramType::ShipTrianglesColorStress;
    else if (lstr == "ship_triangles_color_heatoverlay")
        return ProgramType::ShipTrianglesColorHeatOverlay;
    else if (lstr == "ship_triangles_color_heatoverlay_stress")
        return ProgramType::ShipTrianglesColorHeatOverlayStress;
    else if (lstr == "ship_triangles_color_incandescence")
        return ProgramType::ShipTrianglesColorIncandescence;
    else if (lstr == "ship_triangles_color_incandescence_stress")
        return ProgramType::ShipTrianglesColorIncandescenceStress;
    else if (lstr == "ship_triangles_decay")
        return ProgramType::ShipTrianglesDecay;
    else if (lstr == "ship_triangles_internal_pressure")
        return ProgramType::ShipTrianglesInternalPressure;
    else if (lstr == "ship_triangles_strength")
        return ProgramType::ShipTrianglesStrength;
    else if (lstr == "ship_triangles_texture")
        return ProgramType::ShipTrianglesTexture;
    else if (lstr == "ship_triangles_texture_stress")
        return ProgramType::ShipTrianglesTextureStress;
    else if (lstr == "ship_triangles_texture_heatoverlay")
        return ProgramType::ShipTrianglesTextureHeatOverlay;
    else if (lstr == "ship_triangles_texture_heatoverlay_stress")
        return ProgramType::ShipTrianglesTextureHeatOverlayStress;
    else if (lstr == "ship_triangles_texture_incandescence")
        return ProgramType::ShipTrianglesTextureIncandescence;
    else if (lstr == "ship_triangles_texture_incandescence_stress")
        return ProgramType::ShipTrianglesTextureIncandescenceStress;
    else if (lstr == "ship_vectors")
        return ProgramType::ShipVectors;
    else if (lstr == "sky")
        return ProgramType::Sky;
    else if (lstr == "stars")
        return ProgramType::Stars;
    else if (lstr == "text")
        return ProgramType::Text;
    else if (lstr == "texture_notifications")
        return ProgramType::TextureNotifications;
    else if (lstr == "wind_sphere")
        return ProgramType::WindSphere;
    else if (lstr == "world_border")
        return ProgramType::WorldBorder;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
        case ProgramType::AABBs:
            return "AABBs";
        case ProgramType::AMBombPreImplosion:
            return "AMBombPreImplosion";
        case ProgramType::BlastToolHalo:
            return "BlastToolHalo";
        case ProgramType::CloudsBasic:
            return "CloudsBasic";
        case ProgramType::CloudsDetailed:
            return "CloudsDetailed";
        case ProgramType::CrossOfLight:
            return "CrossOfLight";
        case ProgramType::FireExtinguisherSpray:
            return "FireExtinguisherSpray";
        case ProgramType::FishesBasic:
            return "FishesBasic";
        case ProgramType::FishesDetailed:
            return "FishesDetailed";
        case ProgramType::GenericMipMappedTexturesNdc:
            return "GenericMipMappedTexturesNdc";
        case ProgramType::HeatBlasterFlameCool:
            return "HeatBlasterFlameCool";
        case ProgramType::HeatBlasterFlameHeat:
            return "HeatBlasterFlameHeat";
        case ProgramType::InteractiveToolDashedLines:
            return "InteractiveToolDashedLines";
        case ProgramType::LandFlatBasic:
            return "LandFlatBasic";
        case ProgramType::LandFlatDetailed:
            return "LandFlatDetailed";
        case ProgramType::LandTextureBasic:
            return "LandTextureBasic";
        case ProgramType::LandTextureDetailed:
            return "LandTextureDetailed";
        case ProgramType::LaserRay:
            return "LaserRay";
        case ProgramType::Lightning:
            return "Lightning";
        case ProgramType::MultiNotification:
            return "MultiNotification";
        case ProgramType::OceanDepthBasic:
            return "OceanDepthBasic";
        case ProgramType::OceanDepthDetailedBackground:
            return "OceanDepthDetailedBackground";
        case ProgramType::OceanDepthDetailedForeground:
            return "OceanDepthDetailedForeground";
        case ProgramType::OceanFlatBasic:
            return "OceanFlatBasic";
        case ProgramType::OceanFlatDetailedBackground:
            return "OceanFlatDetailedBackground";
        case ProgramType::OceanFlatDetailedForeground:
            return "OceanFlatDetailedForeground";
        case ProgramType::OceanTextureBasic:
            return "OceanTextureBasic";
        case ProgramType::OceanTextureDetailedBackground:
            return "OceanTextureDetailedBackground";
        case ProgramType::OceanTextureDetailedForeground:
            return "OceanTextureDetailedForeground";
        case ProgramType::PhysicsProbePanel:
            return "PhysicsProbePanel";
        case ProgramType::Rain:
            return "Rain";
        case ProgramType::RectSelection:
            return "RectSelection";
        case ProgramType::ShipCenters:
            return "ShipCenters";
        case ProgramType::ShipCircleHighlights:
            return "ShipCircleHighlights";
        case ProgramType::ShipElectricalElementHighlights:
            return "ShipElectricalElementHighlights";
        case ProgramType::ShipElectricSparks:
            return "ShipElectricSparks";
        case ProgramType::ShipExplosions:
            return "ShipExplosions";
        case ProgramType::ShipFlamesBackground:
            return "ShipFlamesBackground";
        case ProgramType::ShipFlamesForeground:
            return "ShipFlamesForeground";
        case ProgramType::ShipFrontierEdges:
            return "ShipFrontierEdges";
        case ProgramType::ShipGenericMipMappedTextures:
            return "ShipGenericMipMappedTextures";
        case ProgramType::ShipJetEngineFlames:
            return "ShipJetEngineFlames";
        case ProgramType::ShipNpcsQuadFlat:
            return "ShipNpcsQuadFlat";
        case ProgramType::ShipNpcsQuadWithRoles:
            return "ShipNpcsQuadWithRoles";
        case ProgramType::ShipNpcsTexture:
            return "ShipNpcsTexture";
        case ProgramType::ShipPointToPointArrows:
            return "ShipPointToPointArrows";
        case ProgramType::ShipPointsColor:
            return "ShipPointsColor";
        case ProgramType::ShipPointsColorStress:
            return "ShipPointsColorStress";
        case ProgramType::ShipPointsColorHeatOverlay:
            return "ShipPointsColorHeatOverlay";
        case ProgramType::ShipPointsColorHeatOverlayStress:
            return "ShipPointsColorHeatOverlayStress";
        case ProgramType::ShipPointsColorIncandescence:
            return "ShipPointsColorIncandescence";
        case ProgramType::ShipPointsColorIncandescenceStress:
            return "ShipPointsColorIncandescenceStress";
        case ProgramType::ShipRopes:
            return "ShipRopes";
        case ProgramType::ShipRopesStress:
            return "ShipRopesStress";
        case ProgramType::ShipRopesHeatOverlay:
            return "ShipRopesHeatOverlay";
        case ProgramType::ShipRopesHeatOverlayStress:
            return "ShipRopesHeatOverlayStress";
        case ProgramType::ShipRopesIncandescence:
            return "ShipRopesIncandescence";
        case ProgramType::ShipRopesIncandescenceStress:
            return "ShipRopesIncandescenceStress";
        case ProgramType::ShipSparkles:
            return "ShipSparkles";
        case ProgramType::ShipSpringsColor:
            return "ShipSpringsColor";
        case ProgramType::ShipSpringsColorStress:
            return "ShipSpringsColorStress";
        case ProgramType::ShipSpringsColorHeatOverlay:
            return "ShipSpringsColorHeatOverlay";
        case ProgramType::ShipSpringsColorHeatOverlayStress:
            return "ShipSpringsColorHeatOverlayStress";
        case ProgramType::ShipSpringsColorIncandescence:
            return "ShipSpringsColorIncandescence";
        case ProgramType::ShipSpringsColorIncandescenceStress:
            return "ShipSpringsColorIncandescenceStress";
        case ProgramType::ShipSpringsDecay:
            return "ShipSpringsDecay";
        case ProgramType::ShipSpringsInternalPressure:
            return "ShipSpringsInternalPressure";
        case ProgramType::ShipSpringsStrength:
            return "ShipSpringsStrength";
        case ProgramType::ShipSpringsTexture:
            return "ShipSpringsTexture";
        case ProgramType::ShipSpringsTextureStress:
            return "ShipSpringsTextureStress";
        case ProgramType::ShipSpringsTextureHeatOverlay:
            return "ShipSpringsTextureHeatOverlay";
        case ProgramType::ShipSpringsTextureHeatOverlayStress:
            return "ShipSpringsTextureHeatOverlayStress";
        case ProgramType::ShipSpringsTextureIncandescence:
            return "ShipSpringsTextureIncandescence";
        case ProgramType::ShipSpringsTextureIncandescenceStress:
            return "ShipSpringsTextureIncandescenceStress";
        case ProgramType::ShipStressedSprings:
            return "ShipStressedSprings";
        case ProgramType::ShipTrianglesColor:
            return "ShipTrianglesColor";
        case ProgramType::ShipTrianglesColorStress:
            return "ShipTrianglesColorStress";
        case ProgramType::ShipTrianglesColorHeatOverlay:
            return "ShipTrianglesColorHeatOverlay";
        case ProgramType::ShipTrianglesColorHeatOverlayStress:
            return "ShipTrianglesColorHeatOverlayStress";
        case ProgramType::ShipTrianglesColorIncandescence:
            return "ShipTrianglesColorIncandescence";
        case ProgramType::ShipTrianglesColorIncandescenceStress:
            return "ShipTrianglesColorIncandescenceStress";
        case ProgramType::ShipTrianglesDecay:
            return "ShipTrianglesDecay";
        case ProgramType::ShipTrianglesInternalPressure:
            return "ShipTrianglesInternalPressure";
        case ProgramType::ShipTrianglesStrength:
            return "ShipTrianglesStrength";
        case ProgramType::ShipTrianglesTexture:
            return "ShipTrianglesTexture";
        case ProgramType::ShipTrianglesTextureStress:
            return "ShipTrianglesTextureStress";
        case ProgramType::ShipTrianglesTextureHeatOverlay:
            return "ShipTrianglesTextureHeatOverlay";
        case ProgramType::ShipTrianglesTextureHeatOverlayStress:
            return "ShipTrianglesTextureHeatOverlayStress";
        case ProgramType::ShipTrianglesTextureIncandescence:
            return "ShipTrianglesTextureIncandescence";
        case ProgramType::ShipTrianglesTextureIncandescenceStress:
            return "ShipTrianglesTextureIncandescenceStress";
        case ProgramType::ShipVectors:
            return "ShipVectors";
        case ProgramType::Sky:
            return "Sky";
        case ProgramType::Stars:
            return "Stars";
        case ProgramType::Text:
            return "Text";
        case ProgramType::TextureNotifications:
            return "TextureNotifications";
        case ProgramType::WindSphere:
            return "WindSphere";
        case ProgramType::WorldBorder:
            return "WorldBorder";
    }

    assert(false);
    throw GameException("Unsupported ProgramType");
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "AtlasTile1Dx")
        return ProgramParameterType::AtlasTile1Dx;
    else if (str == "AtlasTile1LeftBottomTextureCoordinates")
        return ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates;
    else if (str == "AtlasTile1Size")
        return ProgramParameterType::AtlasTile1Size;
    else if (str == "CrepuscularColor")
        return ProgramParameterType::CrepuscularColor;
    else if (str == "EffectiveAmbientLightIntensity")
        return ProgramParameterType::EffectiveAmbientLightIntensity;
    else if (str == "EffectiveMoonlightColor")
        return ProgramParameterType::EffectiveMoonlightColor;
    else if (str == "FlameProgress")
        return ProgramParameterType::FlameProgress;
    else if (str == "FlatSkyColor")
        return ProgramParameterType::FlatSkyColor;
    else if (str == "HeatShift")
        return ProgramParameterType::HeatShift;
    else if (str == "KaosAdjustment")
        return ProgramParameterType::KaosAdjustment;
    else if (str == "LampLightColor")
        return ProgramParameterType::LampLightColor;
    else if (str == "LampToolAttributes")
        return ProgramParameterType::LampToolAttributes;
    else if (str == "LandFlatColor")
        return ProgramParameterType::LandFlatColor;
    else if (str == "MatteColor")
        return ProgramParameterType::MatteColor;
    else if (str == "NoiseStrength")
        return ProgramParameterType::NoiseStrength;
    else if (str == "NpcQuadFlatColor")
        return ProgramParameterType::NpcQuadFlatColor;
    else if (str == "OceanTransparency")
        return ProgramParameterType::OceanTransparency;
    else if (str == "OceanDepthColorStart")
        return ProgramParameterType::OceanDepthColorStart;
    else if (str == "OceanDepthColorEnd")
        return ProgramParameterType::OceanDepthColorEnd;
    else if (str == "OceanDepthDarkeningRate")
        return ProgramParameterType::OceanDepthDarkeningRate;
    else if (str == "OceanFlatColor")
        return ProgramParameterType::OceanFlatColor;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "RainAngle")
        return ProgramParameterType::RainAngle;
    else if (str == "RainDensity")
        return ProgramParameterType::RainDensity;
    else if (str == "ShipDepthDarkeningSensitivity")
        return ProgramParameterType::ShipDepthDarkeningSensitivity;
    else if (str == "StarTransparency")
        return ProgramParameterType::StarTransparency;
    else if (str == "StressColorMap")
        return ProgramParameterType::StressColorMap;
    else if (str == "SunRaysInclination")
        return ProgramParameterType::SunRaysInclination;
    else if (str == "TextLighteningStrength")
        return ProgramParameterType::TextLighteningStrength;
    else if (str == "TextureLighteningStrength")
        return ProgramParameterType::TextureLighteningStrength;
    else if (str == "TextureScaling")
        return ProgramParameterType::TextureScaling;
    else if (str == "Time")
        return ProgramParameterType::Time;
    else if (str == "ViewportSize")
        return ProgramParameterType::ViewportSize;
    else if (str == "WaterColor")
        return ProgramParameterType::WaterColor;
    else if (str == "WaterContrast")
        return ProgramParameterType::WaterContrast;
    else if (str == "WaterLevelThreshold")
        return ProgramParameterType::WaterLevelThreshold;
    else if (str == "WidthNdc")
        return ProgramParameterType::WidthNdc;
    else if (str == "Zoom")
        return ProgramParameterType::Zoom;
    // Textures
    else if (str == "SharedTexture")
        return ProgramParameterType::SharedTexture;
    else if (str == "CloudsAtlasTexture")
        return ProgramParameterType::CloudsAtlasTexture;
    else if (str == "ExplosionsAtlasTexture")
        return ProgramParameterType::ExplosionsAtlasTexture;
    else if (str == "FishesAtlasTexture")
        return ProgramParameterType::FishesAtlasTexture;
    else if (str == "GenericLinearTexturesAtlasTexture")
        return ProgramParameterType::GenericLinearTexturesAtlasTexture;
    else if (str == "GenericMipMappedTexturesAtlasTexture")
        return ProgramParameterType::GenericMipMappedTexturesAtlasTexture;
    else if (str == "LandTexture")
        return ProgramParameterType::LandTexture;
    else if (str == "NoiseTexture")
        return ProgramParameterType::NoiseTexture;
    else if (str == "NpcAtlasTexture")
        return ProgramParameterType::NpcAtlasTexture;
    else if (str == "OceanTexture")
        return ProgramParameterType::OceanTexture;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterType::AtlasTile1Dx:
            return "AtlasTile1Dx";
        case ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates:
            return "AtlasTile1LeftBottomTextureCoordinates";
        case ProgramParameterType::AtlasTile1Size:
            return "AtlasTile1Size";
        case ProgramParameterType::CrepuscularColor:
            return "CrepuscularColor";
        case ProgramParameterType::EffectiveAmbientLightIntensity:
            return "EffectiveAmbientLightIntensity";
        case ProgramParameterType::EffectiveMoonlightColor:
            return "EffectiveMoonlightColor";
        case ProgramParameterType::FlameProgress:
            return "FlameProgress";
        case ProgramParameterType::FlatSkyColor:
            return "FlatSkyColor";
        case ProgramParameterType::HeatShift:
            return "HeatShift";
        case ProgramParameterType::KaosAdjustment:
            return "KaosAdjustment";
        case ProgramParameterType::LampLightColor:
            return "LampLightColor";
        case ProgramParameterType::LampToolAttributes:
            return "LampToolAttributes";
        case ProgramParameterType::LandFlatColor:
            return "LandFlatColor";
        case ProgramParameterType::MatteColor:
            return "MatteColor";
        case ProgramParameterType::NoiseStrength:
            return "NoiseStrength";
        case ProgramParameterType::NpcQuadFlatColor:
            return "NpcQuadFlatColor";
        case ProgramParameterType::OceanDepthColorStart:
            return "OceanDepthColorStart";
        case ProgramParameterType::OceanDepthColorEnd:
            return "OceanDepthColorEnd";
        case ProgramParameterType::OceanDepthDarkeningRate:
            return "OceanDepthDarkeningRate";
        case ProgramParameterType::OceanFlatColor:
            return "OceanFlatColor";
        case ProgramParameterType::OceanTransparency:
            return "OceanTransparency";
        case ProgramParameterType::OrthoMatrix:
            return "OrthoMatrix";
        case ProgramParameterType::RainAngle:
            return "RainAngle";
        case ProgramParameterType::RainDensity:
            return "RainDensity";
        case ProgramParameterType::ShipDepthDarkeningSensitivity:
            return "ShipDepthDarkeningSensitivity";
        case ProgramParameterType::StarTransparency:
            return "StarTransparency";
        case ProgramParameterType::StressColorMap:
            return "StressColorMap";
        case ProgramParameterType::SunRaysInclination:
            return "SunRaysInclination";
        case ProgramParameterType::TextLighteningStrength:
            return "TextLighteningStrength";
        case ProgramParameterType::TextureLighteningStrength:
            return "TextureLighteningStrength";
        case ProgramParameterType::TextureScaling:
            return "TextureScaling";
        case ProgramParameterType::Time:
            return "Time";
        case ProgramParameterType::ViewportSize:
            return "ViewportSize";
        case ProgramParameterType::WaterColor:
            return "WaterColor";
        case ProgramParameterType::WaterContrast:
            return "WaterContrast";
        case ProgramParameterType::WaterLevelThreshold:
            return "WaterLevelThreshold";
        case ProgramParameterType::WidthNdc:
            return "WidthNdc";
        case ProgramParameterType::Zoom:
            return "Zoom";
        // Textures
        case ProgramParameterType::SharedTexture:
            return "SharedTexture";
        case ProgramParameterType::CloudsAtlasTexture:
            return "CloudsAtlasTexture";
        case ProgramParameterType::FishesAtlasTexture:
            return "FishesAtlasTexture";
        case ProgramParameterType::ExplosionsAtlasTexture:
            return "ExplosionsAtlasTexture";
        case ProgramParameterType::GenericLinearTexturesAtlasTexture:
            return "GenericLinearTexturesAtlasTexture";
        case ProgramParameterType::GenericMipMappedTexturesAtlasTexture:
            return "GenericMipMappedTexturesAtlasTexture";
        case ProgramParameterType::LandTexture:
            return "LandTexture";
        case ProgramParameterType::NoiseTexture:
            return "NoiseTexture";
        case ProgramParameterType::NpcAtlasTexture:
            return "NpcAtlasTexture";
        case ProgramParameterType::OceanTexture:
            return "OceanTexture";
    }

    assert(false);
    throw GameException("Unsupported ProgramParameterType");
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    // World
    if (Utils::CaseInsensitiveEquals(str, "Sky"))
        return VertexAttributeType::Sky;
    else if (Utils::CaseInsensitiveEquals(str, "Star"))
        return VertexAttributeType::Star;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning1"))
        return VertexAttributeType::Lightning1;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning2"))
        return VertexAttributeType::Lightning2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud1"))
        return VertexAttributeType::Cloud1;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud2"))
        return VertexAttributeType::Cloud2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud3"))
        return VertexAttributeType::Cloud3;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return VertexAttributeType::Land;
    else if (Utils::CaseInsensitiveEquals(str, "OceanBasic"))
        return VertexAttributeType::OceanBasic;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed1"))
        return VertexAttributeType::OceanDetailed1;
    else if (Utils::CaseInsensitiveEquals(str, "OceanDetailed2"))
        return VertexAttributeType::OceanDetailed2;
    else if (Utils::CaseInsensitiveEquals(str, "Fish1"))
        return VertexAttributeType::Fish1;
    else if (Utils::CaseInsensitiveEquals(str, "Fish2"))
        return VertexAttributeType::Fish2;
    else if (Utils::CaseInsensitiveEquals(str, "Fish3"))
        return VertexAttributeType::Fish3;
    else if (Utils::CaseInsensitiveEquals(str, "Fish4"))
        return VertexAttributeType::Fish4;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion1"))
        return VertexAttributeType::AMBombPreImplosion1;
    else if (Utils::CaseInsensitiveEquals(str, "AMBombPreImplosion2"))
        return VertexAttributeType::AMBombPreImplosion2;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight1"))
        return VertexAttributeType::CrossOfLight1;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight2"))
        return VertexAttributeType::CrossOfLight2;
    else if (Utils::CaseInsensitiveEquals(str, "AABB1"))
        return VertexAttributeType::AABB1;
    else if (Utils::CaseInsensitiveEquals(str, "AABB2"))
        return VertexAttributeType::AABB2;
    else if (Utils::CaseInsensitiveEquals(str, "Rain"))
        return VertexAttributeType::Rain;
    else if (Utils::CaseInsensitiveEquals(str, "WorldBorder"))
        return VertexAttributeType::WorldBorder;
    // Ship
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup1"))
        return VertexAttributeType::ShipPointAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAttributeGroup2"))
        return VertexAttributeType::ShipPointAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointColor"))
        return VertexAttributeType::ShipPointColor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointTemperature"))
        return VertexAttributeType::ShipPointTemperature;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointStress"))
        return VertexAttributeType::ShipPointStress;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointAuxiliaryData"))
        return VertexAttributeType::ShipPointAuxiliaryData;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointFrontierColor"))
        return VertexAttributeType::ShipPointFrontierColor;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup1"))
        return VertexAttributeType::NpcAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup2"))
        return VertexAttributeType::NpcAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup3"))
        return VertexAttributeType::NpcAttributeGroup3;
    else if (Utils::CaseInsensitiveEquals(str, "NpcAttributeGroup4"))
        return VertexAttributeType::NpcAttributeGroup4;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricSpark1"))
        return VertexAttributeType::ElectricSpark1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion1"))
        return VertexAttributeType::Explosion1;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion2"))
        return VertexAttributeType::Explosion2;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion3"))
        return VertexAttributeType::Explosion3;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle1"))
        return VertexAttributeType::Sparkle1;
    else if (Utils::CaseInsensitiveEquals(str, "Sparkle2"))
        return VertexAttributeType::Sparkle2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture1"))
        return VertexAttributeType::ShipGenericMipMappedTexture1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture2"))
        return VertexAttributeType::ShipGenericMipMappedTexture2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipGenericMipMappedTexture3"))
        return VertexAttributeType::ShipGenericMipMappedTexture3;
    else if (Utils::CaseInsensitiveEquals(str, "Flame1"))
        return VertexAttributeType::Flame1;
    else if (Utils::CaseInsensitiveEquals(str, "Flame2"))
        return VertexAttributeType::Flame2;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame1"))
        return VertexAttributeType::JetEngineFlame1;
    else if (Utils::CaseInsensitiveEquals(str, "JetEngineFlame2"))
        return VertexAttributeType::JetEngineFlame2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight1"))
        return VertexAttributeType::Highlight1;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight2"))
        return VertexAttributeType::Highlight2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight3"))
        return VertexAttributeType::Highlight3;
    else if (Utils::CaseInsensitiveEquals(str, "VectorArrow"))
        return VertexAttributeType::VectorArrow;
    else if (Utils::CaseInsensitiveEquals(str, "Center1"))
        return VertexAttributeType::Center1;
    else if (Utils::CaseInsensitiveEquals(str, "Center2"))
        return VertexAttributeType::Center2;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow1"))
        return VertexAttributeType::PointToPointArrow1;
    else if (Utils::CaseInsensitiveEquals(str, "PointToPointArrow2"))
        return VertexAttributeType::PointToPointArrow2;
    // Notifications
    else if (Utils::CaseInsensitiveEquals(str, "Text1"))
        return VertexAttributeType::Text1;
    else if (Utils::CaseInsensitiveEquals(str, "Text2"))
        return VertexAttributeType::Text2;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification1"))
        return VertexAttributeType::TextureNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification2"))
        return VertexAttributeType::TextureNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel1"))
        return VertexAttributeType::PhysicsProbePanel1;
    else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel2"))
        return VertexAttributeType::PhysicsProbePanel2;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification1"))
        return VertexAttributeType::MultiNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "MultiNotification2"))
        return VertexAttributeType::MultiNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "FireExtinguisherSpray"))
        return VertexAttributeType::FireExtinguisherSpray;
    else if (Utils::CaseInsensitiveEquals(str, "HeatBlasterFlame"))
        return VertexAttributeType::HeatBlasterFlame;
    else if (Utils::CaseInsensitiveEquals(str, "BlastToolHalo1"))
        return VertexAttributeType::BlastToolHalo1;
    else if (Utils::CaseInsensitiveEquals(str, "BlastToolHalo2"))
        return VertexAttributeType::BlastToolHalo2;
    else if (Utils::CaseInsensitiveEquals(str, "WindSphere1"))
        return VertexAttributeType::WindSphere1;
    else if (Utils::CaseInsensitiveEquals(str, "WindSphere2"))
        return VertexAttributeType::WindSphere2;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay1"))
        return VertexAttributeType::LaserRay1;
    else if (Utils::CaseInsensitiveEquals(str, "LaserRay2"))
        return VertexAttributeType::LaserRay2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection1"))
        return VertexAttributeType::RectSelection1;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection2"))
        return VertexAttributeType::RectSelection2;
    else if (Utils::CaseInsensitiveEquals(str, "RectSelection3"))
        return VertexAttributeType::RectSelection3;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveToolDashedLine1"))
        return VertexAttributeType::InteractiveToolDashedLine1;
    // Global
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc1"))
        return VertexAttributeType::GenericMipMappedTextureNdc1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTextureNdc2"))
        return VertexAttributeType::GenericMipMappedTextureNdc2;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}