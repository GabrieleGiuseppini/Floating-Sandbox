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
    if (lstr == "am_bomb_preimplosion")
        return ProgramType::AMBombPreImplosion;
    else if (lstr == "clouds")
        return ProgramType::Clouds;
    else if (lstr == "cross_of_light")
        return ProgramType::CrossOfLight;
    else if (lstr == "fire_extinguisher_spray")
        return ProgramType::FireExtinguisherSpray;
    else if (lstr == "fishes")
        return ProgramType::Fishes;
    else if (lstr == "heat_blaster_flame_cool")
        return ProgramType::HeatBlasterFlameCool;
    else if (lstr == "heat_blaster_flame_heat")
        return ProgramType::HeatBlasterFlameHeat;
    else if (lstr == "land_flat")
        return ProgramType::LandFlat;
    else if (lstr == "land_texture")
        return ProgramType::LandTexture;
    else if (lstr == "lightning")
        return ProgramType::Lightning;
    else if (lstr == "ocean_depth")
        return ProgramType::OceanDepth;
    else if (lstr == "ocean_flat")
        return ProgramType::OceanFlat;
    else if (lstr == "ocean_texture")
        return ProgramType::OceanTexture;
    else if (lstr == "rain")
        return ProgramType::Rain;
    else if (lstr == "ship_circle_highlights")
        return ProgramType::ShipCircleHighlights;
    else if (lstr == "ship_electrical_element_highlights")
        return ProgramType::ShipElectricalElementHighlights;
    else if (lstr == "ship_explosions")
        return ProgramType::ShipExplosions;
    else if (lstr == "ship_flames_background_1")
        return ProgramType::ShipFlamesBackground1;
    else if (lstr == "ship_flames_background_2")
        return ProgramType::ShipFlamesBackground2;
    else if (lstr == "ship_flames_background_3")
        return ProgramType::ShipFlamesBackground3;
    else if (lstr == "ship_flames_foreground_1")
        return ProgramType::ShipFlamesForeground1;
    else if (lstr == "ship_flames_foreground_2")
        return ProgramType::ShipFlamesForeground2;
    else if (lstr == "ship_flames_foreground_3")
        return ProgramType::ShipFlamesForeground3;
    else if (lstr == "ship_frontier_edges")
        return ProgramType::ShipFrontierEdges;
    else if (lstr == "ship_generic_mipmapped_textures")
        return ProgramType::ShipGenericMipMappedTextures;
    else if (lstr == "ship_points_color")
        return ProgramType::ShipPointsColor;
    else if (lstr == "ship_points_color_with_temperature")
        return ProgramType::ShipPointsColorWithTemperature;
    else if (lstr == "ship_ropes")
        return ProgramType::ShipRopes;
    else if (lstr == "ship_ropes_with_temperature")
        return ProgramType::ShipRopesWithTemperature;
    else if (lstr == "ship_sparkles")
        return ProgramType::ShipSparkles;
    else if (lstr == "ship_springs_color")
        return ProgramType::ShipSpringsColor;
    else if (lstr == "ship_springs_color_with_temperature")
        return ProgramType::ShipSpringsColorWithTemperature;
    else if (lstr == "ship_springs_texture")
        return ProgramType::ShipSpringsTexture;
    else if (lstr == "ship_springs_texture_with_temperature")
        return ProgramType::ShipSpringsTextureWithTemperature;
    else if (lstr == "ship_stressed_springs")
        return ProgramType::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return ProgramType::ShipTrianglesColor;
    else if (lstr == "ship_triangles_color_with_temperature")
        return ProgramType::ShipTrianglesColorWithTemperature;
    else if (lstr == "ship_triangles_decay")
        return ProgramType::ShipTrianglesDecay;
    else if (lstr == "ship_triangles_texture")
        return ProgramType::ShipTrianglesTexture;
    else if (lstr == "ship_triangles_texture_with_temperature")
        return ProgramType::ShipTrianglesTextureWithTemperature;
    else if (lstr == "ship_vectors")
        return ProgramType::ShipVectors;
    else if (lstr == "stars")
        return ProgramType::Stars;
    else if (lstr == "text_notifications")
        return ProgramType::TextNotifications;
    else if (lstr == "texture_notifications")
        return ProgramType::TextureNotifications;
    else if (lstr == "world_border")
        return ProgramType::WorldBorder;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
    case ProgramType::AMBombPreImplosion:
        return "AMBombPreImplosion";
    case ProgramType::Clouds:
        return "Clouds";
    case ProgramType::CrossOfLight:
        return "CrossOfLight";
    case ProgramType::FireExtinguisherSpray:
        return "FireExtinguisherSpray";
    case ProgramType::Fishes:
        return "Fishes";
    case ProgramType::HeatBlasterFlameCool:
        return "HeatBlasterFlameCool";
    case ProgramType::HeatBlasterFlameHeat:
        return "HeatBlasterFlameHeat";
    case ProgramType::LandFlat:
        return "LandFlat";
    case ProgramType::LandTexture:
        return "LandTexture";
    case ProgramType::Lightning:
        return "Lightning";
    case ProgramType::OceanDepth:
        return "OceanDepth";
    case ProgramType::OceanFlat:
        return "OceanFlat";
    case ProgramType::OceanTexture:
        return "OceanTexture";
    case ProgramType::Rain:
        return "Rain";
    case ProgramType::ShipCircleHighlights:
        return "ShipCircleHighlights";
    case ProgramType::ShipElectricalElementHighlights:
        return "ShipElectricalElementHighlights";
    case ProgramType::ShipExplosions:
        return "ShipExplosions";
    case ProgramType::ShipFlamesBackground1:
        return "ShipFlamesBackground1";
    case ProgramType::ShipFlamesBackground2:
        return "ShipFlamesBackground2";
    case ProgramType::ShipFlamesBackground3:
        return "ShipFlamesBackground3";
    case ProgramType::ShipFlamesForeground1:
        return "ShipFlamesForeground1";
    case ProgramType::ShipFlamesForeground2:
        return "ShipFlamesForeground2";
    case ProgramType::ShipFlamesForeground3:
        return "ShipFlamesForeground3";
    case ProgramType::ShipFrontierEdges:
        return "ShipFrontierEdges";
    case ProgramType::ShipGenericMipMappedTextures:
        return "ShipGenericMipMappedTextures";
    case ProgramType::ShipPointsColor:
        return "ShipPointsColor";
    case ProgramType::ShipPointsColorWithTemperature:
        return "ShipPointsColorWithTemperature";
    case ProgramType::ShipRopes:
        return "ShipRopes";
    case ProgramType::ShipRopesWithTemperature:
        return "ShipRopesWithTemperature";
    case ProgramType::ShipSparkles:
        return "ShipSparkles";
    case ProgramType::ShipSpringsColor:
        return "ShipSpringsColor";
    case ProgramType::ShipSpringsColorWithTemperature:
        return "ShipSpringsColorWithTemperature";
    case ProgramType::ShipSpringsTexture:
        return "ShipSpringsTexture";
    case ProgramType::ShipSpringsTextureWithTemperature:
        return "ShipSpringsTextureWithTemperature";
    case ProgramType::ShipStressedSprings:
        return "ShipStressedSprings";
    case ProgramType::ShipTrianglesColor:
        return "ShipTrianglesColor";
    case ProgramType::ShipTrianglesColorWithTemperature:
        return "ShipTrianglesColorWithTemperature";
    case ProgramType::ShipTrianglesDecay:
        return "ShipTrianglesDecay";
    case ProgramType::ShipTrianglesTexture:
        return "ShipTrianglesTexture";
    case ProgramType::ShipTrianglesTextureWithTemperature:
        return "ShipTrianglesTextureWithTemperature";
    case ProgramType::ShipVectors:
        return "ShipVectors";
    case ProgramType::Stars:
        return "Stars";
    case ProgramType::TextNotifications:
        return "TextNotifications";
    case ProgramType::TextureNotifications:
        return "TextureNotifications";
    case ProgramType::WorldBorder:
        return "WorldBorder";
    default:
        assert(false);
        throw GameException("Unsupported ProgramType");
    }
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "AtlasTile1Dx")
        return ProgramParameterType::AtlasTile1Dx;
    else if (str == "AtlasTile1LeftBottomTextureCoordinates")
        return ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates;
    else if (str == "AtlasTile1Size")
        return ProgramParameterType::AtlasTile1Size;
    else if (str == "EffectiveAmbientLightIntensity")
        return ProgramParameterType::EffectiveAmbientLightIntensity;
    else if (str == "FlameSpeed")
        return ProgramParameterType::FlameSpeed;
    else if (str == "FlameWindRotationAngle")
        return ProgramParameterType::FlameWindRotationAngle;
    else if (str == "HeatOverlayTransparency")
        return ProgramParameterType::HeatOverlayTransparency;
    else if (str == "LampLightColor")
        return ProgramParameterType::LampLightColor;
    else if (str == "LandFlatColor")
        return ProgramParameterType::LandFlatColor;
    else if (str == "MatteColor")
        return ProgramParameterType::MatteColor;
    else if (str == "OceanTransparency")
        return ProgramParameterType::OceanTransparency;
    else if (str == "OceanDarkeningRate")
        return ProgramParameterType::OceanDarkeningRate;
    else if (str == "OceanDepthColorStart")
        return ProgramParameterType::OceanDepthColorStart;
    else if (str == "OceanDepthColorEnd")
        return ProgramParameterType::OceanDepthColorEnd;
    else if (str == "OceanFlatColor")
        return ProgramParameterType::OceanFlatColor;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "RainDensity")
        return ProgramParameterType::RainDensity;
    else if (str == "StarTransparency")
        return ProgramParameterType::StarTransparency;
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
    else if (str == "NoiseTexture1")
        return ProgramParameterType::NoiseTexture1;
    else if (str == "NoiseTexture2")
        return ProgramParameterType::NoiseTexture2;
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
    case ProgramParameterType::EffectiveAmbientLightIntensity:
        return "EffectiveAmbientLightIntensity";
    case ProgramParameterType::FlameSpeed:
        return "FlameSpeed";
    case ProgramParameterType::FlameWindRotationAngle:
        return "FlameWindRotationAngle";
    case ProgramParameterType::HeatOverlayTransparency:
        return "HeatOverlayTransparency";
    case ProgramParameterType::LampLightColor:
        return "LampLightColor";
    case ProgramParameterType::LandFlatColor:
        return "LandFlatColor";
    case ProgramParameterType::MatteColor:
        return "MatteColor";
    case ProgramParameterType::OceanTransparency:
        return "OceanTransparency";
    case ProgramParameterType::OceanDarkeningRate:
        return "OceanDarkeningRate";
    case ProgramParameterType::OceanDepthColorStart:
        return "OceanDepthColorStart";
    case ProgramParameterType::OceanDepthColorEnd:
        return "OceanDepthColorEnd";
    case ProgramParameterType::OceanFlatColor:
        return "OceanFlatColor";
    case ProgramParameterType::OrthoMatrix:
        return "OrthoMatrix";
    case ProgramParameterType::RainDensity:
        return "RainDensity";
    case ProgramParameterType::StarTransparency:
        return "StarTransparency";
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
    case ProgramParameterType::NoiseTexture1:
        return "NoiseTexture1";
    case ProgramParameterType::NoiseTexture2:
        return "NoiseTexture2";
    case ProgramParameterType::OceanTexture:
        return "OceanTexture";
    default:
        assert(false);
        throw GameException("Unsupported ProgramParameterType");
    }
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    // World
    if (Utils::CaseInsensitiveEquals(str, "Star"))
        return VertexAttributeType::Star;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning1"))
        return VertexAttributeType::Lightning1;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning2"))
        return VertexAttributeType::Lightning2;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud1"))
        return VertexAttributeType::Cloud1;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud2"))
        return VertexAttributeType::Cloud2;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return VertexAttributeType::Land;
    else if (Utils::CaseInsensitiveEquals(str, "Ocean"))
        return VertexAttributeType::Ocean;
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
    else if (Utils::CaseInsensitiveEquals(str, "Rain"))
        return VertexAttributeType::Rain;
    else if (Utils::CaseInsensitiveEquals(str, "FireExtinguisherSpray"))
        return VertexAttributeType::FireExtinguisherSpray;
    else if (Utils::CaseInsensitiveEquals(str, "HeatBlasterFlame"))
        return VertexAttributeType::HeatBlasterFlame;
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
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointFrontierColor"))
        return VertexAttributeType::ShipPointFrontierColor;
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
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTexture1"))
        return VertexAttributeType::GenericMipMappedTexture1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTexture2"))
        return VertexAttributeType::GenericMipMappedTexture2;
    else if (Utils::CaseInsensitiveEquals(str, "GenericMipMappedTexture3"))
        return VertexAttributeType::GenericMipMappedTexture3;
    else if (Utils::CaseInsensitiveEquals(str, "Flame1"))
        return VertexAttributeType::Flame1;
    else if (Utils::CaseInsensitiveEquals(str, "Flame2"))
        return VertexAttributeType::Flame2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight1"))
        return VertexAttributeType::Highlight1;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight2"))
        return VertexAttributeType::Highlight2;
    else if (Utils::CaseInsensitiveEquals(str, "Highlight3"))
        return VertexAttributeType::Highlight3;
    else if (Utils::CaseInsensitiveEquals(str, "VectorArrow"))
        return VertexAttributeType::VectorArrow;
    // Notifications
    else if (Utils::CaseInsensitiveEquals(str, "TextNotification1"))
        return VertexAttributeType::TextNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "TextNotification2"))
        return VertexAttributeType::TextNotification2;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification1"))
        return VertexAttributeType::TextureNotification1;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNotification2"))
        return VertexAttributeType::TextureNotification2;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}